import blocksci
import pytest

JOINMARKET_TAG = 5  # heuristics::CoinJoinType::JoinMarket


def hashes(transactions):
    return {str(tx.hash) for tx in transactions}


def linked_coinjoins(chain, data):
    return [chain.tx_with_hash(data[key]) for key in ["linked-joinmarket-first-tx", "linked-joinmarket-second-tx"]]


@pytest.mark.btc
def test_coinjoin_tag_of_joinmarket_and_regular_transactions(linked_coinjoin_chain, linked_coinjoin_data):
    chain = linked_coinjoin_chain
    coinjoins = linked_coinjoins(chain, linked_coinjoin_data)

    assert [tx.coinjoin_tag for tx in coinjoins] == [JOINMARKET_TAG, JOINMARKET_TAG]
    regular = [tx for tx in chain.blocks.txes.to_list() if tx not in coinjoins]
    assert {tx.coinjoin_tag for tx in regular} == {int(blocksci.CoinJoinType.NoCJ)}


@pytest.mark.btc
@pytest.mark.xfail(
    raises=AttributeError,
    strict=True,
    reason="the Python CoinJoinType enum lacks JoinMarket, so coinjoin_tag 5 has no name",
)
def test_coinjoin_type_names_the_joinmarket_tag(linked_coinjoin_chain, linked_coinjoin_data):
    coinjoin = linked_coinjoins(linked_coinjoin_chain, linked_coinjoin_data)[0]
    assert coinjoin.coinjoin_tag == int(blocksci.CoinJoinType.JoinMarket)


@pytest.mark.btc
def test_protocol_predicates_reject_joinmarket_coinjoins(linked_coinjoin_chain, linked_coinjoin_data):
    for tx in linked_coinjoins(linked_coinjoin_chain, linked_coinjoin_data):
        assert blocksci.heuristics.is_coinjoin(tx)
        assert not tx.is_wasabi1_coinjoin
        assert not tx.is_wasabi2_coinjoin
        assert not tx.is_whirlpool_coinjoin
        assert not tx.is_ashigaru_coinjoin


def test_mempool_space_link(chain, json_data):
    tx = chain.tx_with_hash(json_data["fan-8-tx"])
    assert tx.mempool_space_link == "https://mempool.space/tx/" + json_data["fan-8-tx"]


@pytest.mark.btc
def test_coinjoin_consolidations_find_the_converging_spend(
    converging_consolidation_chain, converging_consolidation_data
):
    chain = converging_consolidation_chain
    data = converging_consolidation_data

    consolidations = chain.get_coinjoin_consolidations(0, len(chain), 1.0, "joinmarket", 3)

    assert [str(coinjoin.hash) for coinjoin, _ in consolidations] == [data["converging-coinjoin-tx"]]
    (_, found), = consolidations
    assert {kind: hashes(txes) for kind, txes in found.items()} == {
        "certain": {data["converging-consolidation-tx"]},
        "possible": set(),
    }


@pytest.mark.btc
def test_traverses_between_coinjoins_ignore_remixes_of_the_same_type(
    linked_coinjoin_chain, linked_coinjoin_data
):
    chain = linked_coinjoin_chain
    first, second = linked_coinjoins(chain, linked_coinjoin_data)

    traverses = chain.find_traverses_between_coinjoins(0, len(chain))

    # The second JoinMarket CoinJoin spends outputs of the first one; a remix
    # within one protocol is not a traverse.
    assert set(traverses) <= {first.block_height, second.block_height}
    assert all(not counts for by_source in traverses.values() for counts in by_source.values())


@pytest.mark.btc
def test_wasabi_research_scans_find_nothing_without_wasabi_coinjoins(linked_coinjoin_chain):
    chain = linked_coinjoin_chain
    assert chain.find_friends_who_dont_pay(0, len(chain)) == []
    assert chain.find_hw_sw_coinjoins(0, len(chain)) == []
