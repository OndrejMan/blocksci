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

namespace {

constexpr uint16_t participantCount = 50;
constexpr int64_t wasabi2Denomination = 10'000'000;

// This transaction deliberately satisfies the broad JoinMarket shape while
// satisfying every Wasabi 2 invariant: 50 ordered, unique P2WPKH inputs and
// 50 ordered, unique equal-valued Wasabi 2-denomination outputs.
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
          // At this height Wasabi 2 requires 50 inputs, matching this fixture.
          transaction(data, 0, 800'000, 1, access) {
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

TEST(JoinMarketCoinJoinTest, ExcludesTransactionsRecognizedAsWasabi2) {
    SyntheticWasabi2JoinMarketOverlap overlap;

    ASSERT_TRUE(blocksci::heuristics::isWasabi2CoinJoin(overlap.transaction));
    EXPECT_FALSE(blocksci::heuristics::isJoinMarketCoinJoin(overlap.transaction));
    EXPECT_FALSE(blocksci::heuristics::isCoinjoinOfGivenType(overlap.transaction, "joinmarket"));
}
