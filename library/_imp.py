#!/usr/bin/env python3
"""_imp provides basic importlib support."""


# This is the patch decorator, injected by our boot process. flake8 has no
# knowledge about its definition and will complain without this gross circular
# helper here.
_patch = _patch  # noqa: F821
_str_guard = _str_guard  # noqa: F821


@_patch
def create_builtin(spec):
    pass


@_patch
def _create_dynamic(name, path):
    pass


def create_dynamic(spec):
    _str_guard(spec.name)
    _str_guard(spec.origin)
    return _create_dynamic(spec.name, spec.origin)


@_patch
def exec_builtin(module):
    pass


@_patch
def acquire_lock():
    pass


def exec_dynamic(mod):
    # TODO(T39542987): Enable multi-phase module initialization
    pass


@_patch
def extension_suffixes():
    pass


@_patch
def _fix_co_filename(code, path):
    pass


@_patch
def get_frozen_object(name):
    pass


@_patch
def is_builtin(name):
    pass


@_patch
def is_frozen(name):
    pass


@_patch
def is_frozen_package(name):
    pass


@_patch
def release_lock():
    pass
