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

BitStream encodeAlphaNumeric(std::string input) {
  BitStream bits;
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

BitStream encodeNumeric(std::string input) {
  BitStream bitset;
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

BitStream encodeByteMode(std::string input) {
  BitStream bitset;
  const char *bytes = input.c_str();
  for (int i = 0; i < input.length(); i++) {
    for (int j = 7; j >= 0; j--) {
      bool bit = (bytes[i] & (1 << j)) >> j;
      bitset.append(bit);
    }
  }
  return bitset;
}

BitStream encodeData(std::string input, int inputLength, int version, ErrorCorrection level, EncodingMode mode) {
  std::string modeIndicators[3] = {"0001", "0010", "0100"};
  BitStream indicator(modeIndicators[mode]);

  BitStream data;
  if (mode == EncodingMode::NUMERIC) {
    data = encodeNumeric(input);
  } else if (mode == EncodingMode::ALPHA_NUMERIC) {
    data = encodeAlphaNumeric(input);
  } else {
    data = encodeByteMode(input);
  }

  // Pad the character indicator bits to meet the required length
  int length = 0;
  if (version >= 1 && version <= 9) {
    length = characterIndicatorLengths[0][mode];
  } else if (version >= 10 && version <= 26) {
    length = characterIndicatorLengths[1][mode];
  } else if (version >= 27 && version <= 40) {
    length = characterIndicatorLengths[2][mode];
  }
  BitStream bitCount(inputLength);
  bitCount.padLeft(length);

  BitStream bits = indicator + bitCount + data;
  int requiredLength = errorCorrectionInfo[version - 1][level][0] * 8;

  // Add a terminator of at most 4 bits
  int terminatorLength = 4;
  while (bits.length() < requiredLength && terminatorLength > 0) {
    bits.append(0);
    terminatorLength --;
  }

  // Pad to ensure the bitstream is a multiple of 8
  int target = nextMultiple(bits.length(), 8);
  while (bits.length() < target) {
    bits.append(0);
  }

  // Pad with alternating byte to ensure the bitstream
  // is the required length
  int remaining = (requiredLength - bits.length()) / 8;
  for (int i = 0; i < remaining; i++) {
    int num = i % 2 == 0 ? 236 : 17;
    BitStream pad(num);
    bits = bits + pad;
  }

  return bits;
}

class QR {
public:
  QR(std::string input, ErrorCorrection level);
  void create();
private:
  std::string mInput;
  int mInputLength;
  EncodingMode mMode;
  ErrorCorrection mLevel;
  int mVersion;
  int mMatrixSize;
};

QR::QR(std::string input, ErrorCorrection level) {
  mInput = input;
  mLevel = level;
  mInputLength = utf8::distance(input.begin(), input.end());
  mMode = getOptimalEncodingScheme(mInput);
  mVersion = findSmallestVersion(mInputLength, mLevel, mMode);
  mMatrixSize = 25 + 4 * (mVersion - 1);
}

void QR::create() {}

int main() {}