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

  void print();
  void setSize(int width, int height);
  void save(const char *filename);
  void fill(uint8_t r, uint8_t g, uint8_t b);
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
      int index = _width * 3 * y + x * 3;
      uint8_t avg = (_rgbData[index] + _rgbData[index + 1] + _rgbData[index + 2]) / 3;
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
  void setModule(int x, int y, bool on);
  void drawFinderPattern(int startX, int startY);

  int _size;
  int _moduleSize;
  Image _img;
  Encoder _encoder;
};

void QRImage::setModule(int x, int y, bool on) {
  int realX = x * _moduleSize;
  int realY = y * _moduleSize;
  int c = on ? 0 : 255;
  _img.setPixel(realX, realY, c, c, c);
}

void QRImage::drawFinderPattern(int startX, int startY) {
  int size = 7; // Finder patterns are 7x7 always
  // A pattern with a bordered nxn square, with a n-2xn-2 square inside
  for (int y = 0; y <= size; y++) {
    for (int x = 0; x <= size; x++) {
      bool isBorder = x == 0 || x == size || y == 0 || y == size;
      bool inInnerSquare = x - 2 >= 0 && x < size - 1 && y - 2 >= 0 && y < size - 1;
      bool on = isBorder || inInnerSquare;
      setModule(startX + x, startY + y, on);
    }
  }

  // Draw separators around the pattern
  // Separators must be inside the qr image
  int x = startX - 1 > 0 ? -1 : 8;
  int y = startY - 1 > 0 ? -1 : 8;
  for (int i = 0; i < size + 1; i++) {
    setModule(startX + i, startY + y, false);
    setModule(startX + x, startY + i, false);
  }
  setModule(startX + x, startY + y, false);
}

void QRImage::create() {
  _encoder.encode();
  _moduleSize = 1;
  _size = 21 + (_encoder.getQrVersion() - 1) * 4;
  _img.setSize(_size * _moduleSize, _size * _moduleSize);
  _img.fill(BLANK, BLANK, BLANK);

  drawFinderPattern(0, 0); // Top right corner
  drawFinderPattern(0, _size - 8); // Bottom right corner
  drawFinderPattern(_size - 8, 0); // Top left corner

  _img.save("output.png");
}

int main() {
  QRImage img("Hello world", ErrorCorrection::LOW);
  img.create();
}