#include <cstdint>

#define STB_IMAGE_IMPLEMENTATION
#include "deps/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "deps/stb/stb_image_write.h"

#include "bitstream.h"
#include "encoder.h"
#include "galois.h"
#include "tables.h"

// Find the smallest qr version that will fit the data
int findSmallestVersion(int size, ErrorCorrection l, EncodingMode m) {
  int version = 0;
  while (true) {
    int capacity = characterCapacities[version][l][m];
    if (size < capacity) {
      break;
    }
    version++;
    size -= capacity;
  }
  return version + 1;
}

class QR {
public:
  QR(std::string input, ErrorCorrection level) : _encoder(input) {
    _input = input;
    _level = level;
    _version = findSmallestVersion(_inputLength, _level, _mode);
    _matrixSize = 25 + 4 * (_version - 1);
  }

  void create();

private:
  int _inputLength;
  int _version;
  int _matrixSize;
  std::string _input;
  EncodingMode _mode;
  ErrorCorrection _level;
  Encoder _encoder;
};

void QR::create() {}

int main() {}