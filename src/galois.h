#pragma once

#include <cstdint>
#include <map>
#include <ostream>
#include <vector>

// The Galois Field is a finite field of numbers.
// The QR specification uses a Galois Field with 256 elements -- GF(256).
// Which means that numbers in the field are between 0 and 255 (inclusive).
namespace galois {

// Log table for values in the Galois 256 field
// Each value in the field can be thought of as `v = 2 ** n`
// So, this table gives the value of v for each value of n
// Index by n
static const uint8_t galoisValueLogs[256] = {
    1,   2,   4,   8,   16,  32,  64,  128, 29,  58,  116, 232, 205, 135, 19,
    38,  76,  152, 45,  90,  180, 117, 234, 201, 143, 3,   6,   12,  24,  48,
    96,  192, 157, 39,  78,  156, 37,  74,  148, 53,  106, 212, 181, 119, 238,
    193, 159, 35,  70,  140, 5,   10,  20,  40,  80,  160, 93,  186, 105, 210,
    185, 111, 222, 161, 95,  190, 97,  194, 153, 47,  94,  188, 101, 202, 137,
    15,  30,  60,  120, 240, 253, 231, 211, 187, 107, 214, 177, 127, 254, 225,
    223, 163, 91,  182, 113, 226, 217, 175, 67,  134, 17,  34,  68,  136, 13,
    26,  52,  104, 208, 189, 103, 206, 129, 31,  62,  124, 248, 237, 199, 147,
    59,  118, 236, 197, 151, 51,  102, 204, 133, 23,  46,  92,  184, 109, 218,
    169, 79,  158, 33,  66,  132, 21,  42,  84,  168, 77,  154, 41,  82,  164,
    85,  170, 73,  146, 57,  114, 228, 213, 183, 115, 230, 209, 191, 99,  198,
    145, 63,  126, 252, 229, 215, 179, 123, 246, 241, 255, 227, 219, 171, 75,
    150, 49,  98,  196, 149, 55,  110, 220, 165, 87,  174, 65,  130, 25,  50,
    100, 200, 141, 7,   14,  28,  56,  112, 224, 221, 167, 83,  166, 81,  162,
    89,  178, 121, 242, 249, 239, 195, 155, 43,  86,  172, 69,  138, 9,   18,
    36,  72,  144, 61,  122, 244, 245, 247, 243, 251, 235, 203, 139, 11,  22,
    44,  88,  176, 125, 250, 233, 207, 131, 27,  54,  108, 216, 173, 71,  142,
    1};

// Antilog table for values in the Galois 256 field
// Each value in the field can be thought of as `v = 2 ** n`
// So, this table gives the value of n for each value of v
// Index by v
static const uint8_t galoisValueAntilogs[256] = {
    0,   0,   1,   25,  2,   50,  26,  198, 3,   223, 51,  238, 27,  104, 199,
    75,  4,   100, 224, 14,  52,  141, 239, 129, 28,  193, 105, 248, 200, 8,
    76,  113, 5,   138, 101, 47,  225, 36,  15,  33,  53,  147, 142, 218, 240,
    18,  130, 69,  29,  181, 194, 125, 106, 39,  249, 185, 201, 154, 9,   120,
    77,  228, 114, 166, 6,   191, 139, 98,  102, 221, 48,  253, 226, 152, 37,
    179, 16,  145, 34,  136, 54,  208, 148, 206, 143, 150, 219, 189, 241, 210,
    19,  92,  131, 56,  70,  64,  30,  66,  182, 163, 195, 72,  126, 110, 107,
    58,  40,  84,  250, 133, 186, 61,  202, 94,  155, 159, 10,  21,  121, 43,
    78,  212, 229, 172, 115, 243, 167, 87,  7,   112, 192, 247, 140, 128, 99,
    13,  103, 74,  222, 237, 49,  197, 254, 24,  227, 165, 153, 119, 38,  184,
    180, 124, 17,  68,  146, 217, 35,  32,  137, 46,  55,  63,  209, 91,  149,
    188, 207, 205, 144, 135, 151, 178, 220, 252, 190, 97,  242, 86,  211, 171,
    20,  42,  93,  158, 132, 60,  57,  83,  71,  109, 65,  162, 31,  45,  67,
    216, 183, 123, 164, 118, 196, 23,  73,  236, 127, 12,  111, 246, 108, 161,
    59,  82,  41,  157, 85,  170, 251, 96,  134, 177, 187, 204, 62,  90,  203,
    89,  95,  176, 156, 169, 160, 81,  11,  245, 22,  235, 122, 117, 44,  215,
    79,  174, 213, 233, 230, 231, 173, 232, 116, 214, 244, 234, 168, 80,  88,
    175};

// A number within GF(256)
class Int {
 public:
  Int() { _num = 0; }

  Int(uint8_t num) : _num(num) {}

  int exponent() const { return galoisValueAntilogs[_num]; }

  friend std::ostream& operator<<(std::ostream& os, const Int& g) {
    os << "a" << galoisValueAntilogs[g._num];
    return os;
  }

  inline bool operator==(const Int& rhs) const { return _num == rhs._num; }

  inline bool operator!=(const Int& rhs) const { return _num != rhs._num; }

  // Addition and subtraction is the Galois Field is done by
  // XORing the 2 numbers together
  friend Int operator+(Int& lhs, const Int& rhs) { return lhs._num ^ rhs._num; }

  Int operator+=(const Int& rhs) {
    *this = *this + rhs;
    return *this;
  }

  // Convert the ints into exponential form, add the exponents
  // then convert back into int form
  friend Int operator*(Int& lhs, const Int& rhs) {
    int sum = galoisValueAntilogs[lhs._num] + galoisValueAntilogs[rhs._num];
    if (sum > 255)
      sum %= 255;  // Wrap around
    return galoisValueLogs[sum];
  }

 private:
  uint8_t _num;
};

// A polynomial within GF(256)
class Polynomial {
 public:
  // Constructs the polynomial from a vector of coefficient exponents.
  // For example, {0, 1, 3} will create the polynomial a0x2 a1x1 a3x0
  Polynomial(std::vector<int> exponents) {
    for (int c : exponents) {
      _coefficients.push_back(Int(galoisValueLogs[c]));
    }
  }

  // Constructs the polynomial from a vector of coefficients directly.
  Polynomial(std::vector<Int> coefficients) : _coefficients(coefficients) {}

  // Multiply each term on the left to each term on the right
  // whilst combining like terms
  friend Polynomial operator*(Polynomial& lhs, const Polynomial& rhs) {
    std::map<int, Int> nomials;
    int lhs_degree = lhs._coefficients.size() - 1;
    for (Int x : lhs._coefficients) {
      int rhs_degree = rhs._coefficients.size() - 1;
      for (Int y : rhs._coefficients) {
        Int coefficient = x * y;
        int term = lhs_degree + rhs_degree;
        nomials[term] += coefficient;  // Add like terms
        rhs_degree -= 1;
      }
      lhs_degree -= 1;
    }

    std::vector<int> new_coefficients;
    for (auto const& [key, val] : nomials) {
      int exponent = val.exponent();
      new_coefficients.insert(new_coefficients.begin(), exponent);
    }

    return Polynomial(new_coefficients);
  }

  friend Polynomial operator+(Polynomial& lhs, Polynomial& rhs) {
    std::vector<Int> coefficients;
    int lhsSize = lhs._coefficients.size();
    int rhsSize = rhs._coefficients.size();
    int max = std::max(lhsSize, rhsSize);
    for (int i = 0; i < max; i++) {
      Int left = i >= lhsSize ? 0 : lhs._coefficients[i];
      Int right = i >= rhsSize ? 0 : rhs._coefficients[i];
      coefficients.push_back(left + right);
    }
    return Polynomial(coefficients);
  }

  friend std::ostream& operator<<(std::ostream& os, const Polynomial& p) {
    int degree = p._coefficients.size() - 1;
    for (int i = 0; i < p._coefficients.size(); i++) {
      if (i > 0) {
        os << " + ";  // Numbers in GF(256) are always positive
      }
      os << p._coefficients[i] << "x" << degree;
      degree -= 1;
    }
    return os;
  }

  inline bool operator==(const Polynomial& rhs) const {
    if (_coefficients.size() != rhs._coefficients.size()) {
      return false;
    }

    for (int i = 0; i < _coefficients.size(); i++) {
      if (_coefficients[i] != rhs._coefficients[i]) {
        return false;
      }
    }

    return true;
  }

  // Create a generator polynomial for a specific number
  // of error codewords
  static Polynomial createGenerator(int numErrorWords) {
    std::vector<int> exponents = {0, 0};
    Polynomial generator(exponents);
    for (int i = 1; i < numErrorWords; i++) {
      exponents = {0, i};
      Polynomial multiplier(exponents);
      generator = generator * multiplier;
    }
    return generator;
  }

  Polynomial firstTerm() { return Polynomial({_coefficients[0]}); }

  void removeFirstTerm() { _coefficients.erase(_coefficients.begin() + 0); }

  // Convert the coefficients from numbers in GF(256) to regular numbers
  std::vector<uint8_t> getCoefficients() {
    std::vector<uint8_t> nums;
    for (Int c : _coefficients) {
      nums.push_back(galoisValueLogs[c.exponent()]);
    }
    return nums;
  }

 private:
  std::vector<Int> _coefficients;
};

};  // namespace galois