#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Dynamically sized bitset
// TODO: make it more efficient
class BitStream {
public:
  BitStream() {}

  BitStream(std::string from) {
    for (char c : from) {
      append(c == '0' ? 0 : 1);
    }
  }

  BitStream(uint8_t byte) {
    for (int i = 7; i >= 0; i--) {
      bool bit = (byte & (1 << i)) >> i;
      _bits.push_back(bit);
    }
  }

  void append(bool bit) {
    _bits.push_back(bit);
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

  friend BitStream operator+(BitStream lhs, const BitStream& rhs) {
    lhs._bits.insert(lhs._bits.end(), rhs._bits.begin(), rhs._bits.end());
    return lhs;
  }

  std::string toString() {
    std::string str = "";
    for (bool b : _bits) {
      str += b ? "1" : "0";
    }
    return str;
  }

  int length() {
    return _bits.size();
  }
private:
  std::vector<bool> _bits;
};