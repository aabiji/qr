#pragma once

#include <string>

#include "tables.h"

class BitStream;

class Encoder {
public:
  Encoder(std::string input) : input_(input) {}
  BitStream encode(ErrorCorrection level, int version);
private:
  BitStream encodeNumeric();
  BitStream encodeAlphaNumeric();
  BitStream encodeByteMode();

  std::string input_;
  EncodingMode getOptimalEncodingMode();
};