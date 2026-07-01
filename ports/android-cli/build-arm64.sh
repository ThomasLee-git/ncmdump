#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)

ANDROID_NDK_HOME=${ANDROID_NDK_HOME:-/mnt/data/backups/android-ndk/android-ndk-r27d}
BUILD_DIR=${BUILD_DIR:-"$ROOT_DIR/build-android-arm64"}
ANDROID_PLATFORM=${ANDROID_PLATFORM:-android-23}

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM="$ANDROID_PLATFORM" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build "$BUILD_DIR"
