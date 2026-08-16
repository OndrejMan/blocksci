import json
import os
import subprocess

import pytest


@pytest.fixture(scope="session")
def linked_coinjoin_chain(tmpdir_factory):
    chain_dir = str(tmpdir_factory.mktemp("linked_coinjoin"))
    test_dir = os.path.dirname(os.path.realpath(__file__))
    fixture_dir = os.path.join(test_dir, "../files/linked-coinjoin/btc/regtest")
    config_path = os.path.join(chain_dir, "config.json")

    subprocess.run(
        [
            "blocksci_parser",
            config_path,
            "generate-config",
            "bitcoin_regtest",
            chain_dir,
            "--disk",
            fixture_dir,
        ],
        check=True,
    )
    subprocess.run(["blocksci_parser", config_path, "update"], check=True)

    import blocksci
    return blocksci.Blockchain(config_path)


def txids(transactions):
    return {str(tx.hash) for tx in transactions}


def test_linked_coinjoin_filter_excludes_false_positive_predecessor(linked_coinjoin_chain):
    test_dir = os.path.dirname(os.path.realpath(__file__))
    with open(os.path.join(test_dir, "../files/linked-coinjoin/btc/output.json")) as output_file:
        fixture_data = json.load(output_file)

    first_txid = fixture_data["linked-joinmarket-first-tx"]
    second_txid = fixture_data["linked-joinmarket-second-tx"]

    linked = linked_coinjoin_chain.filter_coinjoin_txes(0, len(linked_coinjoin_chain), "joinmarket")
    assert second_txid in txids(linked)

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
def test_linked_coinjoin_filter_rejects_negative_min_input_count(linked_coinjoin_chain):
    with pytest.raises(ValueError, match="min_input_count must be non-negative"):
        linked_coinjoin_chain.filter_coinjoin_txes(
            0,
            len(linked_coinjoin_chain),
            "wasabi2",
            min_input_count=-1,
        )
