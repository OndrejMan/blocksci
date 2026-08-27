#include "gtest/gtest.h"

#include <blocksci/chain/transaction.hpp>
#include <blocksci/core/inout.hpp>
#include <blocksci/core/raw_transaction.hpp>
#include <blocksci/heuristics/tx_identification.hpp>

#include "../../src/internal/data_access.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <optional>
#include <stdexcept>

namespace {

constexpr uint16_t participantCount = 20;
constexpr int64_t wasabi2Denomination = 10'000'000;

// This transaction deliberately satisfies the broad JoinMarket shape while
// satisfying every Wasabi 2 invariant: 20 ordered, unique P2WPKH inputs and
// 20 ordered, unique equal-valued Wasabi 2-denomination outputs.
class SyntheticWasabi2JoinMarketOverlap {
    alignas(std::max_align_t)
        std::array<std::byte, sizeof(blocksci::RawTransaction) + 2 * participantCount * sizeof(blocksci::Inout)>
            storage{};
    std::array<uint16_t, participantCount> spentOutputNums{};
    std::array<uint32_t, participantCount> sequenceNums{};
    int32_t version = 1;
    blocksci::uint256 hash;
    blocksci::DataAccess access;
    blocksci::RawTransaction *rawTx;
    blocksci::TxData data;

public:
    blocksci::Transaction transaction;

    SyntheticWasabi2JoinMarketOverlap()
        : rawTx(new (storage.data()) blocksci::RawTransaction(0, 0, 0, participantCount, participantCount)),
          data{rawTx, &version, &hash, spentOutputNums.data(), sequenceNums.data()},
          // At this height Wasabi 2's production minimum is 20 inputs.
          transaction(data, 0, 900'000, 1, access) {
        auto *inouts = reinterpret_cast<blocksci::Inout *>(rawTx + 1);
        for (uint16_t index = 0; index < participantCount; ++index) {
            new (&inouts[index]) blocksci::Inout(
                0, index, blocksci::AddressType::WITNESS_PUBKEYHASH, wasabi2Denomination);
            new (&inouts[participantCount + index]) blocksci::Inout(
                0, participantCount + index, blocksci::AddressType::WITNESS_PUBKEYHASH, wasabi2Denomination);
        }
    }
};

}  // namespace

TEST(JoinMarketCoinJoinTest, RecognizesOverlapButClassifiesAsWasabi2) {
    SyntheticWasabi2JoinMarketOverlap overlap;

    ASSERT_TRUE(blocksci::heuristics::isWasabi2CoinJoin(overlap.transaction));
    EXPECT_TRUE(blocksci::heuristics::isJoinMarketCoinJoin(overlap.transaction));
    EXPECT_TRUE(blocksci::heuristics::isCoinjoinOfGivenType(overlap.transaction, "joinmarket"));
    EXPECT_EQ(blocksci::heuristics::getCoinjoinTag(overlap.transaction),
              blocksci::heuristics::CoinJoinType::WW2PostzkSNACKs);
}

TEST(JoinMarketCoinJoinTest, RejectsWasabi2OnlyMinInputCountOverride) {
    SyntheticWasabi2JoinMarketOverlap overlap;

    EXPECT_THROW(blocksci::heuristics::isCoinjoinOfGivenType(
                     overlap.transaction, "joinmarket", std::nullopt, std::optional<uint64_t>{5}),
                 std::invalid_argument);
}

TEST(Wasabi2CoinJoinTest, AppliesMinInputCountOverride) {
    SyntheticWasabi2JoinMarketOverlap transaction;

    EXPECT_FALSE(blocksci::heuristics::isCoinjoinOfGivenType(
        transaction.transaction, "wasabi2", std::nullopt, std::optional<uint64_t>{50}));
    EXPECT_TRUE(blocksci::heuristics::isCoinjoinOfGivenType(
        transaction.transaction, "wasabi2", std::nullopt, std::optional<uint64_t>{20}));
}
