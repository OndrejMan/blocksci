#include "gtest/gtest.h"

#include <blocksci/chain/coinjoin_utils.hpp>

#include <cmath>
#include <vector>

namespace {
    constexpr long satoshisPerBitcoin = 100000000L;
    // Same Wasabi 2 limits as CoinjoinUtils::compute_ww2_denominations(), ported from Dumplings.
    constexpr long minSatoshis = 5000; // 0.00005 BTC
    constexpr long maxSatoshis = 134375000000; // 1343.75 BTC

    long pow(long base, unsigned exponent) {
        return static_cast<long>(std::pow(base, exponent));
    }
}  // namespace

// The 2 * 3^n series validates each candidate denomination before inserting
// it, so only values within the configured range are retained.
TEST(WasabiDenominationsTest, ThreePowerDoubleSeriesStopsAtMaximum) {
    const auto &denominations = blocksci::CoinjoinUtils::ww2_denominations;

    EXPECT_EQ(denominations.count(2L * pow(3L, 22)), 1u)
        << "largest valid 2 * 3^n is missing";
    EXPECT_EQ(denominations.count(2L * pow(3L, 23)), 0u)
        << "2 * 3^n overshoots the maximum";
}

TEST(WasabiDenominationsTest, TenPowerDoubleSeriesStopsAtMaximum) {
    const auto &denominations = blocksci::CoinjoinUtils::ww2_denominations;

    EXPECT_EQ(denominations.count(2L * pow(10L, 10)), 1u)
        << "largest valid 2 * 10^n is missing";
    EXPECT_EQ(denominations.count(2L * pow(10L, 11)), 0u)
        << "2 * 10^n overshoots the maximum";
}

// The 1-2-5 series generates the 5 * 10^n denominations by validating each
// multiplied candidate before inserting it.
TEST(WasabiDenominationsTest, TenPowerQuintupleSeriesIsGeneratedWithinRange) {
    const auto &denominations = blocksci::CoinjoinUtils::ww2_denominations;

    const std::vector<long> expected = {
        5L * pow(10L, 4),
        5L * pow(10L, 5),
        5L * pow(10L, 6),
        5L * pow(10L, 7),
        5L * pow(10L, 8),
        5L * pow(10L, 9),
        5L * pow(10L, 10),
    };
    for (long value : expected) {
        EXPECT_EQ(denominations.count(value), 1u) << "missing denomination " << value;
    }

    EXPECT_EQ(denominations.count(5L * pow(10L, 11)), 0u)
        << "5 * 10^n overshoots the maximum";
}

TEST(WasabiDenominationsTest, StaysWithinConfiguredRange) {
    const auto &denominations = blocksci::CoinjoinUtils::ww2_denominations;

    for (long value : denominations) {
        EXPECT_GE(value, minSatoshis);
        EXPECT_LE(value, maxSatoshis);
    }
}
