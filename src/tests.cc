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
  galois::Polynomial p1({0, 0});
  galois::Polynomial p2({0, 1});
  galois::Polynomial p3({0, 25, 1});
  galois::Polynomial p4 = p1 * p2;
  EXPECT_EQ(p4, p3);
}

TEST(Polynomial, Generator) {
  galois::Polynomial p0 = galois::Polynomial::create_generator(2);
  galois::Polynomial p1({0, 25, 1});
  EXPECT_EQ(p0, p1);

  galois::Polynomial p2 = galois::Polynomial::create_generator(6);
  galois::Polynomial p3({0, 166, 0, 134, 5, 176, 15});
  EXPECT_EQ(p2, p3);

  galois::Polynomial p4 = galois::Polynomial::create_generator(15);
  galois::Polynomial p5({0, 8, 183, 61, 91, 202, 37, 51, 58, 58, 237, 140, 124, 5, 99, 105});
  EXPECT_EQ(p4, p5);

  galois::Polynomial p6 = galois::Polynomial::create_generator(7);
  galois::Polynomial p7({0, 87, 229, 146, 149, 238, 102, 21});
  EXPECT_EQ(p6, p7);
}

TEST(DataEncoding, Full) {
  Encoder enc("HELLO WORLD");
  BitStream bits = enc.encode(ErrorCorrection::QUANTILE, 1);
  std::string required = "00100000010110110000101101111000110100010111001011011"
                         "100010011010100001101000000111011000001000111101100";
  EXPECT_EQ(bits.toString(), required);
};

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}