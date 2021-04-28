#!/bin/sh
#
# Run tests in a facebook environment and exclude some that are known to fail.
# This script should be run from a cpython build directory.
set -eux

FILTER=""
# TODO(T64862519, T89667735) Reenable ssl tests.
FILTER+=" | grep -v test.test_ssl.BasicSocketTests.test_match_hostname"
FILTER+=" | grep -v test.test_ssl.BasicSocketTests.test_openssl_version"
FILTER+=" | grep -v test.test_ssl.BasicSocketTests.test_parse_all_sans"
FILTER+=" | grep -v test.test_ssl.BasicSocketTests.test_parse_cert_CVE_2013_4238"
FILTER+=" | grep -v test.test_ssl.ThreadedTests.test_default_ecdh_curve"
FILTER+=" | grep -v test.test_ssl.ThreadedTests.test_dh_params"
FILTER+=" | grep -v test.test_ssl.ThreadedTests.test_no_shared_ciphers"
FILTER+=" | grep -v test.test_ssl.ThreadedTests.test_npn_protocols"
FILTER+=" | grep -v test.test_ssl.ThreadedTests.test_parse_cert_CVE_2013_4238"
FILTER+=" | grep -v test.test_ssl.ThreadedTests.test_session"
FILTER+=" | grep -v test.test_ssl.ThreadedTests.test_session_handling"
FILTER+=" | grep -v test.test_ssl.ThreadedTests.test_version_basic"
# This test is sketchy as it makes assumption about default options that can
# depend on build environment, ...
FILTER+=" | grep -v test.test_ssl.ContextTests.test_options"
eval ./python -m test test_ssl --list-cases $FILTER > test_ssl_cases.txt

# TODO(T89719678): fix refleaks in fork
EXCLUDE=""
if [[ "$@" == *"-R"* ]]; then
EXCLUDE+=" -x test_concurrent_futures"
EXCLUDE+=" -x test_multiprocessing_fork"
EXCLUDE+=" -x test_multiprocessing_forkserver"
EXCLUDE+=" -x test_multiprocessing_spawn"
fi

./python -s -m test -x test_ssl $EXCLUDE "$@"
./python -s -m test test_ssl --matchfile test_ssl_cases.txt "$@"
