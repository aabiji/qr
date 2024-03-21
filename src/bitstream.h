#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Dynamically sized bitset
// TODO: make it more efficient
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

  std::string toString() {
    std::string str = "";
    for (bool b : bits) {
      str += b ? "1" : "0";
    }
    return str;
  }

  // TODO: implement this properly
  std::vector<uint8_t> toBytes() {
    int i = 0;
    std::vector<uint8_t> bytes;
    while (i < bits.size()) {
      int shift = 7;
      uint8_t byte = 0;
      for (int j = 0; j < 8; j++) {
        byte |= ((uint8_t)bits[i + j] << shift);
        shift -= 1;
      }
      i += 8;
      bytes.push_back(byte);
    }
    return bytes;
  }
};