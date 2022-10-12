#!/bin/bash

set -e
cd build
rm -rf *

ANDROID_NDK="${HOME}/Android/Sdk/android-ndk-r22b"
cmake -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK}/build/cmake/android.toolchain.cmake" \
      -DCMAKE_BUILD_TYPE="Debug" \
      -DANDROID_ABI="arm64-v8a" \
      -DANDROID_PLATFORM="android-30" \
      ../

make -j