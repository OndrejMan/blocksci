//
//  test_block_range.cpp
//  blocksci_unittest
//

#include "unit_test.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace blocksci {

class BlockRangeTest : public BlockSciTest {};

/**
 Checks the general contract of BlockRange::segment() on the configured test chain.
 Segment counts up to 64 cover the expected range of thread counts used by mapReduce().
 */
TEST_F(BlockRangeTest, SegmentCoversWholeChain) {
    BlockHeight chainStart = (*chain.begin()).height();
    uint32_t chainTxCount = chain.endTxIndex() - chain.firstTxIndex();

    for (unsigned int segmentCount = 1; segmentCount <= 64; segmentCount++) {
        auto segments = chain.segment(segmentCount);

        ASSERT_FALSE(segments.empty()) << "segment count " << segmentCount;
        // segment() splits on transaction counts, so it may overshoot the requested
        // number slightly. Every segment holds at least one block though, which is
        // what the runaway loop used to violate.
        ASSERT_LE(segments.size(), static_cast<size_t>(chain.size())) << "segment count " << segmentCount;

        BlockHeight expectedStart = chainStart;
        uint32_t txCount = 0;
        for (auto &segment : segments) {
            ASSERT_GT(segment.size(), BlockHeight{0}) << "segment count " << segmentCount;
            EXPECT_EQ((*segment.begin()).height(), expectedStart) << "segment count " << segmentCount;
            expectedStart += segment.size();
            txCount += segment.endTxIndex() - segment.firstTxIndex();
        }

        EXPECT_EQ(expectedStart, chainStart + chain.size()) << "segment count " << segmentCount;
        EXPECT_EQ(txCount, chainTxCount) << "segment count " << segmentCount;
    }
}

TEST_F(BlockRangeTest, SegmentRejectsZeroCount) {
    EXPECT_THROW(chain.segment(0), std::invalid_argument);
}

namespace {

struct SyntheticBlock {
    uint32_t firstTransaction;

    uint32_t firstTxIndex() const {
        return firstTransaction;
    }
};

}  // namespace

// Regression input for the end-iterator guard in BlockRange::segment(). The target
// transaction lies inside the final block, so no block begins at or after it. The
// production code must detect this result and let the final segment cover the remainder.
TEST(BlockRangeSegmentBoundaryTest, DenseTrailingBlockReturnsChainEnd) {
    const std::vector<SyntheticBlock> blocks = {{0}, {10}, {11}, {12}};
    constexpr uint32_t lastTx = 112;
    constexpr unsigned int segmentCount = 4;
    const uint32_t segmentSize = lastTx / segmentCount;

    ASSERT_GT(lastTx - blocks.front().firstTxIndex(), segmentSize);

    const auto endIt = std::lower_bound(
        blocks.begin(), blocks.end(), blocks.front().firstTxIndex() + segmentSize,
        [](const SyntheticBlock &block, uint32_t txNum) {
            return block.firstTxIndex() < txNum;
        });

    EXPECT_EQ(endIt, blocks.end());
}

}  // namespace blocksci
