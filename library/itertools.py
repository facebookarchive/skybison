#!/usr/bin/env python3
"""Functional tools for creating and using iterators."""
# TODO(T42113424) Replace stubs with an actual implementation
_Unbound = _Unbound  # noqa: F821
_list_len = _list_len  # noqa: F821
_tuple_len = _tuple_len  # noqa: F821
_unimplemented = _unimplemented  # noqa: F821


class accumulate:
    def __init__(self, p, func=_Unbound):
        _unimplemented()


class chain:
    def __init__(self, p, q, *args):
        _unimplemented()

    @staticmethod
    def from_iterable(*args):
        _unimplemented()


class combinations:
    def __init__(self, p, r):
        _unimplemented()


class combinations_with_replacement:
    def __init__(self, p, r):
        _unimplemented()


class compress:
    def __init__(self, data, selectors):
        _unimplemented()


class count:
    def __init__(self, start=0, step=1):
        start_type = type(start)
        if not hasattr(start_type, "__add__") or not hasattr(start_type, "__sub__"):
            raise TypeError("a number is required")
        self.count = start
        self.step = step

    def __iter__(self):
        return self

    def __next__(self):
        result = self.count
        self.count += self.step
        return result

    def __repr__(self):
        return f"count({self.count})"


class cycle:
    def __init__(self, p):
        _unimplemented()


class dropwhile:
    def __init__(self, pred, seq):
        _unimplemented()


class filterfalse:
    def __init__(self, pred, seq):
        _unimplemented()


class groupby:
    def __init__(self, iterable, keyfunc=_Unbound):
        _unimplemented()


class islice:
    def __init__(self, seq, *args):
        _unimplemented()


class permutations:
    def __init__(self, p, r=_Unbound):
        _unimplemented()


class product:
    def __init__(self, *iterables, repeat=1):
        if not isinstance(repeat, int):
            raise TypeError
        length = _tuple_len(iterables) if repeat else 0
        i = 0
        repeated = [None] * length
        while i < length:
            item = tuple(iterables[i])
            if not item:
                self._iterables = self._digits = None
                return
            repeated[i] = item
            i += 1
        repeated *= repeat
        self._iterables = repeated
        self._digits = [0] * (length * repeat)

    def __iter__(self):
        return self

    def __next__(self):
        iterables = self._iterables
        if iterables is None:
            raise StopIteration
        digits = self._digits
        length = _list_len(iterables)
        result = [None] * length
        i = length - 1
        carry = 1
        while i >= 0:
            j = digits[i]
            result[i] = iterables[i][j]
            j += carry
            if j < _tuple_len(iterables[i]):
                carry = 0
                digits[i] = j
            else:
                carry = 1
                digits[i] = 0
            i -= 1
        if carry:
            # counter overflowed, stop iteration
            self._iterables = self._digits = None
        return tuple(result)


class repeat:
    def __init__(self, elem, n=_Unbound):
        _unimplemented()


class starmap:
    def __init__(self, fun, seq):
        _unimplemented()


def tee(it, n=2):
    _unimplemented()


class takewhile:
    def __init__(self, pred, seq):
        _unimplemented()


class zip_longest:
    def __init__(self, p, q, *args):
        _unimplemented()
