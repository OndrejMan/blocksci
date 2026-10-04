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

constexpr uint16_t inputCount = 2;
constexpr uint16_t outputCount = 2;
constexpr int64_t inputValue = 1'500'000'000;
constexpr int64_t largeOutputValue = 2'900'000'000;
constexpr int64_t smallOutputValue = 100'000'000;

class SyntheticLargeConsolidation {
    alignas(std::max_align_t)
        std::array<std::byte, sizeof(blocksci::RawTransaction) + (inputCount + outputCount) * sizeof(blocksci::Inout)>
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

    SyntheticLargeConsolidation()
        : rawTx(new (storage.data()) blocksci::RawTransaction(0, 0, 0, inputCount, outputCount)),
          data{rawTx, &version, &hash, spentOutputNums.data(), sequenceNums.data()},
          transaction(data, 0, 1, 1, access) {
        auto *inouts = reinterpret_cast<blocksci::Inout *>(rawTx + 1);
        for (uint16_t index = 0; index < inputCount; ++index) {
            new (&inouts[index]) blocksci::Inout(
                0, index, blocksci::AddressType::PUBKEYHASH, inputValue);
        }
        new (&inouts[inputCount]) blocksci::Inout(
            0, inputCount, blocksci::AddressType::PUBKEYHASH, largeOutputValue);
        new (&inouts[inputCount + 1]) blocksci::Inout(
            0, inputCount + 1, blocksci::AddressType::PUBKEYHASH, smallOutputValue);
    }
};

}  // namespace

TEST(ConsolidationTypeTest, SumsLargeOutputsInInt64) {
    SyntheticLargeConsolidation transaction;

    // The outputs sum to 30 BTC. A 32-bit accumulate seed overflows and turns
    // the input-vs-total check into a false negative.
    EXPECT_EQ(blocksci::heuristics::getConsolidationType(transaction.transaction),
              blocksci::heuristics::ConsolidationType::Possible);
}
