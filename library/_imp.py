#!/usr/bin/env python3
"""_imp provides basic importlib support."""

import sys

from _builtins import (
    _builtin,
    _code_check,
    _code_guard,
    _code_set_filename,
    _str_guard,
    _str_rpartition,
    _unimplemented,
)


check_hash_based_pycs = "default"


def create_builtin(spec):
    _builtin()


def _create_dynamic(name, path):
    _builtin()


def create_dynamic(spec):
    _str_guard(spec.name)
    _str_guard(spec.origin)
    spec_name = spec.name
    # Remove the import path from the extension module name
    # TODO(T69088495): Encode in ASCII/punycode and replace '-' with '_'
    if "." in spec_name:
        short_name = _str_rpartition(spec_name, ".")[-1]
    else:
        short_name = spec_name
    result = _create_dynamic(short_name, spec.origin)
    sys.modules[spec_name] = result
    return result


def exec_builtin(module):
    _builtin()


def acquire_lock():
    _builtin()


def exec_dynamic(mod):
    # TODO(T39542987): Enable multi-phase module initialization
    pass


def extension_suffixes():
    return [".pyro.so", ".abi3.so", ".so"]


def _code_update_filenames_recursive(code, old_name, new_name):
    _code_set_filename(code, new_name)
    for const in code.co_consts:
        if _code_check(const):
            _code_update_filenames_recursive(const, old_name, new_name)


def _fix_co_filename(code, path):
    _code_guard(code)
    _str_guard(path)
    old_name = code.co_filename
    if old_name == path:
        return
    _code_update_filenames_recursive(code, old_name, path)


def get_frozen_object(name):
    _builtin()


def init_frozen(name):
    _unimplemented()


def is_builtin(name):
    _builtin()


def is_frozen(name):
    _builtin()


def is_frozen_package(name):
    _builtin()


def lock_held():
    _builtin()


def release_lock():
    _builtin()


def source_hash(key, source):
    _builtin()
