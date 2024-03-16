#include "tables.h"
#include "encoder.h"

// Find the smallest qr version that will fit the data
int findSmallestVersion(int size, ErrorCorrection l, EncodingMode m) {
  int version = 0;
  while (true) {
    int capacity = characterCapacities[version][l][m];
    if (size < capacity) {
      break;
    }
    version ++;
    size -= capacity;
  }
  return version + 1;
}

class QR {
public:
  QR(std::string input, ErrorCorrection level);
  void create();
private:
  std::string input_;
  int inputLength_;
  EncodingMode mode_;
  ErrorCorrection level_;
  int version_;
  int matrixSize_;
};

QR::QR(std::string input, ErrorCorrection level) {
  input_ = input;
  level_ = level;
  version_ = findSmallestVersion(inputLength_, level_, mode_);
  matrixSize_ = 25 + 4 * (version_ - 1);
}

void QR::create() {}

int main() {}