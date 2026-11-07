import statistics

import blocksci
import pytest


def spent_outputs(chain):
    return [output for output in chain.blocks.outputs.to_list() if output.is_spent]


def output_age(output):
    return output.spending_tx.block_height - output.tx.block_height


def input_age(inpt):
    return inpt.tx.block_height - inpt.spent_tx.block_height


def split_threshold(values):
    """A threshold with values on both sides of it."""
    threshold = int(statistics.median(values))
    assert min(values) < threshold <= max(values)
    return threshold


def test_inputs_sent_before_and_after_height(chain):
    inputs = chain.blocks.inputs
    everything = inputs.to_list()
    height = split_threshold([inpt.spent_tx.block_height for inpt in everything])

    before = inputs.sent_before_height(height).to_list()
    after = inputs.sent_after_height(height).to_list()

    assert before == [inpt for inpt in everything if inpt.spent_tx.block_height < height]
    assert after == [inpt for inpt in everything if inpt.spent_tx.block_height >= height]


def test_inputs_with_age_split_at_the_given_age(chain):
    inputs = chain.blocks.inputs
    everything = inputs.to_list()
    age = split_threshold([input_age(inpt) for inpt in everything])

    younger = inputs.with_age_less_than(age).to_list()
    older = inputs.with_age_greater_than(age).to_list()

    assert younger == [inpt for inpt in everything if input_age(inpt) < age]
    assert older == [inpt for inpt in everything if input_age(inpt) >= age]


def test_inputs_with_address_type(chain):
    everything = chain.blocks.inputs.to_list()
    typ = blocksci.address_type.pubkeyhash

    selected = chain.blocks.inputs.with_address_type(typ).to_list()

    assert selected
    assert selected == [inpt for inpt in everything if inpt.address_type == typ]


def test_outputs_unspent(chain):
    everything = chain.blocks.outputs.to_list()
    assert chain.blocks.outputs.unspent().to_list() == [output for output in everything if not output.is_spent]


def test_outputs_unspent_at_height(chain):
    everything = chain.blocks.outputs.to_list()
    height = split_threshold([output.spending_tx.block_height for output in spent_outputs(chain)])

    expected = [
        output for output in everything if not output.is_spent or output.spending_tx.block_height > height
    ]
    assert chain.blocks.outputs.unspent(height).to_list() == expected


def test_outputs_spent_before_and_after_height(chain):
    outputs = chain.blocks.outputs
    spent = spent_outputs(chain)
    height = split_threshold([output.spending_tx.block_height for output in spent])

    before = outputs.spent_before_height(height).to_list()
    after = outputs.spent_after_height(height).to_list()

    assert before == [output for output in spent if output.spending_tx.block_height < height]
    assert after == [output for output in spent if output.spending_tx.block_height >= height]


def test_outputs_spent_with_age_split_at_the_given_age(chain):
    outputs = chain.blocks.outputs
    spent = spent_outputs(chain)
    age = split_threshold([output_age(output) for output in spent])

    younger = outputs.spent_with_age_less_than(age).to_list()
    older = outputs.outputs_spent_with_age_greater_than(age).to_list()

    assert younger == [output for output in spent if output_age(output) < age]
    assert older == [output for output in spent if output_age(output) >= age]


@pytest.mark.xfail(
    strict=True,
    reason="the method is registered as outputs_spent_with_age_greater_than, unlike its spent_* siblings",
)
def test_outputs_spent_with_age_greater_than_follows_the_naming_of_its_siblings():
    for cls in (blocksci.OutputIterator, blocksci.OutputRange):
        assert hasattr(cls, "spent_with_age_greater_than")


def test_outputs_with_address_type(chain):
    everything = chain.blocks.outputs.to_list()
    typ = blocksci.address_type.nulldata

    selected = chain.blocks.outputs.with_address_type(typ).to_list()

    assert selected
    assert selected == [output for output in everything if output.address_type == typ]


def test_txes_including_output_of_type(chain):
    everything = chain.blocks.txes.to_list()
    typ = blocksci.address_type.multisig

    selected = chain.blocks.txes.including_output_of_type(typ).to_list()

    assert selected
    assert selected == [tx for tx in everything if any(output.address_type == typ for output in tx.outputs)]
    assert all(tx.includes_output_of_type(typ) for tx in selected)
    assert not any(tx.includes_output_of_type(typ) for tx in everything if tx not in selected)
