#pragma once

#include "ArduinoPlatform.hpp"
#include <algorithm>
#include <array>
#include <stdint.h>

#include "YM2612Bus.hpp"

namespace re1
{

class YM2612
{
  public:
    explicit YM2612(YM2612Bus& bus) : bus_(bus) {}

    void begin()
    {
        bus_.begin();
        resetChip();
    }

    void resetChip()
    {
        bus_.resetChip();
    }

    void writeAddress(uint8_t port, uint8_t address)
    {
        bus_.writeAddress(normalizePort(port), address);
    }

    void writeData(uint8_t port, uint8_t data)
    {
        bus_.writeData(normalizePort(port), data);
    }

    void writeRegister(uint8_t port, uint8_t address, uint8_t data)
    {
        if (address == 0x21U || address == 0x2CU) {
            return;
        }

        const uint8_t normalizedPort = normalizePort(port);
        bus_.writeRegister(normalizedPort, address, data);
        updateShadowState(normalizedPort, address, data);
    }

    void initializeSafeDefaults()
    {
        setLfoEnabled(false);
        setTimerControl(0x30);
        setDacEnabled(false);
        silenceAllChannels();
    }

  private:
    YM2612Bus& bus_;
    std::array<std::array<uint8_t, 4>, 6> tl_ = {};
    std::array<bool, 6> keyOn_ = {};

    static uint8_t normalizePort(uint8_t port)
    {
        return port & 0x01U;
    }

    void setLfoEnabled(bool enabled)
    {
        writeRegister(0, 0x22, enabled ? 0x08 : 0x00);
    }

    void setTimerControl(uint8_t value)
    {
        writeRegister(0, 0x27, value);
    }

    void setDacEnabled(bool enabled)
    {
        writeRegister(0, 0x2B, enabled ? 0x80 : 0x00);
    }

    void silenceAllChannels()
    {
        for (uint8_t channel = 0; channel < 3; ++channel) {
            writeRegister(0, static_cast<uint8_t>(0x28), channel);
        }

        for (uint8_t channel = 0; channel < 3; ++channel) {
            writeRegister(0, static_cast<uint8_t>(0x28), static_cast<uint8_t>(0x04U | channel));
        }
    }

    void updateShadowState(uint8_t port, uint8_t address, uint8_t data)
    {
        if (address == 0x28U) {
            updateKeyOnState(data);
            return;
        }

        if (address >= 0x40U && address <= 0x4EU) {
            updateTotalLevel(port, address, data);
        }
    }

    void updateKeyOnState(uint8_t data)
    {
        const uint8_t channelBits = static_cast<uint8_t>(data & 0x07U);
        if (channelBits == 0x03U || channelBits == 0x07U) {
            return;
        }

        uint8_t channel = static_cast<uint8_t>(channelBits & 0x03U);
        if ((channelBits & 0x04U) != 0U) {
            channel = static_cast<uint8_t>(channel + 3U);
        }
        if (channel >= keyOn_.size()) {
            return;
        }

        keyOn_[channel] = (data & 0xF0U) != 0U;
    }

    void updateTotalLevel(uint8_t port, uint8_t address, uint8_t data)
    {
        const uint8_t channel = static_cast<uint8_t>(address & 0x03U);
        if (channel >= 3U) {
            return;
        }

        const uint8_t slotGroup = static_cast<uint8_t>((address - 0x40U) / 4U);
        if (slotGroup >= 4U) {
            return;
        }

        tl_[static_cast<size_t>(port) * 3U + channel][slotGroup] = static_cast<uint8_t>(data & 0x7FU);
    }
};

} // namespace re1
