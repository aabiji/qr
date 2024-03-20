#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Dynamically sized bitset
// TODO: make it more efficient
class BitStream {
 public:
  BitStream() {}

  BitStream(uint8_t byte) { appendByte(byte); }

  int length() { return _bits.size(); }

  void appendBit(bool bit) { _bits.push_back(bit); }

  void appendByte(uint8_t byte, int numBits = 8) {
    for (int i = numBits - 1; i >= 0; i--) {
      bool bit = (byte & (1 << i)) >> i;
      _bits.push_back(bit);
    }
  }

  friend BitStream operator+(BitStream lhs, const BitStream& rhs) {
    lhs._bits.insert(lhs._bits.end(), rhs._bits.begin(), rhs._bits.end());
    return lhs;
  }

  // Pad left to meet the target size
  void padLeft(int targetSize) {
    if (targetSize <= _bits.size()) {
      return;
    }
    std::vector<bool> pad(targetSize - _bits.size(), 0);
    pad.insert(pad.end(), _bits.begin(), _bits.end());
    _bits = pad;
  }

  std::string toString() {
    std::string str = "";
    for (bool b : _bits) {
      str += b ? "1" : "0";
    }
    return str;
  }

  // TODO: implement this properly
  std::vector<uint8_t> toBytes() {
    int i = 0;
    std::vector<uint8_t> bytes;
    while (i < _bits.size()) {
      int shift = 7;
      uint8_t byte = 0;
      for (int j = 0; j < 8; j++) {
        byte |= ((uint8_t)_bits[i + j] << shift);
        shift -= 1;
      }
      i += 8;
      bytes.push_back(byte);
    }
    return bytes;
  }

 private:
  std::vector<bool> _bits;
};