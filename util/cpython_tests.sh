#!/bin/bash

if [[ -z $PYRO_BUILD_DIR ]]; then
  PYRO_BUILD_DIR="$(dirname "$0")/../build"
fi

CPYTHON_TESTS=$PYRO_BUILD_DIR/bin/cpython-tests
if [[ ! -x $CPYTHON_TESTS ]]; then
  cat 1>&2 <<EOF
$CPYTHON_TESTS does not exist or isn't executable. If you're seeing this on
Sandcastle, our build scripts are broken. Otherwise, provide your build root
in PYRO_BUILD_DIR and make sure you've run a build.
EOF
  exit 1
fi

BINARY_LIBS=$(ls -d "$PYRO_BUILD_DIR"/third-party/cpython/build/lib*)
export ASAN_OPTIONS=detect_leaks=0
PYTHONPATH="$PYRO_BUILD_DIR/third-party/cpython/Lib:$BINARY_LIBS"
COMMAND=("$CPYTHON_TESTS" "--gtest_filter=-*Pyro" "$@")
if [[ -n $DEBUG_CPYTHON_TESTS ]]; then
    if ! command -v lldb >/dev/null; then
        echo "lldb not found in PATH." 1>&2
        exit 1
    fi

    # Don't set PYTHONPATH for lldb, since it uses Python internally and would
    # get confused.
    lldb -s <(echo "env PYTHONPATH=$PYTHONPATH") -- "${COMMAND[@]}"
else
    PYTHONPATH=$PYTHONPATH "${COMMAND[@]}"
fi
