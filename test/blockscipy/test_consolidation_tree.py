import pytest


def hashes(transactions):
    return {str(tx.hash) for tx in transactions}


@pytest.mark.btc
def test_consolidation_tree_groups_reached_transactions_by_hop(
    converging_consolidation_chain, converging_consolidation_data
):
    chain = converging_consolidation_chain
    data = converging_consolidation_data
    coinjoin = chain.tx_with_hash(data["converging-coinjoin-tx"])

    trees = chain.get_coinjoin_consolidation_tree(0, len(chain), "joinmarket", 3)

    assert hashes(trees) == {data["converging-coinjoin-tx"]}
    tree = trees[coinjoin]
    assert {hop: hashes(reached) for hop, reached in tree.items()} == {
        1: {data["converging-short-branch-tx"], data["converging-long-branch-first-tx"]},
        2: {data["converging-long-branch-second-tx"], data["converging-consolidation-tx"]},
    }
    short_branch = chain.tx_with_hash(data["converging-short-branch-tx"])
    assert [(output.tx_index, output.index) for output in tree[1][short_branch]] == [(coinjoin.index, 0)]


@pytest.mark.btc
def test_consolidation_tree_stops_at_max_hops(converging_consolidation_chain, converging_consolidation_data):
    chain = converging_consolidation_chain
    coinjoin = chain.tx_with_hash(converging_consolidation_data["converging-coinjoin-tx"])

    tree = chain.get_coinjoin_consolidation_tree(0, len(chain), "joinmarket", 1)[coinjoin]

    assert list(tree) == [1]


@pytest.mark.btc
def test_consolidation_tree_merges_outputs_from_branches_of_different_length(
    converging_consolidation_chain, converging_consolidation_data
):
    chain = converging_consolidation_chain
    data = converging_consolidation_data
    coinjoin = chain.tx_with_hash(data["converging-coinjoin-tx"])
    consolidation = chain.tx_with_hash(data["converging-consolidation-tx"])

    tree = chain.get_coinjoin_consolidation_tree(0, len(chain), "joinmarket", 3)[coinjoin]

    # S is reached through A at hop 2, before the walk reaches C; the output
    # that arrives through B -> C must still be counted.
    reached = sorted((output.tx_index, output.index) for output in tree[2][consolidation])
    assert reached == [(coinjoin.index, 0), (coinjoin.index, 1)]
