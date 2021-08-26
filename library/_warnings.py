#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
"""_warnings provides basic warning filtering support."""

from _builtins import _builtin


def warn(message, category=None, stacklevel=1, source=None):
    _builtin()


# TODO(T43357250): These need to be real things, not stubbed out
filters = []
defaultaction = "default"
onceregistry = {}

_filters_version = 1


def _filters_mutated():
    global _filters_version
    _filters_version += 1
