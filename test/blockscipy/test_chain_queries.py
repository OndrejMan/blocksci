import collections
import operator

import blocksci
import pytest


def all_txes(chain):
    return [tx for block in chain for tx in block]


def test_tx_with_index_round_trips(chain):
    for tx in all_txes(chain):
        assert chain.tx_with_index(tx.index) == tx


def test_tx_with_index_rejects_out_of_range_index(chain):
    last = all_txes(chain)[-1]
    with pytest.raises(IndexError):
        chain.tx_with_index(last.index + 1)


def test_address_from_index_round_trips(chain, json_data):
    for key in ["address-p2pkh-spend-0", "address-p2sh-spend-1", "merge-addr-1"]:
        address = chain.address_from_string(json_data[key])
        assert chain.address_from_index(address.address_num, address.type) == address


@pytest.mark.parametrize(
    "key",
    [
        "address-p2pkh-spend-0",
        "address-p2sh-spend-0",
        pytest.param("address-p2wpkh-spend-0", marks=[pytest.mark.btc, pytest.mark.ltc]),
        pytest.param("address-p2wsh-spend-0", marks=[pytest.mark.btc, pytest.mark.ltc]),
    ],
)
def test_addresses_with_prefix(chain, json_data, key):
    address_string = json_data[key]
    prefix = address_string[:8]

    found = chain.addresses_with_prefix(prefix)

    assert chain.address_from_string(address_string) in found
    assert all(address.address_string.startswith(prefix) for address in found)


def test_addresses_with_prefix_ignores_an_empty_prefix(chain):
    assert chain.addresses_with_prefix("") == []


def test_reload_keeps_the_chain_readable(chain):
    reopened = blocksci.Blockchain(chain.config_location)
    tip_hash = str(reopened[-1].hash)

    reopened.reload()

    assert len(reopened) == len(chain)
    assert str(reopened[-1].hash) == tip_hash


@pytest.mark.parametrize("fee", [0, 9_999, 10_000])
def test_filter_fee_greater_than(chain, fee):
    expected = [tx for tx in all_txes(chain) if tx.fee > fee]
    assert chain.filter_fee_greater_than(0, len(chain), fee) == expected


def test_filter_in_keys(chain, json_data):
    keys = {json_data["fan-8-tx"], json_data["op-return-tx"]}
    found = chain.filter_in_keys(keys, 0, len(chain))
    assert {str(tx.hash) for tx in found} == keys


def test_filter_seen_in_timeframe(chain):
    start_time = chain[10].timestamp
    end_time = chain[len(chain) // 2].timestamp
    expected = [block for block in chain if start_time <= block.timestamp <= end_time]

    assert chain.filter_seen_in_timeframe(0, len(chain), start_time, end_time) == expected


def test_get_address_types_counts_outputs(chain):
    expected = collections.Counter(output.address_type for tx in all_txes(chain) for output in tx.outputs)
    assert chain.get_address_types(0, len(chain)) == dict(expected)


def test_get_count_of_address_types_for_each_day(chain):
    expected = collections.defaultdict(collections.Counter)
    for tx in all_txes(chain):
        for output in tx.outputs:
            expected[output.address_type][tx.block.timestamp // 86400] += 1

    counts = chain.get_count_of_address_types_for_each_day(0, len(chain))
    assert {typ: dict(days) for typ, days in counts.items()} == {typ: dict(days) for typ, days in expected.items()}


@pytest.mark.parametrize("cpu_count", [1, 3])
def test_mapreduce_txes(chain, cpu_count):
    total = chain.mapreduce_txes(lambda tx: tx.output_value, operator.add, 0, cpu_count=cpu_count)
    assert total == sum(tx.output_value for tx in all_txes(chain))


@pytest.mark.parametrize("cpu_count", [1, 3])
def test_mapreduce_blocks(chain, cpu_count):
    total = chain.mapreduce_blocks(lambda block: block.tx_count, operator.add, 0, cpu_count=cpu_count)
    assert total == len(all_txes(chain))


@pytest.mark.parametrize("cpu_count", [1, 3])
def test_mapreduce_block_ranges(chain, cpu_count):
    total = chain.mapreduce_block_ranges(lambda blocks: len(blocks), operator.add, cpu_count=cpu_count)
    assert total == len(chain)


def test_mapreduce_blocks_over_a_subrange(chain):
    heights = chain.mapreduce_blocks(lambda block: [block.height], operator.add, [], start=10, end=20, cpu_count=3)
    assert sorted(heights) == list(range(10, 20))


@pytest.mark.xfail(
    raises=TypeError,
    strict=True,
    reason="with start but no end, mapreduce_block_ranges passes end=None to _segment_indexes",
)
def test_mapreduce_blocks_from_start_to_the_tip(chain):
    count = chain.mapreduce_blocks(lambda block: 1, operator.add, 0, start=10, cpu_count=3)
    assert count == len(chain) - 10


def test_map_blocks(chain):
    assert chain.map_blocks(lambda block: block.height, cpu_count=3) == list(range(len(chain)))


def test_most_valuable_addresses(chain):
    balances = collections.Counter()
    for tx in all_txes(chain):
        for output in tx.outputs:
            if not output.is_spent:
                balances[output.address] += output.value

    richest = chain.most_valuable_addresses(5)

    assert [value for _, value in richest] == sorted(balances.values(), reverse=True)[:5]
    assert all(balances[address] == value for address, value in richest)
