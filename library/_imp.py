#!/usr/bin/env python3
"""_imp provides basic importlib support."""

from _builtins import _builtin, _str_guard, _unimplemented


def create_builtin(spec):
    _builtin()


def _create_dynamic(name, path):
    _builtin()


def create_dynamic(spec):
    _str_guard(spec.name)
    _str_guard(spec.origin)
    return _create_dynamic(spec.name, spec.origin)


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
    _unimplemented()


def get_frozen_object(name):
    _unimplemented()


def is_builtin(name):
    _builtin()


def is_frozen(name):
    _builtin()


def is_frozen_package(name):
    _unimplemented()


def release_lock():
    _builtin()
