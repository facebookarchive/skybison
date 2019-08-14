#!/usr/bin/env python3
import sys
import unittest


def pyro_only(test):
    return unittest.skipUnless(
        sys.implementation.name == "pyro", "requires PyRo runtime"
    )(test)
