#include <string>
#include <vector>
#include <cstdint>
#include <utf8.h>

uint32_t codepointAt(std::string str, int index) {
  auto it = str.begin();
  for (int i = 0; i < index && it != str.end(); i++) {
    utf8::next(it, str.end());
  }

  if (it != str.end()) {
    return utf8::next(it, str.end());
  }
  return 0;
}

template <class T>
bool contains(T* arr, T v, int size) {
  for (int i = 0; i < size; i++) {
    if (arr[i] == v) return true;
  }
  return false;
}

// Dynamically sized qr specific bitset
class BitSet {
public:
  void append(bool bit) {
    bits.push_back(bit);
  }

  std::string toString() {
    std::string str = "";
    for (bool b : bits) {
      str += b ? "1" : "0";
    }
    return str;
  }
private:
  std::vector<bool> bits;
};