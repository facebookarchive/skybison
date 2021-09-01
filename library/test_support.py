#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
import sys
import unittest


def cpython_only(test):
    return unittest.skipUnless(
        sys.implementation.name == "cpython", "requires CPython runtime"
    )


def pyro_only(test):
    return unittest.skipUnless(
        sys.implementation.name == "skybison", "requires PyRo runtime"
    )(test)
