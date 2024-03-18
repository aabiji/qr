#pragma once

#include <cstdint>
#include <iostream>
#include <map>
#include <vector>

#include "tables.h"

namespace Galois {

// A number within GF(256)
class Int {
public:
  Int() { _num = 0; }

  // _num is a number belonging in the Galois Field
  // within the range of 0 - 255 (inclusive)
  Int(uint8_t num) : _num(num) {}

  int exponent() const { return galoisValueAntilogs[_num]; }

  void debug() {
    std::cout << "2 ** " << galoisValueAntilogs[_num] << " = " << int(_num);
  }

  friend std::ostream &operator<<(std::ostream &os, const Int &g) {
    os << "a" << galoisValueAntilogs[g._num];
    return os;
  }

  inline bool operator==(const Int &rhs) const {
    return _num == rhs._num;
  }

  inline bool operator!=(const Int &rhs) const {
    return _num != rhs._num;
  }

  inline bool operator<(const Int &rhs) const { return _num < rhs._num; }

  inline bool operator>=(const int &v) const { return _num >= v; }

  // Addition and subtraction is the Galois Field is done by
  // XORing the 2 numbers together
  friend Int operator+(Int &lhs, const Int &rhs) {
    return lhs._num ^ rhs._num;
  }

  Int operator+=(const Int &rhs) {
    *this = *this + rhs;
    return *this;
  }

  // Convert the ints into exponential form, add the exponents
  // then convert back into int form
  friend Int operator*(Int &lhs, const Int &rhs) {
    int sum = galoisValueAntilogs[lhs._num] + galoisValueAntilogs[rhs._num];
    if (sum > 255)
      sum %= 255; // Wrap around
    return galoisValueLogs[sum];
  }

private:
  uint8_t _num;
};

static bool contains(std::vector<int> arr, int v) {
  for (int i = 0; i < arr.size(); i++) {
    if (arr[i] == v)
      return true;
  }
  return false;
}

// Galois Field specific polynomials
// TODO: refactor this and optimise this, please
//       frankly, revise this entire file
class Polynomial {
public:
  // Constructs the polynomial from a vector of coefficient exponents.
  // For example, {0, 1, 3} will create the polynomial a0x2 a1x1 a3x0
  Polynomial(std::vector<int> exponents) {
    for (int c : exponents) {
      _coefficients.push_back(Int(galoisValueLogs[c]));
    }
  }

  friend Polynomial operator*(Polynomial &lhs, const Polynomial &rhs) {
    // Map nomial degrees to their coefficients;
    std::map<int, Int> nomials;

    // Multiply each term on the left to each term on the right
    // whilst combining like terms
    int lhs_degree = lhs._coefficients.size() - 1;
    for (int i = 0; i < lhs._coefficients.size(); i++) {
      int rhs_degree = rhs._coefficients.size() - 1;
      for (int j = 0; j < rhs._coefficients.size(); j++) {
        Int coefficient = lhs._coefficients[i] * rhs._coefficients[j];
        int term = lhs_degree + rhs_degree;
        nomials[term] += coefficient; // Add like terms
        rhs_degree -= 1;
      }
      lhs_degree -= 1;
    }

    std::vector<int> new_coefficients;
    for (auto const &[key, val] : nomials) {
      int exponent = val.exponent();
      new_coefficients.insert(new_coefficients.begin(), exponent);
    }

    return Polynomial(new_coefficients);
  }

  friend std::ostream &operator<<(std::ostream &os, const Polynomial &p) {
    int degree = p._coefficients.size() - 1;
    for (int i = 0; i < p._coefficients.size(); i++) {
      if (i != 0)
        os << " + "; // Coefficients are always positive
      os << p._coefficients[i] << "x" << degree;
      degree -= 1;
    }
    return os;
  }

  inline bool operator==(const Polynomial &rhs) const {
    for (int i = 0; i < _coefficients.size(); i++) {
      if (_coefficients[i] != rhs._coefficients[i])
        return false;
    }
    return true;
  }

private:
  std::vector<Int> _coefficients;
};

}; // Namespace Galois