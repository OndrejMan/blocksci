# change heuristics are tested in test_change.py

import blocksci
import pytest


def test_simple_coinjoin(chain, json_data):
    tx = chain.tx_with_hash(json_data["simple-coinjoin-tx"])
    assert not blocksci.heuristics.is_coinjoin(tx)


@pytest.mark.parametrize("detector", ["possible", "definite"])
def test_subset_matching_scan_finds_simple_coinjoin(chain, json_data, detector):
    detected, skipped = chain.scan_coinjoins_by_subset_matching(
        0,
        len(chain),
        detector,
        min_base_fee=10_000,
        percentage_fee=0.0,
        max_depth=200_000,
    )

    assert json_data["simple-coinjoin-tx"] in {str(tx.hash) for tx in detected}
    assert skipped == []


@pytest.mark.parametrize(
    ("detector", "min_base_fee", "percentage_fee", "error"),
    [
        ("unsupported", 0, 0.0, 'detector must be "possible" or "definite"'),
        ("definite", -1, 0.0, "min_base_fee must be non-negative"),
        ("definite", 0, float("nan"), "percentage_fee must be finite and between 0 and 1"),
    ],
)
def test_subset_matching_scan_rejects_invalid_parameters(
    chain, detector, min_base_fee, percentage_fee, error
):
    with pytest.raises(ValueError, match=error):
        chain.scan_coinjoins_by_subset_matching(
            0,
            len(chain),
            detector,
            min_base_fee=min_base_fee,
            percentage_fee=percentage_fee,
        )


def test_no_coinjoin(chain, json_data):
    for key in [
        "fan-8-tx",
        "peeling-chain-4-tx",
        "tx-chain-10-tx-1",
        "funding-tx-2-in-2-out",
    ]:
        tx = chain.tx_with_hash(json_data[key])
        assert not blocksci.heuristics.is_coinjoin(tx)


def test_is_peeling_chain(chain, json_data):
    for i in range(0, 9):
        txid = json_data["peeling-chain-{}-tx".format(i)]
        tx = chain.tx_with_hash(txid)
        assert blocksci.heuristics.is_peeling_chain(tx)


def test_no_peeling_chain(chain, json_data):
    for key in [
        "fan-8-tx",
        "simple-coinjoin-tx",
        "tx-chain-10-tx-1",
    ]:
        tx = chain.tx_with_hash(json_data[key])
        assert not blocksci.heuristics.is_peeling_chain(tx)
