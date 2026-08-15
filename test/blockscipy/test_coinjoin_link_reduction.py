import json

import pytest


def txids(transactions):
    return {str(tx.hash) for tx in transactions}


# Every protocol detector except JoinMarket is gated on a first-seen block
# height. Regtest chains sit far below the mainnet values, so the thresholds
# have to be zeroed in the chain configuration or detection can never fire on
# emulated data.
COINJOIN_THRESHOLDS = [
    "FirstSamouraiBlock",
    "FirstWasabiBlock",
    "FirstWasabi2Block",
    "FirstWasabiNoCoordAddressBlock",
    "FirstAshigaruBlock",
]


@pytest.mark.btc
def test_regtest_config_disables_coinjoin_height_thresholds(linked_coinjoin_chain):
    with open(linked_coinjoin_chain.config_location) as config_file:
        coinjoin_config = json.load(config_file)["chainConfig"]["coinJoinConfiguration"]

    for threshold in COINJOIN_THRESHOLDS:
        assert coinjoin_config[threshold] == 0, threshold


@pytest.mark.btc
def test_linked_coinjoin_filter_excludes_false_positive_predecessor(
    linked_coinjoin_chain, linked_coinjoin_data
):
    first_txid = linked_coinjoin_data["linked-joinmarket-first-tx"]
    second_txid = linked_coinjoin_data["linked-joinmarket-second-tx"]

    # The fixture's second CoinJoin spends five mix outputs from the first one.
    # A linked result must still contain exactly the two distinct endpoints.
    linked = linked_coinjoin_chain.filter_coinjoin_txes(0, len(linked_coinjoin_chain), "joinmarket")
    assert len(linked) == 2
    assert txids(linked) == {first_txid, second_txid}

    linked_without_false_positive = linked_coinjoin_chain.compute_anonymity_degradation(
        0,
        len(linked_coinjoin_chain),
        0,
        "joinmarket",
        coinjoinSubType="linked-regression",
        falseCoinjoins={first_txid},
    )
    assert txids(linked_without_false_positive) == set()


@pytest.mark.btc
def test_raw_coinjoin_filter_keeps_match_without_in_range_link(linked_coinjoin_chain):
    linked = linked_coinjoin_chain.filter_coinjoin_txes(0, len(linked_coinjoin_chain), "joinmarket")
    first_coinjoin_height = min(tx.block.height for tx in linked)

    # The first fixture CoinJoin is linked only by the second one, in the next
    # block. A single-block range therefore has a heuristic match but no
    # in-range CoinJoin-to-CoinJoin link to reduce to.
    raw = linked_coinjoin_chain.filter_coinjoin_txes_raw(
        first_coinjoin_height, first_coinjoin_height + 1, "joinmarket"
    )
    reduced = linked_coinjoin_chain.filter_coinjoin_txes(
        first_coinjoin_height, first_coinjoin_height + 1, "joinmarket"
    )

    assert len(raw) == 1
    assert not reduced


@pytest.mark.btc
def test_linked_coinjoin_filter_rejects_negative_min_input_count(linked_coinjoin_chain):
    with pytest.raises(ValueError, match="min_input_count must be non-negative"):
        linked_coinjoin_chain.filter_coinjoin_txes(
            0,
            len(linked_coinjoin_chain),
            "wasabi2",
            min_input_count=-1,
        )
