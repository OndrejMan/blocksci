#!/bin/bash

set -e

THREADS=1

usage() {
    echo "Usage: $0 [--threads=N]" >&2
    exit 1
}

for arg in "$@"; do
    case "$arg" in
        --threads=*) THREADS="${arg#--threads=}" ;;
        *) usage ;;
    esac
done

if ! [[ "$THREADS" =~ ^[1-9][0-9]*$ ]]; then
    echo "--threads expects a positive integer, got '$THREADS'" >&2
    exit 1
fi

if [ ! -d /mnt/blocksci/build ]; then
    echo "The C++ tests require an existing /mnt/blocksci/build directory; run build.sh first." >&2
    exit 1
fi

cd /mnt/blocksci/build
make -j"$THREADS" blocksci_unittest

CPP_TEST_CHAIN_DIR="$(mktemp -d)"
trap 'rm -rf "$CPP_TEST_CHAIN_DIR"' EXIT
CPP_TEST_CONFIG="$CPP_TEST_CHAIN_DIR/config.json"
CPP_TEST_FIXTURE_DIR="/mnt/blocksci/test/files/btc/regtest"

./tools/parser/blocksci_parser "$CPP_TEST_CONFIG" generate-config \
    bitcoin_regtest "$CPP_TEST_CHAIN_DIR" --disk "$CPP_TEST_FIXTURE_DIR" --max-block 100
./tools/parser/blocksci_parser "$CPP_TEST_CONFIG" update
./tools/parser/blocksci_parser "$CPP_TEST_CONFIG" generate-config \
    bitcoin_regtest "$CPP_TEST_CHAIN_DIR" --disk "$CPP_TEST_FIXTURE_DIR"
./tools/parser/blocksci_parser "$CPP_TEST_CONFIG" update
./test/blocksci/blocksci_unittest "$CPP_TEST_CONFIG"
