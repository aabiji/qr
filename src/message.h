#pragma once

#include <string>

#include "bitstream.h"
#include "tables.h"

class BitStream;

class Message {
 public:
  Message(std::string input, ErrorCorrection level, int version) {
    _input = input;
    _qrVersion = version;
    _level = level;
  }

  Message(std::vector<uint8_t> encodedData, ErrorCorrection level, int version)
      : _encodedData(encodedData) {
    _qrVersion = version;
    _level = level;
  }

  BitStream encode();  // todo: make this private
  BitStream generate();

 private:
  BitStream encodeNumeric();
  BitStream encodeAlphaNumeric();
  BitStream encodeByteMode();
  EncodingMode getOptimalEncodingMode();
  std::vector<uint8_t> generateCorrectionCodes(
      int blockStart,
      int blockEnd);  // generate error correction codes for a block of data
  void interleaveData();

  int _qrVersion;
  std::string _input;
  ErrorCorrection _level;
  std::vector<uint8_t> _encodedData;
  BitStream _preparedData;
};