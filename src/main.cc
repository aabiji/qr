#include <iostream>
#include <cmath>
#include <cassert>

#include "utils.h"
#include "tables.h"

EncodingMode getOptimalEncodingScheme(std::string str) {
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
    return EncodingMode::BYTE;
  } else if (numCount == (numCount + specialCharCount)) {
    return EncodingMode::NUMERIC;
  }
  return EncodingMode::ALPHA_NUMERIC;
}

int findSmallestVersion(int count, ErrorCorrection l, EncodingMode m) {
  int version = 0;
  while (true) {
    int capacity = characterCapacities[version][l][m];
    if (count < capacity) {
      break;
    }
    version ++;
    count -= capacity;
  }
  return version + 1;
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
  QR(std::string input, ErrorCorrection level);
  void create();
private:
  std::string mInput;
  int mCharCount;
  EncodingMode mMode;
  ErrorCorrection mLevel;
  int mVersion;
  int mSize;

  void encodeData();
};

QR::QR(std::string input, ErrorCorrection level) {
  mInput = input;
  mLevel = level;
  mCharCount = utf8::distance(input.begin(), input.end());
  mMode = getOptimalEncodingScheme(mInput);
  mVersion = findSmallestVersion(mCharCount, mLevel, mMode);
  mSize = 25 + 4 * (mVersion - 1);
}

void QR::encodeData() {
  BitSet data;
  switch (mMode) {
    case EncodingMode::NUMERIC:
      data = encodeNumeric(mInput);
      break;
    case EncodingMode::ALPHA_NUMERIC:
      data = encodeAlphaNumeric(mInput);
      break;
    case EncodingMode::BYTE:
      data = encodeByteMode(mInput);
      break;
  }

  // Mode indicator bits for the encoding mode used
  std::string modeIndicators[3] = {"0001", "0010", "0100"};
  BitSet indicator(modeIndicators[mMode]);

  // Character count bits
  int index = 0;
  if (mVersion >= 1 && mVersion <= 9) {
    index = 0;
  } else if (mVersion >= 10 && mVersion <= 26) {
    index = 1;
  } else if (mVersion >= 27 && mVersion <= 40) {
    index = 2;
  }
  int length = characterIndicatorLengths[index][mMode];
  BitSet count(mCharCount);
  count.padLeft(length);

  BitSet bits = indicator + count + data;
  int maxBits = errorCorrectionInfo[mVersion - 1][mLevel][0] * 8;

  // The terminator can be at most 4 bits
  int terminatorLength = 4;
  if (bits.length() + terminatorLength > maxBits) {
    terminatorLength = (bits.length() + terminatorLength) - maxBits;
  }
  for (int i = 0; i < terminatorLength; i++) {
    bits.append(0);
  }

  // Pad to ensure the bitset is a multiple of 8
  int needed = nextMultiple(bits.length(), 8) - bits.length();
  for (int i = 0; i < needed; i++) {
    bits.append(0);
  }

  int remaining = (maxBits - bits.length()) / 8;
  // Pad the bitstream with alternating bytes to make the bitset the required size
  for (int i = 0; i < remaining; i++) {
    if (i % 2 == 0) {
      BitSet pad(236);
      bits = bits + pad;
    } else {
      BitSet pad(17);
      bits = bits + pad;
    }
  }
}

void QR::create() {
  encodeData();
}

void test() {
  EncodingMode s1 = getOptimalEncodingScheme("123");
  assert(s1 == EncodingMode::NUMERIC);

  EncodingMode s2 = getOptimalEncodingScheme("hello");
  assert(s2 == EncodingMode::BYTE);

  EncodingMode s3 = getOptimalEncodingScheme("YO!");
  assert(s3 == EncodingMode::BYTE);

  EncodingMode s4 = getOptimalEncodingScheme("Yo123");
  assert(s4 == EncodingMode::BYTE);

  EncodingMode s5 = getOptimalEncodingScheme("HELLO WORLD");
  assert(s5 == EncodingMode::ALPHA_NUMERIC);

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

  QR qr1("HELLO WORLD", ErrorCorrection::QUANTILE);
  qr1.create();

  QR qr2("🤡", ErrorCorrection::LOW);
  qr2.create();

  QR qr3("abcdefghijklmopqrstuvwxyz", ErrorCorrection::LOW);
  qr3.create();
}

int main() {
  test();
}