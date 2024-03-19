#include <gtest/gtest.h>
#include <string>

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
  galois::Polynomial p0 = galois::Polynomial::create_generator(2);
  std::vector exponents = {0, 25, 1};
  galois::Polynomial p1(exponents);
  EXPECT_EQ(p0, p1);

  galois::Polynomial p2 = galois::Polynomial::create_generator(6);
  exponents = {0, 166, 0, 134, 5, 176, 15};
  galois::Polynomial p3(exponents);
  EXPECT_EQ(p2, p3);

  galois::Polynomial p4 = galois::Polynomial::create_generator(15);
  exponents = {0,  8,  183, 61,  91,  202, 37, 51,
               58, 58, 237, 140, 124, 5,   99, 105};
  galois::Polynomial p5(exponents);
  EXPECT_EQ(p4, p5);

  galois::Polynomial p6 = galois::Polynomial::create_generator(7);
  exponents = {0, 87, 229, 146, 149, 238, 102, 21};
  galois::Polynomial p7(exponents);
  EXPECT_EQ(p6, p7);
}

TEST(DataEncoding, Full) {
  Encoder enc("HELLO WORLD");
  BitStream bits = enc.encode(ErrorCorrection::QUANTILE, 1);
  std::string required = "00100000010110110000101101111000110100010111001011011"
                         "100010011010100001101000000111011000001000111101100";
  EXPECT_EQ(bits.toString(), required);

  Encoder enc1("HELLO WORLD");
  BitStream bits1 = enc1.encode(ErrorCorrection::MEDIUM, 1);
  std::string required1 =
      "001000000101101100001011011110001101000101110010110111000100110101000011"
      "01000000111011000001000111101100000100011110110000010001";
  EXPECT_EQ(bits1.toString(), required1);
  std::vector<uint8_t> bytes = {32, 91, 11, 120, 209, 114, 220, 77, 67, 64, 236, 17, 236, 17, 236, 17};
  EXPECT_EQ(bits1.toBytes(), bytes);
};

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}