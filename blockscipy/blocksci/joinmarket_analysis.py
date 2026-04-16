# -*- coding: utf-8 -*-
"""JoinMarket-focused detection and liquidity analysis helpers for BlockSci.

This module adds a higher-level analysis layer on top of BlockSci's
``tx.is_joinmarket_coinjoin`` transaction classifier.
"""

from __future__ import annotations

import copy
import datetime
import logging
from collections import Counter
from typing import Any, Dict, Optional, Tuple


MIX_ENTER = "MIX_ENTER"
MIX_REMIX = "MIX_REMIX"
MIX_LEAVE = "MIX_LEAVE"
MIX_STAY = "MIX_STAY"


def _format_broadcast_time(value: Any) -> str:
    """Convert a datetime-like value into BlockSci's broadcast_time string format."""
    if isinstance(value, datetime.datetime):
        return value.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
    return str(value)


def _parse_broadcast_time(value: str) -> datetime.datetime:
    return datetime.datetime.strptime(value, "%Y-%m-%d %H:%M:%S.%f")


def _extract_txid_from_inout_string(inout_string: str) -> Tuple[str, str]:
    if inout_string.startswith("vin_") or inout_string.startswith("vout_"):
        return (
            inout_string[inout_string.find("_") + 1 : inout_string.rfind("_")],
            inout_string[inout_string.rfind("_") + 1 :],
        )
    raise ValueError("Invalid in/out reference string: {}".format(inout_string))


def _compute_relative_ordering(coinjoins: Dict[str, Dict[str, Any]]) -> Dict[str, int]:
    """Compute ordering based on remix links between JoinMarket coinjoins."""
    sorted_txids = sorted(coinjoins.keys(), key=lambda txid: _parse_broadcast_time(coinjoins[txid]["broadcast_time"]))
    relative_order = {txid: 0 for txid in sorted_txids}

    for txid in sorted_txids:
        prev_distances = []
        inputs = coinjoins[txid].get("inputs", {})
        for index in inputs:
            prev_tx_str = inputs[index].get("spending_tx")
            if not prev_tx_str:
                continue
            prev_txid, _ = _extract_txid_from_inout_string(prev_tx_str)
            if prev_txid in relative_order:
                prev_distances.append(relative_order[prev_txid])
        relative_order[txid] = max(prev_distances) + 1 if prev_distances else 0

    return relative_order


def extract_joinmarket_coinjoins(chain: Any, start: Optional[Any] = None, end: Optional[Any] = None, cpu_count: int = 1) -> Dict[str, Dict[str, Any]]:
    """Extract JoinMarket coinjoin records from a BlockSci chain.

    Returns a dictionary shaped similarly to ``coinjoin-analysis`` transaction
    records, containing inputs/outputs, spend links and broadcast times.
    """
    txs = chain.filter_txes_legacy(
        lambda tx: tx.is_joinmarket_coinjoin,
        start=start,
        end=end,
        cpu_count=cpu_count,
    )

    records: Dict[str, Dict[str, Any]] = {}

    for tx in txs:
        txid = str(tx.hash)
        tx_record: Dict[str, Any] = {
            "txid": txid,
            "broadcast_time": _format_broadcast_time(tx.block_time),
            "inputs": {},
            "outputs": {},
        }

        for inpt in tx.inputs:
            input_index = str(inpt.index)
            input_record: Dict[str, Any] = {
                "value": int(inpt.value),
                "address": str(inpt.address),
            }
            if inpt.spent_tx is not None:
                input_record["spending_tx"] = "vout_{}_{}".format(str(inpt.spent_tx.hash), inpt.spent_tx_index)
            tx_record["inputs"][input_index] = input_record

        for out in tx.outputs:
            output_index = str(out.index)
            output_record: Dict[str, Any] = {
                "value": int(out.value),
                "address": str(out.address),
            }
            if out.is_spent and out.spending_tx is not None:
                output_record["spend_by_tx"] = "vin_{}_{}".format(str(out.spending_tx.hash), out.spending_tx_index)
            tx_record["outputs"][output_index] = output_record

        records[txid] = tx_record

    return records


def analyze_joinmarket_liquidity(
    coinjoins: Dict[str, Dict[str, Any]],
    postmix_spend: Optional[Dict[str, Dict[str, Any]]] = None,
    warn_if_not_found_in_postmix: bool = True,
) -> Dict[str, Any]:
    """Analyze JoinMarket liquidity flow for extracted coinjoin records.

    Labels each input/output with one of:
    - ``MIX_ENTER``
    - ``MIX_REMIX``
    - ``MIX_LEAVE``
    - ``MIX_STAY``

    Also computes burn-time metrics in seconds and in coinjoin-transaction hops.
    """
    if postmix_spend is None:
        postmix_spend = {}

    analyzed = copy.deepcopy(coinjoins)
    if not analyzed:
        return {"coinjoins": analyzed, "summary": {}}

    broadcast_times = {
        txid: _parse_broadcast_time(analyzed[txid]["broadcast_time"])
        for txid in analyzed.keys()
    }
    for txid in postmix_spend.keys():
        if "broadcast_time" in postmix_spend[txid]:
            broadcast_times[txid] = _parse_broadcast_time(postmix_spend[txid]["broadcast_time"])

    sorted_txids = sorted(analyzed.keys(), key=lambda txid: broadcast_times[txid])
    coinjoins_index = {txid: i for i, txid in enumerate(sorted_txids)}
    coinjoins_relative_order = _compute_relative_ordering(analyzed)

    total_inputs = 0
    total_outputs = 0
    total_mix_entering = 0
    total_mix_leaving = 0
    total_utxos = 0

    for txid in analyzed:
        analyzed[txid]["relative_order"] = coinjoins_relative_order[txid]

        for index in analyzed[txid]["inputs"]:
            total_inputs += 1
            inp = analyzed[txid]["inputs"][index]
            spending = inp.get("spending_tx")

            if spending:
                spending_txid, _ = _extract_txid_from_inout_string(spending)
                if spending_txid in analyzed:
                    inp["mix_event_type"] = MIX_REMIX
                    inp["burn_time"] = round((broadcast_times[txid] - broadcast_times[spending_txid]).total_seconds(), 0)
                    inp["burn_time_cjtxs_as_mined"] = coinjoins_index[txid] - coinjoins_index[spending_txid]
                    inp["burn_time_cjtxs_relative"] = coinjoins_relative_order[txid] - coinjoins_relative_order[spending_txid]
                    inp["burn_time_cjtxs"] = inp["burn_time_cjtxs_relative"]
                else:
                    total_mix_entering += 1
                    inp["mix_event_type"] = MIX_ENTER
            else:
                total_mix_entering += 1
                inp["mix_event_type"] = MIX_ENTER

        for index in analyzed[txid]["outputs"]:
            total_outputs += 1
            out = analyzed[txid]["outputs"][index]
            spending = out.get("spend_by_tx")

            if not spending:
                total_utxos += 1
                out["mix_event_type"] = MIX_STAY
                continue

            spend_by_txid, _ = _extract_txid_from_inout_string(spending)
            if spend_by_txid in analyzed:
                out["mix_event_type"] = MIX_REMIX
                out["burn_time"] = round((broadcast_times[spend_by_txid] - broadcast_times[txid]).total_seconds(), 0)
                out["burn_time_cjtxs_as_mined"] = coinjoins_index[spend_by_txid] - coinjoins_index[txid]
                out["burn_time_cjtxs_relative"] = coinjoins_relative_order[spend_by_txid] - coinjoins_relative_order[txid]
                out["burn_time_cjtxs"] = out["burn_time_cjtxs_relative"]
            else:
                if spend_by_txid not in postmix_spend and warn_if_not_found_in_postmix:
                    logging.warning("Could not find spend_by_tx %s in postmix_spend txs", spend_by_txid)
                elif spend_by_txid in broadcast_times:
                    out["burn_time"] = round((broadcast_times[spend_by_txid] - broadcast_times[txid]).total_seconds(), 0)
                total_mix_leaving += 1
                out["mix_event_type"] = MIX_LEAVE

    summary = {
        "total_coinjoins": len(analyzed),
        "total_inputs": total_inputs,
        "total_outputs": total_outputs,
        "total_mix_entering": total_mix_entering,
        "total_mix_leaving": total_mix_leaving,
        "total_mix_staying": total_utxos,
        "event_counts": {
            MIX_ENTER: 0,
            MIX_REMIX: 0,
            MIX_LEAVE: 0,
            MIX_STAY: 0,
        },
    }

    for txid in analyzed:
        for index in analyzed[txid]["inputs"]:
            evt = analyzed[txid]["inputs"][index].get("mix_event_type")
            if evt in summary["event_counts"]:
                summary["event_counts"][evt] += 1
        for index in analyzed[txid]["outputs"]:
            evt = analyzed[txid]["outputs"][index].get("mix_event_type")
            if evt in summary["event_counts"]:
                summary["event_counts"][evt] += 1

    summary["standard_output_denominations"] = _compute_standard_denoms(analyzed)

    return {"coinjoins": analyzed, "summary": summary}


def _compute_standard_denoms(coinjoins: Dict[str, Dict[str, Any]]) -> Dict[str, Dict[str, int]]:
    """Compute repeated output denominations per transaction (anonset-like signal)."""
    standard_denoms: Dict[str, Dict[str, int]] = {}
    for txid in coinjoins:
        values = [coinjoins[txid]["outputs"][idx]["value"] for idx in coinjoins[txid]["outputs"]]
        counts = Counter(values)
        repeated = {str(value): int(cnt) for value, cnt in counts.items() if cnt > 1}
        if repeated:
            standard_denoms[txid] = repeated
    return standard_denoms
