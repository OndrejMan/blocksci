CoinJoin detector extensions
============================

This fork exposes transaction-level CoinJoin heuristics and two different bulk
scan contracts. Callers must distinguish a direct detector scan from the
legacy linked-transaction reduction.

JoinMarket range scan
---------------------

``Blockchain.filter_joinmarket_txes(start, stop, detector, min_base_fee,
percentage_fee, max_depth)`` scans every transaction in the range. ``detector``
is ``"possible"`` or ``"definite"``. It returns a pair ``(detected, skipped)``;
``skipped`` contains transactions whose subset search reached ``max_depth``.
A timeout is not a negative detection and should be reported separately.

Generic/Wasabi linked scan
--------------------------

``Blockchain.filter_coinjoin_txes(start, stop, coinjoin_type,
min_input_count=None)`` first applies the requested transaction heuristic, then
runs ``findLinkedCjTxes``. The returned collection contains both endpoints of
connections between matched transactions. Isolated heuristic matches are
intentionally omitted.

``Blockchain.filter_coinjoin_txes_raw(...)`` returns every transaction-level
heuristic match without the linked reduction. Use this API when the research
question requires all independent positives. When using
``filter_coinjoin_txes``, describe the result as linked CoinJoin transactions.

Wasabi 2 minimum-input behavior
-------------------------------

``isWasabi2CoinJoin(tx, inputCount=None)`` treats ``inputCount`` as a minimum,
not an exact count. When it is supplied, it overrides the internal minimum. If
it is absent, the current internal threshold is 50 before block 850237 (20 when
test values are enabled) and 20 from block 850237 onward.

``blocksci.heuristics.set_test_values_enabled(True)`` only affects the internal
threshold path. It has no input-count effect when a caller also supplies an
explicit minimum.
