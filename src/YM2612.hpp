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
