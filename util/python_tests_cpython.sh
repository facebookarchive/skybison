#!/bin/bash

if [[ -z $PYRO_BUILD_DIR ]]; then
  PYRO_BUILD_DIR="$(dirname "$0")/../build"
fi

# Add .exe for cpython's MacOS binary
CPYTHON_BIN="$PYRO_BUILD_DIR/third-party/cpython/python"
if [[ "$(uname)" == "Darwin" ]]; then
  CPYTHON_BIN+=".exe"
fi

if [[ ! -x "$CPYTHON_BIN" ]]; then
  echo "$CPYTHON_BIN is not executable. Please build using 'cmake --build . --target cpython'" 1>&2
  exit 1
fi

PYRO_BUILD_DIR="$PYRO_BUILD_DIR" PYTHON_BIN="$CPYTHON_BIN" \
    "$(dirname "$0")/python_tests_pyro.sh" "$@"
