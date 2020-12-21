#!/usr/bin/env python3

from builtins import (
    _index,
    _non_heaptype,
    _sequence_repr,
)

from _builtins import (
    _builtin,
    _deque_guard,
    _dict_guard,
    _int_guard,
    _object_type_hasattr,
    _repr_enter,
    _repr_leave,
    _tuple_check,
    _tuple_getitem,
    _type,
    _unimplemented,
)


_Unbound = _Unbound  # noqa: F821


class OrderedDict(dict):
    "Dictionary that remembers insertion order"
    # NOTE: This OrderedDict implementation deviates from CPython's: instaed of keeping
    # the ordering as a linked list, this implementation harnesses the ordering
    # maintained by `dict`.

    def move_to_end(self, key, last=True):
        value = self.pop(key)
        if not last:
            _unimplemented()
        self[key] = value

    def __repr__(self):
        return _sequence_repr(f"{self.__class__.__name__}([", self.items(), "])")


class _deque_iterator(bootstrap=True):
    def __iter__(self):
        return self

    def __length_hint__(self):
        _builtin()

    @staticmethod
    def __new__(cls, deq, index=0):
        _builtin()

    def __next__(self):
        _builtin()

    def __reduce__(self):
        _builtin()


class _deque_reverse_iterator(bootstrap=True):
    def __iter__(self):
        return self

    def __length_hint__(self):
        _builtin()

    @staticmethod
    def __new__(cls, deq, index=0):
        _builtin()

    def __next__(self):
        _builtin()

    def __reduce__(self):
        _builtin()


def _deque_delitem(self, index):
    _builtin()


def _deque_getitem(self, index):
    _builtin()


def _deque_set_maxlen(self, maxlen):
    _builtin()


def _deque_setitem(self, index, value):
    _builtin()


class _tuplegetter(metaclass=_non_heaptype):
    __slots__ = ("_index", "_doc")

    @property
    def __doc__(self):
        return self._doc

    @__doc__.setter
    def __doc__(self, value):
        self._doc = value

    def __get__(self, instance, owner):
        if not _tuple_check(instance):
            if instance is None:
                return self
            raise TypeError(
                f"descriptor for index '{self._index}' for tuple subclasses doesn't apply to '{_type(instance).__name__}' object"
            )
        return _tuple_getitem(instance, self._index)

    @staticmethod
    def __new__(cls, index, doc):
        result = super().__new__(cls)
        result._index = _index(index)
        result._doc = doc
        return result


class defaultdict(dict):
    def __copy__(self):
        _unimplemented()

    def __init__(self, default_factory=None, *args, **kwargs):
        super().__init__(*args, **kwargs)
        if default_factory is not None and not callable(default_factory):
            raise TypeError("default_factory must be callable or None")
        self.default_factory = default_factory

    def __getitem__(self, key):
        _dict_guard(self)
        result = dict.get(self, key, _Unbound)
        if result is _Unbound:
            return _type(self).__missing__(self, key)
        return result

    def __missing__(self, key):
        default_factory = self.default_factory
        if default_factory is None:
            raise KeyError(key)
        value = default_factory()
        dict.__setitem__(self, key, value)
        return value

    def __reduce__(self):
        _unimplemented()

    def __repr__(self):
        return f"defaultdict({self.default_factory!r}, {dict.__repr__(self)})"

    def copy(self):
        _unimplemented()


class deque(bootstrap=True):
    def __add__(self, other):
        _unimplemented()

    def __bool__(self):
        _deque_guard(self)
        return len(self) > 0

    def __contains__(self, value):
        _deque_guard(self)
        for item in self:
            if item is value or item == value:
                return True
        return False

    def __copy__(self):
        _deque_guard(self)
        return self.__class__(self, self.maxlen)

    def __delitem__(self, index):
        result = _deque_delitem(self, index)
        if result is not _Unbound:
            return result
        if _object_type_hasattr(index, "__index__"):
            return _deque_delitem(self, _index(index))
        raise TypeError(
            f"sequence index must be integer, not '{_type(index).__name__}'"
        )

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
        "$intrinsic$"
        result = _deque_getitem(self, index)
        if result is not _Unbound:
            return result
        if _object_type_hasattr(index, "__index__"):
            return _deque_getitem(self, _index(index))
        raise TypeError(
            f"sequence index must be integer, not '{_type(index).__name__}'"
        )

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
        deque.clear(self)
        _deque_set_maxlen(self, maxlen)
        self.extend(iterable)

    def __iter__(self):
        _builtin()

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

    @staticmethod
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
        _builtin()

    def __rmul__(self, n):
        _unimplemented()

    def __setitem__(self, index, value):
        "$intrinsic$"
        result = _deque_setitem(self, index, value)
        if result is not _Unbound:
            return result
        if _object_type_hasattr(index, "__index__"):
            return _deque_setitem(self, _index(index), value)
        raise TypeError(
            f"sequence index must be integer, not '{_type(index).__name__}'"
        )

    def append(self, x):
        _builtin()

    def appendleft(self, x):
        _builtin()

    def clear(self):
        _builtin()

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
        _builtin()

    def popleft(self):
        _builtin()

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
        _builtin()

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
