#!/usr/bin/env python3
"""_warnings provides basic warning filtering support."""


@_patch  # noqa: F821
def warn(message, category=None, stacklevel=1, source=None):
    pass


# TODO(T43357250): These need to be real things, not stubbed out
filters = []
defaultaction = "default"
onceregistry = {}

_filters_version = 1


def _filters_mutated():
    global _filters_version
    _filters_version += 1
