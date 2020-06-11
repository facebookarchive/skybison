#!/usr/bin/env python3

from builtins import _index

from _builtins import (
    _int_check,
    _int_guard,
    _repr_enter,
    _repr_leave,
    _type,
    _unimplemented,
)


_DEQUE_NUM_ELTS = 30
_DEQUE_LEFT_LINK = _DEQUE_NUM_ELTS
_DEQUE_RIGHT_LINK = _DEQUE_NUM_ELTS + 1
_DEQUE_BLOCK_SIZE = _DEQUE_NUM_ELTS + 2


def _deque_guard(self):
    if not isinstance(self, deque):
        raise TypeError(
            "requires a 'collections.deque' object but received a "
            f"{_type(self).__name__!r}"
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


class deque:
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
        _deque_guard(self)
        block, index = deque.__getref(self, index)
        return block[index]

    def __getref(self, index):
        if not _int_check(index):
            index = _index(index)
        if index >= 0:
            block = self.left
            while block:
                l, r = 0, _DEQUE_NUM_ELTS
                if block is self.left:
                    l = self.leftndx  # noqa: E741
                if block is self.right:
                    r = self.rightndx + 1
                span = r - l
                if index < span:
                    return block, l + index
                index -= span
                block = block[_DEQUE_RIGHT_LINK]
        else:
            block = self.right
            while block:
                l, r = 0, _DEQUE_NUM_ELTS
                if block is self.left:
                    l = self.leftndx  # noqa: E741
                if block is self.right:
                    r = self.rightndx + 1
                negative_span = l - r
                if index >= negative_span:
                    return block, r + index
                index -= negative_span
                block = block[_DEQUE_LEFT_LINK]
        raise IndexError("deque index out of range")

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
        if maxlen is not None:
            _int_guard(maxlen)
            if maxlen < 0:
                raise ValueError("maxlen must be non-negative")
        self._maxlen = maxlen
        append = deque.append
        for elem in iterable:
            append(self, elem)

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
        _deque_guard(self)
        return self.length

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
        self = super(deque, cls).__new__(cls)
        deque.clear(self)
        return self

    def __reduce__(self):
        _unimplemented()

    def __reduce_ex__(self, proto):
        _unimplemented()

    def __repr__(self):
        _deque_guard(self)
        if _repr_enter(self):
            return "[...]"
        if self._maxlen is not None:
            result = f"deque({list(self)!r}, maxlen={self._maxlen})"
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
        if self.state != original_state:
            giveup()
        block = self.left
        while block:
            l, r = 0, _DEQUE_NUM_ELTS
            if block is self.left:
                l = self.leftndx  # noqa: E741
            if block is self.right:
                r = self.rightndx + 1
            for elem in block[l:r]:
                yield elem
                if self.state != original_state:
                    giveup()
            block = block[_DEQUE_RIGHT_LINK]

    def _reversed_impl(self, original_state, giveup):
        if self.state != original_state:
            giveup()
        block = self.right
        while block:
            l, r = 0, _DEQUE_NUM_ELTS
            if block is self.left:
                l = self.leftndx  # noqa: E741
            if block is self.right:
                r = self.rightndx + 1
            for elem in reversed(block[l:r]):
                yield elem
                if self.state != original_state:
                    giveup()
            block = block[_DEQUE_LEFT_LINK]

    def append(self, x):
        _deque_guard(self)
        self.state += 1
        self.rightndx += 1
        if self.rightndx == _DEQUE_NUM_ELTS:
            newblock = [None] * _DEQUE_BLOCK_SIZE
            self.right[_DEQUE_RIGHT_LINK] = newblock
            newblock[_DEQUE_LEFT_LINK] = self.right
            self.right = newblock
            self.rightndx = 0
        self.length += 1
        self.right[self.rightndx] = x
        if self._maxlen is not None and self.length > self._maxlen:
            deque.popleft(self)

    def appendleft(self, x):
        _deque_guard(self)
        self.state += 1
        self.leftndx -= 1
        if self.leftndx == -1:
            newblock = [None] * _DEQUE_BLOCK_SIZE
            self.left[_DEQUE_LEFT_LINK] = newblock
            newblock[_DEQUE_RIGHT_LINK] = self.left
            self.left = newblock
            self.leftndx = _DEQUE_NUM_ELTS - 1
        self.length += 1
        self.left[self.leftndx] = x
        if self._maxlen is not None and self.length > self._maxlen:
            deque.pop(self)

    def clear(self):
        _deque_guard(self)
        self.right = self.left = [None] * _DEQUE_BLOCK_SIZE
        self.rightndx = _DEQUE_NUM_ELTS // 2  # points to last written element
        self.leftndx = _DEQUE_NUM_ELTS // 2 + 1
        self.length = 0
        self.state = 0

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

    @property
    def maxlen(self):
        _deque_guard(self)
        return self._maxlen

    def pop(self):
        _deque_guard(self)
        if self.left is self.right and self.leftndx > self.rightndx:
            raise IndexError("pop from an empty deque")
        x = self.right[self.rightndx]
        self.right[self.rightndx] = None
        self.length -= 1
        self.rightndx -= 1
        self.state += 1
        if self.rightndx == -1:
            prevblock = self.right[_DEQUE_LEFT_LINK]
            if prevblock is None:
                # the deque has become empty; recenter instead of freeing block
                self.rightndx = _DEQUE_NUM_ELTS // 2
                self.leftndx = _DEQUE_NUM_ELTS // 2 + 1
            else:
                prevblock[_DEQUE_RIGHT_LINK] = None
                self.right[_DEQUE_LEFT_LINK] = None
                self.right = prevblock
                self.rightndx = _DEQUE_NUM_ELTS - 1
        return x

    def popleft(self):
        _deque_guard(self)
        if self.left is self.right and self.leftndx > self.rightndx:
            raise IndexError("pop from an empty deque")
        x = self.left[self.leftndx]
        self.left[self.leftndx] = None
        self.length -= 1
        self.leftndx += 1
        self.state += 1
        if self.leftndx == _DEQUE_NUM_ELTS:
            prevblock = self.left[_DEQUE_RIGHT_LINK]
            if prevblock is None:
                # the deque has become empty; recenter instead of freeing block
                self.rightndx = _DEQUE_NUM_ELTS // 2
                self.leftndx = _DEQUE_NUM_ELTS // 2 + 1
            else:
                prevblock[_DEQUE_LEFT_LINK] = None
                self.left[_DEQUE_RIGHT_LINK] = None
                self.left = prevblock
                self.leftndx = 0
        return x

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
        _deque_guard(self)
        leftblock = self.left
        rightblock = self.right
        leftindex = self.leftndx
        rightindex = self.rightndx
        for _ in range(self.length // 2):
            # Validate that pointers haven't met in the middle
            assert leftblock != rightblock or leftindex < rightindex

            # Swap
            (rightblock[rightindex], leftblock[leftindex]) = (
                leftblock[leftindex],
                rightblock[rightindex],
            )

            # Advance left block/index pair
            leftindex += 1
            if leftindex == _DEQUE_NUM_ELTS:
                leftblock = leftblock[_DEQUE_RIGHT_LINK]
                assert leftblock is not None
                leftindex = 0

            # Step backwards with the right block/index pair
            rightindex -= 1
            if rightindex == -1:
                rightblock = rightblock[_DEQUE_LEFT_LINK]
                assert rightblock is not None
                rightindex = _DEQUE_NUM_ELTS - 1

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
