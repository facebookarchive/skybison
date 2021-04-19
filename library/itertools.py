#!/usr/bin/env python3
"""Functional tools for creating and using iterators."""
# TODO(T42113424) Replace stubs with an actual implementation

import operator
from builtins import _number_check

from _builtins import (
    _int_check,
    _int_guard,
    _list_len,
    _list_new,
    _tuple_len,
    _Unbound,
    _unimplemented,
)


class accumulate:
    def __iter__(self):
        return self

    def __new__(cls, iterable, func=None, initial=None):
        result = object.__new__(cls)
        result._it = iter(iterable)
        result._func = operator.add if func is None else func
        result._initial = initial
        result._accumulated = None
        return result

    def __next__(self):
        initial = self._initial
        if initial is not None:
            self._accumulated = initial
            self._initial = None
            return initial

        result = self._accumulated

        if result is None:
            result = next(self._it)
            self._accumulated = result
            return result

        result = self._func(result, next(self._it))
        self._accumulated = result
        return result

    def __reduce__(self):
        _unimplemented()

    def __setstate__(self):
        _unimplemented()


class chain:
    def __iter__(self):
        return self

    def __new__(cls, *iterables):
        result = object.__new__(cls)
        result._it = None
        result._iterables = iter(iterables)
        return result

    def __next__(self):
        while True:
            if self._it is None:
                try:
                    self._it = iter(next(self._iterables))
                except StopIteration:
                    raise
            try:
                result = next(self._it)
            except StopIteration:
                self._it = None
                continue
            return result

    def __reduce__(self):
        _unimplemented()

    def __setstate__(self):
        _unimplemented()

    @classmethod
    def from_iterable(cls, iterable):
        result = object.__new__(cls)
        result._it = None
        result._iterables = iter(iterable)
        return result


class combinations:
    def __iter__(self):
        return self

    def __new__(cls, iterable, r):
        _int_guard(r)
        if r < 0:
            raise ValueError("r must be non-negative")

        result = object.__new__(cls)

        seq = tuple(iterable)
        n = _tuple_len(seq)

        if r > n:
            result._seq = None
            return result

        result._seq = seq
        result._indices = list(range(r))
        result._r = r
        result._index_delta = n - r
        return result

    def __next__(self):
        seq = self._seq
        if seq is None:
            raise StopIteration

        r = self._r
        indices = self._indices
        index_delta = self._index_delta

        # The result is the elements of the sequence at the current indices
        result = (*(seq[indices[i]] for i in range(r)),)

        # Scan indices right-to-left until finding one that is not at its
        # maximum (i + n - r).
        i = r - 1
        while i >= 0:
            if indices[i] < i + index_delta:
                # Increment the current index which we know is not at its
                # maximum.  Then move back to the right setting each index
                # to its lowest possible value (one higher than the index
                # to its left -- this maintains the sort order invariant).
                indices[i] += 1
                for j in range(i + 1, r):
                    indices[j] = indices[j - 1] + 1
                break
            i -= 1
        else:
            # The indices are all at their maximum values and we're done.
            self._seq = None

        return result

    def __reduce__(self):
        _unimplemented()

    def __setstate__(self):
        _unimplemented()

    def __sizeof__(self):
        _unimplemented()


class combinations_with_replacement:
    def __iter__(self):
        return self

    def __new__(cls, iterable, r):
        _int_guard(r)
        if r < 0:
            raise ValueError("r must be non-negative")

        result = object.__new__(cls)

        seq = tuple(iterable)

        # We can't create combinations is if seq is empty and r > 0
        if not seq and r:
            result._seq = None
            return result

        result._seq = seq
        result._indices = _list_new(r, 0)
        result._r = r
        result._max_index = _tuple_len(seq) - 1
        return result

    def __next__(self):
        seq = self._seq
        if seq is None:
            raise StopIteration

        r = self._r
        indices = self._indices
        max_index = self._max_index

        # The result is the elements of the sequence at the current indices
        result = (*(seq[indices[i]] for i in range(r)),)

        # Scan indices right-to-left until finding one that is not at its
        # maximum (n - 1).
        i = r - 1
        while i >= 0:
            if indices[i] < max_index:
                # Increment the current index which we know is not at its
                # maximum.  Then set all to the right to the same value.
                index = indices[i] = indices[i] + 1
                for j in range(i, r):
                    indices[j] = index
                break
            i -= 1
        else:
            # The indices are all at their maximum values and we're done.
            self._seq = None

        return result

    def __reduce__(self):
        _unimplemented()

    def __setstate__(self):
        _unimplemented()

    def __sizeof__(self):
        _unimplemented()


class compress:
    def __iter__(self):
        return self

    def __new__(cls, data, selectors):
        result = object.__new__(cls)
        result._data = iter(data)
        result._selectors = iter(selectors)
        return result

    def __next__(self):
        data = self._data
        selectors = self._selectors

        while True:
            datum = next(data)
            selector = next(selectors)
            if selector:
                return datum

    def __reduce__(self):
        _unimplemented()


class count:
    def __iter__(self):
        return self

    def __new__(cls, start=0, step=1):
        if not _number_check(start):
            raise TypeError("a number is required")

        result = object.__new__(cls)
        result.count = start
        result.step = step
        return result

    def __next__(self):
        result = self.count
        self.count += self.step
        return result

    def __reduce__(self):
        _unimplemented()

    def __repr__(self):
        return f"count({self.count})"


class cycle:
    def __iter__(self):
        return self

    def __new__(cls, seq):
        result = object.__new__(cls)
        result._seq = iter(seq)
        result._saved = []
        result._first_pass = True
        return result

    def __next__(self):
        try:
            result = next(self._seq)
            if self._first_pass:
                self._saved.append(result)
            return result
        except StopIteration:
            self._first_pass = False
            self._seq = iter(self._saved)
            return next(self._seq)

    def __reduce__(self):
        _unimplemented()

    def __setstate__(self):
        _unimplemented()


class dropwhile:
    def __iter__(self):
        return self

    def __new__(cls, predicate, iterable):
        result = object.__new__(cls)
        result._it = iter(iterable)
        result._func = predicate
        result._start = False
        return result

    def __next__(self):
        if self._start:
            return next(self._it)

        func = self._func

        while True:
            item = next(self._it)
            if not func(item):
                self._start = True
                return item

    def __reduce__(self):
        _unimplemented()

    def __setstate__(self):
        _unimplemented()


class filterfalse:
    def __iter__(self):
        return self

    def __new__(cls, predicate, iterable):
        result = object.__new__(cls)
        result._it = iter(iterable)
        result._predicate = bool if predicate is None else predicate
        return result

    def __next__(self):
        while True:
            item = next(self._it)
            if not self._predicate(item):
                return item

    def __reduce__(self):
        _unimplemented()


# internal helper class for groupby
class _groupby_iterator:
    def __iter__(self):
        return self

    def __new__(cls, parent, cur):

        obj = object.__new__(cls)
        obj._parent = parent
        obj._currkey = cur
        return obj

    def __next__(self):
        parent = self._parent
        if parent._currkey == self._currkey:
            val = parent._currval
            try:
                parent._currval = next(parent._it)
                parent._currkey = (
                    parent._currval
                    if parent._keyfunc is None
                    else parent._keyfunc(parent._currval)
                )
            except StopIteration:
                parent._currkey = _Unbound
            return val
        raise StopIteration


class groupby:
    def __iter__(self):
        return self

    def __new__(cls, iterable, key=None):

        obj = object.__new__(cls)
        obj._it = iter(iterable)
        obj._tgtkey = obj._currkey = obj._currval = _Unbound
        obj._keyfunc = key
        return obj

    def __next__(self):
        # In middle of previous iterator
        while self._currkey == self._tgtkey:
            self._currval = next(self._it)
            self._currkey = (
                self._currval if self._keyfunc is None else self._keyfunc(self._currval)
            )
        if self._currkey is _Unbound:
            raise StopIteration
        # remember group of returned iterator
        self._tgtkey = self._currkey
        return self._currkey, _groupby_iterator(self, self._currkey)


class islice:
    def __new__(cls, seq, stop_or_start, stop=_Unbound, step=_Unbound):
        result = object.__new__(cls)
        result._it = iter(seq)
        result._count = 0
        if stop is _Unbound:
            start = 0
            stop = stop_or_start
            step = 1
        else:
            start = 0 if stop_or_start is None else stop_or_start
            if step is _Unbound or step is None:
                step = 1
            elif not _int_check(step) or step < 1:
                raise ValueError(
                    "Step for islice() must be a positive integer or None."
                )
        if stop is None:
            stop = -1
        elif not _int_check(stop) or stop == -1:
            raise ValueError(
                "Stop argument for islice() must be None or an "
                "integer: 0 <= x <= sys.maxsize."
            )
        if not _int_check(start) or start < 0 or stop < -1:
            raise ValueError(
                "Indices for islice() must be None or an integer: "
                "0 <= x <= sys.maxsize."
            )
        result._next = start
        result._stop = stop
        result._step = step
        return result

    def __iter__(self):
        return self

    def __next__(self):
        it = self._it
        if it is None:
            raise StopIteration
        count = self._count
        new_next = self._next
        while count < new_next:
            try:
                next(it)
            except Exception as exc:
                self._it = None
                raise exc
            count += 1
        stop = self._stop
        if count >= stop and stop != -1:
            self._it = None
            raise StopIteration
        try:
            item = next(it)
        except Exception as exc:
            self._it = None
            raise exc
        self._count = count + 1
        new_next += self._step
        if new_next > stop and stop != -1:
            new_next = stop
        self._next = new_next
        return item

    def __reduce__(self):
        _unimplemented()

    def __setstate__(self):
        _unimplemented()


class permutations:
    def __iter__(self):
        return self

    def __new__(cls, iterable, r=None):
        seq = tuple(iterable)
        n = _tuple_len(seq)

        result = object.__new__(cls)

        if r is None:
            r = n
        elif r > n:
            result._seq = None
            return result

        result._seq = seq
        result._r = r
        result._indices = list(range(n))
        result._cycles = list(range(n, n - r, -1))
        return result

    def __next__(self):
        seq = self._seq
        if seq is None:
            raise StopIteration
        r = self._r
        indices = self._indices
        indices_len = _list_len(indices)
        result = (*(seq[indices[i]] for i in range(r)),)
        cycles = self._cycles
        i = r - 1
        while i >= 0:
            j = cycles[i] - 1
            if j > 0:
                cycles[i] = j
                indices[i], indices[-j] = indices[-j], indices[i]
                break
            cycles[i] = indices_len - i
            tmp = indices[i]
            k = i + 1
            while k < indices_len:
                indices[k - 1] = indices[k]
                k += 1
            indices[k - 1] = tmp
            i -= 1
        else:
            self._seq = None
        return result

    def __reduce__(self):
        _unimplemented()

    def __setstate__(self):
        _unimplemented()

    def __sizeof__(self):
        _unimplemented()


class product:
    def __iter__(self):
        return self

    def __new__(cls, *iterables, repeat=1):
        if not _int_check(repeat):
            raise TypeError
        length = _tuple_len(iterables) if repeat else 0
        i = 0
        repeated = _list_new(length)
        result = object.__new__(cls)
        while i < length:
            item = tuple(iterables[i])
            if not item:
                result._iterables = None
                return result
            repeated[i] = item
            i += 1
        repeated *= repeat
        result._iterables = repeated
        result._digits = _list_new(length * repeat, 0)
        return result

    def __next__(self):
        iterables = self._iterables
        if iterables is None:
            raise StopIteration
        digits = self._digits
        length = _list_len(iterables)
        result = _list_new(length)
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
            self._iterables = None
        return tuple(result)

    def __reduce__(self):
        _unimplemented()

    def __setstate__(self):
        _unimplemented()

    def __sizeof__(self):
        _unimplemented()


class repeat:
    def __iter__(self):
        return self

    def __new__(cls, elem, times=None):
        result = object.__new__(cls)
        result._elem = elem
        if times is not None:
            _int_guard(times)
        result._times = times
        return result

    def __next__(self):
        if self._times is None:
            return self._elem
        if self._times > 0:
            self._times -= 1
            return self._elem
        raise StopIteration

    def __length_hint__(self):
        _unimplemented()

    def __reduce__(self):
        _unimplemented()

    def __repr__(self):
        _unimplemented()


class starmap:
    def __iter__(self):
        return self

    def __new__(cls, function, iterable):
        result = object.__new__(cls)
        result._it = iter(iterable)
        result._func = function
        return result

    def __next__(self):
        args = next(self._it)
        return self._func(*args)

    def __reduce__(self):
        _unimplemented()


def tee(iterable, n=2):
    _int_guard(n)
    if n < 0:
        raise ValueError("n must be >= 0")
    if n == 0:
        return ()

    it = iter(iterable)
    copyable = it if hasattr(it, "__copy__") else _tee.from_iterable(it)
    copyfunc = copyable.__copy__
    return tuple(copyable if i == 0 else copyfunc() for i in range(n))


# Internal cache for tee, a linked list where each link is a cached window to
# a section of the source iterator
class _tee_dataobject:
    # CPython sets this at 57 to align exactly with cache line size. We choose
    # 55 to align with cache lines in our system: Arrays <=255 elements have 1
    # word of header. The header and each data element is 8 bytes on a 64-bit
    # machine.  Cache lines are 64-bytes on all x86 machines though they tend to
    # be fetched in pairs, so any multiple of 8 minus 1 up to 255 is fine.
    _MAX_VALUES = 55

    def __init__(self, it):
        self._num_read = 0
        self._next_link = _Unbound
        self._it = it
        self._values = []

    def get_item(self, i):
        assert i < self.__class__._MAX_VALUES

        if i < self._num_read:
            return self._values[i]
        else:
            assert i == self._num_read
            value = next(self._it)
            self._num_read += 1
            # mutable tuple might be a nice future optimization here
            self._values.append(value)
            return value

    def next_link(self):
        if self._next_link is _Unbound:
            self._next_link = self.__class__(self._it)
        return self._next_link


class _tee:
    def __copy__(self):
        return self.__class__(self._data, self._index)

    def __init__(self, data, index):
        self._data = data
        self._index = index

    def __iter__(self):
        return self

    def __next__(self):
        if self._index >= _tee_dataobject._MAX_VALUES:
            self._data = self._data.next_link()
            self._index = 0

        value = self._data.get_item(self._index)
        self._index += 1
        return value

    def __reduce__(self):
        _unimplemented()

    def __setstate__(self):
        _unimplemented()

    @classmethod
    def from_iterable(cls, iterable):
        it = iter(iterable)

        if isinstance(it, _tee):
            return it.__copy__()
        else:
            return cls(_tee_dataobject(it), 0)


class takewhile:
    def __iter__(self):
        return self

    def __new__(cls, predicate, iterable):
        result = object.__new__(cls)
        result._it = iter(iterable)
        result._func = predicate
        result._stop = False
        return result

    def __next__(self):
        if self._stop:
            raise StopIteration

        item = next(self._it)
        if self._func(item):
            return item

        self._stop = True
        raise StopIteration

    def __reduce__(self):
        _unimplemented()

    def __setstate__(self):
        _unimplemented()


class zip_longest:
    def __iter__(self):
        return self

    def __new__(cls, *seqs, fillvalue=None):
        length = _tuple_len(seqs)
        result = object.__new__(cls)
        result._iters = [iter(seq) for seq in seqs]
        result._num_iters = length
        result._num_active = length
        result._fillvalue = fillvalue
        return result

    def __next__(self):
        iters = self._iters
        if not self._num_active:
            raise StopIteration
        fillvalue = self._fillvalue
        values = _list_new(self._num_iters, fillvalue)
        for i, it in enumerate(iters):
            try:
                values[i] = next(it)
            except StopIteration:
                self._num_active -= 1
                if not self._num_active:
                    raise
                self._iters[i] = repeat(fillvalue)
        return tuple(values)

    def __reduce__(self):
        _unimplemented()

    def __setstate__(self):
        _unimplemented()
