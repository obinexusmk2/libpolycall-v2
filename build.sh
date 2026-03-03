#!/bin/bash
set -e

# Ensure clean PATH without Windows paths containing parentheses
export PATH=/tmp/cmake-3.28/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

# Install cmake if missing
if ! command -v cmake &>/dev/null; then
    echo "=== Downloading cmake 3.28.3 ==="
    mkdir -p /tmp/cmake-3.28
    curl -sL https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-linux-x86_64.tar.gz | tar xz -C /tmp/cmake-3.28 --strip-components=1
    echo "cmake installed: $(cmake --version | head -1)"
fi

cd "/mnt/c/Users/Public/Public Workspace/OBINexus_LibPolycallV2viaSemeverX/libpolycall-v2"

make clean 2>&1 | tail -1
echo ""
echo "=== Building core ==="
make core 2>&1
echo ""
echo "=== Building daemon ==="
make daemon 2>&1
