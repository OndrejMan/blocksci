# Where the `complete` stage takes its toolchain from. By default it is the
# `dependencies` stage below; --build-arg DEPS_IMAGE=<prebuilt dependency image>
# reuses an existing toolchain instead of rebuilding it.
ARG DEPS_IMAGE=dependencies
ARG UV_IMAGE=ghcr.io/astral-sh/uv:0.12.1

FROM ${UV_IMAGE} AS uv

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
    libgirepository1.0-dev libdbus-1-dev libglib2.0-dev ccache

# This image is only the reusable dependency/toolchain base; the BlockSci
# sources are copied and compiled by the `complete` stage below.
WORKDIR /blocksci

# uv comes from the UV_IMAGE pinned at the top of this file.
COPY --from=uv /uv /uvx /bin/

RUN uv python install 3.8.20
RUN uv python pin 3.8.20

ENV VIRTUAL_ENV="/blocksci/.venv"
RUN uv venv $VIRTUAL_ENV
ENV PATH="$VIRTUAL_ENV/bin:$PATH"

# Install the locked Python environment without copying or compiling the
# BlockSci source tree.
COPY pyproject.toml uv.lock /blocksci/
RUN uv sync --locked

# Set the default command for the container
CMD ["/bin/bash"]

FROM ${DEPS_IMAGE} AS complete

ARG NTHREADS=1

WORKDIR /mnt
COPY . /mnt/blocksci

# Every source change invalidates the COPY above, so ccache keeps compiler output in
# a cache mount between builds. Its /usr/lib/ccache wrappers shadow the gcc-7/g++-7
# that build.sh calls; a DEPS_IMAGE without ccache simply compiles uncached.
RUN --mount=type=cache,id=blocksci-ccache,target=/root/.ccache \
    cd /mnt/blocksci/ && \
    export PATH="/usr/lib/ccache:$PATH" CCACHE_MAXSIZE=10G && \
    ./build.sh --threads="${NTHREADS}" --portable-build --skip-jupyter

CMD ["/bin/bash"]

FROM complete AS test

ARG NTHREADS=1

RUN --mount=type=cache,id=blocksci-ccache,target=/root/.ccache \
    cd /mnt/blocksci/ && \
    export PATH="/usr/lib/ccache:$PATH" CCACHE_MAXSIZE=10G && \
    ./test-cpp.sh --threads="${NTHREADS}" && \
    ./test-python.sh
