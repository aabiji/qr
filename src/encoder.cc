#include <cmath>
#include <cstdint>

#include <utf8.h>

#include "bitstream.h"
#include "encoder.h"
#include "galois.h"

Encoder::Encoder(std::string input, ErrorCorrection level) {
  _input = input;
  _level = level;

  getOptimalEncodingMode();
  findSmallestVersion();
  getGroupInfo();
}

int Encoder::getQrVersion() {
  return _version;
}

template <class T>
bool contains(T* arr, T v, int size) {
  for (int i = 0; i < size; i++) {
    if (arr[i] == v)
      return true;
  }
  return false;
}

// Find the smallest qr version that will fit the data
void Encoder::findSmallestVersion() {
  _version = 1;
  int size = _input.length();
  while (true) {
    int capacity = characterCapacities[_version - 1][_level][_mode];
    if (size < capacity) {
      break;
    }
    _version += 1;
    size -= capacity;
  }
}

void Encoder::getOptimalEncodingMode() {
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

BitStream Encoder::encodeAlphaNumeric() {
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

BitStream Encoder::encodeNumeric() {
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

BitStream Encoder::encodeByteMode() {
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

void Encoder::process() {
  int inputLength = utf8::distance(_input.begin(), _input.end());

  // The mode incator of 4 bits
  BitStream indicator;
  uint8_t modeIndicators[3] = {0b001, 0b0010, 0b0100};
  indicator.appendByte(modeIndicators[_mode], 4);

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
  if (_version >= 1 && _version <= 9) {
    length = characterIndicatorLengths[0][_mode];
  } else if (_version >= 10 && _version <= 26) {
    length = characterIndicatorLengths[1][_mode];
  } else if (_version >= 27 && _version <= 40) {
    length = characterIndicatorLengths[2][_mode];
  }
  BitStream bitCount(inputLength);
  bitCount.padLeft(length);

  BitStream processed = indicator + bitCount + data;
  int requiredLength = dataInfo[_version - 1][_level][0] * 8;

  // Add a terminator of at most 4 bits
  int terminatorLength = 4;
  while (processed.bits.size() < requiredLength && terminatorLength > 0) {
    processed.bits.push_back(0);
    terminatorLength--;
  }

  // Pad to ensure the bitstream is a multiple of 8
  int target = nextMultiple(processed.bits.size(), 8);
  while (processed.bits.size() < target) {
    processed.bits.push_back(0);
  }

  // Pad with alternating byte to ensure the bitstream
  // is the required length
  int remaining = (requiredLength - processed.bits.size()) / 8;
  for (int i = 0; i < remaining; i++) {
    int num = i % 2 == 0 ? 236 : 17;
    BitStream pad(num);
    processed = processed + pad;
  }

  _preparedData = processed.toBytes();
}

std::vector<uint8_t> Encoder::generateCorrectionCodes(int rangeStart,
                                                      int rangeEnd) {
  auto block = std::vector<uint8_t>(_preparedData.begin() + rangeStart,
                                    _preparedData.begin() + rangeEnd);

  std::vector<galois::Int> coefficients;
  for (uint8_t byte : block) {
    coefficients.push_back(galois::Int(byte));
  }
  galois::Polynomial message(coefficients);
  int numCoefficients = coefficients.size();

  int numErrorCodes = dataInfo[_version - 1][_level][1];
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

void Encoder::getGroupInfo() {
  // Group 1
  _groups[0].iter = 0;
  _groups[0].numBlocks = dataInfo[_version - 1][_level][2];
  _groups[0].blockSize = dataInfo[_version - 1][_level][3];

  // Group 2
  _groups[1].iter = 0;
  _groups[1].numBlocks = dataInfo[_version - 1][_level][4];
  _groups[1].blockSize = dataInfo[_version - 1][_level][5];
}

void Encoder::interleaveData() {
  // Index into blockCodes by group (0 or 1), then by block index, then by
  // specific byte index
  std::vector<std::vector<std::vector<uint8_t>>> blockCodes(2);

  // Generate error correction codes for each block in each group
  for (int i = 0; i < 2; i++) {
    _groups[i].iter = 0;
    if (i == 1) {  // Group 2 indexing
      _groups[i].iter = _groups[0].numBlocks * _groups[0].blockSize;
    }

    for (int j = 0; j < _groups[i].numBlocks; j++) {
      int size = _groups[i].blockSize;
      auto codes =
          generateCorrectionCodes(_groups[i].iter, _groups[i].iter + size);
      blockCodes[i].push_back(codes);
      _groups[i].iter += size;
    }
  }

  // Interleave data blocks:
  // Put the first byte from the first block,
  // Then the first byte from the second block,
  // Then the second byte from the first block,
  // Then the second byte from the second block,
  // And so on ...
  int maxSize = std::max(_groups[0].blockSize, _groups[1].blockSize);
  for (int i = 0; i < maxSize; i++) {
    _groups[0].iter = 0;
    _groups[1].iter = _groups[0].numBlocks * _groups[0].blockSize;
    for (int j = 0; j < 2; j++) {
      // Block sizes for the first and second groups might
      // not be the same...
      if (i >= _groups[j].blockSize) {
        continue;
      }

      for (int b = 0; b < _groups[j].numBlocks; b++) {
        int size = _groups[j].blockSize;
        uint8_t byte = _preparedData[_groups[j].iter + i];
        _encodedData.appendByte(byte);
        _groups[j].iter += size;
      }
    }
  }

  // Interleave error codes in the same way we interleaved data blocks
  int numErrorCodes = dataInfo[_version - 1][_level][1];
  for (int i = 0; i < numErrorCodes; i++) {
    for (int j = 0; j < 2; j++) {
      for (int n = 0; n < _groups[j].numBlocks; n++) {
        _encodedData.appendByte(blockCodes[j][n][i]);
      }
    }
  }
}

BitStream Encoder::encode() {
  if (_encodedData.bits.size() > 0) {
    return _encodedData; // Already encoded
  }

  process();
  int totalBlocks = _groups[0].numBlocks + _groups[1].numBlocks;

  // No need to interleave data if there's only one block
  if (totalBlocks == 1) {
    for (uint8_t byte : _preparedData) {
      _encodedData.appendByte(byte);
    }
    auto errorCodes = generateCorrectionCodes(0, _preparedData.size());
    for (uint8_t code : errorCodes) {
      _encodedData.appendByte(code);
    }
  } else {
    interleaveData();
  }

  BitStream remainder;
  for (int i = 0; i < remainderBits[_version - 1]; i++) {
    remainder.bits.push_back(0);
  }

  _encodedData = _encodedData + remainder;
  return _encodedData;
}