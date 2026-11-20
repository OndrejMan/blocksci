#ifndef BLOCKSCI_COINJOIN_MIX_EVENTS_HPP
#define BLOCKSCI_COINJOIN_MIX_EVENTS_HPP

#include <optional>
#include <unordered_set>

namespace blocksci { namespace coinjoin {

    /**
     * Movement of a CoinJoin input or output across the mix boundary, the
     * transaction-centric liquidity classification of Svenda et al. (PETS 2026,
     * section 4.3.2). Python sees it as blocksci.MixEventType, whose names match
     * coinjoin-analysis' MIX_EVENT_TYPE.
     */
    enum class MixEventType { Enter, Remix, Leave, Stay };

    namespace detail {

        /** An input brings liquidity into the mix unless it spends an output of another CoinJoin. */
        template <typename Transaction>
        MixEventType classifyCoinjoinInput(const std::unordered_set<Transaction> &coinjoins,
                                           const Transaction &spentTransaction) {
            if (coinjoins.find(spentTransaction) != coinjoins.end()) {
                return MixEventType::Remix;
            }
            return MixEventType::Enter;
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
            if (coinjoins.find(*spendingTransaction) != coinjoins.end()) {
                return MixEventType::Remix;
            }
            return MixEventType::Leave;
        }

    }  // namespace detail
}}  // namespace blocksci::coinjoin

#endif  // BLOCKSCI_COINJOIN_MIX_EVENTS_HPP
