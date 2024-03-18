#include <string>
#include <gtest/gtest.h>

#include "bitstream.h"
#include "encoder.h"
#include "galois.h"
#include "tables.h"

TEST(GaloisField256, Add) {
  GaloisInt a(56);
  GaloisInt b(14);
  GaloisInt c(54);
  EXPECT_EQ(a + b, c);
}

TEST(GaloisField256, Multiply) {
  GaloisInt a(76);
  GaloisInt b(43);
  GaloisInt c(251);
  EXPECT_EQ(a * b, c);

  GaloisInt d(16);
  GaloisInt e(32);
  GaloisInt f(58);
  EXPECT_EQ(d * e, f);

  GaloisInt g(198);
  GaloisInt h(215);
  GaloisInt i(240);
  EXPECT_EQ(g * h, i);
}

// TODO: refactor this
// implement the generator polynomial
TEST(Polynomial, Multiply) {
  Polynomial p1({GaloisInt(1), GaloisInt(1)});
  Polynomial p2({GaloisInt(1), GaloisInt(2)});
  std::vector<int> exponents = {0, 25, 1};
  Polynomial p3(exponents);
  Polynomial p4 = p1 * p2;
  EXPECT_EQ(p4, p3);

  std::vector<int> exponents1 = {0, 2};
  Polynomial p5(exponents1);
  std::vector<int> exponents2 = {0, 198, 199, 3};
  Polynomial p6(exponents2);
  Polynomial p7 = p4 * p5;
  EXPECT_EQ(p7, p6);

  std::vector<int> exponents3 = {0, 3};
  Polynomial p8(exponents3);
  std::vector<int> exponents4 = {0, 75, 249, 78, 6};
  Polynomial p9(exponents4);
  Polynomial p10 = p7 * p8;
  EXPECT_EQ(p10, p9);
}

TEST(DataEncoding, Full) {
  Encoder enc("HELLO WORLD");
  BitStream bits = enc.encode(ErrorCorrection::QUANTILE, 1);
  std::string required = "00100000010110110000101101111000110100010111001011011100010011010100001101000000111011000001000111101100";
  EXPECT_EQ(bits.toString(), required);
};

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}