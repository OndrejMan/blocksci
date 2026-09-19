#include "gtest/gtest.h"

#include "../../src/heuristics/witness_unknown_key.hpp"

#include <vector>

TEST(WitnessUnknownKeyTest, IncludesWitnessVersionAndExactProgram) {
    const std::vector<unsigned char> programA = {0x01, 0x00, 0x02};
    const std::vector<unsigned char> programB = {0x01, 0x00, 0x03};

    const auto versionOneA = blocksci::heuristics::detail::witnessUnknownAddressKey(
        1, programA.begin(), programA.end());
    const auto versionTwoA = blocksci::heuristics::detail::witnessUnknownAddressKey(
        2, programA.begin(), programA.end());
    const auto versionOneB = blocksci::heuristics::detail::witnessUnknownAddressKey(
        1, programB.begin(), programB.end());

    EXPECT_NE(versionOneA, versionTwoA);
    EXPECT_NE(versionOneA, versionOneB);
    ASSERT_EQ(versionOneA.size(), programA.size() + 1);
    EXPECT_EQ(static_cast<unsigned char>(versionOneA[0]), 1);
    EXPECT_EQ(static_cast<unsigned char>(versionOneA[2]), 0);
}
