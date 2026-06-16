#!/usr/bin/env bash
# Build one or all .cpp files in this directory.
#   ./build.sh              # builds every *.cpp -> ./bin/<basename>
#   ./build.sh 01_warmup    # builds just 01_warmup.cpp -> ./bin/01_warmup
#   ./build.sh 01_warmup.cpp
# Add -r as the last arg to run the binary after a successful build.
# Add -d to build with -O0 instead of -O2 (better gdb experience).

set -euo pipefail

CXX="${CXX:-g++}"
CXXFLAGS=(-std=c++23 -Wall -Wextra -Wpedantic -O2 -g)
OUTDIR="bin"

mkdir -p "$OUTDIR"

run_after=0
args=()
for a in "$@"; do
    if [[ "$a" == "-r" ]]; then
        run_after=1
    elif [[ "$a" == "-d" ]]; then
        CXXFLAGS=(-std=c++23 -Wall -Wextra -Wpedantic -O0 -g)
    else
        args+=("$a")
    fi
done

build_one() {
    local src="$1"
    [[ "$src" != *.cpp ]] && src="${src}.cpp"
    if [[ ! -f "$src" ]]; then
        echo "error: $src not found" >&2
        return 1
    fi
    local base
    base="$(basename "$src" .cpp)"
    local out="$OUTDIR/$base"
    echo ">> $CXX ${CXXFLAGS[*]} $src -o $out"
    "$CXX" "${CXXFLAGS[@]}" "$src" -o "$out"
    echo "   built $out"
    if (( run_after )); then
        echo ">> $out"
        "$out"
    fi
}

if (( ${#args[@]} == 0 )); then
    shopt -s nullglob
    files=(*.cpp)
    if (( ${#files[@]} == 0 )); then
        echo "no .cpp files in $(pwd)" >&2
        exit 1
    fi
    for f in "${files[@]}"; do
        build_one "$f"
    done
else
    for a in "${args[@]}"; do
        build_one "$a"
    done
fi
