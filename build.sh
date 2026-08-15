#!/bin/bash

THREADS=1
SKIP_JUPYTER=0

usage() {
    echo "Usage: $0 [--threads=N] [--skip-jupyter]" >&2
    exit 1
}

for arg in "$@"; do
    case "$arg" in
        --threads=*) THREADS="${arg#--threads=}" ;;
        --skip-jupyter) SKIP_JUPYTER=1 ;;
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
    CC=gcc-7 CXX=g++-7 cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$THREADS && \
    make install) || exit 1

cd /mnt/blocksci

pip3 install -r pip-all-requirements.txt || exit 1

if [ "$SKIP_JUPYTER" != "1" ]; then
    pip3 install jupyter notebook
    pip3 install jupyter_contrib_nbextensions
    jupyter contrib nbextension install --user
fi


NTHREADS="$THREADS" CC=gcc-7 CXX=g++-7 pip3 install -e blockscipy || exit 1

##### COMPILE COMMANDS JSON MERGE FOR DEVELOPMENT #####
cp build/compile_commands.json build/compile_commands.json.backup
jq -s 'add' build/compile_commands.json blockscipy/build/temp.linux-x86_64-3.8/compile_commands.json > build/compile_commands_merged.json
mv build/compile_commands_merged.json build/compile_commands.json

cd Notebooks

if [ "$SKIP_JUPYTER" != "1" ]; then
    jupyter notebook --no-browser --ip="0.0.0.0" --allow-root || exit 1
fi
