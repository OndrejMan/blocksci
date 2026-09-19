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

constexpr uint16_t inputCount = 3;
constexpr uint16_t mixOutputCount = 5;
constexpr int64_t mixDenomination = 1'000'000;

class SyntheticUnderfundedJoinMarketShape {
    alignas(std::max_align_t)
        std::array<std::byte, sizeof(blocksci::RawTransaction) + (inputCount + mixOutputCount) * sizeof(blocksci::Inout)>
            storage{};
    std::array<uint16_t, inputCount> spentOutputNums{};
    std::array<uint32_t, inputCount> sequenceNums{};
    int32_t version = 1;
    blocksci::uint256 hash;
    blocksci::DataAccess access;
    blocksci::RawTransaction *rawTx;
    blocksci::TxData data;

public:
    blocksci::Transaction transaction;

    SyntheticUnderfundedJoinMarketShape()
        : rawTx(new (storage.data()) blocksci::RawTransaction(0, 0, 0, inputCount, mixOutputCount)),
          data{rawTx, &version, &hash, spentOutputNums.data(), sequenceNums.data()},
          transaction(data, 0, 1, 1, access) {
        auto *inouts = reinterpret_cast<blocksci::Inout *>(rawTx + 1);
        for (uint16_t index = 0; index < inputCount; ++index) {
            new (&inouts[index]) blocksci::Inout(
                0, index, blocksci::AddressType::WITNESS_PUBKEYHASH, mixDenomination);
        }
        for (uint16_t index = 0; index < mixOutputCount; ++index) {
            new (&inouts[inputCount + index]) blocksci::Inout(
                0, inputCount + index, blocksci::AddressType::WITNESS_PUBKEYHASH, mixDenomination);
        }
    }
};

}  // namespace

TEST(JoinMarketCoinJoinTest, RejectsFewerInputsThanEqualMixOutputs) {
    SyntheticUnderfundedJoinMarketShape transaction;

    EXPECT_FALSE(blocksci::heuristics::isJoinMarketCoinJoin(transaction.transaction));
    EXPECT_FALSE(blocksci::heuristics::isCoinjoinOfGivenType(transaction.transaction, "joinmarket"));
}
