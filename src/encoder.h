#pragma once

#include <string>

#include "bitstream.h"
#include "tables.h"

struct Group {
  int iter;
  int numBlocks;
  int blockSize;
};

class Encoder {
 public:
  Encoder(std::string input, ErrorCorrection level);

  // Process the input, add error correction codes
  // and interleave the encoded data.
  BitStream encode();

  int getQrVersion();

 private:
  // Get the encoding mode that will most optimally
  // encode the data.
  void getOptimalEncodingMode();

  // Find the smallest qr version that will fit the data
  void findSmallestVersion();

  // Data encoding per encoding mode.
  BitStream encodeNumeric();
  BitStream encodeAlphaNumeric();
  BitStream encodeByteMode();

  // Not only encode based on mode, but also
  // add metadata according to the qr specification.
  void process();

  // Generate error correction codes for a range of
  // data in our processed data bitstream.
  std::vector<uint8_t> generateCorrectionCodes(int rangeStart, int rangeEnd);

  // Encoded data is split into 2 groups each containing
  // different amount of blocks. Get info pertaining to such groups.
  void getGroupInfo();

  // For higher versions of the qr code, the qr spec
  // requires the encoded data to be interleaved.
  void interleaveData();

  int _version;
  std::string _input;

  Group _groups[2];
  EncodingMode _mode;
  ErrorCorrection _level;

  std::vector<uint8_t> _preparedData;
  BitStream _encodedData;
};