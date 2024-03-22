#include <gtest/gtest.h>

#include "bitstream.h"
#include "encoder.h"
#include "galois.h"
#include "tables.h"

TEST(GaloisField256, Add) {
  galois::Int a(56);
  galois::Int b(14);
  galois::Int c(54);
  EXPECT_EQ(a + b, c);
}

TEST(GaloisField256, Multiply) {
  galois::Int a(76);
  galois::Int b(43);
  galois::Int c(251);
  EXPECT_EQ(a * b, c);

  galois::Int d(16);
  galois::Int e(32);
  galois::Int f(58);
  EXPECT_EQ(d * e, f);

  galois::Int g(198);
  galois::Int h(215);
  galois::Int i(240);
  EXPECT_EQ(g * h, i);
}

TEST(Polynomial, Multiply) {
  std::vector<int> exponents = {0, 0};
  galois::Polynomial p1(exponents);

  exponents = {0, 1};
  galois::Polynomial p2(exponents);

  exponents = {0, 25, 1};
  galois::Polynomial p3(exponents);

  galois::Polynomial p4 = p1 * p2;
  EXPECT_EQ(p4, p3);
}

TEST(Polynomial, Generator) {
  galois::Polynomial p0 = galois::Polynomial::createGenerator(2);
  std::vector exponents = {0, 25, 1};
  galois::Polynomial p1(exponents);
  EXPECT_EQ(p0, p1);

  galois::Polynomial p2 = galois::Polynomial::createGenerator(6);
  exponents = {0, 166, 0, 134, 5, 176, 15};
  galois::Polynomial p3(exponents);
  EXPECT_EQ(p2, p3);

  galois::Polynomial p4 = galois::Polynomial::createGenerator(15);
  exponents = {0,  8,  183, 61,  91,  202, 37, 51,
               58, 58, 237, 140, 124, 5,   99, 105};
  galois::Polynomial p5(exponents);
  EXPECT_EQ(p4, p5);

  galois::Polynomial p6 = galois::Polynomial::createGenerator(7);
  exponents = {0, 87, 229, 146, 149, 238, 102, 21};
  galois::Polynomial p7(exponents);
  EXPECT_EQ(p6, p7);
}

TEST(DataEncoding, Test1) {
  Encoder enc("HELLO WORLD", ErrorCorrection::QUANTILE);
  std::vector<uint8_t> expected = {32, 91,  11, 120, 209, 114, 220, 77, 67,
                                   64, 236, 17, 236, 168, 72,  22,  82, 217,
                                   54, 156, 1,  46,  15,  180, 122, 16};
  EXPECT_EQ(enc.encode().toBytes(), expected);
};

TEST(DataEncoding, Test2) {
  Encoder enc("HELLO WORLD", ErrorCorrection::MEDIUM);
  BitStream bits = enc.encode();
  std::vector<uint8_t> bytes = {32, 91,  11,  120, 209, 114, 220, 77,  67,
                                64, 236, 17,  236, 17,  236, 17,  196, 35,
                                39, 119, 235, 215, 231, 226, 93,  23};
  EXPECT_EQ(bits.toBytes(), bytes);
}

TEST(DataEncoding, Full) {
  std::string input =
      "This is a test. Lorem ipsum bla bla bla.This is a test. Lorem ipsum bla "
      "bla bla.This is a test. Lorem ipsum bla bla bla.This is a test. Lorem "
      "ipsum bla bla bla.This is a test. Lorem ipsum bla bla bla.This is a "
      "test. Lorem ipsum bla bla bla.This is a test. Lorem ipsum bla bla "
      "bla.This is a test. Lorem ipsum bla bla bla.This is a test. Lorem ipsum "
      "bla bla bla.This is a test. Lorem ipsum bla bla bla.This is a test. "
      "Lorem ipsum bla bla bla.";
  Encoder enc(input, ErrorCorrection::HIGH);
  BitStream out = enc.encode();
  EXPECT_EQ(enc.getQrVersion(), 10);
  std::vector<uint8_t> expected = {
      64,  55,  6,   151, 247, 38,  6,   7,   11,  66,  38,  50,  38,  198, 18,
      55,  133, 226, 198, 6,   86,  18,  7,   86,  70,  4,   18,  151, 210, 6,
      70,  210, 134, 198, 6,   50,  6,   38,  87,  6,   151, 247, 38,  6,   151,
      198, 55,  38,  50,  38,  198, 18,  7,   18,  66,  198, 6,   86,  18,  7,
      55,  229, 226, 18,  151, 210, 6,   70,  86,  70,  4,   6,   50,  6,   38,
      87,  210, 134, 198, 38,  6,   151, 198, 55,  6,   151, 247, 198, 18,  7,
      18,  66,  38,  50,  38,  18,  7,   55,  229, 226, 198, 6,   86,  6,   70,
      86,  70,  4,   18,  151, 210, 38,  87,  210, 134, 198, 6,   50,  6,   198,
      151, 18,  141, 141, 25,  45,  158, 65,  27,  203, 53,  111, 20,  176, 226,
      142, 222, 219, 11,  136, 178, 87,  216, 234, 105, 250, 44,  94,  112, 243,
      32,  14,  83,  21,  4,   97,  8,   57,  212, 1,   2,   163, 86,  247, 63,
      113, 196, 255, 170, 242, 94,  208, 173, 209, 57,  35,  167, 19,  170, 231,
      131, 28,  161, 206, 194, 191, 195, 115, 67,  190, 194, 163, 214, 14,  228,
      107, 194, 194, 80,  231, 154, 25,  181, 1,   207, 182, 235, 241, 30,  4,
      62,  10,  53,  251, 107, 156, 189, 131, 241, 6,   162, 215, 64,  44,  196,
      42,  121, 185, 153, 217, 80,  113, 91,  140, 72,  184, 189, 167, 155, 178,
      149, 91,  98,  200, 199, 42,  142, 155, 21,  98,  245, 56,  235, 255, 158,
      1,   239, 130, 66,  52,  69,  240, 49,  174, 89,  176, 251, 45,  131, 232,
      146, 175, 110, 124, 175, 50,  183, 179, 221, 192, 148, 182, 124, 139, 65,
      95,  220, 230, 192, 169, 117, 223, 243, 241, 43,  168, 100, 248, 135, 50,
      107, 93,  204, 141, 3,   57,  67,  162, 171, 140, 52,  218, 140, 184, 26,
      209, 182, 161, 206, 205, 4,   124, 179, 116, 135, 172, 147, 228, 105, 179,
      16,  91,  96,  253, 189, 36,  70,  4,   170, 69,  133, 132, 150, 149, 29,
      174};
  EXPECT_EQ(out.toBytes(), expected);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}