#!/usr/bin/env bash
set -euo pipefail

ARCH=$(uname -m)
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)

echo "=== Installing solver_bench dependencies on $(uname -s) ${ARCH} ==="

apt-get update -qq
apt-get install -y \
    build-essential cmake git \
    gfortran \
    libopenmpi-dev openmpi-bin \
    liblapack-dev liblapacke-dev \
    libscalapack-mpi-dev \
    libmetis-dev libscotch-dev \
    libsuperlu-dev \
    time

echo "=== Building MUMPS 5.8.2 from source ==="
MUMPS_VERSION="5.8.2"
MUMPS_ARCHIVE="MUMPS_${MUMPS_VERSION}.tar.gz"
MUMPS_URL="https://coin-or-tools.github.io/ThirdParty-Mumps/${MUMPS_ARCHIVE}"
MUMPS_DIR="/tmp/MUMPS_${MUMPS_VERSION}"

if [ ! -f "/usr/local/lib/libdmumps.a" ]; then
    cd /tmp
    wget -q "${MUMPS_URL}" -O "${MUMPS_ARCHIVE}"
    tar -xzf "${MUMPS_ARCHIVE}"
    cp "${MUMPS_DIR}/Make.inc/Makefile.debian.PAR" "${MUMPS_DIR}/Makefile.inc"
    cd "${MUMPS_DIR}"
    make -j$(nproc) d
    cp include/*.h /usr/local/include/
    cp lib/libdmumps.a lib/libmumps_common.a /usr/local/lib/
    cp PORD/lib/libpord.a /usr/local/lib/
    ldconfig
    echo "=== MUMPS ${MUMPS_VERSION} installed ==="
else
    echo "=== MUMPS already installed, skipping ==="
fi

WITH_PARDISO=OFF

if [ "${ARCH}" != "riscv64" ]; then
    echo "=== Installing Intel MKL (PARDISO) ==="
    wget -qO- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
        | gpg --dearmor -o /usr/share/keyrings/oneapi-archive-keyring.gpg
    echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] \
https://apt.repos.intel.com/oneapi all main" \
        > /etc/apt/sources.list.d/oneAPI.list
    apt-get update -qq
    apt-get install -y intel-oneapi-mkl intel-oneapi-mkl-devel

    # Активировать окружение Intel для текущей сессии
    # setvars.sh использует необъявленные переменные — временно снимаем set -u
    set +u
    source /opt/intel/oneapi/setvars.sh || true
    set -u

    # Прописать автозагрузку для всех пользователей
    if ! grep -q "setvars.sh" /etc/bash.bashrc; then
        echo "source /opt/intel/oneapi/setvars.sh > /dev/null 2>&1" >> /etc/bash.bashrc
    fi

    # Прописать LD_PRELOAD для iomp5 чтобы libmkl_intel_thread находил его при запуске
    IOMP5=$(find /opt/intel/oneapi -name "libiomp5.so" 2>/dev/null | head -1)
    if [ -n "${IOMP5}" ]; then
        if ! grep -q "libiomp5" /etc/bash.bashrc; then
            echo "export LD_PRELOAD=${IOMP5}\${LD_PRELOAD:+:\$LD_PRELOAD}" >> /etc/bash.bashrc
        fi
        export LD_PRELOAD="${IOMP5}${LD_PRELOAD:+:$LD_PRELOAD}"
    fi

    WITH_PARDISO=ON
    echo "=== MKL installed and activated ==="
else
    echo "=== RISC-V detected: skipping Intel MKL (PARDISO not supported) ==="
fi

echo "=== Building solver_bench ==="
cd "${PROJECT_DIR}"
mkdir -p build
cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_MUMPS=ON \
    -DWITH_SUPERLU=ON \
    -DWITH_PARDISO=${WITH_PARDISO}
make -j$(nproc)

echo "=== Running tests ==="
cd "${PROJECT_DIR}"
./build/tests

echo ""
echo "=== All done! Run benchmark with: ==="
echo "  cd ${PROJECT_DIR}"
echo "  ./build/solver_bench \\"
echo "    --matrices ./matrices/ \\"
if [ "${WITH_PARDISO}" = "ON" ]; then
    echo "    --solvers mumps,superlu,pardiso \\"
else
    echo "    --solvers mumps,superlu \\"
fi
echo "    --threads 1,2,4,8 \\"
echo "    --output ./results/bench.csv"
