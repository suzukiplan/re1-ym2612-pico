#pragma once

#include "ArduinoPlatform.hpp"
#include <stdint.h>

#include "YM2612.hpp"

namespace re1 {

class VGMPlayer {
 public:
  VGMPlayer(YM2612& ym2612, const uint8_t* data, size_t size)
      : ym2612_(ym2612), data_(data), size_(size) {}

  bool begin() {
    if (!parseHeader()) {
      return false;
    }

    position_ = dataStart_;
    nextActionMicros_ = micros();
    waiting_ = false;
    finished_ = false;
    unknownCommand_ = 0x00;
    return true;
  }

  void tick() {
    if (finished_) {
      return;
    }

    if (waiting_ && !isWaitElapsed()) {
      return;
    }
    waiting_ = false;

    while (!finished_ && !waiting_) {
      if (position_ >= size_) {
        rewindToLoopPoint();
        continue;
      }

      const uint8_t command = readByte();
      executeCommand(command);
    }
  }

  bool isReady() const {
    return !finished_;
  }

  uint8_t lastUnknownCommand() const {
    return unknownCommand_;
  }

  uint32_t version() const {
    return version_;
  }

  size_t dataStartOffset() const {
    return dataStart_;
  }

  size_t loopOffset() const {
    return loopOffset_;
  }

 private:
  static constexpr uint32_t kSamplesPerSecond = 44100;

  YM2612& ym2612_;
  const uint8_t* data_;
  size_t size_;
  size_t position_ = 0;
  size_t dataStart_ = 0;
  size_t loopOffset_ = 0;
  uint32_t version_ = 0;
  uint32_t nextActionMicros_ = 0;
  bool waiting_ = false;
  bool finished_ = false;
  uint8_t unknownCommand_ = 0x00;

  bool parseHeader() {
    if (size_ < 0x40) {
      return false;
    }

    if (data_[0] != 'V' || data_[1] != 'g' || data_[2] != 'm' || data_[3] != ' ') {
      return false;
    }

    version_ = readLe32(0x08);

    const uint32_t dataOffset = readLe32(0x34);
    dataStart_ = (dataOffset == 0) ? 0x40 : static_cast<size_t>(0x34U + dataOffset);
    if (dataStart_ >= size_) {
      return false;
    }

    const uint32_t loopRelativeOffset = readLe32(0x1C);
    if (loopRelativeOffset != 0) {
      loopOffset_ = static_cast<size_t>(0x1CU + loopRelativeOffset);
      if (loopOffset_ >= size_) {
        loopOffset_ = dataStart_;
      }
    } else {
      loopOffset_ = dataStart_;
    }

    return true;
  }

  uint32_t readLe32(size_t offset) const {
    return static_cast<uint32_t>(data_[offset]) |
           (static_cast<uint32_t>(data_[offset + 1]) << 8U) |
           (static_cast<uint32_t>(data_[offset + 2]) << 16U) |
           (static_cast<uint32_t>(data_[offset + 3]) << 24U);
  }

  uint16_t readLe16AtCursor() {
    const uint16_t value = static_cast<uint16_t>(data_[position_]) |
                           (static_cast<uint16_t>(data_[position_ + 1]) << 8U);
    position_ += 2;
    return value;
  }

  uint8_t readByte() {
    return data_[position_++];
  }

  bool canRead(size_t length) const {
    return position_ + length <= size_;
  }

  void executeCommand(uint8_t command) {
    switch (command) {
      case 0x52:
        handleYm2612Write(0);
        return;
      case 0x53:
        handleYm2612Write(1);
        return;
      case 0x61:
        handleWait(readLe16AtCursor());
        return;
      case 0x62:
        handleWait(735);
        return;
      case 0x63:
        handleWait(882);
        return;
      case 0x66:
        rewindToLoopPoint();
        return;
      default:
        break;
    }

    if (command >= 0x70 && command <= 0x7F) {
      handleWait(static_cast<uint16_t>((command & 0x0FU) + 1U));
      return;
    }

    if (command >= 0x80 && command <= 0x8F) {
      handleWait(static_cast<uint16_t>(command & 0x0FU));
      return;
    }

    if (!skipUnsupportedCommand(command)) {
      unknownCommand_ = command;
      finished_ = true;
    }
  }

  void handleYm2612Write(uint8_t port) {
    if (!canRead(2)) {
      finished_ = true;
      return;
    }

    const uint8_t address = readByte();
    const uint8_t value = readByte();
    ym2612_.writeRegister(port, address, value);
  }

  void handleWait(uint16_t samples) {
    const uint32_t waitMicros = samplesToMicros(samples);
    nextActionMicros_ = micros() + waitMicros;
    waiting_ = true;
  }

  uint32_t samplesToMicros(uint16_t samples) const {
    const uint64_t microsValue =
        (static_cast<uint64_t>(samples) * 1000000ULL) / static_cast<uint64_t>(kSamplesPerSecond);
    return static_cast<uint32_t>(microsValue);
  }

  bool isWaitElapsed() const {
    return static_cast<int32_t>(micros() - nextActionMicros_) >= 0;
  }

  void rewindToLoopPoint() {
    position_ = loopOffset_;
    waiting_ = false;
    nextActionMicros_ = micros();
  }

  bool skipUnsupportedCommand(uint8_t command) {
    switch (command) {
      case 0x4F:
      case 0x50:
      case 0x94:
        return skipBytes(1);
      case 0x51:
      case 0x54:
      case 0x55:
      case 0x56:
      case 0x57:
      case 0x58:
      case 0x59:
      case 0x5A:
      case 0x5B:
      case 0x5C:
      case 0x5D:
      case 0x5E:
      case 0x5F:
      case 0xA0:
      case 0xB0:
      case 0xB1:
      case 0xB2:
      case 0xB3:
      case 0xB4:
      case 0xB5:
      case 0xB6:
      case 0xB7:
      case 0xB8:
      case 0xB9:
      case 0xBA:
      case 0xBB:
      case 0xBC:
      case 0xBD:
      case 0xBE:
      case 0xBF:
        return skipBytes(2);
      case 0x90:
      case 0x91:
      case 0x95:
        return skipBytes(4);
      case 0x92:
        return skipBytes(5);
      case 0x93:
        return skipBytes(10);
      case 0x67:
        return skipDataBlock();
      case 0x68:
        return skipBytes(11);
      case 0xC0:
      case 0xC1:
      case 0xC2:
      case 0xC3:
      case 0xC4:
      case 0xC5:
      case 0xC6:
      case 0xC7:
      case 0xD0:
      case 0xD1:
      case 0xD2:
      case 0xD3:
      case 0xD4:
      case 0xD5:
      case 0xD6:
      case 0xD7:
        return skipBytes((command >= 0xD0) ? 3 : 3);
      case 0xE0:
        return skipBytes(4);
      default:
        return false;
    }
  }

  bool skipDataBlock() {
    if (!canRead(6)) {
      return false;
    }

    const uint8_t compatibility = readByte();
    const uint8_t dataType = readByte();
    (void)compatibility;
    (void)dataType;

    const uint32_t blockSize = readLe32(position_);
    position_ += 4;
    return skipBytes(blockSize);
  }

  bool skipBytes(size_t count) {
    if (!canRead(count)) {
      return false;
    }

    position_ += count;
    return true;
  }
};

}  // namespace re1
