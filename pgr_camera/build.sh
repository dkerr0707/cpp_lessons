#!/usr/bin/env bash
# Build (and optionally run) the Spinnaker hello-world.
#   ./build.sh           # configure (if needed) + build
#   ./build.sh -r        # build and run
#   ./build.sh -c        # clean (delete build/) and rebuild from scratch
#
# Override Spinnaker location:
#   SPINNAKER_ROOT=/your/path ./build.sh

set -euo pipefail

cd "$(dirname "$0")"

BUILD_DIR="build"
SPINNAKER_ROOT="${SPINNAKER_ROOT:-/opt/spinnaker}"

run_after=0
clean=0
for a in "$@"; do
    case "$a" in
        -r) run_after=1 ;;
        -c) clean=1 ;;
        *)  echo "unknown arg: $a" >&2; exit 2 ;;
    esac
done

if (( clean )); then
    rm -rf "$BUILD_DIR"
fi

if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    echo ">> cmake -S . -B $BUILD_DIR -DSPINNAKER_ROOT=$SPINNAKER_ROOT"
    cmake -S . -B "$BUILD_DIR" -DSPINNAKER_ROOT="$SPINNAKER_ROOT"
fi

echo ">> cmake --build $BUILD_DIR"
cmake --build "$BUILD_DIR"

if (( run_after )); then
    echo ">> bin/hello_world"
    bin/hello_world
fi
