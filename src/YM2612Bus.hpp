#pragma once

#include "ArduinoPlatform.hpp"
#include <hardware/clocks.h>
#include <hardware/pwm.h>
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
    static constexpr uint8_t kRD = 28;
    static constexpr uint8_t kSCLK = 29;
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
        pinMode(YM2612Pins::kRD, OUTPUT);

        digitalWrite(YM2612Pins::kA0, LOW);
        digitalWrite(YM2612Pins::kA1, LOW);
        digitalWrite(YM2612Pins::kWR, HIGH);
        digitalWrite(YM2612Pins::kCS, HIGH);
        digitalWrite(YM2612Pins::kIC, HIGH);
        digitalWrite(YM2612Pins::kRD, HIGH);

        beginMasterClock();
    }

    void resetChip() const
    {
        digitalWrite(YM2612Pins::kCS, HIGH);
        digitalWrite(YM2612Pins::kWR, HIGH);
        digitalWrite(YM2612Pins::kA0, LOW);
        digitalWrite(YM2612Pins::kA1, LOW);
        digitalWrite(YM2612Pins::kRD, HIGH);
        driveDataBus(0x00);

        digitalWrite(YM2612Pins::kIC, LOW);
        delayMicroseconds(kResetPulseUs);
        digitalWrite(YM2612Pins::kIC, HIGH);
        delayMicroseconds(kResetRecoveryUs);
    }

    void writeAddress(uint8_t port, uint8_t address) const
    {
        writePhase(normalizePort(port), false, address);
    }

    void writeData(uint8_t port, uint8_t data) const
    {
        writePhase(normalizePort(port), true, data);
    }

    void writeRegister(uint8_t port, uint8_t address, uint8_t data) const
    {
        const uint8_t normalizedPort = normalizePort(port);
        selectPort(normalizedPort);

        writePhase(normalizedPort, false, address);
        delayMicroseconds(kAddressWriteRecoveryUs);

        writePhase(normalizedPort, true, data);
        delayMicroseconds(postWriteDelayUs(address));
    }

  private:
    static constexpr uint32_t kMasterClockHz = 7670454;
    static constexpr uint16_t kWritePulseUs = 1;
    static constexpr uint16_t kAddressWriteRecoveryUs = 10;
    static constexpr uint16_t kResetPulseUs = 1000;
    static constexpr uint16_t kResetRecoveryUs = 1000;

    static uint8_t normalizePort(uint8_t port)
    {
        return port & 0x01U;
    }

    static uint16_t postWriteDelayUs(uint8_t address)
    {
        (void)address;
        return 10;
    }

    static void beginMasterClock()
    {
        gpio_set_function(YM2612Pins::kSCLK, GPIO_FUNC_PWM);

        const uint slice = pwm_gpio_to_slice_num(YM2612Pins::kSCLK);
        const uint channel = pwm_gpio_to_channel(YM2612Pins::kSCLK);
        pwm_config config = pwm_get_default_config();
        pwm_config_set_wrap(&config, 1);
        pwm_config_set_clkdiv(&config, static_cast<float>(clock_get_hz(clk_sys)) / (2.0F * kMasterClockHz));
        pwm_init(slice, channel, &config, false);
        pwm_set_chan_level(slice, channel, 1);
        pwm_set_enabled(slice, true);
    }

    void writePhase(uint8_t port, bool isDataPhase, uint8_t value) const
    {
        selectPort(port);
        digitalWrite(YM2612Pins::kA0, isDataPhase ? HIGH : LOW);
        driveDataBus(value);
        pulseWrite(isDataPhase);
    }

    void pulseWrite(bool isDataPhase) const
    {
        digitalWrite(YM2612Pins::kCS, LOW);
        digitalWrite(YM2612Pins::kWR, LOW);

        delayMicroseconds(kWritePulseUs);

        digitalWrite(YM2612Pins::kWR, HIGH);
        digitalWrite(YM2612Pins::kCS, HIGH);
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
