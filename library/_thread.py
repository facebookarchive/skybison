#!/usr/bin/env python3

from _builtins import _unimplemented


# TODO(T53322979): Re-write to be thread-safe
class Lock:
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


allocate_lock = Lock


def get_ident():
    return 0
