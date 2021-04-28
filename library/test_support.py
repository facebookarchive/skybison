#!/usr/bin/env python3
import sys
import unittest


def cpython_only(test):
    return unittest.skipUnless(
        sys.implementation.name == "cpython", "requires CPython runtime"
    )


def pyro_only(test):
    return unittest.skipUnless(
        sys.implementation.name == "pyro", "requires PyRo runtime"
    )(test)
