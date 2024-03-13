#include <iostream>
#include <string>
#include <cstdint>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <cassert>

template <class T>
bool contains(T* arr, T v, int size) {
  for (int i = 0; i < size; i++) {
    if (arr[i] == v) return true;
  }
  return false;
}

// Dynamically sized qr specific bitset
class BitSet {
public:
  void append(bool bit);
  std::string toString();
  std::vector<uint8_t> toStream();
private:
  std::vector<bool> bits;
};

void BitSet::append(bool bit) {
  bits.push_back(bit);
}

std::string BitSet::toString() {
  std::string str = "";
  for (bool b : bits) {
    str += b ? "1" : "0";
  }
  return str;
}

std::vector<uint8_t> BitSet::toStream() {
  std::vector<uint8_t> empty;
  return empty;
}

enum EncodingScheme {
  NUMERIC = 1,
  ALPHA_NUMERIC = 2,
  BYTE = 3,
};

EncodingScheme getOptimalEncodingScheme(std::string str) {
  int numCount = 0;
  int lowerCaseCount = 0;
  int specialCharCount = 0;
  char special[] = {' ', '$', '%', '*', '+', '-', '.', '/', ':'};
  for (char c : str) {
    if (c >= 48 && c <= 57) {
      numCount += 1;
    } else if ((c >= 65 && c <= 90) ||
               contains(special, c, 9)) {
      specialCharCount += 1;
    } else {
      lowerCaseCount += 1;
    }
  }

  if (lowerCaseCount > 0) {
    return EncodingScheme::BYTE;
  } else if (numCount == (numCount + specialCharCount)) {
    return EncodingScheme::NUMERIC;
  }
  return EncodingScheme::ALPHA_NUMERIC;
}

static std::unordered_map<char, int> alphaNumericValues = {
  {'0',  0}, {'1',  1}, {'2',  2}, {'3',  3}, {'4',  4}, {'5',  5},
  {'6',  6}, {'7',  7}, {'8',  8}, {'9',  9}, {'A', 10}, {'B', 11},
  {'C', 12}, {'D', 13}, {'E', 14}, {'F', 15}, {'G', 16}, {'H', 17},
  {'I', 18}, {'J', 19}, {'K', 20}, {'L', 21}, {'M', 22}, {'N', 23},
  {'O', 24}, {'P', 25}, {'Q', 26}, {'R', 27}, {'S', 28}, {'T', 29},
  {'U', 30}, {'V', 31}, {'W', 32}, {'X', 33}, {'Y', 34}, {'Z', 35},
  {' ', 36}, {'$', 37}, {'%', 38}, {'*', 39}, {'+', 40}, {'-', 41},
  {'.', 42}, {'/', 43}, {':', 44}
};

BitSet encodeAlphaNumeric(std::string input) {
  BitSet bits;
  int size = input.length();

  for (int i = 0; i < size; i += 2) {
    int numBits = 11;
    uint16_t value = 0;
    if (i + 1 < size) {
      // Compute pair of characters
      int first = alphaNumericValues[input[i]];
      int last = alphaNumericValues[input[i + 1]];
      value = 45 * first + last;
    } else {
      value = alphaNumericValues[input[i]];
      numBits = 6;
    }

    // Write bits
    for (int j = numBits - 1; j >= 0; j--) {
      bool bit = (value & (1 << j)) >> j;
      bits.append(bit);
    }
  }

  return bits;
}

BitSet encodeNumeric(std::string input) {
  BitSet bitset;
  int size = input.length();

  for (int i = 0; i < size; i += 3) {
    // The substring length should be 3 or less
    int length = i + 3 < size ? 3 : size - i;

    // Convert the substring into int
    uint16_t value = 0;
    for (int j = 0; j < length; j++) {
      int digit = input[i + j] - 48;
      int exponent = std::abs(j - (length - 1));
      value += digit * std::pow(10, exponent);
    }

    // Write bits
    for (int j = 3 * length; j >= 0; j--) {
      bool bit = (value & (1 << j)) >> j;
      bitset.append(bit);
    }
  }

  return bitset;
}

class QR {
public:
  QR(std::string input);
  void create();
private:
  std::string mInput;
};

QR::QR(std::string input) {
  mInput = input;
}

void QR::create() {
  BitSet bits;
  EncodingScheme scheme = getOptimalEncodingScheme(mInput);
  if (scheme == EncodingScheme::NUMERIC) {
    bits = encodeNumeric(mInput);
  }
  bits.toString();
}

void test() {
  EncodingScheme s1 = getOptimalEncodingScheme("123");
  assert(s1 == EncodingScheme::NUMERIC);

  EncodingScheme s2 = getOptimalEncodingScheme("hello");
  assert(s2 == EncodingScheme::BYTE);

  EncodingScheme s3 = getOptimalEncodingScheme("YO!");
  assert(s3 == EncodingScheme::BYTE);

  EncodingScheme s4 = getOptimalEncodingScheme("Yo123");
  assert(s4 == EncodingScheme::BYTE);

  EncodingScheme s5 = getOptimalEncodingScheme("HELLO WORLD");
  assert(s5 == EncodingScheme::ALPHA_NUMERIC);

  BitSet bits1 = encodeNumeric("8675309");
  assert(bits1.toString() == "110110001110000100101001");

  BitSet bits2 = encodeNumeric("291");
  assert(bits2.toString() == "0100100011");

  BitSet bits3 = encodeNumeric("76");
  assert(bits3.toString() == "1001100");

  BitSet bits4 = encodeNumeric("4");
  assert(bits4.toString() == "0100");

  BitSet bits5 = encodeAlphaNumeric("HELLO WORLD");
  assert(bits5.toString() == "0110000101101111000110100010111001011011100010011010100001101");
}

int main() {
  test();
}