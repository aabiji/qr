#include <string>
#include <gtest/gtest.h>

#include "bitstream.h"
#include "encoder.h"
#include "galois.h"
#include "tables.h"

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