#include <iostream>
#include <string>
#include <cstdint>
#include <cmath>
#include <vector>
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

  EncodingScheme s4 = getOptimalEncodingScheme("YO123");
  assert(s4 == EncodingScheme::ALPHA_NUMERIC);

  EncodingScheme s5 = getOptimalEncodingScheme("HELLO HELLO");
  assert(s5 == EncodingScheme::ALPHA_NUMERIC);

  BitSet bits = encodeNumeric("8675309");
  assert(bits.toString() == "110110001110000100101001");
}

int main() {
  test();
}