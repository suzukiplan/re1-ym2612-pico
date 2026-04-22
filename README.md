# RE1-YM2612 for Raspberry Pi Pico

Raspberry Pi Pico (RP2040) + PlatformIO + Arduino framework project for driving an RE1-YM2612 board with a minimal direct GPIO bus.

## Wiring

| RE1-YM2612 | Raspberry Pi Pico |
| --- | --- |
| D0 | GPIO2 |
| D1 | GPIO3 |
| D2 | GPIO4 |
| D3 | GPIO5 |
| D4 | GPIO6 |
| D5 | GPIO7 |
| D6 | GPIO8 |
| D7 | GPIO9 |
| A0 | GPIO10 |
| A1 | GPIO11 |
| /WR | GPIO18 |
| /CS | GPIO19 |
| /IC | GPIO20 |
| GND | GND |

## Project layout

- `platformio.ini`: PlatformIO environment for Raspberry Pi Pico with the Arduino framework.
- `src/YM2612Bus.hpp`: Safe digitalWrite-based 8-bit bus access layer.
- `src/YM2612.hpp`: YM2612 device abstraction layer built on top of the bus.
- `src/VGMPlayer.hpp`: VGM parser/player that executes YM2612 writes and wait commands.
- `src/rom_song.hpp`: Embedded VGM byte array generated from `vgm/song.vgm`.
- `src/main.cpp`: Startup sequence, serial logging, and playback loop.

## Design notes

- The bus layer and YM2612 layer are separated so the register-level API stays reusable from the VGM player and from future higher-level playback code.
- Data lines D0-D7 are driven with `digitalWrite()` for conservative and predictable behavior.
- `/CS` and `/WR` are treated as active-low control lines.
- `/IC` is exposed through `resetChip()` and used during startup.
- Timing uses intentionally long `delayMicroseconds()` values to prioritize reliable bring-up over throughput.
- VGM playback runs as a small state machine in `loop()` and currently implements:
  - YM2612 register writes (`0x52`, `0x53`)
  - wait commands (`0x61`, `0x62`, `0x63`, `0x70-0x7F`, `0x80-0x8F`)
  - loop/end handling (`0x66`)
  - skipping of unrelated chip commands that appear in the source VGM
- Port selection uses `A1`:
  - `port = 0` drives `A1 = LOW`
  - `port = 1` drives `A1 = HIGH`
- Register address vs. data phase uses `A0`:
  - address write drives `A0 = LOW`
  - data write drives `A0 = HIGH`

## Build and upload

1. Install [PlatformIO](https://platformio.org/).
2. Open this directory in VS Code with the PlatformIO extension, or use the CLI.
3. Build:

```sh
pio run
```

4. Upload:

```sh
pio run -t upload
```

5. Open the serial monitor:

```sh
pio device monitor -b 115200
```

## Startup behavior

At boot, `setup()` does the following:

1. Starts `Serial` at `115200`.
2. Logs the active pin mapping.
3. Initializes the GPIO bus.
4. Pulses `/IC` to reset the YM2612.
5. Applies a few safe default registers:
   - disable LFO
   - reset timer control
   - disable DAC
   - key-off all 6 channels
6. Parses the embedded `rom_song` VGM header and logs version/data offsets.
7. Starts playback.

`loop()` repeatedly calls the VGM player, which executes YM2612 writes immediately and schedules wait commands using `micros()`.

## Extending toward a VGM player

The current structure is designed so you can later add:

- a higher-performance bus backend while keeping the same chip API
- channel helpers for instrument setup and note-on/note-off control
- support for more VGM command types if you need additional chips or PCM features

## Files

### `platformio.ini`

```ini
[env:pico]
platform = raspberrypi
board = pico
framework = arduino
monitor_speed = 115200
build_flags =
    -Wall
    -Wextra
    -Wpedantic
```

### `src/YM2612Bus.hpp`

```cpp
#pragma once

#include <Arduino.h>
#include <stdint.h>

namespace re1 {

struct YM2612Pins {
  static constexpr uint8_t kData0 = 2;
  static constexpr uint8_t kData1 = 3;
  static constexpr uint8_t kData2 = 4;
  static constexpr uint8_t kData3 = 5;
  static constexpr uint8_t kData4 = 6;
  static constexpr uint8_t kData5 = 7;
  static constexpr uint8_t kData6 = 8;
  static constexpr uint8_t kData7 = 9;

  static constexpr uint8_t kA0 = 10;
  static constexpr uint8_t kA1 = 11;

  static constexpr uint8_t kWR = 18;
  static constexpr uint8_t kCS = 19;
  static constexpr uint8_t kIC = 20;
};

class YM2612Bus {
 public:
  YM2612Bus() = default;

  void begin() const {
    const uint8_t dataPins[8] = {
        YM2612Pins::kData0, YM2612Pins::kData1, YM2612Pins::kData2, YM2612Pins::kData3,
        YM2612Pins::kData4, YM2612Pins::kData5, YM2612Pins::kData6, YM2612Pins::kData7,
    };

    for (uint8_t pin : dataPins) {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
    }

    pinMode(YM2612Pins::kA0, OUTPUT);
    pinMode(YM2612Pins::kA1, OUTPUT);
    pinMode(YM2612Pins::kWR, OUTPUT);
    pinMode(YM2612Pins::kCS, OUTPUT);
    pinMode(YM2612Pins::kIC, OUTPUT);

    digitalWrite(YM2612Pins::kA0, LOW);
    digitalWrite(YM2612Pins::kA1, LOW);
    digitalWrite(YM2612Pins::kWR, HIGH);
    digitalWrite(YM2612Pins::kCS, HIGH);
    digitalWrite(YM2612Pins::kIC, HIGH);
  }

  void resetChip() const {
    digitalWrite(YM2612Pins::kCS, HIGH);
    digitalWrite(YM2612Pins::kWR, HIGH);
    digitalWrite(YM2612Pins::kA0, LOW);
    digitalWrite(YM2612Pins::kA1, LOW);
    driveDataBus(0x00);

    digitalWrite(YM2612Pins::kIC, LOW);
    delay(10);
    digitalWrite(YM2612Pins::kIC, HIGH);
    delay(100);
  }

  void writeAddress(uint8_t port, uint8_t address) const {
    writeCycle(port, false, address);
  }

  void writeData(uint8_t port, uint8_t data) const {
    writeCycle(port, true, data);
  }

 private:
  static constexpr uint16_t kSetupDelayUs = 8;
  static constexpr uint16_t kWritePulseUs = 8;
  static constexpr uint16_t kHoldDelayUs = 8;
  static constexpr uint16_t kAddressToDataDelayUs = 32;

  void writeCycle(uint8_t port, bool isDataPhase, uint8_t value) const {
    selectPort(port);
    digitalWrite(YM2612Pins::kA0, isDataPhase ? HIGH : LOW);
    driveDataBus(value);

    delayMicroseconds(kSetupDelayUs);
    digitalWrite(YM2612Pins::kCS, LOW);
    delayMicroseconds(kSetupDelayUs);
    digitalWrite(YM2612Pins::kWR, LOW);
    delayMicroseconds(kWritePulseUs);
    digitalWrite(YM2612Pins::kWR, HIGH);
    delayMicroseconds(kHoldDelayUs);
    digitalWrite(YM2612Pins::kCS, HIGH);

    if (isDataPhase) {
      delayMicroseconds(kHoldDelayUs);
    } else {
      delayMicroseconds(kAddressToDataDelayUs);
    }
  }

  void selectPort(uint8_t port) const {
    digitalWrite(YM2612Pins::kA1, (port & 0x01U) ? HIGH : LOW);
  }

  void driveDataBus(uint8_t value) const {
    digitalWrite(YM2612Pins::kData0, (value & 0x01U) ? HIGH : LOW);
    digitalWrite(YM2612Pins::kData1, (value & 0x02U) ? HIGH : LOW);
    digitalWrite(YM2612Pins::kData2, (value & 0x04U) ? HIGH : LOW);
    digitalWrite(YM2612Pins::kData3, (value & 0x08U) ? HIGH : LOW);
    digitalWrite(YM2612Pins::kData4, (value & 0x10U) ? HIGH : LOW);
    digitalWrite(YM2612Pins::kData5, (value & 0x20U) ? HIGH : LOW);
    digitalWrite(YM2612Pins::kData6, (value & 0x40U) ? HIGH : LOW);
    digitalWrite(YM2612Pins::kData7, (value & 0x80U) ? HIGH : LOW);
  }
};

}  // namespace re1
```

### `src/YM2612.hpp`

```cpp
#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "YM2612Bus.hpp"

namespace re1 {

class YM2612 {
 public:
  explicit YM2612(YM2612Bus& bus) : bus_(bus) {}

  void begin() {
    bus_.begin();
    resetChip();
  }

  void resetChip() {
    bus_.resetChip();
  }

  void writeAddress(uint8_t port, uint8_t address) {
    bus_.writeAddress(normalizePort(port), address);
  }

  void writeData(uint8_t port, uint8_t data) {
    bus_.writeData(normalizePort(port), data);
  }

  void writeRegister(uint8_t port, uint8_t address, uint8_t data) {
    const uint8_t normalizedPort = normalizePort(port);
    bus_.writeAddress(normalizedPort, address);
    bus_.writeData(normalizedPort, data);
  }

  void initializeSafeDefaults() {
    setLfoEnabled(false);
    setTimerControl(0x30);
    setDacEnabled(false);
    silenceAllChannels();
  }

 private:
  YM2612Bus& bus_;

  static uint8_t normalizePort(uint8_t port) {
    return port & 0x01U;
  }

  void setLfoEnabled(bool enabled) {
    writeRegister(0, 0x22, enabled ? 0x08 : 0x00);
  }

  void setTimerControl(uint8_t value) {
    writeRegister(0, 0x27, value);
  }

  void setDacEnabled(bool enabled) {
    writeRegister(0, 0x2B, enabled ? 0x80 : 0x00);
  }

  void silenceAllChannels() {
    for (uint8_t channel = 0; channel < 3; ++channel) {
      writeRegister(0, static_cast<uint8_t>(0x28), channel);
    }

    for (uint8_t channel = 0; channel < 3; ++channel) {
      writeRegister(0, static_cast<uint8_t>(0x28), static_cast<uint8_t>(0x04U | channel));
    }
  }
};

}  // namespace re1
```

### `src/main.cpp`

```cpp
#include <Arduino.h>

#include "YM2612.hpp"
#include "YM2612Bus.hpp"

namespace {

re1::YM2612Bus g_bus;
re1::YM2612 g_ym2612(g_bus);

void writeStartupTestRegisters() {
  Serial.println("Writing safe YM2612 test registers...");

  g_ym2612.writeRegister(0, 0x22, 0x00);
  Serial.println("  port 0, reg 0x22 <- 0x00 (LFO disabled)");

  g_ym2612.writeRegister(0, 0x27, 0x30);
  Serial.println("  port 0, reg 0x27 <- 0x30 (timers reset)");

  g_ym2612.writeRegister(0, 0x2B, 0x00);
  Serial.println("  port 0, reg 0x2B <- 0x00 (DAC disabled)");

  g_ym2612.writeRegister(1, 0xB6, 0xC0);
  Serial.println("  port 1, reg 0xB6 <- 0xC0 (safe pan/AMS/PMS default)");
}

void logPinMapping() {
  Serial.println("Pin mapping:");
  Serial.println("  D0-D7 -> GPIO2-GPIO9");
  Serial.println("  A0    -> GPIO10");
  Serial.println("  A1    -> GPIO11");
  Serial.println("  /WR   -> GPIO18");
  Serial.println("  /CS   -> GPIO19");
  Serial.println("  /IC   -> GPIO20");
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("RE1-YM2612 minimal bus bring-up");
  logPinMapping();

  g_ym2612.begin();
  Serial.println("YM2612 reset complete.");

  g_ym2612.initializeSafeDefaults();
  Serial.println("YM2612 safe defaults applied.");

  writeStartupTestRegisters();
  Serial.println("Initialization finished.");
}

void loop() {
}
```
*** End Patch
++ End Patch
