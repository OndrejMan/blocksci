# Where the `complete` stage takes its toolchain from. By default it is the
# `dependencies` stage below; --build-arg DEPS_IMAGE=<prebuilt dependency image>
# reuses an existing toolchain instead of rebuilding it.
ARG DEPS_IMAGE=dependencies

# Use Ubuntu 20.04 LTS as the base image
FROM ubuntu:20.04 AS dependencies

# Avoid prompts from apt-get
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y && apt-get update

RUN apt-get install -y cmake libtool autoconf libboost-filesystem-dev \
    libboost-iostreams-dev libboost-serialization-dev libboost-thread-dev \
    libboost-test-dev libssl-dev libjsoncpp-dev libcurl4-openssl-dev \
    libjsonrpccpp-dev libsnappy-dev zlib1g-dev libbz2-dev \
    liblz4-dev libzstd-dev libjemalloc-dev libsparsehash-dev python3-dev \
    python3-pip pkg-config git g++-7 gcc-7 ffmpeg libcairo2 libcairo2-dev curl jq \
    libgirepository1.0-dev libdbus-1-dev libglib2.0-dev

# This image is only the reusable dependency/toolchain base; the BlockSci
# sources are copied and compiled by the `complete` stage below.
WORKDIR /blocksci

RUN curl -LsSf https://astral.sh/uv/install.sh | sh
ENV PATH="/root/.local/bin:$PATH"

RUN uv python install 3.8.20
RUN uv python pin 3.8.20

ENV VIRTUAL_ENV="/blocksci/.venv"
RUN uv venv $VIRTUAL_ENV
ENV PATH="$VIRTUAL_ENV/bin:$PATH"

# Install Python dependencies into the persistent venv without copying or
# compiling the BlockSci source tree.
COPY pip-all-requirements.txt /blocksci/pip-all-requirements.txt
RUN CC=gcc-7 CXX=g++-7 uv pip install -r /blocksci/pip-all-requirements.txt

# Set the default command for the container
CMD ["/bin/bash"]

FROM ${DEPS_IMAGE} AS complete

ARG NTHREADS=1
ARG RUN_CPP_TESTS=0
ARG RUN_PYTHON_TESTS=0

WORKDIR /mnt
COPY . /mnt/blocksci

RUN cd /mnt/blocksci/ && \
    RUN_CPP_TESTS=${RUN_CPP_TESTS} RUN_PYTHON_TESTS=${RUN_PYTHON_TESTS} ./build.sh --threads="${NTHREADS}" --portable-build --skip-jupyter

CMD ["/bin/bash"]

FROM complete AS test

ARG NTHREADS=1

RUN cd /mnt/blocksci/ && \
    TEST_ONLY=1 RUN_CPP_TESTS=1 RUN_PYTHON_TESTS=1 ./build.sh --threads="${NTHREADS}"
