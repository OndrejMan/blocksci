#ifndef BLOCKSCI_COINJOIN_LINK_REDUCTION_HPP
#define BLOCKSCI_COINJOIN_LINK_REDUCTION_HPP

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
        template <typename Transaction, typename InputsForTransaction, typename SpentTransactionForInput>
        std::unordered_set<Transaction> findLinkedCoinjoinTransactions(
            const std::unordered_set<Transaction> &candidates, InputsForTransaction inputsForTransaction,
            SpentTransactionForInput spentTransactionForInput) {
            std::unordered_set<Transaction> result;
            for (const auto &transaction : candidates) {
                for (const auto &input : inputsForTransaction(transaction)) {
                    const auto spentTransaction = spentTransactionForInput(input);
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
