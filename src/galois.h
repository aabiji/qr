#pragma once

#include <cstdint>
#include <iostream>

#include "tables.h"

// A number within GF(256)
class GaloisInt {
public:
  GaloisInt(uint8_t num) : _num(num) {}

  bool equals(const GaloisInt& b) const {
    return _num == b._num;
  }

  // Addition and subtraction is the Galois Field is done by
  // XORing the 2 numbers together
  friend GaloisInt operator+(GaloisInt& lhs, const GaloisInt& rhs) {
    return lhs._num ^ rhs._num;
  }

  friend GaloisInt operator-(GaloisInt& lhs, const GaloisInt& rhs) {
    return lhs._num ^ rhs._num;
  }

  // Convert the ints into exponential form, add the exponents
  // then convert back into int form
  friend GaloisInt operator*(GaloisInt& lhs, const GaloisInt& rhs) {
    int sum = galoisValueAntilogs[lhs._num] + galoisValueAntilogs[rhs._num];
    if (sum > 255) sum %= 255; // Wrap around
    return galoisValueLogs[sum];
  }
private:
  uint8_t _num;
};

inline bool operator==(const GaloisInt& lhs, const GaloisInt& rhs) {
  return lhs.equals(rhs);
}