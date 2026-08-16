//
//  test_block_range.cpp
//  blocksci_unittest
//

#include "unit_test.h"


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

}  // namespace blocksci
