#!/bin/bash
# Docker setup script for KiCad build dependencies

set -e

echo "Installing KiCad build dependencies..."

apt-get update

apt-get install -y --no-install-recommends \
    cmake \
    ninja-build \
    build-essential \
    pkg-config \
    git \
    libcairo2-dev \
    libglu1-mesa-dev \
    libgl1-mesa-dev \
    libx11-dev \
    mesa-common-dev \
    libgtk-3-dev \
    libglm-dev \
    libwxgtk3.2-dev \
    libwxgtk-webview3.2-dev \
    python3-wxgtk4.0 \
    libbz2-dev \
    libssl-dev \
    libzstd-dev \
    zlib1g-dev \
    python3-dev \
    swig \
    libocct-modeling-algorithms-dev \
    libocct-modeling-data-dev \
    libocct-data-exchange-dev \
    libocct-visualization-dev \
    libocct-foundation-dev \
    libocct-ocaf-dev \
    libgit2-dev \
    libsecret-1-dev \
    libboost-all-dev \
    libcurl4-openssl-dev \
    libngspice0-dev \
    unixodbc-dev \
    libspnav-dev \
    libnng-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libzint-dev \
    ccache \
    wget \
    curl \
    ca-certificates

echo "Dependencies installed successfully!"
echo "Ready to build KiCad."
exec bash
