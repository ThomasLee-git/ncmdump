#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)

BUILD_DIR=${BUILD_DIR:-"$ROOT_DIR/build-macos-arm64"}
CMAKE_GENERATOR=${CMAKE_GENERATOR:-Ninja}

set -- \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0

if [ -n "${CMAKE_TOOLCHAIN_FILE:-}" ]; then
  set -- "$@" -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE"
fi

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -G "$CMAKE_GENERATOR" "$@"

cmake --build "$BUILD_DIR"
