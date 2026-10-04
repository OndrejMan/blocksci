#include "gtest/gtest.h"

#include "../../src/scripts/bitcoin_bech32.hpp"
#include "../../src/scripts/bitcoin_segwit_addr.hpp"

TEST(SegwitAddressTest, EncodesAndDecodesTaprootWithBech32m) {
    const std::vector<uint8_t> witness_program = {
        0xc1, 0xcc, 0x1b, 0x77, 0x27, 0xc2, 0x5e, 0x85,
        0xe1, 0x41, 0x31, 0xd8, 0xa4, 0xe4, 0xfb, 0x31,
        0xb0, 0xe3, 0x81, 0x49, 0xa4, 0xb9, 0x70, 0xbf,
        0xd1, 0x2f, 0x7d, 0xe1, 0x0e, 0xff, 0xf8, 0x83,
    };

    const auto address = segwit_addr::encode("bcrt", 1, witness_program);
    EXPECT_EQ(address, "bcrt1pc8xpkae8cf0gtc2px8v2fe8mxxcw8q2f5juhp0739a77zrhllzps6pk4lz");
    const auto bech32_decoded = bech32::decodeWithEncoding(address);
    ASSERT_FALSE(std::get<1>(bech32_decoded).empty());
    EXPECT_EQ(std::get<1>(bech32_decoded)[0], 1); // Taproot witness v1
    EXPECT_EQ(std::get<2>(bech32_decoded), bech32::Encoding::BECH32M);

    const auto decoded = segwit_addr::decode("bcrt", address);
    EXPECT_EQ(decoded.first, 1);
    EXPECT_EQ(decoded.second, witness_program);
}

TEST(SegwitAddressTest, EncodesAndDecodesWitnessV0WithBech32) {
    const std::vector<uint8_t> witness_program = {
        0x75, 0x1e, 0x76, 0xe8, 0x19, 0x91, 0x96, 0xd4, 0x54, 0x94,
        0x1c, 0x45, 0xd1, 0xb3, 0xa3, 0x23, 0xf1, 0x43, 0x3b, 0xd6,
    };

    const auto address = segwit_addr::encode("bcrt", 0, witness_program);
    EXPECT_EQ(address, "bcrt1qw508d6qejxtdg4y5r3zarvary0c5xw7kygt080");
    const auto bech32_decoded = bech32::decodeWithEncoding(address);
    ASSERT_FALSE(std::get<1>(bech32_decoded).empty());
    EXPECT_EQ(std::get<1>(bech32_decoded)[0], 0); // SegWit witness v0
    EXPECT_EQ(std::get<2>(bech32_decoded), bech32::Encoding::BECH32);

    const auto decoded = segwit_addr::decode("bcrt", address);
    EXPECT_EQ(decoded.first, 0);
    EXPECT_EQ(decoded.second, witness_program);
}

TEST(SegwitAddressTest, RejectsInvalidEncodeInputsBeforeBech32Encoding) {
    const std::vector<uint8_t> valid_program(20, 0x42);

    EXPECT_EQ(segwit_addr::encode("bcrt", -1, valid_program), "");
    EXPECT_EQ(segwit_addr::encode("bcrt", 32, valid_program), "");
    EXPECT_EQ(segwit_addr::encode("bcrt", 0, std::vector<uint8_t>(21, 0x42)), "");
    EXPECT_EQ(segwit_addr::encode("bcrt", 1, std::vector<uint8_t>(1, 0x42)), "");
    EXPECT_EQ(segwit_addr::encode("bcrt", 1, std::vector<uint8_t>(41, 0x42)), "");
}

TEST(SegwitAddressTest, RejectsBech32ValuesOutsideFiveBitRange) {
    EXPECT_EQ(bech32::encode("bcrt", std::vector<uint8_t>{32}), "");
    EXPECT_EQ(bech32::encode("bcrt", std::vector<uint8_t>{255}), "");
}

// Witness version 0 must use Bech32 and version 1 and above must use Bech32m.
// The same payload carrying the other variant's checksum has to be rejected.
TEST(SegwitAddressTest, RejectsWitnessV0CarryingBech32mChecksum) {
    const std::string address = "bcrt1qw508d6qejxtdg4y5r3zarvary0c5xw7k35mrzd";
    const auto bech32_decoded = bech32::decodeWithEncoding(address);
    ASSERT_FALSE(std::get<1>(bech32_decoded).empty());
    EXPECT_EQ(std::get<1>(bech32_decoded)[0], 0); // SegWit witness v0
    EXPECT_EQ(std::get<2>(bech32_decoded), bech32::Encoding::BECH32M);

    const auto decoded = segwit_addr::decode("bcrt", address);
    EXPECT_EQ(decoded.first, -1);
}

TEST(SegwitAddressTest, RejectsTaprootCarryingBech32Checksum) {
    const std::string address = "bcrt1pc8xpkae8cf0gtc2px8v2fe8mxxcw8q2f5juhp0739a77zrhllzps0axe6q";
    const auto bech32_decoded = bech32::decodeWithEncoding(address);
    ASSERT_FALSE(std::get<1>(bech32_decoded).empty());
    EXPECT_EQ(std::get<1>(bech32_decoded)[0], 1); // Taproot witness v1
    EXPECT_EQ(std::get<2>(bech32_decoded), bech32::Encoding::BECH32);

    const auto decoded = segwit_addr::decode("bcrt", address);
    EXPECT_EQ(decoded.first, -1);
}
