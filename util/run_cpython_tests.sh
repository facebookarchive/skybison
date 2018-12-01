#!/bin/bash

if [[ -n $1 ]]; then
  BUILD_DIR=$1
else
  BUILD_DIR="$(dirname "$0")/../build"
fi

CPYTHON_TESTS=$BUILD_DIR/cpython-tests
if [[ ! -x $CPYTHON_TESTS ]]; then
  cat 1>&2 <<EOF
$CPYTHON_TESTS does not exist or isn't executable. If you're seeing this on
Sandcastle, our build scripts are broken. Otherwise, provide your build root as
an argument to this script and make sure you've run a build.
EOF
  exit 1
fi

BINARY_LIBS=$(ls -d "$BUILD_DIR"/third-party/cpython/build/lib*)
export ASAN_OPTIONS=detect_leaks=0
export PYTHONPATH="$BUILD_DIR/third-party/cpython/Lib:$BINARY_LIBS"
"$CPYTHON_TESTS" "--gtest_filter=-*Pyro" "$@"
