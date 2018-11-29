#!/bin/sh
BUILD_DIR="$(dirname "$0")/../build"
BINARY_LIBS=$(ls -d "$BUILD_DIR"/third-party/cpython/build/lib*)
export ASAN_OPTIONS=detect_leaks=0
export PYTHONPATH="$BUILD_DIR/third-party/cpython/Lib:$BINARY_LIBS"
"$BUILD_DIR"/cpython-tests --gtest_filter=-*_Pyro "$@"
