#!/usr/bin/env python3
# $builtin-init-module$

from _builtins import _address, _builtin, _ContextVar_guard, _Token_guard, _Unbound


def _ContextVar_default_value(ctxvar):
    _builtin()


def _ContextVar_name(ctxvar):
    _builtin()


def _Token_used(token):
    _builtin()


def _Token_var(token):
    _builtin()


def _thread_context():
    _builtin()


class Context(bootstrap=True):
    def __contains__(self, var):
        _builtin()

    def __eq__(self, other):
        _builtin()

    def __getitem__(self, var):
        _builtin()

    __hash__ = None

    def __iter__(self):
        _builtin()

    def __new__(cls):
        _builtin()

    def __len__(self):
        _builtin()

    def copy(self):
        _builtin()

    def get(self, var, default=None):
        _builtin()

    def items(self):
        _builtin()

    def keys(self):
        _builtin()

    def run(self, callable, *args, **kwargs):
        _builtin()

    def values(self):
        _builtin()


class ContextVar(bootstrap=True):
    def __new__(cls, name, default=_Unbound):
        _builtin()

    def __repr__(self):
        _ContextVar_guard(self)
        name = _ContextVar_name(self)
        default_value = _ContextVar_default_value(self)
        default_value_str = (
            "" if default_value is _Unbound else f"default={default_value} "
        )
        return f"<ContextVar name='{name}' {default_value_str}at {_address(self):#x}>"

    def get(self, default=_Unbound):
        _builtin()

    def reset(self, token):
        _builtin()

    def set(self, value):
        _builtin()


class Token(bootstrap=True):
    MISSING = _Unbound

    def __new__(cls, context, var, old_value):
        raise RuntimeError("Tokens can only be created by ContextVars")

    def __repr__(self):
        _Token_guard(self)
        used = "used " if _Token_used(self) else ""
        return f"<Token {used}var={_Token_var(self)} at {_address(self):#x}>"


def copy_context():
    return _thread_context().copy()
