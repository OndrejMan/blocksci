# Use Ubuntu 20.04 LTS as the base image
FROM ubuntu:20.04

# Avoid prompts from apt-get
ARG DEBIAN_FRONTEND=noninteractive
# Lower threads to prevent Out-Of-Memory (OOM) crashes during C++ compilation
ARG NTHREADS=1 

RUN apt-get update && apt-get install -y software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y && apt-get update

# Install system dependencies
RUN apt-get install -y cmake libtool autoconf libboost-filesystem-dev \
    libboost-iostreams-dev libboost-serialization-dev libboost-thread-dev \
    libboost-test-dev libssl-dev libjsoncpp-dev libcurl4-openssl-dev \
    libjsonrpccpp-dev libsnappy-dev zlib1g-dev libbz2-dev \
    liblz4-dev libzstd-dev libjemalloc-dev libsparsehash-dev python3-dev \
    python3-pip pkg-config git g++-7 gcc-7 ffmpeg libcairo2 libcairo2-dev curl jq

# Add project files and set working directory
ADD . /blocksci
WORKDIR /blocksci

# Install uv and add it to PATH
RUN curl -LsSf https://astral.sh/uv/install.sh | sh
ENV PATH="/root/.local/bin:$PATH"

# Install and pin Python 3.8.20
RUN uv python install 3.8.20
RUN uv python pin 3.8.20
#RUN apt-get update && apt-get install -y python3-distro-info

# Create the virtual environment ONCE and activate it globally for all subsequent RUN steps
ENV VIRTUAL_ENV="/blocksci/.venv"
RUN uv venv $VIRTUAL_ENV
ENV PATH="$VIRTUAL_ENV/bin:$PATH"

# Install requirements into the persistent venv
RUN CC=gcc-7 CXX=g++-7 uv pip install -r pip-all-requirements.txt

# Build BlockSci
# We explicitly pass the Python executable to CMake so it matches the uv environment exactly
RUN rm -rf build && \
    mkdir build && \
    cd build && \
    CC=gcc-7 CXX=g++-7 cmake -DCMAKE_BUILD_TYPE=Release -DPYTHON_EXECUTABLE=$(which python3) .. && \
    make -j${NTHREADS} && \
    make install

# Install BlockSci Python bindings
# We removed the `--clear` flag so we don't destroy our previously installed requirements
RUN rm -rf blockscipy/build && \
    CC=gcc-7 CXX=g++-7 uv pip install -e blockscipy

# Clean up
RUN rm -rf blockscipy/build

# Set the default command for the container
CMD ["/bin/bash"]