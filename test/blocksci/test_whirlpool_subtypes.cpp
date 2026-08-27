#include "gtest/gtest.h"

#include <blocksci/chain/transaction.hpp>
#include <blocksci/core/inout.hpp>
#include <blocksci/core/raw_transaction.hpp>
#include <blocksci/heuristics/tx_identification.hpp>

#include "../../src/internal/data_access.hpp"

#include <array>
#include <cstddef>
#include <new>

namespace {

constexpr uint16_t participantCount = 5;
constexpr int64_t poolSatoshis = 5'000'000;

// Whirlpool accepts fee-paying inputs that differ by fewer than 110,000 sats
// from the equal-valued outputs. Construct the smallest valid transaction so
// subtype detection can be exercised without a chain fixture.
class SyntheticWhirlpoolTransaction {
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

    explicit SyntheticWhirlpoolTransaction(int64_t firstInputValue)
        : rawTx(new (storage.data()) blocksci::RawTransaction(0, 0, 0, participantCount, participantCount)),
          data{rawTx, &version, &hash, spentOutputNums.data(), sequenceNums.data()},
          transaction(data, 0, 900'000, 1, access) {
        auto *inouts = reinterpret_cast<blocksci::Inout *>(rawTx + 1);
        for (uint16_t index = 0; index < participantCount; ++index) {
            const auto inputValue = index == 0 ? firstInputValue : poolSatoshis;
            new (&inouts[index]) blocksci::Inout(0, 0, blocksci::AddressType::PUBKEYHASH, inputValue);
            new (&inouts[participantCount + index]) blocksci::Inout(
                0, 0, blocksci::AddressType::PUBKEYHASH, poolSatoshis);
        }
    }
};

}  // namespace

TEST(WhirlpoolSubtypeTest, UsesTheEqualOutputPoolSizeRatherThanTheFirstInput) {
    SyntheticWhirlpoolTransaction poolWithNearPoolFirstInput{poolSatoshis + 1};

    EXPECT_TRUE(blocksci::heuristics::isWhirlpoolCoinJoin(poolWithNearPoolFirstInput.transaction));
    EXPECT_TRUE(blocksci::heuristics::isCoinjoinOfGivenType(
        poolWithNearPoolFirstInput.transaction, "whirlpool", "5m"));
    EXPECT_FALSE(blocksci::heuristics::isCoinjoinOfGivenType(
        poolWithNearPoolFirstInput.transaction, "whirlpool", "1m"));
}
