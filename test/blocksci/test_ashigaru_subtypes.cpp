#include "gtest/gtest.h"

#include <blocksci/chain/transaction.hpp>
#include <blocksci/core/inout.hpp>
#include <blocksci/core/raw_transaction.hpp>
#include <blocksci/heuristics/tx_identification.hpp>

#include "../../src/internal/data_access.hpp"

#include <array>
#include <cstddef>
#include <new>
#include <string>

namespace {

constexpr uint16_t participantCount = 5;
constexpr int64_t largePoolSatoshis = 25'000'000;
constexpr int64_t smallPoolSatoshis = 2'500'000;

// isAshigaruCoinJoin only reads the transaction's input/output values.  This
// minimal in-memory transaction makes the subtype test independent of a block
// fixture and exercises isCoinjoinOfGivenType directly.
class SyntheticAshigaruTransaction {
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

    explicit SyntheticAshigaruTransaction(int64_t poolSize, int64_t firstInputValue = 0)
        : rawTx(new (storage.data()) blocksci::RawTransaction(0, 0, 0, participantCount, participantCount)),
          data{rawTx, &version, &hash, spentOutputNums.data(), sequenceNums.data()},
          transaction(data, 0, 900'000, 1, access) {
        auto *inouts = reinterpret_cast<blocksci::Inout *>(rawTx + 1);
        for (uint16_t index = 0; index < participantCount; ++index) {
            const auto inputValue = index == 0 && firstInputValue != 0 ? firstInputValue : poolSize;
            new (&inouts[index]) blocksci::Inout(0, 0, blocksci::AddressType::PUBKEYHASH, inputValue);
            new (&inouts[participantCount + index]) blocksci::Inout(
                0, 0, blocksci::AddressType::PUBKEYHASH, poolSize);
        }
    }
};

bool isAshigaruOfSubtype(const blocksci::Transaction &transaction, const std::string &subtype) {
    return blocksci::heuristics::isCoinjoinOfGivenType(transaction, "ashigaru", subtype);
}

}  // namespace

TEST(AshigaruSubtypeTest, ClassifiesBothOfficialPoolSizes) {
    SyntheticAshigaruTransaction largePool{largePoolSatoshis};
    EXPECT_TRUE(isAshigaruOfSubtype(largePool.transaction, "25m"));
    EXPECT_FALSE(isAshigaruOfSubtype(largePool.transaction, "2.5m"));

    SyntheticAshigaruTransaction smallPool{smallPoolSatoshis};
    EXPECT_TRUE(isAshigaruOfSubtype(smallPool.transaction, "2.5m"));
    EXPECT_FALSE(isAshigaruOfSubtype(smallPool.transaction, "25m"));
    EXPECT_FALSE(isAshigaruOfSubtype(smallPool.transaction, "250k"));
}

TEST(AshigaruSubtypeTest, UsesTheEqualOutputPoolSizeRatherThanTheFirstInput) {
    // Ashigaru permits an input to differ from the pool size by fewer than
    // 110,000 satoshis.  The subtype must still be determined by its equal
    // outputs, which define the pool denomination.
    SyntheticAshigaruTransaction smallPoolWithNearPoolInput{
        smallPoolSatoshis, smallPoolSatoshis + 1};
    EXPECT_TRUE(isAshigaruOfSubtype(smallPoolWithNearPoolInput.transaction, "2.5m"));
}
