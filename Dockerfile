# Use Ubuntu 20.04 LTS as the base image
FROM ubuntu:20.04

# Avoid prompts from apt-get
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y && apt-get update

# Install system dependencies
RUN apt-get install -y cmake libtool autoconf libboost-filesystem-dev \
    libboost-iostreams-dev libboost-serialization-dev libboost-thread-dev \
    libboost-test-dev libssl-dev libjsoncpp-dev libcurl4-openssl-dev \
    libjsonrpccpp-dev libsnappy-dev zlib1g-dev libbz2-dev \
    liblz4-dev libzstd-dev libjemalloc-dev libsparsehash-dev python3-dev \
    python3-pip pkg-config git g++-7 gcc-7 ffmpeg libcairo2 libcairo2-dev curl jq \
    libgirepository1.0-dev libdbus-1-dev libglib2.0-dev

# This image is only the reusable dependency/toolchain base. The BlockSci
# sources are copied and compiled by Dockerfile_complete.
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

RUN apt-get update && apt-get install -y python3-apt distro-info unattended-upgrades

# Install Python dependencies into the persistent venv without copying or
# compiling the BlockSci source tree.
COPY pip-all-requirements.txt /blocksci/pip-all-requirements.txt
RUN CC=gcc-7 CXX=g++-7 uv pip install -r /blocksci/pip-all-requirements.txt

# Set the default command for the container
CMD ["/bin/bash"]
