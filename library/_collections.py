#!/usr/bin/env python3
# $builtin-init-module$

from builtins import _index

from _builtins import (
    _builtin,
    _deque_guard,
    _int_check,
    _int_guard,
    _repr_enter,
    _repr_leave,
    _type,
    _unimplemented,
)


class _deque_iterator:
    def __init__(self, deq, itergen):
        self.counter = len(deq)

        def giveup():
            self.counter = 0
            raise RuntimeError("deque mutated during iteration")

        self._gen = itergen(deq.state, giveup)

    def __next__(self):
        res = next(self._gen)
        self.counter -= 1
        return res

    def __iter__(self):
        return self


def _deque_set_maxlen(self, maxlen):
    _builtin()


class deque(bootstrap=True):
    def __add__(self, other):
        _unimplemented()

    def __bool__(self):
        _deque_guard(self)
        return self.length > 0

    def __contains__(self, value):
        _deque_guard(self)
        for item in self:
            if item is value or item == value:
                return True
        return False

    def __copy__(self):
        _deque_guard(self)
        return self.__class__(self, self._maxlen)

    def __delitem__(self, index):
        _deque_guard(self)
        if not _int_check(index):
            index = _index(index)
        length = len(self)
        if index >= 0:
            if index >= length:
                raise IndexError("deque index out of range")
            deque.rotate(self, -index)
            deque.popleft(self)
            deque.rotate(self, index)
        else:
            index = ~index
            if index >= length:
                raise IndexError("deque index out of range")
            deque.rotate(self, index)
            deque.pop(self)
            deque.rotate(self, -index)

    # TODO(emacs): Make comparison more efficient, perhaps by checking length
    # first.
    def __eq__(self, other):
        _deque_guard(self)
        if isinstance(other, deque):
            return list(self) == list(other)
        else:
            return NotImplemented

    def __ge__(self, other):
        _deque_guard(self)
        if isinstance(other, deque):
            return list(self) >= list(other)
        else:
            return NotImplemented

    def __getitem__(self, index):
        _unimplemented()

    def __gt__(self, other):
        _deque_guard(self)
        if isinstance(other, deque):
            return list(self) > list(other)
        else:
            return NotImplemented

    __hash__ = None

    def __iadd__(self, other):
        _deque_guard(self)
        deque.extend(self, other)
        return self

    def __init__(self, iterable=(), maxlen=None):
        _deque_guard(self)
        # TODO(T67100024): call clear here after its implemented
        _deque_set_maxlen(self, maxlen)
        # TODO(T67099753): handle iterable

    def __iter__(self):
        _deque_guard(self)
        return _deque_iterator(self, self._iter_impl)

    def __le__(self, other):
        _deque_guard(self)
        if isinstance(other, deque):
            return list(self) <= list(other)
        else:
            return NotImplemented

    def __len__(self):
        _builtin()

    def __lt__(self, other):
        _deque_guard(self)
        if isinstance(other, deque):
            return list(self) < list(other)
        else:
            return NotImplemented

    def __mul__(self, n):
        _unimplemented()

    def __ne__(self, other):
        _deque_guard(self)
        if isinstance(other, deque):
            return list(self) != list(other)
        else:
            return NotImplemented

    def __new__(cls, iterable=(), *args, **kw):
        _builtin()

    def __reduce__(self):
        _unimplemented()

    def __reduce_ex__(self, proto):
        _unimplemented()

    def __repr__(self):
        _deque_guard(self)
        if _repr_enter(self):
            return "[...]"
        if self.maxlen is not None:
            result = f"deque({list(self)!r}, maxlen={self.maxlen})"
        else:
            result = f"deque({list(self)!r})"
        _repr_leave(self)
        return result

    def __reversed__(self):
        _deque_guard(self)
        return _deque_iterator(self, self._reversed_impl)

    def __rmul__(self, n):
        _unimplemented()

    def __setitem__(self, index, value):
        _deque_guard(self)
        block, index = deque.__getref(self, index)
        block[index] = value

    def _iter_impl(self, original_state, giveup):
        _unimplemented()

    def _reversed_impl(self, original_state, giveup):
        _unimplemented()

    def append(self, x):
        _builtin()

    def appendleft(self, x):
        _builtin()

    def clear(self):
        _unimplemented()

    def count(self, value):
        _deque_guard(self)
        c = 0
        for item in self:
            if item is value or item == value:
                c += 1
        return c

    def extend(self, iterable):
        _deque_guard(self)
        if iterable is self:
            iterable = list(iterable)
        for elem in iterable:
            deque.append(self, elem)

    def extendleft(self, iterable):
        _deque_guard(self)
        if iterable is self:
            iterable = list(iterable)
        for elem in iterable:
            deque.appendleft(self, elem)

    def index(self, value):
        _unimplemented()

    def insert(self, value):
        _unimplemented()

    def pop(self):
        _unimplemented()

    def popleft(self):
        _unimplemented()

    def remove(self, value):
        _deque_guard(self)
        i = 0
        try:
            for i in range(len(self)):  # noqa: B007
                self_value = self[0]
                if self_value is value or self_value == value:
                    deque.popleft(self)
                    return
                deque.append(self, deque.popleft(self))
            i += 1
            raise ValueError("deque.remove(x): x not in deque")
        finally:
            deque.rotate(self, i)

    def reverse(self):
        _unimplemented()

    def rotate(self, n=1):
        _deque_guard(self)
        _int_guard(n)
        length = len(self)
        if length <= 1:
            return
        halflen = length >> 1
        if n > halflen or n < -halflen:
            n %= length
            if n > halflen:
                n -= length
            elif n < -halflen:
                n += length
        while n > 0:
            deque.appendleft(self, deque.pop(self))
            n -= 1
        while n < 0:
            deque.append(self, deque.popleft(self))
            n += 1
