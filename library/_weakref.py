#!/usr/bin/env python3
"""Weak-reference support module."""
_Unbound = _Unbound  # noqa: F821
_unimplemented = _unimplemented  # noqa: F821


class CallableProxyType:
    def __init__(self, *args, **kwargs):
        _unimplemented()


class ProxyType:
    def __init__(self, *args, **kwargs):
        _unimplemented()


def _remove_dead_weakref(object):
    _unimplemented()


def getweakrefs(object):
    _unimplemented()


def getweakrefcount(object):
    _unimplemented()


def proxy(object, callback=_Unbound):
    _unimplemented()


class ref(bootstrap=True):
    def __call__(self):
        pass

    def __new__(cls, referent, callback=None):
        pass

    def __init__(self, referent, callback=None):
        return None

    def __hash__(self):
        obj = self.__call__()
        if obj is None:
            raise TypeError("weak object has gone away")
        return obj.__hash__()


ReferenceType = ref
