#pragma once

#include "ArduinoPlatform.hpp"
#include <stdint.h>

namespace re1
{

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

class YM2612Bus
{
  public:
    YM2612Bus() = default;

    void begin() const
    {
        const uint8_t dataPins[8] = {
            YM2612Pins::kData0,
            YM2612Pins::kData1,
            YM2612Pins::kData2,
            YM2612Pins::kData3,
            YM2612Pins::kData4,
            YM2612Pins::kData5,
            YM2612Pins::kData6,
            YM2612Pins::kData7,
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

    void resetChip() const
    {
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

    void writeAddress(uint8_t port, uint8_t address) const
    {
        writeCycle(port, false, address);
    }

    void writeData(uint8_t port, uint8_t data) const
    {
        writeCycle(port, true, data);
    }

  private:
    static constexpr uint16_t kSetupDelayUs = 8;
    static constexpr uint16_t kWritePulseUs = 8;
    static constexpr uint16_t kHoldDelayUs = 8;
    static constexpr uint16_t kAddressToDataDelayUs = 32;

    void writeCycle(uint8_t port, bool isDataPhase, uint8_t value) const
    {
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

    void selectPort(uint8_t port) const
    {
        digitalWrite(YM2612Pins::kA1, (port & 0x01U) ? HIGH : LOW);
    }

    void driveDataBus(uint8_t value) const
    {
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

} // namespace re1
