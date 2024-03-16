#include <string>
#include <gtest/gtest.h>

#include "bitstream.h"
#include "encoder.h"
#include "tables.h"

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