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
      bits_.push_back(bit);
    }
  }

  void append(bool bit) {
    bits_.push_back(bit);
  }

  // Pad left to meet the target size
  void padLeft(int targetSize) {
    if (targetSize <= bits_.size()) {
      return;
    }
    std::vector<bool> pad(targetSize - bits_.size(), 0);
    pad.insert(pad.end(), bits_.begin(), bits_.end());
    bits_ = pad;
  }

  friend BitStream operator+(BitStream lhs, const BitStream& rhs) {
    lhs.bits_.insert(lhs.bits_.end(), rhs.bits_.begin(), rhs.bits_.end());
    return lhs;
  }

  std::string toString() {
    std::string str = "";
    for (bool b : bits_) {
      str += b ? "1" : "0";
    }
    return str;
  }

  int length() {
    return bits_.size();
  }
private:
  std::vector<bool> bits_;
};