#!/usr/bin/env python3
"""_imp provides basic importlib support."""


# This is the patch decorator, injected by our boot process. flake8 has no
# knowledge about its definition and will complain without this gross circular
# helper here.
_patch = _patch  # noqa: F821


@_patch
def create_builtin(spec):
    pass


@_patch
def exec_builtin(module):
    pass
