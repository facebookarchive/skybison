#!/bin/bash

function die {
  echo "$@" 1>&2
  exit 1
}

SOURCE_DIR="$(realpath "$(dirname "$0")/..")"

if [[ -z $PYRO_BUILD_DIR ]]; then
  PYRO_BUILD_DIR="$(dirname "$0")/../build"
fi

if [[ -z $PYTHON_BIN ]]; then
  PYTHON_BIN="$PYRO_BUILD_DIR/python"
fi

PYRO_BUILD_DIR="$(realpath "$PYRO_BUILD_DIR")"
SOURCE_DIR="$(realpath "$SOURCE_DIR")"
PYTHON_BIN="$(realpath "$PYTHON_BIN")"

if [[ ! -x $PYTHON_BIN ]]; then
    # Assume that this path is impossible if we're being called by
    # python_tests_cpython.sh because it does its own validation.
    die "$PYTHON_BIN is not executable. Please build using 'make python'"
fi

if [[ -z $FIND_FILTER ]]; then
  FIND_FILTER="*_test.py"
fi

if [[ -n $1 ]]; then
  if [[ ! -f "$SOURCE_DIR/library/$1" ]]; then
    REAL_PATH=$(realpath "$SOURCE_DIR")
    die "There's no test file named: $1 in $REAL_PATH/library"
  fi
  FIND_FILTER="$1"
fi

cd "$PYRO_BUILD_DIR" || exit 1
rm -rf tests
mkdir tests
find "$SOURCE_DIR/library/" -name "$FIND_FILTER" -exec cp {} tests/ \;

if command -v parallel >/dev/null; then
    TEST_RUNNER=(parallel -v)
else
    NUM_CPUS="$(python -c 'import multiprocessing; print(multiprocessing.cpu_count())')"
    TEST_RUNNER=(xargs -t -P "$NUM_CPUS")
fi
find "$PYRO_BUILD_DIR/tests/" -name "$FIND_FILTER" -print0 |
    PYRO_RECORD_TRACEBACKS=1 "${TEST_RUNNER[@]}" -0 -n1 "$PYTHON_BIN"
