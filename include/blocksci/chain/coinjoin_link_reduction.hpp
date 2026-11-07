#ifndef BLOCKSCI_COINJOIN_LINK_REDUCTION_HPP
#define BLOCKSCI_COINJOIN_LINK_REDUCTION_HPP

#include <blocksci/chain/input.hpp>
#include <blocksci/chain/transaction.hpp>

#include <unordered_set>

namespace blocksci {
    namespace detail {

        /**
         * Return every candidate CoinJoin which has a direct candidate CoinJoin
         * parent, together with every such parent.
         *
         * A transaction may spend outputs from more than one candidate parent;
         * inspect every input so that all direct links are retained.
         */
        inline std::unordered_set<Transaction> findLinkedCoinjoinTransactions(
            const std::unordered_set<Transaction> &candidates) {
            std::unordered_set<Transaction> result;
            for (const auto &transaction : candidates) {
                for (const auto &input : transaction.inputs()) {
                    const auto spentTransaction = input.getSpentTx();
                    if (candidates.find(spentTransaction) != candidates.end()) {
                        result.insert(transaction);
                        result.insert(spentTransaction);
                    }
                }
            }
            return result;
        }

    }  // namespace detail
}  // namespace blocksci

#endif  // BLOCKSCI_COINJOIN_LINK_REDUCTION_HPP
