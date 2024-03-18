#include <gtest/gtest.h>
#include <string>

#include "bitstream.h"
#include "encoder.h"
#include "galois.h"
#include "tables.h"

using namespace Galois;

TEST(GaloisField256, Add) {
  Int a(56);
  Int b(14);
  Int c(54);
  EXPECT_EQ(a + b, c);
}

TEST(GaloisField256, Multiply) {
  Int a(76);
  Int b(43);
  Int c(251);
  EXPECT_EQ(a * b, c);

  Int d(16);
  Int e(32);
  Int f(58);
  EXPECT_EQ(d * e, f);

  Int g(198);
  Int h(215);
  Int i(240);
  EXPECT_EQ(g * h, i);
}

// TODO: refactor this
// implement the generator polynomial
TEST(Polynomial, Multiply) {
  Polynomial p1({0, 0});
  Polynomial p2({0, 1});
  Polynomial p3({0, 25, 1});
  Polynomial p4 = p1 * p2;
  EXPECT_EQ(p4, p3);

  Polynomial p5({0, 2});
  Polynomial p6({0, 198, 199, 3});
  Polynomial p7 = p4 * p5;
  EXPECT_EQ(p7, p6);

  Polynomial p8({0, 3});
  Polynomial p9({0, 75, 249, 78, 6});
  Polynomial p10 = p7 * p8;
  EXPECT_EQ(p10, p9);
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