#include <cmath>
#include <cstdint>

#include <utf8.h>

#include "bitstream.h"
#include "encoder.h"

template <class T>
bool contains(T* arr, T v, int size) {
  for (int i = 0; i < size; i++) {
    if (arr[i] == v) return true;
  }
  return false;
}

EncodingMode Encoder::getOptimalEncodingMode() {
  int numCount = 0;
  int lowerCaseCount = 0;
  int specialCharCount = 0;
  uint32_t special[] = {' ', '$', '%', '*', '+', '-', '.', '/', ':'};

  for (auto it = input_.begin(); it != input_.end();) {
    uint32_t codepoint = utf8::next(it, input_.end());
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

BitStream Encoder::encodeAlphaNumeric() {
  BitStream stream;
  int size = input_.length();

  for (int i = 0; i < size; i += 2) {
    int numBits = 11;
    uint16_t value = 0;
    if (i + 1 < size) {
      // Compute pair of characters
      int first = alphaNumericValues.at(input_[i]);
      int last = alphaNumericValues.at(input_[i + 1]);
      value = 45 * first + last;
    } else {
      value = alphaNumericValues.at(input_[i]);
      numBits = 6;
    }

    // Write bits
    for (int j = numBits - 1; j >= 0; j--) {
      bool bit = (value & (1 << j)) >> j;
      stream.append(bit);
    }
  }

  return stream;
}

BitStream Encoder::encodeNumeric() {
  BitStream stream;
  int size = input_.length();

  for (int i = 0; i < size; i += 3) {
    // The substring length should be 3 or less
    int length = i + 3 < size ? 3 : size - i;

    // Convert the substring into int
    uint16_t value = 0;
    for (int j = 0; j < length; j++) {
      int digit = input_[i + j] - 48;
      int exponent = std::abs(j - (length - 1));
      value += digit * std::pow(10, exponent);
    }

    // Write bits
    for (int j = 3 * length; j >= 0; j--) {
      bool bit = (value & (1 << j)) >> j;
      stream.append(bit);
    }
  }

  return stream;
}

BitStream Encoder::encodeByteMode() {
  BitStream stream;
  const char *bytes = input_.c_str();
  for (int i = 0; i < input_.length(); i++) {
    for (int j = 7; j >= 0; j--) {
      bool bit = (bytes[i] & (1 << j)) >> j;
      stream.append(bit);
    }
  }
  return stream;
}

// Get the next multiple of a number after start number
int nextMultiple(int start, int multiple) {
  if (start % multiple) {
    return start + (multiple - start % multiple);
  }
  return start;
}

BitStream Encoder::encode(ErrorCorrection level, int version) {
  EncodingMode mode = getOptimalEncodingMode();
  int inputLength = utf8::distance(input_.begin(), input_.end());

  std::string modeIndicators[3] = {"0001", "0010", "0100"};
  BitStream indicator(modeIndicators[mode]);

  BitStream data;
  if (mode == EncodingMode::NUMERIC) {
    data = encodeNumeric();
  } else if (mode == EncodingMode::ALPHA_NUMERIC) {
    data = encodeAlphaNumeric();
  } else {
    data = encodeByteMode();
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