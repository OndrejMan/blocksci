#include "gtest/gtest.h"

#include <blocksci/chain/coinjoin_mix_events.hpp>

#include <optional>
#include <string>
#include <unordered_set>

using blocksci::MixEventType;
using blocksci::mixEventName;
using blocksci::detail::classifyCoinjoinInput;
using blocksci::detail::classifyCoinjoinOutput;

namespace {
    // A and B are the CoinJoins of the mix; F funds A and P is a postmix spend.
    const std::unordered_set<std::string> coinjoins = {"A", "B"};
}  // namespace

TEST(CoinjoinMixEventsTest, InputFromAnotherCoinjoinIsRemix) {
    EXPECT_EQ(classifyCoinjoinInput(coinjoins, std::string{"A"}), MixEventType::Remix);
}

TEST(CoinjoinMixEventsTest, InputFromOutsideTheMixEnters) {
    EXPECT_EQ(classifyCoinjoinInput(coinjoins, std::string{"F"}), MixEventType::Enter);
}

TEST(CoinjoinMixEventsTest, OutputSpentByCoinjoinIsRemix) {
    EXPECT_EQ(classifyCoinjoinOutput(coinjoins, std::optional<std::string>{"B"}), MixEventType::Remix);
}

TEST(CoinjoinMixEventsTest, OutputSpentOutsideTheMixLeaves) {
    EXPECT_EQ(classifyCoinjoinOutput(coinjoins, std::optional<std::string>{"P"}), MixEventType::Leave);
}

TEST(CoinjoinMixEventsTest, UnspentOutputStays) {
    EXPECT_EQ(classifyCoinjoinOutput(coinjoins, std::optional<std::string>{}), MixEventType::Stay);
}

// The pipeline compares these names with coinjoin-analysis' MIX_EVENT_TYPE values.
TEST(CoinjoinMixEventsTest, NamesMatchCoinjoinAnalysis) {
    EXPECT_EQ(mixEventName(MixEventType::Enter), "MIX_ENTER");
    EXPECT_EQ(mixEventName(MixEventType::Remix), "MIX_REMIX");
    EXPECT_EQ(mixEventName(MixEventType::Leave), "MIX_LEAVE");
    EXPECT_EQ(mixEventName(MixEventType::Stay), "MIX_STAY");
}
