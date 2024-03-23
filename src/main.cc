#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "deps/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "deps/stb/stb_image_write.h"

class Image {
public:
  Image(const char* filename, int width, int height);
  ~Image();

  void save();
  void fill(uint8_t r, uint8_t g, uint8_t b);
  void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
private:
  int _width;
  int _height;
  const char *_filename;
  uint8_t* _rgbData;
};

Image::Image(const char *filename, int width, int height) {
  _width = width;
  _height = height;
  _filename = filename;
  _rgbData = new uint8_t[_width * _height * 3];
}

Image::~Image() {
  delete[] _rgbData;
}

void Image::save() {
  stbi_write_png(_filename, _width, _height, 3, _rgbData, _width * 3);
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

int main() {
  Image img("output.png", 100, 100);
  img.fill(255, 255, 255);
  img.save();
}