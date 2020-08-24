#!/usr/bin/env python3
# $builtin-init-module$

from _builtins import _builtin, _int_guard, _type_subclass_guard, _unimplemented


def _mmap_new(cls, fileno, length, flags, prot, offset):
    _builtin()


ACCESS_READ = 1
ACCESS_WRITE = 2
ACCESS_COPY = 3


class mmap(bootstrap=True):
    @staticmethod
    def __new__(cls, fileno, length, flags=1, prot=3, access=0, offset=0):
        """
        Creates a new mmap object.

        Values for flags are ints:
        * MAP_SHARED = 1
        * MAP_PRIVATE = 2

        Values for prot are ints:
        * PROT_READ = 1
        * PROT_WRITE = 2
        * PROT_EXEC = 4

        Some operating systems / file systems could provide additional values.
        """
        _type_subclass_guard(cls, mmap)
        _int_guard(fileno)
        _int_guard(length)
        _int_guard(flags)
        _int_guard(prot)
        _int_guard(access)
        _int_guard(offset)
        if length < 0:
            raise OverflowError("memory mapped length must be positive")
        if offset < 0:
            raise OverflowError("memory mapped offset must be positive")
        if access != 0 and (flags != 1 or prot != 3):
            raise ValueError("mmap can't specify both access and flags, prot.")
        if access == ACCESS_READ:
            flags = 1
            prot = 1
        elif access == ACCESS_WRITE:
            flags = 1
            prot = 3
        elif access == ACCESS_COPY:
            flags = 2
            prot = 3
        elif access != 0:
            raise ValueError("mmap invalid access parameter.")
        return _mmap_new(cls, fileno, length, flags, prot, offset)

    def close(self):
        _builtin()
