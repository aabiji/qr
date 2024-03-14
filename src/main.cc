#include <iostream>
#include <cmath>
#include <cassert>

#include "utils.h"
#include "tables.h"

EncodingScheme getOptimalEncodingScheme(std::string str) {
  int numCount = 0;
  int lowerCaseCount = 0;
  int specialCharCount = 0;
  uint32_t special[] = {' ', '$', '%', '*', '+', '-', '.', '/', ':'};

  for (auto it = str.begin(); it != str.end();) {
    uint32_t codepoint = utf8::next(it, str.end());
    if (codepoint >= 48 && codepoint <= 57) {
      numCount += 1;
    } else if ((codepoint >= 65 && codepoint <= 90) ||
               contains(special, codepoint, 9)) {
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

BitSet encodeAlphaNumeric(std::string input) {
  BitSet bits;
  int size = input.length();

  for (int i = 0; i < size; i += 2) {
    int numBits = 11;
    uint16_t value = 0;
    if (i + 1 < size) {
      // Compute pair of characters
      int first = alphaNumericValues.at(input[i]);
      int last = alphaNumericValues.at(input[i + 1]);
      value = 45 * first + last;
    } else {
      value = alphaNumericValues.at(input[i]);
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

BitSet encodeByteMode(std::string input) {
  BitSet bitset;
  const char *bytes = input.c_str();
  for (int i = 0; i < input.length(); i++) {
    for (int j = 7; j >= 0; j--) {
      bool bit = (bytes[i] & (1 << j)) >> j;
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
  int mInputSize;
};

QR::QR(std::string input) {
  mInput = input;
  mInputSize = utf8::distance(input.begin(), input.end());
}

void QR::create() {
  BitSet bits;
  EncodingScheme scheme = getOptimalEncodingScheme(mInput);
  switch (scheme) {
    case EncodingScheme::NUMERIC:
      bits = encodeNumeric(mInput);
      break;
    case EncodingScheme::ALPHA_NUMERIC:
      bits = encodeAlphaNumeric(mInput);
      break;
    case EncodingScheme::BYTE:
      bits = encodeByteMode(mInput);
      break;
  }
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

  BitSet bits6 = encodeByteMode("Hello");
  assert(bits6.toString() == "0100100001100101011011000110110001101111");

  QR qr1("HELLO");
  qr1.create();

  QR qr2("🤡");
  qr2.create();
}

int main() {
  test();
}