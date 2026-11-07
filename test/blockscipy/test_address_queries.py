import hashlib

import blocksci
import pytest

BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"


def base58check_payload(address_string):
    number = 0
    for char in address_string:
        number = number * 58 + BASE58_ALPHABET.index(char)
    raw = number.to_bytes(25, "big")
    payload, checksum = raw[:-4], raw[-4:]
    assert hashlib.sha256(hashlib.sha256(payload).digest()).digest()[:4] == checksum
    return payload[1:]


def test_address_types_lists_every_address_type():
    assert set(blocksci.address_type.types) == set(blocksci.address_type.__members__.values())


def test_first_tx_is_the_first_receiving_transaction(chain, json_data):
    for key in ["address-p2pkh-spend-2", "merge-addr-1"]:
        address = chain.address_from_string(json_data[key])
        receiving = sorted(address.output_txes.to_list(), key=lambda tx: tx.index)
        assert address.first_tx == receiving[0]


def test_deprecated_address_aliases(chain, json_data):
    address = chain.address_from_string(json_data["address-p2pkh-spend-2"])

    assert address.ins.to_list() == address.inputs.to_list()
    assert address.in_txes.to_list() == address.input_txes.to_list()
    assert address.out_txes.to_list() == address.output_txes.to_list()
    assert address.in_txes_count() == address.input_txes_count()
    assert address.out_txes_count() == address.output_txes_count()


@pytest.mark.btc
@pytest.mark.ltc
def test_scripthash_raw_address_is_the_encoded_hash(chain, json_data):
    address = chain.address_from_string(json_data["address-p2sh-spend-1"])
    # uint160 prints its bytes in reverse order, like Bitcoin Core's GetHex.
    script_hash = base58check_payload(address.address_string)
    assert str(address.raw_address) == script_hash[::-1].hex()


def test_equiv_address_reports_script_equivalence(chain, json_data):
    address = chain.address_from_string(json_data["address-p2sh-spend-1"])
    assert address.equiv(True).is_script_equiv
    assert not address.equiv(False).is_script_equiv


def test_multisig_pubkey_finds_its_multisig(chain, json_data):
    multisig = chain.tx_with_hash(json_data["raw-multisig-tx"]).outputs[0].address

    for pubkey in multisig.addresses.to_list():
        assert multisig in pubkey.find_multisigs().to_list()


def test_tagged_clusters(chain, json_data, tmpdir_factory):
    manager = blocksci.cluster.ClusterManager.create_clustering(
        str(tmpdir_factory.mktemp("tagged_clusters")),
        chain,
        heuristic=blocksci.heuristics.change.none,
    )
    tags = {
        chain.address_from_string(json_data["merge-addr-1"]): "merge",
        chain.address_from_string(json_data["address-p2pkh-spend-0"]): "spend",
    }

    tagged = manager.tagged_clusters(tags).to_list()

    assert len(tagged) == 2
    for tagged_cluster in tagged:
        (tagged_address,) = tagged_cluster.tagged_addresses.to_list()
        assert tags[tagged_address.address] == tagged_address.tag
        assert tagged_cluster.cluster.index == manager.cluster_with_address(tagged_address.address).index
