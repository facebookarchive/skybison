#!/bin/bash

# Add .exe for cpython's MacOS binary
CPYTHON_BIN="third-party/cpython/python"
if [ "$(uname)" == "Darwin" ]; then
  CPYTHON_BIN="$CPYTHON_BIN.exe"
fi

if [[ ! -f "$CPYTHON_BIN" ]]; then
  echo "Please build using 'make cpython-tests'"
  exit 1
fi

PYTHON_BIN="$CPYTHON_BIN" FIND_FILTER="[a-zA-Z]*_test.py" ./python-tests-pyro.sh "$@"
