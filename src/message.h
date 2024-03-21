#pragma once

#include <string>

#include "bitstream.h"
#include "tables.h"

class BitStream;

class Message {
 public:
  Message(std::string input, ErrorCorrection level) {
    _input = input;
    _level = level;
    getOptimalEncodingMode();
    findSmallestVersion();
  }

  Message(std::vector<uint8_t> encodedData, ErrorCorrection level, int version)
      : _encodedData(encodedData) {
    _level = level;
    _qrVersion = version;
  }

  BitStream encode();
 private:
  BitStream encodeNumeric();
  BitStream encodeAlphaNumeric();
  BitStream encodeByteMode();
  std::vector<uint8_t> generateCorrectionCodes(
      int blockStart,
      int blockEnd);  // generate error correction codes for a block of data
  void interleaveData();
  BitStream process();

  void getOptimalEncodingMode();

  // Find the smallest qr version that will fit the data
  void findSmallestVersion();

  int _qrVersion;
  std::string _input;
  ErrorCorrection _level;
  EncodingMode _mode;
  std::vector<uint8_t> _encodedData;
  BitStream _preparedData;
};