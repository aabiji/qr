#include <gtest/gtest.h>

#include "bitstream.h"
#include "galois.h"
#include "message.h"
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

TEST(MessageGeneration, EncodeData1) {
  Message msg("HELLO WORLD", ErrorCorrection::QUANTILE);
  BitStream bits = msg.encode();
  std::string required =
      "001000000101101100001011011110001101000101110010110111000100110101000011"
      "010000001110110000010001111011001010100001001000000101100101001011011001"
      "0011011010011100000000010010111000001111101101000111101000010000";
  EXPECT_EQ(bits.toString(), required);
};

TEST(MessageGeneration, EncodeData2) {
  Message msg("HELLO WORLD", ErrorCorrection::MEDIUM);
  BitStream bits = msg.encode();
  std::string expected =
      "001000000101101100001011011110001101000101110010110111000100110101000011"
      "010000001110110000010001111011000001000111101100000100011100010000100011"
      "0010011101110111111010111101011111100111111000100101110100010111";
  EXPECT_EQ(bits.toString(), expected);
  std::vector<uint8_t> bytes = {32, 91,  11,  120, 209, 114, 220, 77,  67,
                                64, 236, 17,  236, 17,  236, 17,  196, 35,
                                39, 119, 235, 215, 231, 226, 93,  23};
  EXPECT_EQ(bits.toBytes(), bytes);
}

TEST(MessageGeneration, Full) {
  std::vector<uint8_t> input = {
      67,  85,  70,  134, 87,  38,  85,  194, 119, 50,  6,   18,  6,
      103, 38,  246, 246, 66,  7,   118, 134, 242, 7,   38,  86,  22,
      198, 199, 146, 6,   182, 230, 247, 119, 50,  7,   118, 134, 87,
      38,  82,  6,   134, 151, 50,  7,   70,  247, 118, 86,  194, 6,
      151, 50,  16,  236, 17,  236, 17,  236, 17,  236};
  Message msg(input, ErrorCorrection::QUANTILE, 5);
  BitStream out = msg.encode();
  std::string expected =
      "010000111111011010110110010001100101010111110110111001101111011101000110"
      "010000101111011101110110100001100000011101110111010101100101011101110110"
      "001100101100001000100110100001100000011100000110010101011111001001110110"
      "100101111100001000000111100001100011001001110111001001100101011100010000"
      "001100100101011000100110111011000000011000010110010100100001000100010010"
      "110001100000011011101100000001101100011110000110000100010110011110010010"
      "100101111110110000100110000001100011001000010001000001111110110011010101"
      "010101111001010011101011110001111100110001110100100111110000101101100000"
      "101100010000010100101101001111001101010010101101011100111100101001001100"
      "000110001111011110110110100001011001001111110001011111000100101100111011"
      "110111111001110111110010001000011110010111001000111011100110101011111000"
      "100001100100110000101000100110100001101111000011111111110111010110000001"
      "111001101010110010011010110100011011110101010010011011110001000100001010"
      "000000100101011010100011011011001000001110100001101000111111000000100000"
      "0110111101111000110000001011001000100111100001011000110111101100";
  EXPECT_EQ(out.toString(), expected);
}

TEST(Message, Full) {
  Message msg("hello :)", ErrorCorrection::LOW);
  BitStream out = msg.encode();
  std::cout << out.bits.size() << "\n";
  std::vector<uint8_t> expected = {0x71, 0xa4, 0x08, 0x68, 0x65, 0x6c, 0x6c,
                                   0x6f, 0x20, 0x3a, 0x29, 0x00, 0xec, 0x11,
                                   0xec, 0x11, 0xec, 0x11, 0xec};
  EXPECT_EQ(out.toBytes(), expected);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}