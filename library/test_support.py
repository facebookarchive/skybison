#!/usr/bin/env python3
import sys
import unittest


def pyro_only(test):
    return unittest.skipUnless(
        sys.implementation.name == "pyro", "requires PyRo runtime"
    )(test)


# TODO(T81730447): Remove this function and all callers once we have upgraded
# our Python unit tests to run under CPython 3.8
def supports_38_feature(test):
    return unittest.skipUnless(
        sys.implementation.name == "pyro" or sys.version_info >= (3, 8),
        "requires PyRo or other 3.8 compatible runtime",
    )(test)
