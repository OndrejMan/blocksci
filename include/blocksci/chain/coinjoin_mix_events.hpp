#ifndef BLOCKSCI_COINJOIN_MIX_EVENTS_HPP
#define BLOCKSCI_COINJOIN_MIX_EVENTS_HPP

#include <optional>
#include <string>
#include <unordered_set>

namespace blocksci {

    /**
     * Movement of a CoinJoin input or output across the mix boundary, the
     * transaction-centric liquidity classification of Svenda et al. (PETS 2026,
     * section 4.3.2). The names match coinjoin-analysis' MIX_EVENT_TYPE.
     */
    enum class MixEventType { Enter, Remix, Leave, Stay };

    inline std::string mixEventName(MixEventType event) {
        switch (event) {
            case MixEventType::Enter:
                return "MIX_ENTER";
            case MixEventType::Remix:
                return "MIX_REMIX";
            case MixEventType::Leave:
                return "MIX_LEAVE";
            case MixEventType::Stay:
                return "MIX_STAY";
        }
        return "";
    }

    namespace detail {

        /** An input brings liquidity into the mix unless it spends an output of another CoinJoin. */
        template <typename Transaction>
        MixEventType classifyCoinjoinInput(const std::unordered_set<Transaction> &coinjoins,
                                           const Transaction &spentTransaction) {
            return coinjoins.find(spentTransaction) != coinjoins.end() ? MixEventType::Remix : MixEventType::Enter;
        }

        /**
         * An unspent output stays in the mix, one spent by a CoinJoin is remixed and
         * one spent by any other transaction leaves the mix.
         */
        template <typename Transaction>
        MixEventType classifyCoinjoinOutput(const std::unordered_set<Transaction> &coinjoins,
                                            const std::optional<Transaction> &spendingTransaction) {
            if (!spendingTransaction.has_value()) {
                return MixEventType::Stay;
            }
            return coinjoins.find(*spendingTransaction) != coinjoins.end() ? MixEventType::Remix : MixEventType::Leave;
        }

    }  // namespace detail
}  // namespace blocksci

#endif  // BLOCKSCI_COINJOIN_MIX_EVENTS_HPP
