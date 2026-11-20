import pytest

import blocksci

ENTER = blocksci.MixEventType.MIX_ENTER
REMIX = blocksci.MixEventType.MIX_REMIX
LEAVE = blocksci.MixEventType.MIX_LEAVE
STAY = blocksci.MixEventType.MIX_STAY

# The fixture's first CoinJoin has five funded inputs and outputs 0-4 of equal
# value; the second CoinJoin spends exactly those five outputs. No other output
# of either CoinJoin is spent.
FIRST_MIX_OUTPUTS = 5
OUTPUT_COUNT = 10


def events_by_txid(chain, txids):
    events = chain.classify_coinjoin_mix_events([chain.tx_with_hash(txid) for txid in txids])
    return {str(tx.hash): tx_events for tx, tx_events in events.items()}


@pytest.mark.btc
def test_mix_events_of_linked_coinjoins(linked_coinjoin_chain, linked_coinjoin_data):
    first_txid = linked_coinjoin_data["linked-joinmarket-first-tx"]
    second_txid = linked_coinjoin_data["linked-joinmarket-second-tx"]

    events = events_by_txid(linked_coinjoin_chain, [first_txid, second_txid])

    assert events[first_txid] == {
        "inputs": [ENTER] * 5,
        "outputs": [REMIX] * FIRST_MIX_OUTPUTS + [STAY] * (OUTPUT_COUNT - FIRST_MIX_OUTPUTS),
    }
    assert events[second_txid] == {
        "inputs": [REMIX] * FIRST_MIX_OUTPUTS,
        "outputs": [STAY] * OUTPUT_COUNT,
    }


@pytest.mark.btc
def test_mix_events_are_relative_to_the_given_coinjoins(linked_coinjoin_chain, linked_coinjoin_data):
    # Without the second CoinJoin in the mix, spending the first one's outputs is a postmix spend.
    first_txid = linked_coinjoin_data["linked-joinmarket-first-tx"]

    events = events_by_txid(linked_coinjoin_chain, [first_txid])

    assert events[first_txid]["outputs"] == (
        [LEAVE] * FIRST_MIX_OUTPUTS + [STAY] * (OUTPUT_COUNT - FIRST_MIX_OUTPUTS)
    )


# The pipeline compares the names with coinjoin-analysis' MIX_EVENT_TYPE values.
def test_mix_event_names_match_coinjoin_analysis():
    assert [event.name for event in (ENTER, REMIX, LEAVE, STAY)] == ["MIX_ENTER", "MIX_REMIX", "MIX_LEAVE", "MIX_STAY"]


@pytest.mark.btc
def test_mix_events_of_no_coinjoins(linked_coinjoin_chain):
    assert linked_coinjoin_chain.classify_coinjoin_mix_events([]) == {}
