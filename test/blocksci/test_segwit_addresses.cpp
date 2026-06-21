#include "gtest/gtest.h"

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

    const auto decoded = segwit_addr::decode("bcrt", address);
    EXPECT_EQ(decoded.first, 1);
    EXPECT_EQ(decoded.second, witness_program);
}
