#include <stdlib.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "deps/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "deps/stb/stb_image_write.h"

#include "encoder.h"

enum Color {
  BLACK = 0,
  WHITE = 255,
  NOT_SET = 128,
  RESERVED = 200,
};

class QR {
 public:
  QR(std::string input, ErrorCorrection level);
  ~QR();

  void generate();
  void debug();
  void save(const char* filename);

 private:
  uint8_t getModule(int x, int y);
  void setModule(int x, int y, Color c);

  // A pattern with a bordered nxn square, with a n-2xn-2 square inside
  void drawSquarePattern(int startX, int startY, int size);
  void drawFinderPattern(int startX, int startY);
  void drawAlignmentPatterns();

  // Draw alternating black and white modules along the
  // sixth column and row in between the timing patterns
  void drawTimingPatterns();

  void reserveFormatInfoArea();

  void drawEncodedData();

  int _moduleSize;  // A module is a nxn group of pixels
  int _size;        // Size of the qr square in modules
  int _realSize;    // Size of the qr square in pixels
  uint8_t* _pixels;

  int _version;
  BitStream _data;
  Encoder _encoder;
};

QR::QR(std::string input, ErrorCorrection level) : _encoder(input, level) {
  _data = _encoder.encode();
  _moduleSize = 1;
  _version = _encoder.getQrVersion();
  _size = 21 + (_version - 1) * 4;
  _realSize = _size * _moduleSize;
  _pixels = new uint8_t[_realSize * _realSize * 3];
  memset(_pixels, NOT_SET, _realSize * _realSize * 3);
}

QR::~QR() {
  delete[] _pixels;
}

void QR::setModule(int x, int y, Color c) {
  int realX = x * _moduleSize;
  int realY = y * _moduleSize;
  for (int i = 0; i < _moduleSize; i++) {
    for (int j = 0; j < _moduleSize; j++) {
      int index = _realSize * 3 * (realY + i) + (realX + j) * 3;
      _pixels[index] = _pixels[index + 1] = _pixels[index + 2] = c;
    }
  }
}

uint8_t QR::getModule(int x, int y) {
  int realX = x * _moduleSize;
  int realY = y * _moduleSize;
  int index = _realSize * 3 * realY + realX * 3;
  return _pixels[index];
}

void QR::drawSquarePattern(int startX, int startY, int size) {
  for (int y = 0; y <= size; y++) {
    for (int x = 0; x <= size; x++) {
      bool isBorder = x == 0 || x == size || y == 0 || y == size;
      bool inInnerSquare =
          x - 2 >= 0 && x < size - 1 && y - 2 >= 0 && y < size - 1;
      Color color = isBorder || inInnerSquare ? BLACK : WHITE;
      setModule(startX + x, startY + y, color);
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
    setModule(rowX + i, rowY, WHITE);
    setModule(colX, colY - i, WHITE);
  }

  // Draw dark module at the side of the bottom
  // left finder pattern
  if (startX == 0 && startY == _size - 7) {
    setModule(colX + 1, rowY, BLACK);
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
      if (color == NOT_SET) {
        drawSquarePattern(x, y, size - 1);
      }
    }
  }
}

void QR::drawTimingPatterns() {
  for (int i = 8; i < _size - 8; i++) {
    int colorX = getModule(i, 6);
    Color c = i % 2 == 0 ? BLACK : WHITE;
    if (colorX == NOT_SET) {
      setModule(i, 6, c);
    }
    int colorY = getModule(6, i);
    if (colorY == NOT_SET) {
      setModule(6, i, c);
    }
  }
}

void QR::reserveFormatInfoArea() {
  for (int i = 0; i < 8; i++) {
    // Right side of the bottom left timing pattern
    setModule(8, _size - i, RESERVED);
    // Bottom side of the top right timing pattern
    setModule(_size - i - 1, 8, RESERVED);
    // Bottom side of the top left timing pattern
    setModule(i - 2 > 0 ? i - 2 : 0, 8, RESERVED);
    // Right side of the top left timing pattern
    setModule(8, i - 2 > 0 ? i - 2 : 0, RESERVED);
  }

  // Corner of the top left timing pattern
  setModule(7, 8, RESERVED);
  setModule(8, 7, RESERVED);
  setModule(8, 8, RESERVED);

  if (_version >= 7) {
    for (int y = 0; y < 3; y++) {
      for (int x = 0; x < 6; x++) {
        // Top side of the bottom left timing pattern
        setModule(x, _size - y - 9, RESERVED);
        // Right side of the top right timing pattern
        setModule(_size - y - 9, x, RESERVED);
      }
    }
  }
}

void QR::drawEncodedData() {
  int i = 0;
  int x = _size - 1;
  int y = _size - 1;
  int direction = -1;
  // Go right, left, up, repeat when going upwards
  // Go right, left, down, repeat when going downwards
  while (x >= 0) {
    if (getModule(x, y) == NOT_SET) {
      Color rightColor = _data.bits[i] ? BLACK : WHITE;
      setModule(x, y, rightColor);
      i += 1;
    }

    if (getModule(x - 1, y) == NOT_SET) {
      Color leftColor = _data.bits[i] ? BLACK : WHITE;
      setModule(x - 1, y, leftColor);
      i += 1;
    }

    if (direction == -1 && y - 1 < 0) {
      direction = 1;
      x -= 2;
    } else if (direction == 1 && y + 1 >= _size) {
      direction = -1;
      x -= 2;
    } else {
      y += direction;
    }

    // Skip vertical timing pattern
    if (x == 6) x -= 1;
  }
}

void QR::generate() {
  drawFinderPattern(0, 0);          // At the top left corner
  drawFinderPattern(0, _size - 7);  // At the bottom left corner
  drawFinderPattern(_size - 7, 0);  // At the top left corner
  if (_version > 1) {
    drawAlignmentPatterns();
  }
  drawTimingPatterns();
  reserveFormatInfoArea();
  drawEncodedData();
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