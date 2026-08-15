#!/bin/bash

THREADS=${1:-${NTHREADS:-1}}
SKIP_JUPYTER=${SKIP_JUPYTER:-0}

echo "Using $THREADS threads"
cd /mnt/blocksci

(mkdir -p build && \
    cd build && \
    CC=gcc-7 CXX=g++-7 cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DBLOCKSCI_PORTABLE_BUILD="${BLOCKSCI_PORTABLE_BUILD:-OFF}" \
        -DPYTHON_EXECUTABLE="$(which python3)" .. && \
    make -j$THREADS && \
    make install) || exit 1

cd /mnt/blocksci

pip3 install -r pip-all-requirements.txt || exit 1

if [ "$SKIP_JUPYTER" != "1" ]; then
    pip3 install jupyter notebook
    pip3 install jupyter_contrib_nbextensions
    jupyter contrib nbextension install --user
fi


BLOCKSCIPY_COMPILE_DB="$(mktemp)" || exit 1
trap 'rm -f "$BLOCKSCIPY_COMPILE_DB"' EXIT

BLOCKSCIPY_COMPILE_COMMANDS_OUTPUT="$BLOCKSCIPY_COMPILE_DB" \
    CC=gcc-7 CXX=g++-7 pip3 install -e blockscipy || exit 1

##### COMPILE COMMANDS JSON MERGE FOR DEVELOPMENT #####
cp build/compile_commands.json build/compile_commands.json.backup || exit 1
jq -e -s 'add' build/compile_commands.json "$BLOCKSCIPY_COMPILE_DB" \
    > build/compile_commands_merged.json || exit 1
mv build/compile_commands_merged.json build/compile_commands.json || exit 1

cd Notebooks

if [ "$SKIP_JUPYTER" != "1" ]; then
    jupyter notebook --no-browser --ip="0.0.0.0" --allow-root || exit 1
fi
