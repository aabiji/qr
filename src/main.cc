#include <stdlib.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "deps/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "deps/stb/stb_image_write.h"

#include "encoder.h"

// Value representing a unset pixel
const int BLANK = 128;

class QR {
 public:
  QR(std::string input, ErrorCorrection level);
  ~QR();

  void generate();
  void debug();
  void save(const char* filename);

 private:
  uint8_t getModule(int x, int y);
  void setModule(int x, int y, bool on);

  // A pattern with a bordered nxn square, with a n-2xn-2 square inside
  void drawSquarePattern(int startX, int startY, int size);
  void drawFinderPattern(int startX, int startY);
  void drawAlignmentPatterns();

  // Draw alternating black and white modules along the
  // sixth column and row in between the timing patterns
  void drawTimingPatterns();

  int _moduleSize;  // A module is a nxn group of pixels
  int _size;        // Size of the qr square in modules
  int _realSize;    // Size of the qr square in pixels
  uint8_t* _pixels;

  int _version;
  Encoder _encoder;
};

QR::QR(std::string input, ErrorCorrection level) : _encoder(input, level) {
  _encoder.encode();
  _moduleSize = 1;
  _version = _encoder.getQrVersion();
  _size = 21 + (_version - 1) * 4;
  _realSize = _size * _moduleSize;
  _pixels = new uint8_t[_realSize * _realSize * 3];
  memset(_pixels, BLANK, _realSize * _realSize * 3);
}

QR::~QR() {
  delete[] _pixels;
}

void QR::setModule(int x, int y, bool on) {
  int realX = x * _moduleSize;
  int realY = y * _moduleSize;
  int width = _size * _moduleSize;
  int c = on ? 0 : 255;

  for (int i = 0; i < _moduleSize; i++) {
    for (int j = 0; j < _moduleSize; j++) {
      int index = width * 3 * (realY + i) + (realX + j) * 3;
      _pixels[index] = _pixels[index + 1] = _pixels[index + 2] = c;
    }
  }
}

uint8_t QR::getModule(int x, int y) {
  int realX = x * _moduleSize;
  int realY = y * _moduleSize;
  int width = _size * _moduleSize;
  int index = width * 3 * realY + realX * 3;
  return _pixels[index];
}

void QR::drawSquarePattern(int startX, int startY, int size) {
  for (int y = 0; y <= size; y++) {
    for (int x = 0; x <= size; x++) {
      bool isBorder = x == 0 || x == size || y == 0 || y == size;
      bool inInnerSquare =
          x - 2 >= 0 && x < size - 1 && y - 2 >= 0 && y < size - 1;
      bool on = isBorder || inInnerSquare;
      setModule(startX + x, startY + y, on);
    }
  }
}

void QR::drawFinderPattern(int startX, int startY) {
  int size = 7;  // Finder patterns are 7x7 always
  drawSquarePattern(startX, startY, size - 1);

  // Draw separators around the finder patterns
  int rowX = startX != 0 ? startX - 1 : startX;
  int rowY = startY == 0 ? startY + 7 : startY - 1;
  int colX = startX == 0 ? 7 : startX - 1;
  int colY = startY == 0 ? rowY : startY + 7;
  for (int i = 0; i < size + 1; i++) {
    setModule(rowX + i, rowY, false);
    setModule(colX, colY - i, false);
  }

  // Draw dark module at the side of the bottom
  // left finder pattern
  if (startX == 0 && startY == _size - 7) {
    setModule(colX + 1, rowY, true);
  }
}

void QR::drawAlignmentPatterns() {
  int size = 5;
  int numPatterns = alignmentPatternLocations[_version - 2][0];
  for (int i = 1; i <= numPatterns; i++) {
    for (int j = 1; j <= numPatterns; j++) {
      int x = alignmentPatternLocations[_version - 2][i] - 2;
      int y = alignmentPatternLocations[_version - 2][j] - 2;
      int color = getModule(x + 2, y + 2);
      // Alignment patterns cannot overlap with finder patterns
      if (color == BLANK) {
        drawSquarePattern(x, y, size - 1);
      }
    }
  }
}

void QR::drawTimingPatterns() {
  for (int i = 8; i < _size - 8; i++) {
    int colorX = getModule(i, 6);
    if (colorX == BLANK) {
      setModule(i, 6, i % 2 == 0);
    }

    int colorY = getModule(6, i);
    if (colorY == BLANK) {
      setModule(6, i, i % 2 == 0);
    }
  }
}

void QR::generate() {
  drawFinderPattern(0, 0);          // Top right corner
  drawFinderPattern(0, _size - 7);  // Bottom right corner
  drawFinderPattern(_size - 7, 0);  // Top left corner
  if (_version > 1) {
    drawAlignmentPatterns();
  }
  drawTimingPatterns();
}

void QR::save(const char* filename) {
  stbi_write_png(filename, _realSize, _realSize, 3, _pixels, _realSize * 3);
}

void QR::debug() {
  for (int y = 0; y < _size; y++) {
    for (int x = 0; x < _size; x++) {
      uint8_t avg = getModule(x, y);
      std::cout << (avg > 128 ? "██" : "  ");
    }
    std::cout << "\n";
  }
}

int main() {
  std::string input = "Hello world!Hello world!Hello world!";
  QR qr(input, ErrorCorrection::LOW);
  qr.generate();
  qr.save("output.png");
}