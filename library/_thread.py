#!/usr/bin/env python3

# This is the patch decorator, injected by our boot process. flake8 has no
# knowledge about its definition and will complain without this gross circular
# helper here.
_unimplemented = _unimplemented  # noqa: F821


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


allocate_lock = Lock


def get_ident():
    return 0
