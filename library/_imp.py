#!/usr/bin/env python3
"""_imp provides basic importlib support."""

from _builtins import _builtin, _str_guard, _str_rpartition, _unimplemented


def create_builtin(spec):
    _builtin()


def _create_dynamic(name, path):
    _builtin()


def create_dynamic(spec):
    _str_guard(spec.name)
    _str_guard(spec.origin)
    name = spec.name
    # Remove the import path from the extension module name
    if "." in name:
        name = _str_rpartition(name, ".")[-1]
    return _create_dynamic(name, spec.origin)


def exec_builtin(module):
    _builtin()


def acquire_lock():
    _builtin()


def exec_dynamic(mod):
    # TODO(T39542987): Enable multi-phase module initialization
    pass


def extension_suffixes():
    _builtin()


def _fix_co_filename(code, path):
    if code.co_filename == path:
        return
    # TODO(T63991004): Implement the rest of _fix_co_filename
    _unimplemented()


def get_frozen_object(name):
    _unimplemented()


def is_builtin(name):
    _builtin()


def is_frozen(name):
    _builtin()


def is_frozen_package(name):
    _unimplemented()


def lock_held():
    _builtin()


def release_lock():
    _builtin()
