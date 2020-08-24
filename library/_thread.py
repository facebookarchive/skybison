#!/usr/bin/env python3

from _builtins import _builtin, _Unbound, _unimplemented


# TODO(T53322979): Re-write to be thread-safe
class LockType:
    def __exit__(self, t, v, tb):
        self.release()

    def __init__(self):
        self._locked = False

    def acquire(self, blocking=True, timeout=-1):
        if self._locked:
            # This would block indefinitely, let's abort instead...
            _unimplemented()
        self._locked = True

    __enter__ = acquire

    acquire_lock = acquire

    def release(self):
        if not self._locked:
            raise RuntimeError("release unlocked lock")
        self._locked = False

    release_lock = release


class RLock:
    """Recursive (pseudo) Lock assuming single-threaded execution."""

    def __init__(self):
        self._count = 0

    def acquire(self, blocking=True, timeout=-1):
        self._count += 1

    __enter__ = acquire

    def release(self):
        if self._count == 0:
            raise RuntimeError("cannot release un-acquired lock")
        self._count -= 1

    def __exit__(self, t, v, tb):
        self.release()


def _count():
    _unimplemented()


class _local:
    pass


def _set_sentinel():
    _unimplemented()


def allocate():
    _unimplemented()


allocate_lock = LockType


error = RuntimeError


def exit():
    _unimplemented()


def exit_thread():
    _unimplemented()


def get_ident():
    _builtin()


def interrupt_main():
    _unimplemented()


def stack_size(size=0):
    _unimplemented()


def start_new(function, args, kwargs=_Unbound):
    _unimplemented()


def start_new_thread(function, args, kwargs=_Unbound):
    _unimplemented()
