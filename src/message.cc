#include <cmath>
#include <cstdint>

#include <utf8.h>

#include "bitstream.h"
#include "galois.h"
#include "message.h"

template <class T>
bool contains(T* arr, T v, int size) {
  for (int i = 0; i < size; i++) {
    if (arr[i] == v)
      return true;
  }
  return false;
}

// Find the smallest qr version that will fit the data
void Message::findSmallestVersion() {
  _qrVersion = 1;
  int size = _input.length();
  while (true) {
    int capacity = characterCapacities[_qrVersion - 1][_level][_mode];
    if (size < capacity) {
      break;
    }
    _qrVersion += 1;
    size -= capacity;
  }
}

void Message::getOptimalEncodingMode() {
  int numCount = 0;
  int lowerCaseCount = 0;
  int specialCharCount = 0;
  uint32_t special[] = {' ', '$', '%', '*', '+', '-', '.', '/', ':'};

  for (auto it = _input.begin(); it != _input.end();) {
    uint32_t codepoint = utf8::next(it, _input.end());
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
    _mode = EncodingMode::BYTE;
  } else if (numCount == (numCount + specialCharCount)) {
    _mode = EncodingMode::NUMERIC;
  } else {
    _mode = EncodingMode::ALPHA_NUMERIC;
  }
}

BitStream Message::encodeAlphaNumeric() {
  BitStream stream;
  int size = _input.length();

  for (int i = 0; i < size; i += 2) {
    int numBits = 11;
    uint16_t value = 0;
    if (i + 1 < size) {
      // Compute pair of characters
      int first = alphaNumericValues.at(_input[i]);
      int last = alphaNumericValues.at(_input[i + 1]);
      value = 45 * first + last;
    } else {
      value = alphaNumericValues.at(_input[i]);
      numBits = 6;
    }

    // Write bits
    for (int j = numBits - 1; j >= 0; j--) {
      bool bit = (value & (1 << j)) >> j;
      stream.bits.push_back(bit);
    }
  }

  return stream;
}

BitStream Message::encodeNumeric() {
  BitStream stream;
  int size = _input.length();

  for (int i = 0; i < size; i += 3) {
    // The substring length should be 3 or less
    int length = i + 3 < size ? 3 : size - i;

    // Convert the substring into int
    uint16_t value = 0;
    for (int j = 0; j < length; j++) {
      int digit = _input[i + j] - 48;
      int exponent = std::abs(j - (length - 1));
      value += digit * std::pow(10, exponent);
    }

    // Write bits
    for (int j = 3 * length; j >= 0; j--) {
      bool bit = (value & (1 << j)) >> j;
      stream.bits.push_back(bit);
    }
  }

  return stream;
}

BitStream Message::encodeByteMode() {
  BitStream stream;
  const char* bytes = _input.c_str();
  for (int i = 0; i < _input.length(); i++) {
    for (int j = 7; j >= 0; j--) {
      bool bit = (bytes[i] & (1 << j)) >> j;
      stream.bits.push_back(bit);
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

BitStream Message::process() {
  int inputLength = utf8::distance(_input.begin(), _input.end());

  // 0001 for EncodingMode::NUMERIC, 0010 for EncodingMode::ALPHA_NUMERIC, 0100
  // for EncodingMode::BYTE
  BitStream indicator;
  indicator.appendByte(std::max(1, _mode * 2), 4);

  BitStream data;
  if (_mode == EncodingMode::NUMERIC) {
    data = encodeNumeric();
  } else if (_mode == EncodingMode::ALPHA_NUMERIC) {
    data = encodeAlphaNumeric();
  } else {
    data = encodeByteMode();
  }

  // Pad the character indicator bits to meet the required length
  int length = 0;
  if (_qrVersion >= 1 && _qrVersion <= 9) {
    length = characterIndicatorLengths[0][_mode];
  } else if (_qrVersion >= 10 && _qrVersion <= 26) {
    length = characterIndicatorLengths[1][_mode];
  } else if (_qrVersion >= 27 && _qrVersion <= 40) {
    length = characterIndicatorLengths[2][_mode];
  }
  BitStream bitCount(inputLength);
  bitCount.padLeft(length);

  BitStream bits = indicator + bitCount + data;
  int requiredLength = dataInfo[_qrVersion - 1][_level][0] * 8;

  // Add a terminator of at most 4 bits
  int terminatorLength = 4;
  while (bits.bits.size() < requiredLength && terminatorLength > 0) {
    bits.bits.push_back(0);
    terminatorLength--;
  }

  // Pad to ensure the bitstream is a multiple of 8
  int target = nextMultiple(bits.bits.size(), 8);
  while (bits.bits.size() < target) {
    bits.bits.push_back(0);
  }

  // Pad with alternating byte to ensure the bitstream
  // is the required length
  int remaining = (requiredLength - bits.bits.size()) / 8;
  for (int i = 0; i < remaining; i++) {
    int num = i % 2 == 0 ? 236 : 17;
    BitStream pad(num);
    bits = bits + pad;
  }

  return bits;
}

std::vector<uint8_t> Message::generateCorrectionCodes(int blockStart,
                                                      int blockEnd) {
  auto block = std::vector<uint8_t>(_encodedData.begin() + blockStart,
                                    _encodedData.begin() + blockEnd);

  std::vector<galois::Int> coefficients;
  for (uint8_t byte : block) {
    coefficients.push_back(galois::Int(byte));
  }
  galois::Polynomial message(coefficients);
  int numCoefficients = coefficients.size();

  int numErrorCodes = dataInfo[_qrVersion - 1][_level][1];
  galois::Polynomial generator =
      galois::Polynomial::createGenerator(numErrorCodes);

  // Divide the message polynomial by the generator polynomial
  for (int i = 0; i < numCoefficients; i++) {
    galois::Polynomial result = generator * message.firstTerm();
    message = result + message;
    message.removeFirstTerm();
  }

  // The error correction codes are the remainder
  // of the division
  return message.getCoefficients();
}

void Message::interleaveData() {
  int numErrorCodes = dataInfo[_qrVersion - 1][_level][1];
  int group1Blocks = dataInfo[_qrVersion - 1][_level][2];
  int group2Blocks = dataInfo[_qrVersion - 1][_level][4];
  int group1BlockSize = dataInfo[_qrVersion - 1][_level][3];
  int group2BlockSize = dataInfo[_qrVersion - 1][_level][5];

  int start = 0;
  // Generate error correction codes for each block in the first group
  std::vector<uint8_t> group1ErrorCodes[group1Blocks] = {};
  for (int block = 0; block < group1Blocks; block++) {
    group1ErrorCodes[block] =
        generateCorrectionCodes(start, start + group1BlockSize);
    start += group1BlockSize;
  }

  // Generate error correction codes for each block in the second group
  std::vector<uint8_t> group2ErrorCodes[group2Blocks] = {};
  for (int block = 0; block < group2Blocks; block++) {
    int end = start + group2BlockSize;
    group2ErrorCodes[block] =
        generateCorrectionCodes(start, start + group2BlockSize);
    start += group2BlockSize;
  }

  int upTo = std::max(group1BlockSize, group2BlockSize);
  for (int i = 0; i < upTo; i++) {
    if (i < group1BlockSize) {
      int group1Start = 0;
      for (int j = 0; j < group1Blocks; j++) {
        _preparedData.appendByte(_encodedData[group1Start + i]);
        group1Start += group1BlockSize;
      }
    }

    if (i < group2BlockSize) {
      int group2Start = group1BlockSize * group1Blocks;
      for (int j = 0; j < group2Blocks; j++) {
        _preparedData.appendByte(_encodedData[group2Start + i]);
        group2Start += group2BlockSize;
      }
    }
  }

  for (int i = 0; i < numErrorCodes; i++) {
    for (int j = 0; j < group1Blocks; j++) {
      _preparedData.appendByte(group1ErrorCodes[j][i]);
    }
    for (int j = 0; j < group2Blocks; j++) {
      _preparedData.appendByte(group2ErrorCodes[j][i]);
    }
  }
}

BitStream Message::encode() {
  if (_encodedData.size() == 0) {
    _encodedData = process().toBytes();
  }

  int group1Blocks = dataInfo[_qrVersion - 1][_level][2];
  int group2Blocks = dataInfo[_qrVersion - 1][_level][4];
  int totalBlocks = group1Blocks + group2Blocks;
  if (totalBlocks > 1) {
    interleaveData();
  } else {
    auto errorCodes = generateCorrectionCodes(0, _encodedData.size());
    for (uint8_t byte : _encodedData) {
      _preparedData.appendByte(byte);
    }
    for (uint8_t code : errorCodes) {
      _preparedData.appendByte(code);
    }
  }

  return _preparedData;
}