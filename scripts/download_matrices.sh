#!/usr/bin/env bash
# Download a representative set of unsymmetric sparse matrices from SuiteSparse
# Sizes: ~1k, ~4k, ~7k, ~14k, ~35k, ~70k rows
set -euo pipefail

DIR="$(cd "$(dirname "$0")/.." && pwd)/matrices"
DIR="${1:-$DIR}"
mkdir -p "$DIR"
cd "$DIR"

download() {
    local group="$1" name="$2" rows="$3"
    if [ -f "${name}.mtx" ]; then
        echo "  [skip] ${name}.mtx already exists"
        return
    fi
    echo "  Downloading ${name} (~${rows} rows)..."
    curl -fsSL "https://sparse.tamu.edu/MM/${group}/${name}.tar.gz" \
        | tar xz --strip-components=1 "${name}/${name}.mtx"
    echo "  OK: ${name}.mtx"
}

echo "=== Downloading SuiteSparse matrices ==="

# ~1k rows (already in repo, but re-download if missing)
download HB          orsirr_1    "1k"
download HB          orsirr_2    "1k"
download HB          sherman1    "1k"
download HB          fs_183_1    "183"

# ~4k rows
download Mallya      lhr04       "4k"

# ~7k rows
download Mallya      lhr07       "7k"

# ~14k rows
download Mallya      lhr14       "14k"

# ~17k rows
download Mallya      lhr17       "17k"

# ~35k rows
download Mallya      lhr34       "35k"

# ~70k rows
download Mallya      lhr71       "70k"

echo ""
echo "=== Done. Matrices in ${DIR}: ==="
ls -lh "${DIR}"/*.mtx | awk '{print $5, $9}'
