#!/usr/bin/env python3
# flake8: noqa
# fmt: off
'''This module implements specialized container datatypes providing
alternatives to Python's general purpose built-in containers, dict,
list, set, and tuple.

* namedtuple   factory function for creating tuple subclasses with named fields
* deque        list-like container with fast appends and pops on either end
* ChainMap     dict-like class for creating a single view of multiple mappings
* Counter      dict subclass for counting hashable objects
* OrderedDict  dict subclass that remembers the order entries were added
* defaultdict  dict subclass that calls a factory function to supply missing values
* UserDict     wrapper around dictionary objects for easier dict subclassing
* UserList     wrapper around list objects for easier list subclassing
* UserString   wrapper around string objects for easier string subclassing

isort:skip_file
'''

__all__ = ['deque', 'defaultdict', 'namedtuple', 'UserDict', 'UserList',
            'UserString', 'Counter', 'OrderedDict', 'ChainMap']

# For backwards compatibility, continue to make the collections ABCs
# available through the collections module.
from _collections_abc import *
import _collections_abc
__all__ += _collections_abc.__all__

from operator import itemgetter as _itemgetter, eq as _eq
from keyword import iskeyword as _iskeyword
import sys as _sys
import heapq as _heapq
from _weakref import proxy as _proxy
from itertools import repeat as _repeat, chain as _chain, starmap as _starmap
from reprlib import recursive_repr as _recursive_repr
from _namedtuple import namedtuple

try:
    from _collections import deque
except ImportError:
    pass
else:
    MutableSequence.register(deque)


################################################################################
### defaultdict
################################################################################

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


################################################################################
### deque
################################################################################

_DEQUE_NUM_ELTS = 30
_DEQUE_LEFT_LINK = _DEQUE_NUM_ELTS
_DEQUE_RIGHT_LINK = _DEQUE_NUM_ELTS + 1
_DEQUE_BLOCK_SIZE = _DEQUE_NUM_ELTS + 2

def _deque_guard(self):
    if not isinstance(self, deque):
        raise TypeError(f"'__copy__' requires a 'collections.deque' object but received a {_type(self).__name__!r}")

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
                    l = self.leftndx
                if block is self.right:
                    r = self.rightndx + 1
                span = r-l
                if index < span:
                    return block, l+index
                index -= span
                block = block[_DEQUE_RIGHT_LINK]
        else:
            block = self.right
            while block:
                l, r = 0, _DEQUE_NUM_ELTS
                if block is self.left:
                    l = self.leftndx
                if block is self.right:
                    r = self.rightndx + 1
                negative_span = l-r
                if index >= negative_span:
                    return block, r+index
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
        return deque_iterator(self, self._iter_impl)

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
            return '[...]'
        if self._maxlen is not None:
            result = f"deque({list(self)!r}, maxlen={self._maxlen})"
        else:
            result = f"deque({list(self)!r})"
        _repr_leave(self)
        return result

    def __reversed__(self):
        _deque_guard(self)
        return deque_iterator(self, self._reversed_impl)

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
                l = self.leftndx
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
                l = self.leftndx
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
            self.leftndx = _DEQUE_NUM_ELTS-1
        self.length += 1
        self.left[self.leftndx] = x
        if self._maxlen is not None and self.length > self._maxlen:
            deque.pop(self)

    def clear(self):
        _deque_guard(self)
        self.right = self.left = [None] * _DEQUE_BLOCK_SIZE
        self.rightndx = _DEQUE_NUM_ELTS//2   # points to last written element
        self.leftndx = _DEQUE_NUM_ELTS//2+1
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
                self.rightndx = _DEQUE_NUM_ELTS//2
                self.leftndx = _DEQUE_NUM_ELTS//2+1
            else:
                prevblock[_DEQUE_RIGHT_LINK] = None
                self.right[_DEQUE_LEFT_LINK] = None
                self.right = prevblock
                self.rightndx = _DEQUE_NUM_ELTS-1
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
                self.rightndx = _DEQUE_NUM_ELTS//2
                self.leftndx = _DEQUE_NUM_ELTS//2+1
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
            for i in range(len(self)):
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
        for i in range(self.length // 2):
            # Validate that pointers haven't met in the middle
            assert leftblock != rightblock or leftindex < rightindex

            # Swap
            (rightblock[rightindex], leftblock[leftindex]) = (
                leftblock[leftindex], rightblock[rightindex])

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



class deque_iterator:
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


################################################################################
### OrderedDict
################################################################################

class _OrderedDictKeysView(KeysView):

    def __reversed__(self):
        yield from reversed(self._mapping)

class _OrderedDictItemsView(ItemsView):

    def __reversed__(self):
        for key in reversed(self._mapping):
            yield (key, self._mapping[key])

class _OrderedDictValuesView(ValuesView):

    def __reversed__(self):
        for key in reversed(self._mapping):
            yield self._mapping[key]

class _Link(object):
    __slots__ = 'prev', 'next', 'key', '__weakref__'

class OrderedDict(dict):
    'Dictionary that remembers insertion order'
    # An inherited dict maps keys to values.
    # The inherited dict provides __getitem__, __len__, __contains__, and get.
    # The remaining methods are order-aware.
    # Big-O running times for all methods are the same as regular dictionaries.

    # The internal self.__map dict maps keys to links in a doubly linked list.
    # The circular doubly linked list starts and ends with a sentinel element.
    # The sentinel element never gets deleted (this simplifies the algorithm).
    # The sentinel is in self.__hardroot with a weakref proxy in self.__root.
    # The prev links are weakref proxies (to prevent circular references).
    # Individual links are kept alive by the hard reference in self.__map.
    # Those hard references disappear when a key is deleted from an OrderedDict.

    def __init__(*args, **kwds):
        '''Initialize an ordered dictionary.  The signature is the same as
        regular dictionaries.  Keyword argument order is preserved.
        '''
        if not args:
            raise TypeError("descriptor '__init__' of 'OrderedDict' object "
                            "needs an argument")
        self, *args = args
        if len(args) > 1:
            raise TypeError('expected at most 1 arguments, got %d' % len(args))
        try:
            self.__root
        except AttributeError:
            self.__hardroot = _Link()
            self.__root = root = _proxy(self.__hardroot)
            root.prev = root.next = root
            self.__map = {}
        self.__update(*args, **kwds)

    def __setitem__(self, key, value,
                    dict_setitem=dict.__setitem__, proxy=_proxy, Link=_Link):
        'od.__setitem__(i, y) <==> od[i]=y'
        # Setting a new item creates a new link at the end of the linked list,
        # and the inherited dictionary is updated with the new key/value pair.
        if key not in self:
            self.__map[key] = link = Link()
            root = self.__root
            last = root.prev
            link.prev, link.next, link.key = last, root, key
            last.next = link
            root.prev = proxy(link)
        dict_setitem(self, key, value)

    def __delitem__(self, key, dict_delitem=dict.__delitem__):
        'od.__delitem__(y) <==> del od[y]'
        # Deleting an existing item uses self.__map to find the link which gets
        # removed by updating the links in the predecessor and successor nodes.
        dict_delitem(self, key)
        link = self.__map.pop(key)
        link_prev = link.prev
        link_next = link.next
        link_prev.next = link_next
        link_next.prev = link_prev
        link.prev = None
        link.next = None

    def __iter__(self):
        'od.__iter__() <==> iter(od)'
        # Traverse the linked list in order.
        root = self.__root
        curr = root.next
        while curr is not root:
            yield curr.key
            curr = curr.next

    def __reversed__(self):
        'od.__reversed__() <==> reversed(od)'
        # Traverse the linked list in reverse order.
        root = self.__root
        curr = root.prev
        while curr is not root:
            yield curr.key
            curr = curr.prev

    def clear(self):
        'od.clear() -> None.  Remove all items from od.'
        root = self.__root
        root.prev = root.next = root
        self.__map.clear()
        dict.clear(self)

    def popitem(self, last=True):
        '''Remove and return a (key, value) pair from the dictionary.

        Pairs are returned in LIFO order if last is true or FIFO order if false.
        '''
        if not self:
            raise KeyError('dictionary is empty')
        root = self.__root
        if last:
            link = root.prev
            link_prev = link.prev
            link_prev.next = root
            root.prev = link_prev
        else:
            link = root.next
            link_next = link.next
            root.next = link_next
            link_next.prev = root
        key = link.key
        del self.__map[key]
        value = dict.pop(self, key)
        return key, value

    def move_to_end(self, key, last=True):
        '''Move an existing element to the end (or beginning if last==False).

        Raises KeyError if the element does not exist.
        When last=True, acts like a fast version of self[key]=self.pop(key).

        '''
        link = self.__map[key]
        link_prev = link.prev
        link_next = link.next
        soft_link = link_next.prev
        link_prev.next = link_next
        link_next.prev = link_prev
        root = self.__root
        if last:
            last = root.prev
            link.prev = last
            link.next = root
            root.prev = soft_link
            last.next = link
        else:
            first = root.next
            link.prev = root
            link.next = first
            first.prev = soft_link
            root.next = link

    def __sizeof__(self):
        sizeof = _sys.getsizeof
        n = len(self) + 1                       # number of links including root
        size = sizeof(self.__dict__)            # instance dictionary
        size += sizeof(self.__map) * 2          # internal dict and inherited dict
        size += sizeof(self.__hardroot) * n     # link objects
        size += sizeof(self.__root) * n         # proxy objects
        return size

    update = __update = MutableMapping.update

    def keys(self):
        "D.keys() -> a set-like object providing a view on D's keys"
        return _OrderedDictKeysView(self)

    def items(self):
        "D.items() -> a set-like object providing a view on D's items"
        return _OrderedDictItemsView(self)

    def values(self):
        "D.values() -> an object providing a view on D's values"
        return _OrderedDictValuesView(self)

    __ne__ = MutableMapping.__ne__

    __marker = object()

    def pop(self, key, default=__marker):
        '''od.pop(k[,d]) -> v, remove specified key and return the corresponding
        value.  If key is not found, d is returned if given, otherwise KeyError
        is raised.

        '''
        if key in self:
            result = self[key]
            del self[key]
            return result
        if default is self.__marker:
            raise KeyError(key)
        return default

    def setdefault(self, key, default=None):
        'od.setdefault(k[,d]) -> od.get(k,d), also set od[k]=d if k not in od'
        if key in self:
            return self[key]
        self[key] = default
        return default

    @_recursive_repr()
    def __repr__(self):
        'od.__repr__() <==> repr(od)'
        if not self:
            return '%s()' % (self.__class__.__name__,)
        return '%s(%r)' % (self.__class__.__name__, list(self.items()))

    def __reduce__(self):
        'Return state information for pickling'
        inst_dict = vars(self).copy()
        for k in vars(OrderedDict()):
            inst_dict.pop(k, None)
        return self.__class__, (), inst_dict or None, None, iter(self.items())

    def copy(self):
        'od.copy() -> a shallow copy of od'
        return self.__class__(self)

    @classmethod
    def fromkeys(cls, iterable, value=None):
        '''OD.fromkeys(S[, v]) -> New ordered dictionary with keys from S.
        If not specified, the value defaults to None.

        '''
        self = cls()
        for key in iterable:
            self[key] = value
        return self

    def __eq__(self, other):
        '''od.__eq__(y) <==> od==y.  Comparison to another OD is order-sensitive
        while comparison to a regular mapping is order-insensitive.

        '''
        if isinstance(other, OrderedDict):
            return dict.__eq__(self, other) and all(map(_eq, self, other))
        return dict.__eq__(self, other)


try:
    from _collections import OrderedDict
except ImportError:
    # Leave the pure Python version in place.
    pass


########################################################################
###  Counter
########################################################################

def _count_elements(mapping, iterable):
    'Tally elements from the iterable.'
    mapping_get = mapping.get
    for elem in iterable:
        mapping[elem] = mapping_get(elem, 0) + 1

try:                                    # Load C helper function if available
    from _collections import _count_elements
except ImportError:
    pass

class Counter(dict):
    '''Dict subclass for counting hashable items.  Sometimes called a bag
    or multiset.  Elements are stored as dictionary keys and their counts
    are stored as dictionary values.

    >>> c = Counter('abcdeabcdabcaba')  # count elements from a string

    >>> c.most_common(3)                # three most common elements
    [('a', 5), ('b', 4), ('c', 3)]
    >>> sorted(c)                       # list all unique elements
    ['a', 'b', 'c', 'd', 'e']
    >>> ''.join(sorted(c.elements()))   # list elements with repetitions
    'aaaaabbbbcccdde'
    >>> sum(c.values())                 # total of all counts
    15

    >>> c['a']                          # count of letter 'a'
    5
    >>> for elem in 'shazam':           # update counts from an iterable
    ...     c[elem] += 1                # by adding 1 to each element's count
    >>> c['a']                          # now there are seven 'a'
    7
    >>> del c['b']                      # remove all 'b'
    >>> c['b']                          # now there are zero 'b'
    0

    >>> d = Counter('simsalabim')       # make another counter
    >>> c.update(d)                     # add in the second counter
    >>> c['a']                          # now there are nine 'a'
    9

    >>> c.clear()                       # empty the counter
    >>> c
    Counter()

    Note:  If a count is set to zero or reduced to zero, it will remain
    in the counter until the entry is deleted or the counter is cleared:

    >>> c = Counter('aaabbc')
    >>> c['b'] -= 2                     # reduce the count of 'b' by two
    >>> c.most_common()                 # 'b' is still in, but its count is zero
    [('a', 3), ('c', 1), ('b', 0)]

    '''
    # References:
    #   http://en.wikipedia.org/wiki/Multiset
    #   http://www.gnu.org/software/smalltalk/manual-base/html_node/Bag.html
    #   http://www.demo2s.com/Tutorial/Cpp/0380__set-multiset/Catalog0380__set-multiset.htm
    #   http://code.activestate.com/recipes/259174/
    #   Knuth, TAOCP Vol. II section 4.6.3

    def __init__(*args, **kwds):
        '''Create a new, empty Counter object.  And if given, count elements
        from an input iterable.  Or, initialize the count from another mapping
        of elements to their counts.

        >>> c = Counter()                           # a new, empty counter
        >>> c = Counter('gallahad')                 # a new counter from an iterable
        >>> c = Counter({'a': 4, 'b': 2})           # a new counter from a mapping
        >>> c = Counter(a=4, b=2)                   # a new counter from keyword args

        '''
        if not args:
            raise TypeError("descriptor '__init__' of 'Counter' object "
                            "needs an argument")
        self, *args = args
        if len(args) > 1:
            raise TypeError('expected at most 1 arguments, got %d' % len(args))
        super(Counter, self).__init__()
        self.update(*args, **kwds)

    def __missing__(self, key):
        'The count of elements not in the Counter is zero.'
        # Needed so that self[missing_item] does not raise KeyError
        return 0

    def most_common(self, n=None):
        '''List the n most common elements and their counts from the most
        common to the least.  If n is None, then list all element counts.

        >>> Counter('abcdeabcdabcaba').most_common(3)
        [('a', 5), ('b', 4), ('c', 3)]

        '''
        # Emulate Bag.sortedByCount from Smalltalk
        if n is None:
            return sorted(self.items(), key=_itemgetter(1), reverse=True)
        return _heapq.nlargest(n, self.items(), key=_itemgetter(1))

    def elements(self):
        '''Iterator over elements repeating each as many times as its count.

        >>> c = Counter('ABCABC')
        >>> sorted(c.elements())
        ['A', 'A', 'B', 'B', 'C', 'C']

        # Knuth's example for prime factors of 1836:  2**2 * 3**3 * 17**1
        >>> prime_factors = Counter({2: 2, 3: 3, 17: 1})
        >>> product = 1
        >>> for factor in prime_factors.elements():     # loop over factors
        ...     product *= factor                       # and multiply them
        >>> product
        1836

        Note, if an element's count has been set to zero or is a negative
        number, elements() will ignore it.

        '''
        # Emulate Bag.do from Smalltalk and Multiset.begin from C++.
        return _chain.from_iterable(_starmap(_repeat, self.items()))

    # Override dict methods where necessary

    @classmethod
    def fromkeys(cls, iterable, v=None):
        # There is no equivalent method for counters because setting v=1
        # means that no element can have a count greater than one.
        raise NotImplementedError(
            'Counter.fromkeys() is undefined.  Use Counter(iterable) instead.')

    def update(*args, **kwds):
        '''Like dict.update() but add counts instead of replacing them.

        Source can be an iterable, a dictionary, or another Counter instance.

        >>> c = Counter('which')
        >>> c.update('witch')           # add elements from another iterable
        >>> d = Counter('watch')
        >>> c.update(d)                 # add elements from another counter
        >>> c['h']                      # four 'h' in which, witch, and watch
        4

        '''
        # The regular dict.update() operation makes no sense here because the
        # replace behavior results in the some of original untouched counts
        # being mixed-in with all of the other counts for a mismash that
        # doesn't have a straight-forward interpretation in most counting
        # contexts.  Instead, we implement straight-addition.  Both the inputs
        # and outputs are allowed to contain zero and negative counts.

        if not args:
            raise TypeError("descriptor 'update' of 'Counter' object "
                            "needs an argument")
        self, *args = args
        if len(args) > 1:
            raise TypeError('expected at most 1 arguments, got %d' % len(args))
        iterable = args[0] if args else None
        if iterable is not None:
            if isinstance(iterable, Mapping):
                if self:
                    self_get = self.get
                    for elem, count in iterable.items():
                        self[elem] = count + self_get(elem, 0)
                else:
                    super(Counter, self).update(iterable) # fast path when counter is empty
            else:
                _count_elements(self, iterable)
        if kwds:
            self.update(kwds)

    def subtract(*args, **kwds):
        '''Like dict.update() but subtracts counts instead of replacing them.
        Counts can be reduced below zero.  Both the inputs and outputs are
        allowed to contain zero and negative counts.

        Source can be an iterable, a dictionary, or another Counter instance.

        >>> c = Counter('which')
        >>> c.subtract('witch')             # subtract elements from another iterable
        >>> c.subtract(Counter('watch'))    # subtract elements from another counter
        >>> c['h']                          # 2 in which, minus 1 in witch, minus 1 in watch
        0
        >>> c['w']                          # 1 in which, minus 1 in witch, minus 1 in watch
        -1

        '''
        if not args:
            raise TypeError("descriptor 'subtract' of 'Counter' object "
                            "needs an argument")
        self, *args = args
        if len(args) > 1:
            raise TypeError('expected at most 1 arguments, got %d' % len(args))
        iterable = args[0] if args else None
        if iterable is not None:
            self_get = self.get
            if isinstance(iterable, Mapping):
                for elem, count in iterable.items():
                    self[elem] = self_get(elem, 0) - count
            else:
                for elem in iterable:
                    self[elem] = self_get(elem, 0) - 1
        if kwds:
            self.subtract(kwds)

    def copy(self):
        'Return a shallow copy.'
        return self.__class__(self)

    def __reduce__(self):
        return self.__class__, (dict(self),)

    def __delitem__(self, elem):
        'Like dict.__delitem__() but does not raise KeyError for missing values.'
        if elem in self:
            super().__delitem__(elem)

    def __repr__(self):
        if not self:
            return '%s()' % self.__class__.__name__
        try:
            items = ', '.join(map('%r: %r'.__mod__, self.most_common()))
            return '%s({%s})' % (self.__class__.__name__, items)
        except TypeError:
            # handle case where values are not orderable
            return '{0}({1!r})'.format(self.__class__.__name__, dict(self))

    # Multiset-style mathematical operations discussed in:
    #       Knuth TAOCP Volume II section 4.6.3 exercise 19
    #       and at http://en.wikipedia.org/wiki/Multiset
    #
    # Outputs guaranteed to only include positive counts.
    #
    # To strip negative and zero counts, add-in an empty counter:
    #       c += Counter()

    def __add__(self, other):
        '''Add counts from two counters.

        >>> Counter('abbb') + Counter('bcc')
        Counter({'b': 4, 'c': 2, 'a': 1})

        '''
        if not isinstance(other, Counter):
            return NotImplemented
        result = Counter()
        for elem, count in self.items():
            newcount = count + other[elem]
            if newcount > 0:
                result[elem] = newcount
        for elem, count in other.items():
            if elem not in self and count > 0:
                result[elem] = count
        return result

    def __sub__(self, other):
        ''' Subtract count, but keep only results with positive counts.

        >>> Counter('abbbc') - Counter('bccd')
        Counter({'b': 2, 'a': 1})

        '''
        if not isinstance(other, Counter):
            return NotImplemented
        result = Counter()
        for elem, count in self.items():
            newcount = count - other[elem]
            if newcount > 0:
                result[elem] = newcount
        for elem, count in other.items():
            if elem not in self and count < 0:
                result[elem] = 0 - count
        return result

    def __or__(self, other):
        '''Union is the maximum of value in either of the input counters.

        >>> Counter('abbb') | Counter('bcc')
        Counter({'b': 3, 'c': 2, 'a': 1})

        '''
        if not isinstance(other, Counter):
            return NotImplemented
        result = Counter()
        for elem, count in self.items():
            other_count = other[elem]
            newcount = other_count if count < other_count else count
            if newcount > 0:
                result[elem] = newcount
        for elem, count in other.items():
            if elem not in self and count > 0:
                result[elem] = count
        return result

    def __and__(self, other):
        ''' Intersection is the minimum of corresponding counts.

        >>> Counter('abbb') & Counter('bcc')
        Counter({'b': 1})

        '''
        if not isinstance(other, Counter):
            return NotImplemented
        result = Counter()
        for elem, count in self.items():
            other_count = other[elem]
            newcount = count if count < other_count else other_count
            if newcount > 0:
                result[elem] = newcount
        return result

    def __pos__(self):
        'Adds an empty counter, effectively stripping negative and zero counts'
        result = Counter()
        for elem, count in self.items():
            if count > 0:
                result[elem] = count
        return result

    def __neg__(self):
        '''Subtracts from an empty counter.  Strips positive and zero counts,
        and flips the sign on negative counts.

        '''
        result = Counter()
        for elem, count in self.items():
            if count < 0:
                result[elem] = 0 - count
        return result

    def _keep_positive(self):
        '''Internal method to strip elements with a negative or zero count'''
        nonpositive = [elem for elem, count in self.items() if not count > 0]
        for elem in nonpositive:
            del self[elem]
        return self

    def __iadd__(self, other):
        '''Inplace add from another counter, keeping only positive counts.

        >>> c = Counter('abbb')
        >>> c += Counter('bcc')
        >>> c
        Counter({'b': 4, 'c': 2, 'a': 1})

        '''
        for elem, count in other.items():
            self[elem] += count
        return self._keep_positive()

    def __isub__(self, other):
        '''Inplace subtract counter, but keep only results with positive counts.

        >>> c = Counter('abbbc')
        >>> c -= Counter('bccd')
        >>> c
        Counter({'b': 2, 'a': 1})

        '''
        for elem, count in other.items():
            self[elem] -= count
        return self._keep_positive()

    def __ior__(self, other):
        '''Inplace union is the maximum of value from either counter.

        >>> c = Counter('abbb')
        >>> c |= Counter('bcc')
        >>> c
        Counter({'b': 3, 'c': 2, 'a': 1})

        '''
        for elem, other_count in other.items():
            count = self[elem]
            if other_count > count:
                self[elem] = other_count
        return self._keep_positive()

    def __iand__(self, other):
        '''Inplace intersection is the minimum of corresponding counts.

        >>> c = Counter('abbb')
        >>> c &= Counter('bcc')
        >>> c
        Counter({'b': 1})

        '''
        for elem, count in self.items():
            other_count = other[elem]
            if other_count < count:
                self[elem] = other_count
        return self._keep_positive()


########################################################################
###  ChainMap
########################################################################

class ChainMap(MutableMapping):
    ''' A ChainMap groups multiple dicts (or other mappings) together
    to create a single, updateable view.

    The underlying mappings are stored in a list.  That list is public and can
    be accessed or updated using the *maps* attribute.  There is no other
    state.

    Lookups search the underlying mappings successively until a key is found.
    In contrast, writes, updates, and deletions only operate on the first
    mapping.

    '''

    def __init__(self, *maps):
        '''Initialize a ChainMap by setting *maps* to the given mappings.
        If no mappings are provided, a single empty dictionary is used.

        '''
        self.maps = list(maps) or [{}]          # always at least one map

    def __missing__(self, key):
        raise KeyError(key)

    def __getitem__(self, key):
        for mapping in self.maps:
            try:
                return mapping[key]             # can't use 'key in mapping' with defaultdict
            except KeyError:
                pass
        return self.__missing__(key)            # support subclasses that define __missing__

    def get(self, key, default=None):
        return self[key] if key in self else default

    def __len__(self):
        return len(set().union(*self.maps))     # reuses stored hash values if possible

    def __iter__(self):
        return iter(set().union(*self.maps))

    def __contains__(self, key):
        return any(key in m for m in self.maps)

    def __bool__(self):
        return any(self.maps)

    @_recursive_repr()
    def __repr__(self):
        return '{0.__class__.__name__}({1})'.format(
            self, ', '.join(map(repr, self.maps)))

    @classmethod
    def fromkeys(cls, iterable, *args):
        'Create a ChainMap with a single dict created from the iterable.'
        return cls(dict.fromkeys(iterable, *args))

    def copy(self):
        'New ChainMap or subclass with a new copy of maps[0] and refs to maps[1:]'
        return self.__class__(self.maps[0].copy(), *self.maps[1:])

    __copy__ = copy

    def new_child(self, m=None):                # like Django's Context.push()
        '''New ChainMap with a new map followed by all previous maps.
        If no map is provided, an empty dict is used.
        '''
        if m is None:
            m = {}
        return self.__class__(m, *self.maps)

    @property
    def parents(self):                          # like Django's Context.pop()
        'New ChainMap from maps[1:].'
        return self.__class__(*self.maps[1:])

    def __setitem__(self, key, value):
        self.maps[0][key] = value

    def __delitem__(self, key):
        try:
            del self.maps[0][key]
        except KeyError:
            raise KeyError('Key not found in the first mapping: {!r}'.format(key))

    def popitem(self):
        'Remove and return an item pair from maps[0]. Raise KeyError is maps[0] is empty.'
        try:
            return self.maps[0].popitem()
        except KeyError:
            raise KeyError('No keys found in the first mapping.')

    def pop(self, key, *args):
        'Remove *key* from maps[0] and return its value. Raise KeyError if *key* not in maps[0].'
        try:
            return self.maps[0].pop(key, *args)
        except KeyError:
            raise KeyError('Key not found in the first mapping: {!r}'.format(key))

    def clear(self):
        'Clear maps[0], leaving maps[1:] intact.'
        self.maps[0].clear()


################################################################################
### UserDict
################################################################################

class UserDict(MutableMapping):

    # Start by filling-out the abstract methods
    def __init__(*args, **kwargs):
        if not args:
            raise TypeError("descriptor '__init__' of 'UserDict' object "
                            "needs an argument")
        self, *args = args
        if len(args) > 1:
            raise TypeError('expected at most 1 arguments, got %d' % len(args))
        if args:
            dict = args[0]
        elif 'dict' in kwargs:
            dict = kwargs.pop('dict')
            import warnings
            warnings.warn("Passing 'dict' as keyword argument is deprecated",
                          DeprecationWarning, stacklevel=2)
        else:
            dict = None
        self.data = {}
        if dict is not None:
            self.update(dict)
        if len(kwargs):
            self.update(kwargs)
    def __len__(self): return len(self.data)
    def __getitem__(self, key):
        if key in self.data:
            return self.data[key]
        if hasattr(self.__class__, "__missing__"):
            return self.__class__.__missing__(self, key)
        raise KeyError(key)
    def __setitem__(self, key, item): self.data[key] = item
    def __delitem__(self, key): del self.data[key]
    def __iter__(self):
        return iter(self.data)

    # Modify __contains__ to work correctly when __missing__ is present
    def __contains__(self, key):
        return key in self.data

    # Now, add the methods in dicts but not in MutableMapping
    def __repr__(self): return repr(self.data)
    def copy(self):
        if self.__class__ is UserDict:
            return UserDict(self.data.copy())
        import copy
        data = self.data
        try:
            self.data = {}
            c = copy.copy(self)
        finally:
            self.data = data
        c.update(self)
        return c
    @classmethod
    def fromkeys(cls, iterable, value=None):
        d = cls()
        for key in iterable:
            d[key] = value
        return d



################################################################################
### UserList
################################################################################

class UserList(MutableSequence):
    """A more or less complete user-defined wrapper around list objects."""
    def __init__(self, initlist=None):
        self.data = []
        if initlist is not None:
            # XXX should this accept an arbitrary sequence?
            if type(initlist) == type(self.data):
                self.data[:] = initlist
            elif isinstance(initlist, UserList):
                self.data[:] = initlist.data[:]
            else:
                self.data = list(initlist)
    def __repr__(self): return repr(self.data)
    def __lt__(self, other): return self.data <  self.__cast(other)
    def __le__(self, other): return self.data <= self.__cast(other)
    def __eq__(self, other): return self.data == self.__cast(other)
    def __gt__(self, other): return self.data >  self.__cast(other)
    def __ge__(self, other): return self.data >= self.__cast(other)
    def __cast(self, other):
        return other.data if isinstance(other, UserList) else other
    def __contains__(self, item): return item in self.data
    def __len__(self): return len(self.data)
    def __getitem__(self, i): return self.data[i]
    def __setitem__(self, i, item): self.data[i] = item
    def __delitem__(self, i): del self.data[i]
    def __add__(self, other):
        if isinstance(other, UserList):
            return self.__class__(self.data + other.data)
        elif isinstance(other, type(self.data)):
            return self.__class__(self.data + other)
        return self.__class__(self.data + list(other))
    def __radd__(self, other):
        if isinstance(other, UserList):
            return self.__class__(other.data + self.data)
        elif isinstance(other, type(self.data)):
            return self.__class__(other + self.data)
        return self.__class__(list(other) + self.data)
    def __iadd__(self, other):
        if isinstance(other, UserList):
            self.data += other.data
        elif isinstance(other, type(self.data)):
            self.data += other
        else:
            self.data += list(other)
        return self
    def __mul__(self, n):
        return self.__class__(self.data*n)
    __rmul__ = __mul__
    def __imul__(self, n):
        self.data *= n
        return self
    def append(self, item): self.data.append(item)
    def insert(self, i, item): self.data.insert(i, item)
    def pop(self, i=-1): return self.data.pop(i)
    def remove(self, item): self.data.remove(item)
    def clear(self): self.data.clear()
    def copy(self): return self.__class__(self)
    def count(self, item): return self.data.count(item)
    def index(self, item, *args): return self.data.index(item, *args)
    def reverse(self): self.data.reverse()
    def sort(self, *args, **kwds): self.data.sort(*args, **kwds)
    def extend(self, other):
        if isinstance(other, UserList):
            self.data.extend(other.data)
        else:
            self.data.extend(other)



################################################################################
### UserString
################################################################################

class UserString(Sequence):
    def __init__(self, seq):
        if isinstance(seq, str):
            self.data = seq
        elif isinstance(seq, UserString):
            self.data = seq.data[:]
        else:
            self.data = str(seq)
    def __str__(self): return str(self.data)
    def __repr__(self): return repr(self.data)
    def __int__(self): return int(self.data)
    def __float__(self): return float(self.data)
    def __complex__(self): return complex(self.data)
    def __hash__(self): return hash(self.data)
    def __getnewargs__(self):
        return (self.data[:],)

    def __eq__(self, string):
        if isinstance(string, UserString):
            return self.data == string.data
        return self.data == string
    def __lt__(self, string):
        if isinstance(string, UserString):
            return self.data < string.data
        return self.data < string
    def __le__(self, string):
        if isinstance(string, UserString):
            return self.data <= string.data
        return self.data <= string
    def __gt__(self, string):
        if isinstance(string, UserString):
            return self.data > string.data
        return self.data > string
    def __ge__(self, string):
        if isinstance(string, UserString):
            return self.data >= string.data
        return self.data >= string

    def __contains__(self, char):
        if isinstance(char, UserString):
            char = char.data
        return char in self.data

    def __len__(self): return len(self.data)
    def __getitem__(self, index): return self.__class__(self.data[index])
    def __add__(self, other):
        if isinstance(other, UserString):
            return self.__class__(self.data + other.data)
        elif isinstance(other, str):
            return self.__class__(self.data + other)
        return self.__class__(self.data + str(other))
    def __radd__(self, other):
        if isinstance(other, str):
            return self.__class__(other + self.data)
        return self.__class__(str(other) + self.data)
    def __mul__(self, n):
        return self.__class__(self.data*n)
    __rmul__ = __mul__
    def __mod__(self, args):
        return self.__class__(self.data % args)
    def __rmod__(self, format):
        return self.__class__(format % args)

    # the following methods are defined in alphabetical order:
    def capitalize(self): return self.__class__(self.data.capitalize())
    def casefold(self):
        return self.__class__(self.data.casefold())
    def center(self, width, *args):
        return self.__class__(self.data.center(width, *args))
    def count(self, sub, start=0, end=_sys.maxsize):
        if isinstance(sub, UserString):
            sub = sub.data
        return self.data.count(sub, start, end)
    def encode(self, encoding=None, errors=None): # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.encode(encoding, errors))
            return self.__class__(self.data.encode(encoding))
        return self.__class__(self.data.encode())
    def endswith(self, suffix, start=0, end=_sys.maxsize):
        return self.data.endswith(suffix, start, end)
    def expandtabs(self, tabsize=8):
        return self.__class__(self.data.expandtabs(tabsize))
    def find(self, sub, start=0, end=_sys.maxsize):
        if isinstance(sub, UserString):
            sub = sub.data
        return self.data.find(sub, start, end)
    def format(self, *args, **kwds):
        return self.data.format(*args, **kwds)
    def format_map(self, mapping):
        return self.data.format_map(mapping)
    def index(self, sub, start=0, end=_sys.maxsize):
        return self.data.index(sub, start, end)
    def isalpha(self): return self.data.isalpha()
    def isalnum(self): return self.data.isalnum()
    def isdecimal(self): return self.data.isdecimal()
    def isdigit(self): return self.data.isdigit()
    def isidentifier(self): return self.data.isidentifier()
    def islower(self): return self.data.islower()
    def isnumeric(self): return self.data.isnumeric()
    def isprintable(self): return self.data.isprintable()
    def isspace(self): return self.data.isspace()
    def istitle(self): return self.data.istitle()
    def isupper(self): return self.data.isupper()
    def join(self, seq): return self.data.join(seq)
    def ljust(self, width, *args):
        return self.__class__(self.data.ljust(width, *args))
    def lower(self): return self.__class__(self.data.lower())
    def lstrip(self, chars=None): return self.__class__(self.data.lstrip(chars))
    maketrans = str.maketrans
    def partition(self, sep):
        return self.data.partition(sep)
    def replace(self, old, new, maxsplit=-1):
        if isinstance(old, UserString):
            old = old.data
        if isinstance(new, UserString):
            new = new.data
        return self.__class__(self.data.replace(old, new, maxsplit))
    def rfind(self, sub, start=0, end=_sys.maxsize):
        if isinstance(sub, UserString):
            sub = sub.data
        return self.data.rfind(sub, start, end)
    def rindex(self, sub, start=0, end=_sys.maxsize):
        return self.data.rindex(sub, start, end)
    def rjust(self, width, *args):
        return self.__class__(self.data.rjust(width, *args))
    def rpartition(self, sep):
        return self.data.rpartition(sep)
    def rstrip(self, chars=None):
        return self.__class__(self.data.rstrip(chars))
    def split(self, sep=None, maxsplit=-1):
        return self.data.split(sep, maxsplit)
    def rsplit(self, sep=None, maxsplit=-1):
        return self.data.rsplit(sep, maxsplit)
    def splitlines(self, keepends=False): return self.data.splitlines(keepends)
    def startswith(self, prefix, start=0, end=_sys.maxsize):
        return self.data.startswith(prefix, start, end)
    def strip(self, chars=None): return self.__class__(self.data.strip(chars))
    def swapcase(self): return self.__class__(self.data.swapcase())
    def title(self): return self.__class__(self.data.title())
    def translate(self, *args):
        return self.__class__(self.data.translate(*args))
    def upper(self): return self.__class__(self.data.upper())
    def zfill(self, width): return self.__class__(self.data.zfill(width))
