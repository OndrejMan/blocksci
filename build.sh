#!/bin/bash

THREADS=${1:-${NTHREADS:-1}}
SKIP_JUPYTER=${SKIP_JUPYTER:-0}
RUN_CPP_TESTS=${RUN_CPP_TESTS:-0}
RUN_PYTHON_TESTS=${RUN_PYTHON_TESTS:-0}
TEST_ONLY=${TEST_ONLY:-0}

run_cpp_tests() (
    set -e
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
)

run_python_tests() {
    (cd /mnt/blocksci/test/blockscipy && uv run pytest)
}

echo "Using $THREADS threads"
cd /mnt/blocksci

if [ "$TEST_ONLY" = "1" ]; then
    if [ ! -d build ]; then
        echo "TEST_ONLY requires an existing /mnt/blocksci/build directory." >&2
        exit 1
    fi

    if [ "$RUN_CPP_TESTS" = "1" ]; then
        run_cpp_tests
        cpp_status=$?
        if [ "$cpp_status" -ne 0 ]; then
            exit "$cpp_status"
        fi
    fi

    if [ "$RUN_PYTHON_TESTS" = "1" ]; then
        run_python_tests || exit 1
    fi

    exit 0
fi

(set -e
    mkdir -p build
    cd build
    CC=gcc-7 CXX=g++-7 cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DBLOCKSCI_PORTABLE_BUILD="${BLOCKSCI_PORTABLE_BUILD:-OFF}" \
        -DPYTHON_EXECUTABLE="$(which python3)" ..
    make -j"$THREADS"
    if [ "$RUN_CPP_TESTS" = "1" ]; then
        run_cpp_tests
    fi
    make install
)
# Not `( ... ) || exit 1`: as part of an AND-OR list the `set -e` above is
# ignored, so a failing test build would be silently skipped over.
build_status=$?
if [ "$build_status" -ne 0 ]; then
    exit "$build_status"
fi

cd /mnt/blocksci

uv pip install -r pip-all-requirements.txt || exit 1

# The Jupyter stack is pinned in pip-all-requirements.txt and therefore already
# installed above. Only the nbextension assets need a separate configuration
# step, and it is best effort: jupyter-contrib-nbextensions targets Notebook 6.x
# and may not apply to the pinned Notebook 7.x.
if [ "$SKIP_JUPYTER" != "1" ]; then
    uv run jupyter contrib nbextension install --user || \
        echo "warning: nbextension setup failed; notebooks will run without extensions"
fi


BLOCKSCIPY_COMPILE_DB="$(mktemp)" || exit 1
trap 'rm -f "$BLOCKSCIPY_COMPILE_DB"' EXIT

BLOCKSCIPY_COMPILE_COMMANDS_OUTPUT="$BLOCKSCIPY_COMPILE_DB" \
    CC=gcc-7 CXX=g++-7 uv pip install -e blockscipy || exit 1

if [ "$RUN_PYTHON_TESTS" = "1" ]; then
    run_python_tests || exit 1
fi

##### COMPILE COMMANDS JSON MERGE FOR DEVELOPMENT #####
cp build/compile_commands.json build/compile_commands.json.backup || exit 1
jq -e -s 'add' build/compile_commands.json "$BLOCKSCIPY_COMPILE_DB" \
    > build/compile_commands_merged.json || exit 1
mv build/compile_commands_merged.json build/compile_commands.json || exit 1

if [ "$SKIP_JUPYTER" != "1" ]; then
    cd Notebooks || exit 1
    uv run jupyter notebook --no-browser --ip="0.0.0.0" --allow-root || exit 1
fi
