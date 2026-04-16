from blocksci.joinmarket_analysis import (
    MIX_ENTER,
    MIX_LEAVE,
    MIX_REMIX,
    MIX_STAY,
    analyze_joinmarket_liquidity,
)


def test_joinmarket_liquidity_classification():
    coinjoins = {
        "txA": {
            "txid": "txA",
            "broadcast_time": "2026-01-01 00:00:00.000",
            "inputs": {
                "0": {"value": 150000, "address": "inA"},
            },
            "outputs": {
                "0": {"value": 100000, "address": "outA0", "spend_by_tx": "vin_txB_0"},
                "1": {"value": 50000, "address": "outA1"},
            },
        },
        "txB": {
            "txid": "txB",
            "broadcast_time": "2026-01-01 00:10:00.000",
            "inputs": {
                "0": {"value": 100000, "address": "inB0", "spending_tx": "vout_txA_0"},
            },
            "outputs": {
                "0": {"value": 100000, "address": "outB0", "spend_by_tx": "vin_postmix_0"},
                "1": {"value": 30000, "address": "outB1"},
                "2": {"value": 30000, "address": "outB2"},
            },
        },
    }

    postmix = {
        "postmix": {
            "broadcast_time": "2026-01-01 00:12:00.000",
        }
    }

    result = analyze_joinmarket_liquidity(coinjoins, postmix_spend=postmix, warn_if_not_found_in_postmix=False)
    analyzed = result["coinjoins"]
    summary = result["summary"]

    assert analyzed["txA"]["inputs"]["0"]["mix_event_type"] == MIX_ENTER
    assert analyzed["txA"]["outputs"]["0"]["mix_event_type"] == MIX_REMIX
    assert analyzed["txA"]["outputs"]["1"]["mix_event_type"] == MIX_STAY

    assert analyzed["txB"]["inputs"]["0"]["mix_event_type"] == MIX_REMIX
    assert analyzed["txB"]["inputs"]["0"]["burn_time"] == 600
    assert analyzed["txB"]["inputs"]["0"]["burn_time_cjtxs"] >= 0

    assert analyzed["txB"]["outputs"]["0"]["mix_event_type"] == MIX_LEAVE
    assert analyzed["txB"]["outputs"]["0"]["burn_time"] == 120

    assert summary["total_coinjoins"] == 2
    assert summary["total_mix_entering"] == 1
    assert summary["total_mix_leaving"] == 1
    assert summary["event_counts"][MIX_REMIX] >= 2

    assert "txB" in summary["standard_output_denominations"]
    assert summary["standard_output_denominations"]["txB"]["30000"] == 2
