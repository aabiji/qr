#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

#include "tables.h"

// A number within GF(256)
class GaloisInt {
public:
  GaloisInt(uint8_t num) : _num(num) {}

  void debug() {
    std::cout << "2 ** " << galoisValueAntilogs[_num] << " = " << int(_num);
  }

  friend std::ostream& operator<<(std::ostream& os, const GaloisInt& g) {
    os << "a" << galoisValueAntilogs[g._num];
    return os;
  }

  inline bool operator==(const GaloisInt& rhs) const {
    return _num == rhs._num;
  }

  inline bool operator!=(const GaloisInt& rhs) const {
    return _num != rhs._num;
  }

  inline bool operator>=(const int& v) const {
    return _num >= v;
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

static bool contains(std::vector<int> arr, int v) {
  for (int i = 0; i < arr.size(); i++) {
    if (arr[i] == v) return true;
  }
  return false;
}

// Galois Field specific polynomials
// TODO: refactor this and optimise this, please
//       frankly, revise this entire file
class Polynomial {
public:
  Polynomial(std::vector<GaloisInt> coefficients) {
    _coefficients = coefficients;
    int degree = _coefficients.size();
    for (int i = degree - 1; i >= 0; i--) {
      _term_exponents.push_back(i);
    }
  }

  // In the form `a ** n`
  Polynomial(std::vector<int> exponents) {
    for (int e : exponents) {
      _coefficients.push_back(GaloisInt(galoisValueLogs[e]));
    }
    int degree = _coefficients.size();
    for (int i = degree - 1; i >= 0; i--) {
      _term_exponents.push_back(i);
    }
  }

  friend Polynomial operator*(Polynomial& lhs, const Polynomial& rhs) {
    int lhs_size = lhs._coefficients.size();
    int rhs_size = rhs._coefficients.size();

    // Create a new polynomial, multiplying each term on the left by each term on the right
    std::vector<int> new_terms;
    std::vector<GaloisInt> new_coefficients;
    for (int i = 0; i < lhs_size; i++) {
      for (int j = 0; j < rhs_size; j++) {
        new_coefficients.push_back(lhs._coefficients[i] * rhs._coefficients[j]);
        new_terms.push_back(lhs._term_exponents[i] + rhs._term_exponents[j]);
      }
    }

    // Combine like terms, forming a new polynomial again
    std::vector<int> combined_terms;
    std::vector<GaloisInt> combined_coefficients;
    std::vector<int> skip;
    for (int i = 0; i < new_coefficients.size(); i++) {
      if (contains(skip, i)) continue; // Skip if it's a like term we've already combined

      GaloisInt g = new_coefficients[i];
      int term = new_terms[i];
      // Search for like terms
      for (int j = 0; j < new_coefficients.size(); j++) {
        if (new_terms[j] == term && j != i) {
          g = g + new_coefficients[j]; // Add like term
          skip.push_back(j);
        }
      }
      combined_terms.push_back(term);
      combined_coefficients.push_back(g);
    }

    return Polynomial(combined_coefficients);
  }

  friend std::ostream& operator<<(std::ostream& os, const Polynomial& p) {
    for (int i = 0; i < p._coefficients.size(); i++) {
      if (i != 0) os << " + "; // Coefficients are always positive
      os << p._coefficients[i] << "x" << p._term_exponents[i];
    }
    return os;
  }

  inline bool operator==(const Polynomial& rhs) const {
    for (int i = 0; i < _coefficients.size(); i++) {
      if (_coefficients[i] != rhs._coefficients[i]) return false;
      if (_term_exponents[i] != rhs._term_exponents[i]) return false;
    }
    return true;
  }
private:
  std::vector<GaloisInt> _coefficients;
  std::vector<int> _term_exponents;
};