import blocksci
import pytest

COINJOIN_RESULT = blocksci.CoinJoinResult.__members__


def is_address_deanon(tx):
    if tx.is_coinbase or tx.output_count == 1:
        return False
    input_types = {inpt.address.type for inpt in tx.inputs}
    if len(input_types) != 1:
        return False
    (input_type,) = input_types
    return sum(output.address_type == input_type for output in tx.outputs) == 1


def test_is_address_deanon(chain, json_data):
    txes = chain.blocks.txes.to_list()
    flagged = [tx for tx in txes if blocksci.heuristics.is_address_deanon(tx)]

    assert chain.tx_with_hash(json_data["change-address-type-tx-0"]) in flagged
    assert flagged == [tx for tx in txes if is_address_deanon(tx)]


def test_is_change_over_requires_one_input_and_one_output_type(chain):
    flagged = [tx for tx in chain.blocks.txes.to_list() if blocksci.heuristics.is_change_over(tx)]

    assert flagged
    for tx in flagged:
        assert not tx.is_coinbase
        assert len({inpt.address.type for inpt in tx.inputs}) == 1
        assert len({output.address.type for output in tx.outputs}) == 1


@pytest.mark.parametrize(
    ("filter_name", "predicate_name"),
    [
        ("coinjoin_txes", "is_coinjoin"),
        ("address_deanon_txes", "is_address_deanon"),
        ("change_over_txes", "is_change_over"),
        ("keyset_change_txes", "is_keyset_change"),
    ],
)
def test_heuristic_tx_filters_match_their_predicates(chain, filter_name, predicate_name):
    txes = chain.blocks.txes
    predicate = getattr(blocksci.heuristics, predicate_name)

    selected = getattr(blocksci.heuristics, filter_name)(txes).to_list()

    assert selected == [tx for tx in txes.to_list() if predicate(tx)]


@pytest.mark.xfail(
    raises=TypeError,
    strict=True,
    reason="possible_coinjoin_txes passes is_possible_coinjoin, which needs fee parameters, as a predicate",
)
def test_possible_coinjoin_txes(chain, json_data):
    selected = blocksci.heuristics.possible_coinjoin_txes(chain.blocks.txes).to_list()
    assert json_data["simple-coinjoin-tx"] in {str(tx.hash) for tx in selected}


@pytest.mark.parametrize("detector_name", ["is_possible_coinjoin", "is_definite_coinjoin"])
def test_subset_matching_predicates_return_coinjoin_result_values(chain, json_data, detector_name):
    detector = getattr(blocksci.heuristics, detector_name)(10_000, 0.0, 200_000)

    assert detector(chain.tx_with_hash(json_data["simple-coinjoin-tx"])) == int(COINJOIN_RESULT["True"])
    assert detector(chain.tx_with_hash(json_data["fan-8-tx"])) == int(COINJOIN_RESULT["False"])
