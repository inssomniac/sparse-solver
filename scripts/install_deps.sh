#!/usr/bin/env bash
set -euo pipefail

ARCH=$(uname -m)
echo "=== Installing solver_bench dependencies on $(uname -s) ${ARCH} ==="

apt-get update -qq
apt-get install -y \
    build-essential cmake git \
    gfortran \
    libopenmpi-dev openmpi-bin \
    liblapack-dev liblapacke-dev \
    libscalapack-mpi-dev \
    libmumps-dev \
    libsuperlu-dev \
    time

if [ "${ARCH}" != "riscv64" ]; then
    echo "=== Installing Intel MKL (PARDISO) ==="
    # Intel oneAPI apt repo
    wget -qO- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
        | gpg --dearmor -o /usr/share/keyrings/oneapi-archive-keyring.gpg
    echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] \
https://apt.repos.intel.com/oneapi all main" \
        > /etc/apt/sources.list.d/oneAPI.list
    apt-get update -qq
    apt-get install -y intel-oneapi-mkl intel-oneapi-mkl-devel
    echo "source /opt/intel/oneapi/setvars.sh" >> /etc/bash.bashrc
    echo "=== MKL installed. Run: source /opt/intel/oneapi/setvars.sh ==="
else
    echo "=== RISC-V detected: skipping Intel MKL (PARDISO not supported) ==="
fi

echo "=== Done. Build with: ==="
echo "  mkdir build && cd build"
if [ "${ARCH}" != "riscv64" ]; then
    echo "  cmake .. -DWITH_MUMPS=ON -DWITH_SUPERLU=ON -DWITH_PARDISO=ON"
else
    echo "  cmake .. -DWITH_MUMPS=ON -DWITH_SUPERLU=ON"
fi
echo "  make -j\$(nproc)"
