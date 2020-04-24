#!/bin/sh
#
# Run tests in a facebook environment and exclude some that are known to fail.
# This script should be run from a cpython build directory.
set -eux

FILTER=""
# TODO(T64862519) Remove this
FILTER+=" | grep -v test.test_ssl.BasicSocketTests.test_openssl_version"
# This test is sketchy as it makes assumption about default options that can
# depend on build environment, ...
FILTER+=" | grep -v test.test_ssl.ContextTests.test_options"
eval ./python -m test test_ssl --list-cases $FILTER > test_ssl_cases.txt

./python -s -m test -x test_ssl "$@" 
./python -s -m test test_ssl --matchfile test_ssl_cases.txt "$@" 
