#!/usr/bin/env python3
"""Tools that operate on functions."""
_Unbound = _Unbound  # noqa: F821
_unimplemented = _unimplemented  # noqa: F821


def reduce(function, sequence, initial=_Unbound):
    _unimplemented()


def cmp_to_key(*args):
    _unimplemented()


# TODO(T42802697): Temporary implementation mentioned in the python docs
# https://docs.python.org/3/library/functools.html#functools.partial
def partial(func, *args, **keywords):
    def newfunc(*fargs, **fkeywords):
        newkeywords = keywords.copy()
        newkeywords.update(fkeywords)
        return func(*args, *fargs, **newkeywords)

    newfunc.func = func
    newfunc.args = args
    newfunc.keywords = keywords
    return newfunc


class _lru_cache_wrapper:
    def __init__(self, *args, **kwargs):
        _unimplemented()
