#include "encoder.h"
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
  QR(std::string input, ErrorCorrection level);
  void create();

private:
  std::string _input;
  int _inputLength;
  EncodingMode _mode;
  ErrorCorrection _level;
  int _version;
  int _matrixSize;
};

QR::QR(std::string input, ErrorCorrection level) {
  _input = input;
  _level = level;
  _version = findSmallestVersion(_inputLength, _level, _mode);
  _matrixSize = 25 + 4 * (_version - 1);
}

void QR::create() {}

int main() {}