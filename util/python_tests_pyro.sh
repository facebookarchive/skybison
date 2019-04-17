#!/bin/bash
if [[ ! -f ./python ]]; then
  echo "Please build using 'make'"
  exit 1
fi

if [[ -z $BUILD_DIR ]]; then
  BUILD_DIR="$(dirname "$0")"
  SOURCE_DIR="$BUILD_DIR/.."
fi

if [[ -z $SOURCE_DIR ]]; then
  echo "SOURCE_DIR is missing"
  exit 1
fi

if [[ -z $PYTHON_BIN ]]; then
  PYTHON_BIN="$BUILD_DIR/python"
fi

if [[ -z $FIND_FILTER ]]; then
  FIND_FILTER="*_test.py"
fi

if [[ -n "$1" ]]; then
  if [[ ! -f "$SOURCE_DIR/library/$1" ]]; then
    REAL_PATH=$(realpath "$SOURCE_DIR")
    echo "There's no test file named: $1 in $REAL_PATH/library"
    exit 1
  fi
  FIND_FILTER="$1"
fi

cd "$BUILD_DIR" || exit 1
rm -rf tests
mkdir tests
find "$SOURCE_DIR/library/" -name "$FIND_FILTER" -exec cp {} "$BUILD_DIR/tests" \;
find "$BUILD_DIR/tests/" -name "$FIND_FILTER" -print0 | xargs -0 -n1 -t "$PYTHON_BIN"
