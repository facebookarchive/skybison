#!/usr/bin/env python3
"""The _weakref module."""


class ref(bootstrap=True):
    def __call__(self):
        if not isinstance(self, ref):
            raise TypeError("'__call__' requires a 'ref' object")
        return self._referent

    def __new__(cls, referent, callback=None):
        pass

    def __init__(self, referent, callback=None):
        return None
