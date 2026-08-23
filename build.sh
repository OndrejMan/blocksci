#!/bin/bash

THREADS=1
SKIP_JUPYTER=0
PORTABLE_BUILD=OFF

usage() {
    echo "Usage: $0 [--threads=N] [--skip-jupyter] [--portable-build]" >&2
    exit 1
}

for arg in "$@"; do
    case "$arg" in
        --threads=*) THREADS="${arg#--threads=}" ;;
        --skip-jupyter) SKIP_JUPYTER=1 ;;
        --portable-build) PORTABLE_BUILD=ON ;;
        *) usage ;;
    esac
done

if ! [[ "$THREADS" =~ ^[1-9][0-9]*$ ]]; then
    echo "--threads expects a positive integer, got '$THREADS'" >&2
    exit 1
fi

echo "Using $THREADS threads"
cd /mnt/blocksci

(mkdir -p build && \
    cd build && \
    CC=gcc-7 CXX=g++-7 cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DBLOCKSCI_PORTABLE_BUILD="$PORTABLE_BUILD" \
        -DPYTHON_EXECUTABLE="$(which python3)" .. && \
    make -j$THREADS && \
    make install) || exit 1

cd /mnt/blocksci

uv sync --project /blocksci --locked || exit 1

# The Jupyter stack is pinned in uv.lock and therefore already installed above.
# Only the nbextension assets need a separate configuration
# step, and it is best effort: jupyter-contrib-nbextensions targets Notebook 6.x
# and may not apply to the pinned Notebook 7.x.
if [ "$SKIP_JUPYTER" != "1" ]; then
    uv run --project /blocksci --locked jupyter contrib nbextension install --user || \
        echo "warning: nbextension setup failed; notebooks will run without extensions"
fi


BLOCKSCIPY_COMPILE_DB="$(mktemp)" || exit 1
trap 'rm -f "$BLOCKSCIPY_COMPILE_DB"' EXIT

BLOCKSCIPY_COMPILE_COMMANDS_OUTPUT="$BLOCKSCIPY_COMPILE_DB" \
    NTHREADS="$THREADS" CC=gcc-7 CXX=g++-7 uv pip install -e blockscipy || exit 1

##### COMPILE COMMANDS JSON MERGE FOR DEVELOPMENT #####
cp build/compile_commands.json build/compile_commands.json.backup || exit 1
jq -e -s 'add' build/compile_commands.json "$BLOCKSCIPY_COMPILE_DB" \
    > build/compile_commands_merged.json || exit 1
mv build/compile_commands_merged.json build/compile_commands.json || exit 1

if [ "$SKIP_JUPYTER" != "1" ]; then
    cd Notebooks || exit 1
    uv run --project /blocksci --locked jupyter notebook --no-browser --ip="0.0.0.0" --allow-root || exit 1
fi
