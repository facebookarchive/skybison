#!/usr/bin/env python3


from _thread import Lock  # noqa: F401


_Unbound = _Unbound  # noqa: F821
_unimplemented = _unimplemented  # noqa: F821


class Barrier:
    def __init__(self, *args, **kwargs):
        _unimplemented()


class BoundedSemaphore:
    def __init__(self, *args, **kwargs):
        _unimplemented()


class BrokenBarrierError(RuntimeError):
    pass


class Condition:
    def __init__(self, *args, **kwargs):
        _unimplemented()


class Event:
    def __init__(self):
        self._flag = False

    def clear(self):
        self._flag = False

    def is_set(self):
        return self._flag

    isSet = is_set

    def set(self):
        self._flag = True

    def wait(self, timeout):
        if not self._flag:
            # Not much of a point waiting for someone to set the flag in a
            # single-threaded setting, let's abort...
            _unimplemented()


def RLock(*args, **kwargs):
    return _RLock()


class Semaphore:
    def __init__(self, *args, **kwargs):
        _unimplemented()


class Thread:
    def __init__(self, *args, **kwargs):
        _unimplemented()


class Timer:
    def __init__(self, *args, **kwargs):
        _unimplemented()


class _PseudoThread:
    def getName(self):
        return "MainThread"

    @property
    def ident(self):
        return 1

    @property
    def name(self):
        return "MainThread"


class _RLock:
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


class _ThreadLocal:
    pass


local = _ThreadLocal

_main_thread = _PseudoThread()


_thread_local = _ThreadLocal()


def active_count():
    return 1


def activeCount():
    return 1


def current_thread():
    return _main_thread


def enumerate():
    _unimplemented()


def get_ident():
    return _main_thread.ident


def main_thread():
    return _main_thread


def setprofile(func):
    _unimplemented()


def settrace(func):
    _unimplemented()


def stack_size(size=_Unbound):
    _unimplemented()
