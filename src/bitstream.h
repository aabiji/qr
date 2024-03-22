#pragma once

#include <cstdint>
#include <vector>

// Dynamically sized bitset
class BitStream {
 public:
  std::vector<bool> bits;

  BitStream() {}

  BitStream(uint8_t byte) { appendByte(byte); }

  friend BitStream operator+(BitStream lhs, const BitStream& rhs) {
    lhs.bits.insert(lhs.bits.end(), rhs.bits.begin(), rhs.bits.end());
    return lhs;
  }

  void appendByte(uint8_t byte, int numBits = 8) {
    for (int i = numBits - 1; i >= 0; i--) {
      bool bit = (byte & (1 << i)) >> i;
      bits.push_back(bit);
    }
  }

  // Pad left to meet the target size
  void padLeft(int targetSize) {
    if (targetSize <= bits.size()) {
      return;
    }
    std::vector<bool> pad(targetSize - bits.size(), 0);
    pad.insert(pad.end(), bits.begin(), bits.end());
    bits = pad;
  }

  std::vector<uint8_t> toBytes() {
    int shift = 0;
    uint8_t byte = 0;
    std::vector<uint8_t> bytes;

    for (bool bit : bits) {
      byte |= (bit ? 1 : 0) << (7 - shift);
      shift += 1;
      if (shift == 8) {
        bytes.push_back(byte);
        byte = 0;
        shift = 0;
      }
    }

    if (shift > 0) {
      byte <<= (8 - shift);
      bytes.push_back(byte);
    }
    return bytes;
  }
};