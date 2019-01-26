#!/usr/bin/env python3
"""Built-in classes, functions, and constants."""


# This is the patch decorator, injected by our boot process. flake8 has no
# knowledge about its definition and will complain without this gross circular
# helper here.
_patch = _patch  # noqa: F821
_UnboundValue = _UnboundValue  # noqa: F821
_address = _address  # noqa: F821


class object(bootstrap=True):  # noqa: E999
    def __str__(self):
        return self.__repr__()

    def __hash__(self):
        pass


class bool(bootstrap=True):
    def __repr__(self):
        return "True" if self else "False"


class int(bootstrap=True):
    def __add__(self, n: int) -> int:
        pass


class ImportError(bootstrap=True):
    def __init__(self, *args, name=None, path=None, **kwargs):
        if len(kwargs) > 0:
            raise TypeError("invalid keyword arguments supplied")
        # TODO(mpage): Call super once we have EX calling working for built-in methods
        self.args = args
        if len(args) == 1:
            self.msg = args[0]
        self.name = name
        self.path = path


class BaseException(bootstrap=True):
    def __str__(self):
        if not isinstance(self, BaseException):
            raise TypeError("not a BaseException object")
        if not self.args:
            return ""
        if len(self.args) == 1:
            return str(self.args[0])
        return str(self.args)


class KeyError(bootstrap=True):
    def __str__(self):
        if isinstance(self.args, tuple) and len(self.args) == 1:
            return repr(self.args[0])
        return super(KeyError, self).__str__()


class tuple(bootstrap=True):
    def __repr__(self):
        num_elems = len(self)
        if num_elems == 1:
            return f"({repr(self[0])},)"
        output = "("
        i = 0
        while i < num_elems:
            if i != 0:
                output += ", "
            output += repr(self[i])
            i += 1
        return output + ")"


class list(bootstrap=True):
    def __init__(self, iterable=()):
        self.extend(iterable)

    def __repr__(self):
        return "[" + ", ".join([i.__repr__() for i in self]) + "]"

    def __contains__(self, key: object) -> bool:
        pass


class slice(bootstrap=True):
    def __new__(cls, start_or_stop, stop=_UnboundValue, step=None):
        pass


class str(bootstrap=True):
    def partition(self, sep):
        if not isinstance(self, str):
            raise TypeError(
                f"descriptor 'partition' requires a 'str' object but received a {type(self).__name__}"
            )
        if not isinstance(sep, str):
            raise TypeError(f"must be str, not {type(sep).__name__}")
        sep_len = len(sep)
        if not sep_len:
            raise ValueError("empty separator")
        sep_0 = sep[0]
        i = 0
        str_len = len(self)
        while i < str_len:
            if self[i] == sep_0:
                j = 1
                while j < sep_len:
                    if i + j >= str_len or self[i + j] != sep[j]:
                        break
                    j += 1
                else:
                    return (self[:i], sep, self[i + j :])
            i += 1
        return (self, "", "")

    def split(self, sep=None, maxsplit=-1):
        if maxsplit == 0:
            return self
        # If the separator is not specified, split on all whitespace characters.
        if sep is None:
            raise NotImplementedError("Splitting on whitespace not yet implemented")
        if not isinstance(sep, str):
            raise TypeError("must be str or None")
        sep_len = len(sep)
        if sep_len == 0:
            raise ValueError("empty separator")

        def strcmp(cmp, other, start):
            """Returns True if other is in cmp at the position start"""
            i = 0
            cmp_len = len(cmp)
            other_len = len(other)
            # If the other string is longer than the rest of the current string,
            # then it is not a match.
            if start + other_len > cmp_len:
                return False
            while i < other_len and start + i < cmp_len:
                if cmp[start + i] != other[i]:
                    return False
                i += 1
            return True

        # TODO(dulinr): This path uses random-access indices on strings, which
        # is not efficient for UTF-8 strings.
        parts = []
        i = 0
        str_len = len(self)
        # The index of the string starting after the last match.
        last_match = 0
        while i < str_len:
            if strcmp(self, sep, i):
                parts.append(self[last_match:i])
                last_match = i + sep_len
                # If we've hit the max number of splits, return early.
                maxsplit -= 1
                if maxsplit == 0:
                    break
                i += sep_len
            else:
                i += 1
        parts.append(self[last_match:])
        return parts

    def startswith(self, prefix, start=0, end=None):
        def real_bounds_from_slice_bounds(start, end, length):
            if start < 0:
                start = length + start
            if start < 0:
                start = 0
            if start > length:
                start = length

            if end is None or end > length:
                end = length
            if end < 0:
                end = length + end
            if end < 0:
                end = 0
            return start, end

        def prefix_match(cmp, prefix, start, end):
            if not isinstance(prefix, str):
                raise TypeError("startswith prefix must be a str")
            prefix_len = len(prefix)
            # If the prefix is longer than the string its comparing against, it
            # can't be a match.
            if end - start < prefix_len:
                return False
            end = start + prefix_len

            # Iterate through cmp from [start, end), checking against
            # the characters in the suffix.
            i = 0
            while i < prefix_len:
                if cmp[start + i] != prefix[i]:
                    return False
                i += 1
            return True

        str_len = len(self)
        start, end = real_bounds_from_slice_bounds(start, end, str_len)
        if not isinstance(prefix, tuple):
            return prefix_match(self, prefix, start, end)

        for pref in prefix:
            if prefix_match(self, pref, start, end):
                return True
        return False

    def endswith(self, suffix, start=0, end=None):
        def real_bounds_from_slice_bounds(start, end, length):
            if start < 0:
                start = length + start
            if start < 0:
                start = 0
            if start > length:
                start = length

            if end is None or end > length:
                end = length
            if end < 0:
                end = length + end
            if end < 0:
                end = 0
            return start, end

        def suffix_match(cmp, sfx, start, end):
            if not isinstance(sfx, str):
                raise TypeError("endswith suffix must be a str")
            sfx_len = len(sfx)
            # If the suffix is longer than the string its comparing against, it
            # can't be a match.
            if end - start < sfx_len:
                return False
            start = end - sfx_len

            # Iterate through cmp from [end - sfx_len, end), checking against
            # the characters in the suffix.
            i = 0
            while i < sfx_len:
                if cmp[start + i] != sfx[i]:
                    return False
                i += 1
            return True

        str_len = len(self)
        start, end = real_bounds_from_slice_bounds(start, end, str_len)
        if not isinstance(suffix, tuple):
            return suffix_match(self, suffix, start, end)

        for suf in suffix:
            if suffix_match(self, suf, start, end):
                return True
        return False

    # TODO(T37437993): Write in C++
    def rsplit(self, sep=None, maxsplit=-1):
        return [s[::-1] for s in self[::-1].split(sep[::-1], maxsplit)[::-1]]

    # TODO(T37438017): Write in C++
    def rpartition(self, sep):
        before, itself, after = self[::-1].partition(sep[::-1])[::-1]
        return before[::-1], itself[::-1], after[::-1]

    def __str__(self):
        return self

    def __add__(self, s: str) -> str:
        pass

    def __mul__(self, n: int) -> str:
        pass

    def __rmul__(self, n: int) -> str:
        pass

    def __lt__(self, x: str) -> bool:
        pass

    def __le__(self, x: str) -> bool:
        pass

    def __gt__(self, x: str) -> bool:
        pass

    def __ge__(self, x: str) -> bool:
        pass

    def __len__(self) -> int:
        pass

    def __contains__(self, s: object) -> bool:
        pass

    # This method requires a non-primitive type for its annotation and cannot
    # yet be typed here.
    def __iter__(self):
        pass

    # This method requires a non-primitive type for its annotation and cannot
    # yet be typed here.
    def __mod__(self, value):
        pass

    # This method requires a non-primitive type for its annotation and cannot
    # yet be typed here.
    def __getitem__(self, i):
        pass


class dict(bootstrap=True):
    def __contains__(self, obj) -> bool:
        pass

    def __delitem__(self, key):
        pass

    def __eq__(self, other):
        pass

    def __len__(self):
        pass

    def __new__(cls):
        pass

    def __getitem__(self, key):
        pass

    def __setitem__(self, key, value):
        pass

    def __iter__(self):
        pass

    def __repr__(self):
        kwpairs = [f"{repr(key)}: {repr(self[key])}" for key in self.keys()]
        return "{" + ", ".join(kwpairs) + "}"

    def get(self, key, default=None):
        pass

    def items(self):
        pass

    def keys(self):
        pass

    def update(self, other):
        pass

    def values(self):
        pass


class module(bootstrap=True):
    def __new__(cls, name):
        pass


class set(bootstrap=True):
    def __repr__(self):
        return f"{{{', '.join([item.__repr__() for item in self])}}}"


class function(bootstrap=True):
    def __repr__(self):
        return f"<function {self.__name__} at 0x{repr(_address(self))}>"


def all(iterable):
    for element in iterable:
        if not element:
            return False
    return True


def any(iterable):
    for element in iterable:
        if element:
            return True
    return False


@_patch
def callable(f):
    pass


@_patch
def chr(c):
    pass


@_patch
def compile(source, filename, mode, flags=0, dont_inherit=False, optimize=-1):
    pass


@_patch
def exec(source, globals=None, locals=None):
    pass


@_patch
def getattr(obj, key, default=_UnboundValue):
    pass


# patch is not patched because that would cause a circularity problem.


@_patch
def range(start_or_stop, stop=_UnboundValue, step=_UnboundValue):
    pass


@_patch
def hasattr(obj, name):
    pass


@_patch
def isinstance(obj, ty):
    pass


@_patch
def issubclass(obj, ty):
    pass


@_patch
def len(seq):
    pass


@_patch
def ord(c):
    pass


@_patch
def repr(obj):
    pass


@_patch
def setattr(obj, name, value):
    pass
