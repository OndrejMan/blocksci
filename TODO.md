# TODO

## JoinMarket v `compute_anonymity_degradation` jde přes linkovací filtr

Zapsáno 2026-11-07. Upozornil na to Standa: JoinMarket CoinJoiny nemusí být navzájem provázané.

**Kde:** `blockscipy/src/chain/coinjoin_module_py.cpp`, funkce `compute_anonymity_degradation`.
Na větvi `pr/10-python-api-and-remixes` (PR #15) je to kolem řádku 600, ve smyčce
`for (type : {"wasabi1", "wasabi2", "whirlpool", "ashigaru", "joinmarket"})`.
Ta smyčka plní sadu pro každý typ přes `findLinkedCjTxes`.

**Problém:** `findLinkedCjTxes` = detektor + `findLinkedCoinjoinTransactions`. Filtr ponechá jen
kandidáty, kteří utrácejí výstup jiného kandidáta stejného typu nebo jejichž výstup jiný kandidát
utrácí. Osamocený JoinMarket CoinJoin, například jednorázový `sendpayment` takera, vypadne.
U Wasabi to nevadí, protože skoro všechno remixuje. U JoinMarketu se tím zahazují skutečné CoinJoiny.

Linkovaná sada se ve funkci používá dvakrát:
1. **Které CoinJoiny se analyzují:** při `coinjoin_type="joinmarket"` se počítají jen provázané.
2. **Co je remix (`ignore_remixes`):** výstup utracený osamoceným JoinMarket CoinJoinem se nepozná
   jako remix a započítá se jako degradace.

**Historie:**
- Před PR #15 (`pr/09-linked-coinjoins`) se sada `joinmarket` neplnila vůbec. Pro joinmarket se
  tedy nepočítalo nic a JoinMarket se nebral ani jako remix.
- Commit „fix(coinjoin): ignore remixes for every collected type“ přidal `ashigaru` a `joinmarket`,
  ale přes linkovací filtr. Výsledek se zlepšil z „nic“ na „jen provázané“, úplná oprava to není.

**Oprava (navrženo):** pro `joinmarket` brát sadu z raw detektoru (`makeCoinjoinDetector("joinmarket")`
/ `isJoinMarketCoinJoin`, stejně jako `filter_coinjoin_txes_raw`) a ostatní typy nechat linkované.
Cena je víc false positives z `isJoinMarketCoinJoin`, protože heuristika je široká. Upravit taky
komentář pod smyčkou („Remix test used by the degradation below…“) a doplnit test
v `test/blockscipy/test_coinjoin_link_reduction.py`: osamocený JoinMarket CoinJoin se musí započítat.
Druhá možnost je `joinmarket` ze smyčky vyřadit.

**Postup:** oprava patří do větve `pr/10-python-api-and-remixes` uprostřed MR stacku. Větve nad ní
se pak musí přeskládat (pozor na `rebase --update-refs` a `mr/*` tagy).

**Dopad na diplomku:** žádný. Unified report ani coinjoin-pipeline nebo coinjoin-analysis
`compute_anonymity_degradation` nevolají. Report bere `filter_coinjoin_txes_raw` bez linkování.

## Související (coinjoin-pipeline): JoinMarket detekce v reportu přes subset matching

Standa to napsal do review PR #37 ke commitu „feat: expose subset-matching CoinJoin scans“
(řádky s `isPossibleCoinjoin` / `isCoinjoinExtra` v `coinjoin_module_py.cpp`).

**Jak je to teď:** `export_blocksci_records` v
`coinjoin-pipeline/pipeline/exporters/blocksci_export/detector.py` (kolem ř. 337) má výhybku.
Wasabi, Whirlpool a Ashigaru jdou přes `filter_coinjoin_txes_raw`, tedy přes heuristický detektor
typu. Jen JoinMarket jde přes `scan_coinjoins_by_subset_matching`, který na každou transakci pustí
Kalodnerův subset matching z roku 2017 (`isPossibleCoinjoin` / `isCoinjoinExtra`). Ten zkouší
rozdělit vstupy do „kbelíků“ cílových částek, tedy dělá backtracking, exponenciální v počtu vstupů.

**Co říká Standa:** na mainnetu je to nepoužitelné. Uvádí hodiny na jednu WW2 transakci a obří RAM.
Upřesnění z čtení kódu: platí to pro upstreamové `max_depth = 0` (bez limitu). Naše vazba má
výchozí `max_depth = 200000` a počítadlo sčítá všechna volání za jednu transakci, takže každá
transakce má strop a pak skončí jako `Timeout` v `skipped`. Paměť drží hloubka rekurze ≤ počet
vstupů. Na WW2 je to spíš nepřesné než pomalé: `isCoinjoinExtra` WW2 odmítne hned, protože má víc
denominací, a `isPossibleCoinjoin` ji nejspíš rychle označí jako „possible“. Problém zůstává
v objemu: strop až 200 000 kroků na každou transakci, která projde předfiltrem (≥ 2 stejné výstupy).
Kolik to na mainnetu dá dohromady, není změřené.

**Přepnutí = smazat výjimku pro JoinMarket ve výhybce.** JoinMarket by pak šel stejně jako ostatní
přes `filter_coinjoin_txes_raw(start, stop, "joinmarket")`. V C++ to je `isJoinMarketCoinJoin`
(`src/heuristics/tx_identification.cpp`, kolem ř. 1062), jeden lineární průchod bez backtrackingu:
- žádný OP_RETURN ani nestandardní výstup,
- nejčastější hodnota výstupu se opakuje aspoň 5× (N) a žádná jiná hodnota se neopakuje,
- vstupů N až 5N, výstupů nejvýš 2N+1,
- žádná adresa se neopakuje ve vstupech ani výstupech,
- mix částka ≥ 10 000 sat, žádný change výstup pod 5 000 sat.

**Co s tím změnit:**
1. `detector.py: export_blocksci_records` volat pro všechny typy `_filter_raw_coinjoin_txes`
   a smazat `_scan_coinjoins_by_subset_matching`.
2. `integration_diagnostics.py: call_joinmarket_detector` dnes volá per-tx `is_possible_coinjoin` /
   `is_definite_coinjoin`. Má volat stejný heuristický detektor jako report.
3. `scripts/metacentrum_mainnet_coinjoin_scan.py` (kolem ř. 91) má stejnou větev pro joinmarket.
4. Smazat `blocksci_skipped_txids` z reportu a parametry `joinmarket_detector`,
   `joinmarket_min_base_fee`, `joinmarket_percentage_fee`, `joinmarket_max_depth`
   (i `JOINMARKET_MAX_DEPTH` a dokumentaci v `coinjoin-pipeline/docs/analysis-semantics.md`).
5. Až to nikdo nevolá, vyhodit `scan_coinjoins_by_subset_matching` z BlockSci MR stacku
   (commit „feat: expose subset-matching CoinJoin scans“ a jeho test v `test_heuristics.py`).

**Riziko:** heuristika chce aspoň 5 stejných výstupů, tedy taker + 4 makeři. Kola s menším počtem
účastníků přestanou být detekovaná, i když je subset matching dnes najde. JoinMarket scénáře mají
`--tumbler-makercountrange` třeba `"5,1"` (průměr 5, rozptyl 1), takže kolo se 3 makery může nastat.
Před přepnutím zjistit z `coinjoin-analysis` výstupu emulace, kolik účastníků kola skutečně mají,
a po přepnutí pustit JoinMarket testy sady, jestli report pořád sedí s coinjoin-analysis.
Druhé riziko: `isJoinMarketCoinJoin` je široká heuristika, na mainnetu dá false positives.

Standa by se subset matchingu zbavil co nejdřív.
