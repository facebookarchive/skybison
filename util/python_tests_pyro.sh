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
    die "$PYTHON_BIN is not executable. Please build using 'cmake --build . --target python'"
fi

PYRO_TEST_FILTER="*_test.py"
TEST_RUNNING_FILTER="*test*.py"
CPYTHON_TESTS=(
  test___future__.py
  test_augassign.py
  test_binop.py
  test_colorsys.py
  test_crashers.py
  test_decorators.py
  test_dictcomps.py
  test_dummy_thread.py
  test_dynamic.py
  test_dynamicclassattribute.py
  test_ensurepip.py
  test_eof.py
  test_errno.py
  test_exception_variations.py
  test_filecmp.py
  test_flufl.py
  test_fnmatch.py
  test_future3.py
  test_future4.py
  test_future5.py
  test_getpass.py
  test_html.py
  test_int_literal.py
  test_longexp.py
  test_macurl2path.py
  test_netrc.py
  test_openpty.py
  test_osx_env.py
  test_strftime.py
  test_typechecks.py
  test_webbrowser.py
)
PYRO_PATCHED_CPYTHON_TESTS=(
  test_dict.py
  test_hmac.py
  test_range.py
  test_zipimport.py
)

if [[ -n $1 ]]; then
  if [[ ! -f "$SOURCE_DIR/library/$1" ]]; then
    REAL_PATH=$(realpath "$SOURCE_DIR")
    echo "There's no test file named: $1 in $REAL_PATH/library, checking CPYTHON_TESTS"
    for i in "${CPYTHON_TESTS[@]}"
    do
        if [ "$i" == "$1" ] ; then
            FOUND=1
        fi
    done
    if [[ ! $FOUND ]]; then
        die "There's no test file named: $1 in CPYTHON_TESTS"
    fi
  fi
  TEST_RUNNING_FILTER="$1"
fi

cd "$PYRO_BUILD_DIR" || exit 1
rm -rf tests
mkdir tests
# Add Pyro tests
find "$SOURCE_DIR/library/" -name "$PYRO_TEST_FILTER" -exec cp {} tests/ \;
# Add stubbed out CPython tests in Pyro
for i in "${PYRO_PATCHED_CPYTHON_TESTS[@]}"; do
    if [[ -d "$SOURCE_DIR/library/test/$i" ]]; then
        die "We don't support running test directories in CPython yet"
    fi
    cp "$SOURCE_DIR/library/test/$i" tests/;
done
# Add CPython tests
for i in "${CPYTHON_TESTS[@]}"; do
    if [[ -d "$SOURCE_DIR/third-party/cpython/Lib/test/$i" ]]; then
        die "We don't support running test directories yet"
    fi
    cp "$SOURCE_DIR/third-party/cpython/Lib/test/$i" tests/;
done

cp "$SOURCE_DIR/library/test_support.py" tests/

if command -v parallel >/dev/null; then
    TEST_RUNNER=(parallel --will-cite -v --halt "now,fail=1")
else
    NUM_CPUS="$(python -c 'import multiprocessing; print(multiprocessing.cpu_count())')"
    TEST_RUNNER=(xargs -t -P "$NUM_CPUS")
fi
find "$PYRO_BUILD_DIR/tests/" -name "$TEST_RUNNING_FILTER" -print0 |
    PYRO_RECORD_TRACEBACKS=1 "${TEST_RUNNER[@]}" -0 -n1 "$PYTHON_BIN"
