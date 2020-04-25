# TODO(T65639170) This stub module needs to be replaced with a real
# implementation or an extension module.
from _builtins import _unimplemented


class Context:
    def __init__(self, *args, **kwargs):
        _unimplemented()


class ContextVar:
    def __init__(self, name):
        pass

    def __repr__(self):
        _unimplemented()

    def __hash__(self):
        _unimplemented()

    def set(self, value):
        _unimplemented()

    def reset(self, token):
        _unimplemented()

    def get(self, default=None):
        _unimplemented()


class Token:
    def __init__(self, *args, **kwargs):
        _unimplemented()


def copy_context():
    _unimplemented()
