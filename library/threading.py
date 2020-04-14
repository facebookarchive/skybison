#!/usr/bin/env python3

from _builtins import _Unbound, _unimplemented
from _thread import Lock  # noqa: F401
from _thread import RLock as _RLock


class Barrier:
    def __init__(self, *args, **kwargs):
        _unimplemented()


class BoundedSemaphore:
    def __init__(self, *args, **kwargs):
        _unimplemented()


class BrokenBarrierError(RuntimeError):
    pass


class Condition:
    def __init__(self, lock=None):
        if lock is None:
            lock = RLock()
        self._lock = lock
        self.acquire = lock.acquire
        self.release = lock.release

    def __enter__(self):
        return self._lock.__enter__()

    def __exit__(self, *args):
        return self._lock.__exit__(*args)

    def notify(self, n=1):
        pass

    def notify_all(self):
        pass

    def wait(self, timeout=None):
        _unimplemented()

    def wait_for(self, predicate, timeout=None):
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

    def wait(self, timeout=None):
        signaled = self._flag
        if not signaled:
            # Not much of a point waiting for someone to set the flag in a
            # single-threaded setting, let's abort...
            _unimplemented()
        return signaled


def RLock(*args, **kwargs):
    return _RLock()


class Semaphore:
    def __init__(self, *args, **kwargs):
        _unimplemented()


class Thread:
    def __init__(self, *args, **kwargs):
        _unimplemented()


class _MainThread(Thread):
    def __init__(self, *args, **kwargs):
        if args == ("_main_thread_secret",):
            return
        _unimplemented()

    def getName(self):
        return "MainThread"

    @property
    def ident(self):
        return 1

    @property
    def name(self):
        return "MainThread"


class Timer:
    def __init__(self, *args, **kwargs):
        _unimplemented()


class _ThreadLocal:
    pass


local = _ThreadLocal

_main_thread = _MainThread("_main_thread_secret")


_thread_local = _ThreadLocal()


def active_count():
    return 1


activeCount = active_count


def current_thread():
    return _main_thread


currentThread = current_thread


def enumerate():
    return [_main_thread]


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
