#!/usr/bin/env python3
"""The _weakref module."""


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
