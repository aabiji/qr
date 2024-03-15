#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <utf8.h>

uint32_t codepointAt(std::string str, int index) {
  auto it = str.begin();
  for (int i = 0; i < index && it != str.end(); i++) {
    utf8::next(it, str.end());
  }

  if (it != str.end()) {
    return utf8::next(it, str.end());
  }
  return 0;
}

template <class T>
bool contains(T* arr, T v, int size) {
  for (int i = 0; i < size; i++) {
    if (arr[i] == v) return true;
  }
  return false;
}

// Get the next multiple of a number after start number
int nextMultiple(int start, int multiple) {
  if (start % multiple) {
    return start + (multiple - start % multiple);
  }
  return start;
}

// Dynamically sized qr specific bitset
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
      mBits.push_back(bit);
    }
  }

  void append(bool bit) {
    mBits.push_back(bit);
  }

  // Pad left to meet the target size
  void padLeft(int targetSize) {
    if (targetSize <= mBits.size()) {
      return;
    }
    std::vector<bool> pad(targetSize - mBits.size(), 0);
    pad.insert(pad.end(), mBits.begin(), mBits.end());
    mBits = pad;
  }

  friend BitStream operator+(BitStream lhs, const BitStream& rhs) {
    lhs.mBits.insert(lhs.mBits.end(), rhs.mBits.begin(), rhs.mBits.end());
    return lhs;
  }

  std::string toString() {
    std::string str = "";
    for (bool b : mBits) {
      str += b ? "1" : "0";
    }
    return str;
  }

  int length() {
    return mBits.size();
  }
private:
  std::vector<bool> mBits;
};