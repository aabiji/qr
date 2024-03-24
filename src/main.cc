#include <iostream>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "deps/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "deps/stb/stb_image_write.h"

#include "encoder.h"

class Image {
public:
  ~Image();

  void setSize(int width, int height);
  void fill(uint8_t r, uint8_t g, uint8_t b);

  void print();
  void save(const char *filename);

  uint8_t getPixel(int x, int y);
  void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
private:
  int _width;
  int _height;
  uint8_t* _rgbData;
};

Image::~Image() {
  delete[] _rgbData;
}

void Image::save(const char *filename) {
  stbi_write_png(filename, _width, _height, 3, _rgbData, _width * 3);
}

void Image::setSize(int width, int height) {
  _width = width;
  _height = height;
  _rgbData = new uint8_t[_width * _height * 3];
  memset(_rgbData, 0, _width * _height * 3);
}

void Image::print() {
  for (int y = 0; y < _height; y++) {
    for (int x = 0; x < _width; x++) {
      uint8_t avg = getPixel(x, y);
      std::cout << (avg > 128 ? "██" : "  ");
    }
    std::cout << "\n";
  }
}

void Image::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  int index = _width * 3 * y + x * 3;
  _rgbData[index] = r;
  _rgbData[index + 1] = g;
  _rgbData[index + 2] = b;
}

uint8_t Image::getPixel(int x, int y) {
  int index = _width * 3 * y + x * 3;
  return (_rgbData[index] + _rgbData[index + 1] + _rgbData[index + 2]) / 3;
}

void Image::fill(uint8_t r, uint8_t g, uint8_t b) {
  for (int y = 0; y < _height; y++) {
    for (int x = 0; x < _width; x++) {
      setPixel(x, y, r, g, b);
    }
  }
}

// Value representing a unset pixel
const int BLANK = 128;

class QRImage {
public:
  QRImage(std::string input, ErrorCorrection level) : _encoder(input, level) {}
  void create();
private:
  // A module is a nxn group of pixels
  uint8_t getModule(int x, int y);
  void setModule(int x, int y, bool on);

  // A pattern with a bordered nxn square, with a n-2xn-2 square inside
  void drawSquarePattern(int startX, int startY, int size);
  void drawFinderPattern(int startX, int startY);
  void drawAlignmentPatterns();

  int _size;
  int _moduleSize;
  int _version;
  Image _img;
  Encoder _encoder;
};

void QRImage::setModule(int x, int y, bool on) {
  int realX = x * _moduleSize;
  int realY = y * _moduleSize;
  int c = on ? 0 : 255;
  _img.setPixel(realX, realY, c, c, c);
}

uint8_t QRImage::getModule(int x, int y) {
  int realX = x * _moduleSize;
  int realY = y * _moduleSize;
  return _img.getPixel(realX, realY);
}

void QRImage::drawSquarePattern(int startX, int startY, int size) {
  for (int y = 0; y <= size; y++) {
    for (int x = 0; x <= size; x++) {
      bool isBorder = x == 0 || x == size || y == 0 || y == size;
      bool inInnerSquare = x - 2 >= 0 && x < size - 1 && y - 2 >= 0 && y < size - 1;
      bool on = isBorder || inInnerSquare;
      setModule(startX + x, startY + y, on);
    }
  }
}

void QRImage::drawFinderPattern(int startX, int startY) {
  int size = 7; // Finder patterns are 7x7 always
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
}

void QRImage::drawAlignmentPatterns() {
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

void QRImage::create() {
  _encoder.encode();
  _moduleSize = 1;
  _version = _encoder.getQrVersion();
  _size = 21 + (_version - 1) * 4;
  _img.setSize(_size * _moduleSize, _size * _moduleSize);
  _img.fill(BLANK, BLANK, BLANK);

  drawFinderPattern(0, 0); // Top right corner
  drawFinderPattern(0, _size - 7); // Bottom right corner
  drawFinderPattern(_size - 7, 0); // Top left corner

  if (_version > 1) {
    drawAlignmentPatterns();
  } 

  _img.save("output.png");
}

int main() {
  std::string input = "Hello world!Hello world!Hello world!";
  QRImage img(input, ErrorCorrection::LOW);
  img.create();
}