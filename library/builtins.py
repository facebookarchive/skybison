#!/usr/bin/env python3
"""Built-in classes, functions, and constants."""


# This is the patch decorator, injected by our boot process. flake8 has no
# knowledge about its definition and will complain without this gross circular
# helper here.
_Unbound = _Unbound  # noqa: F821
_patch = _patch  # noqa: F821


# Begin: Early definitions that are necessary to process the rest of the file:


@_patch
def __build_class__(func, name, *bases, metaclass=_Unbound, bootstrap=False, **kwargs):
    pass


class function(bootstrap=True):
    def __call__(self, *args, **kwargs):
        return self(*args, **kwargs)

    def __get__(self, instance, owner):
        pass

    def __repr__(self):
        # TODO(T32655200): Replace 0x with #x when formatting language is
        # implemented
        return f"<function {self.__name__} at 0x{_address(self)}>"


class classmethod(bootstrap=True):
    def __get__(self, instance, owner):
        pass

    def __init__(self, fn):
        pass

    def __new__(cls, fn):
        pass


class property(bootstrap=True):
    def __get__(self, instance, owner):
        pass

    def __init__(self, fget=None, fset=None, fdel=None, doc=None):
        pass

    def __new__(cls, fget=None, fset=None, fdel=None, doc=None):
        pass

    def __set__(self, instance, value):
        pass

    def deleter(self, fn):
        pass

    def getter(self, fn):
        pass

    def setter(self, fn):
        pass


class staticmethod(bootstrap=True):
    def __get__(self, instance, owner):
        pass

    def __init__(self, fn):
        pass

    def __new__(cls, fn):
        pass


class type(bootstrap=True):
    def __call__(self, *args, **kwargs):
        pass

    def __dir__(self):
        if not isinstance(self, type):
            raise TypeError(
                f"'__dir__' requires a 'type' object "
                "but received a '{type(self).__name__}'"
            )
        result = set()
        type._merge_class_dict_keys(self, result)
        return list(result)

    def __init__(self, name_or_object, bases=_Unbound, dict=_Unbound):
        # Not a patch; just empty
        pass

    def __new__(cls, name_or_object, bases=_Unbound, dict=_Unbound):
        pass

    def __repr__(self):
        return f"<class '{self.__name__}'>"

    def _merge_class_dict_keys(self, result):
        result.update(self.__dict__.keys())
        for base in self.__bases__:
            type._merge_class_dict_keys(base, result)

    def mro(self):
        # TODO(T42302401): Call Runtime computeMro when we support metaclasses.
        return list(self.__mro__)


class object(bootstrap=True):  # noqa: E999
    @property
    def __class__(self):
        return type(self)

    @__class__.setter  # noqa: F811
    def __class__(self, value):
        _unimplemented()

    def __dir__(self):
        _unimplemented()

    def __eq__(self, other):
        if self is other:
            return True
        return NotImplemented

    def __format__(self, format_spec):
        if format_spec != "":
            raise TypeError("format_spec must be empty string")
        return str(self)

    def __hash__(self):
        pass

    def __init__(self, *args, **kwargs):
        pass

    def __ne__(self, other):
        res = type(self).__eq__(self, other)
        if res is NotImplemented:
            return NotImplemented
        return not res

    def __new__(cls, *args, **kwargs):
        pass

    def __repr__(self):
        # TODO(T32655200): Replace with #x when formatting language is
        # implemented
        return f"<{type(self).__name__} object at {_address(self)}>"

    def __sizeof__(self):
        pass

    def __str__(self):
        return type(self).__repr__(self)


# End: Early definitions


class BaseException(bootstrap=True):
    def __init__(self, *args):
        pass

    def __repr__(self):
        if not isinstance(self, BaseException):
            raise TypeError("not a BaseException object")
        return f"{self.__class__.__name__}{self.args!r}"

    def __str__(self):
        if not isinstance(self, BaseException):
            raise TypeError("not a BaseException object")
        if not self.args:
            return ""
        if len(self.args) == 1:
            return str(self.args[0])
        return str(self.args)


Ellipsis = ...


EnvironmentError = OSError


IOError = OSError


class ImportError(bootstrap=True):
    def __init__(self, *args, name=None, path=None):
        # TODO(mpage): Call super once we have EX calling working for built-in methods
        self.args = args
        if len(args) == 1:
            self.msg = args[0]
        self.name = name
        self.path = path


class KeyError(bootstrap=True):
    def __str__(self):
        if isinstance(self.args, tuple) and len(self.args) == 1:
            return repr(self.args[0])
        return super(KeyError, self).__str__()


class NoneType(bootstrap=True):
    def __new__(cls):
        pass

    def __repr__(self):
        pass


class StopIteration(bootstrap=True):
    def __init__(self, *args, **kwargs):
        pass


class SystemExit(bootstrap=True):
    def __init__(self, *args, **kwargs):
        pass


class UnicodeDecodeError(UnicodeError, bootstrap=True):
    def __init__(self, encoding, obj, start, end, reason):
        super(UnicodeDecodeError, self).__init__(encoding, obj, start, end, reason)
        if not isinstance(encoding, str):
            raise TypeError(f"argument 1 must be str, not {type(encoding).__name__}")
        self.encoding = encoding
        self.start = _index(start)
        self.end = _index(end)
        if not isinstance(reason, str):
            raise TypeError(f"argument 5 must be str, not {type(reason).__name__}")
        self.reason = reason
        # TODO(T38246066): Replace with a check for the buffer protocol
        if not isinstance(obj, (bytes, bytearray)):
            raise TypeError(
                f"a bytes-like object is required, not '{type(obj).__name__}'"
            )
        self.object = obj


class UnicodeEncodeError(UnicodeError, bootstrap=True):
    def __init__(self, encoding, obj, start, end, reason):
        super(UnicodeEncodeError, self).__init__(encoding, obj, start, end, reason)
        if not isinstance(encoding, str):
            raise TypeError(f"argument 1 must be str, not {type(encoding).__name__}")
        self.encoding = encoding
        if not isinstance(obj, str):
            raise TypeError(f"argument 2 must be str, not '{type(obj).__name__}'")
        self.object = obj
        self.start = _index(start)
        self.end = _index(end)
        if not isinstance(reason, str):
            raise TypeError(f"argument 5 must be str, not {type(reason).__name__}")
        self.reason = reason


class UnicodeError(bootstrap=True):  # noqa: B903
    def __init__(self, *args):
        self.args = args


class UnicodeTranslateError(UnicodeError, bootstrap=True):
    def __init__(self, obj, start, end, reason):
        super(UnicodeTranslateError, self).__init__(obj, start, end, reason)
        if not isinstance(obj, str):
            raise TypeError(f"argument 1 must be str, not {type(obj).__name__}")
        self.object = obj
        self.start = _index(start)
        self.end = _index(end)
        if not isinstance(reason, str):
            raise TypeError(f"argument 4 must be str, not {type(reason).__name__}")
        self.reason = reason


@_patch
def __import__(name, globals=None, locals=None, fromlist=(), level=0):
    pass


@_patch
def _address(c):
    pass


@_patch
def _bytearray_join(self: bytearray, iterable) -> bytearray:
    pass


@_patch
def _bytes_from_ints(source) -> bytes:
    pass


@_patch
def _bytes_getitem(self, index: int) -> int:
    pass


@_patch
def _bytes_getitem_slice(self, start: int, stop: int, step: int) -> bytes:
    pass


@_patch
def _bytes_join(self: bytes, iterable) -> bytes:
    pass


@_patch
def _bytes_maketrans(frm, to) -> bytes:
    pass


def _bytes_new(source) -> bytes:
    # source should be a bytes-like object (fast case), tuple/list of ints (fast case),
    # or an iterable of int-like objects (slow case)
    # patch is not patched because that would cause a circularity problem.
    result = _bytes_from_ints(source)
    if result is not None:
        # source was a list or tuple of ints in range(0, 256)
        return result
    try:
        iterator = iter(source)
    except TypeError:
        raise TypeError(f"cannot convert '{type(source).__name__}' object to bytes")
    return _bytes_from_ints([_index(x) for x in iterator])


@_patch
def _bytes_repeat(self: bytes, count: int) -> bytes:
    pass


@_patch
def _complex_imag(c):
    pass


@_patch
def _complex_real(c):
    pass


def _index(obj) -> int:
    # equivalent to PyNumber_Index
    if isinstance(obj, int):
        return obj
    try:
        result = obj.__index__()
        if isinstance(result, int):
            return result
        raise TypeError(f"__index__ returned non-int (type {type(result).__name__})")
    except AttributeError:
        raise TypeError(
            f"'{type(obj).__name__}' object cannot be interpreted as an integer"
        )


@_patch
def _list_sort(list):
    pass


def _long_of_obj(obj):
    if type(obj) is int:
        return obj
    if hasattr(obj, "__int__"):
        result = obj.__int__()
        result_type = type(result)
        if result_type is not int:
            raise TypeError(f"__int__ returned non-int (type {result_type.__name__})")
        return result
    if hasattr(obj, "__trunc__"):
        trunc_result = obj.__trunc__()
        if isinstance(trunc_result, int):
            return trunc_result
        return trunc_result.__int__()
    if isinstance(obj, str) or isinstance(obj, bytes) or isinstance(obj, bytearray):
        return int(obj, 10)
    # TODO(T41277979): Create from an object that implements the buffer protocol
    raise TypeError(
        f"int() argument must be a string, a bytes-like object or a number, not"
        f" {type(obj).__name__}"
    )


@_patch
def _repr_enter(obj: object) -> bool:
    pass


@_patch
def _repr_leave(obj: object) -> None:
    pass


def _slice_index(num) -> int:
    if hasattr(num, "__index__"):
        return _index(num)
    raise TypeError(
        "slice indices must be integers or None or have an __index__ method"
    )


@_patch
def _str_escape_non_ascii(s):
    pass


@_patch
def _str_find(self, sub, start, end):
    pass


@_patch
def _str_replace(self, old, newstr, count):
    pass


@_patch
def _str_rfind(self, sub, start, end):
    pass


def _str_split_whitespace(self, maxsplit):
    length = len(self)
    i = 0
    res = []
    num_split = 0
    while True:
        # find the beginning of the next word
        while i < length:
            if not str.__getitem__(self, i).isspace():
                break  # found
            i += 1
        else:
            break  # end of string, finished

        # find the end of the word
        if maxsplit == num_split:
            j = length  # take all the rest of the string
        else:
            j = i + 1
            while j < length and not str.__getitem__(self, j).isspace():
                j += 1
            num_split += 1

        # the word is self[i:j]
        res.append(self[i:j])

        # continue to look from the character following the space after the word
        if j < length:
            i = j + 1
        else:
            break
    return res


@_patch
def _str_splitlines(self, keepends):
    pass


class _structseq_field:
    def __get__(self, instance, owner):
        if self.index is not None:
            return instance[self.index]
        return _structseq_getattr(instance, self.name)

    def __init__(self, name, index):
        self.name = name
        self.index = index

    def __set__(self, instance, value):
        raise TypeError("readonly attribute")


@_patch
def _structseq_getattr(obj, name):
    pass


def _structseq_getitem(self, pos):
    if pos < 0 or pos >= type(self).n_fields:
        raise IndexError("index out of bounds")
    if pos < len(self):
        return self[pos]
    else:
        name = self._structseq_field_names[pos]
        return _structseq_getattr(self, name)


def _structseq_new(cls, sequence, dict={}):  # noqa B006
    seq_tuple = tuple(sequence)
    seq_len = len(seq_tuple)
    max_len = cls.n_fields
    min_len = cls.n_sequence_fields
    if seq_len < min_len:
        raise TypeError(
            f"{cls.__name__} needs at least a {min_len}-sequence "
            f"({seq_len}-sequence given)"
        )
    if seq_len > max_len:
        raise TypeError(
            f"{cls.__name__} needs at most a {max_len}-sequence "
            f"({seq_len}-sequence given)"
        )

    # Create the tuple of size min_len
    structseq = tuple.__new__(cls, seq_tuple[:min_len])

    # Fill the rest of the hidden fields
    for i in range(min_len, seq_len):
        key = cls._structseq_field_names[i]
        _structseq_setattr(structseq, key, seq_tuple[min_len])

    # Fill the remaining from the dict or set to None
    for i in range(seq_len, max_len):
        key = cls._structseq_field_names[i]
        _structseq_setattr(structseq, key, dict.get(key))

    return structseq


def _structseq_repr(self):
    if not isinstance(self, tuple):
        raise TypeError("__repr__(): self is not a tuple")
    if not hasattr(self, "n_sequence_fields"):
        raise TypeError("__repr__(): self is not a self")
    # TODO(T40273054): Iterate attributes and return field names
    tuple_values = ", ".join([i.__repr__() for i in self])
    return f"{type(self).__name__}({tuple_values})"


@_patch
def _structseq_setattr(obj, name, value):
    pass


@_patch
def _unimplemented():
    """Prints a message and a stacktrace, and stops the program execution."""
    pass


def abs(x):
    import operator

    return operator.abs(x)


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


def ascii(obj):
    return _str_escape_non_ascii(repr(obj))


def bin(x):
    _unimplemented()


class bool(bootstrap=True):
    def __new__(cls, val=False):
        pass

    def __repr__(self):
        return "True" if self else "False"

    def __str__(self) -> str:  # noqa: T484
        return bool.__repr__(self)


class bytearray(bootstrap=True):
    def __add__(self, other) -> bytearray:
        pass

    def __contains__(self, key):
        _unimplemented()

    def __delitem__(self, key):
        _unimplemented()

    def __eq__(self, value):
        _unimplemented()

    def __ge__(self, value):
        _unimplemented()

    def __getitem__(self, key):  # -> Union[int, bytearray]
        pass

    def __gt__(self, value):
        _unimplemented()

    def __iadd__(self, other) -> bytearray:
        pass

    def __imul__(self, n: int) -> bytearray:
        pass

    def __init__(self, source=_Unbound, encoding=_Unbound, errors=_Unbound):
        pass

    def __iter__(self):
        pass

    def __le__(self):
        _unimplemented()

    def __len__(self) -> int:
        pass

    def __lt__(self):
        _unimplemented()

    def __mod__(self, value):
        _unimplemented()

    def __mul__(self, n: int) -> bytearray:
        pass

    def __ne__(self, value):
        _unimplemented()

    def __new__(cls, source=_Unbound, encoding=_Unbound, errors=_Unbound):
        pass

    def __repr__(self):
        pass

    def __rmod__(self, value):
        _unimplemented()

    def __rmul__(self, n: int) -> bytearray:
        if not isinstance(self, bytearray):
            raise TypeError("'__rmul__' requires a 'bytearray' instance")
        return bytearray.__mul__(self, n)

    def __setitem__(self, key, value):
        _unimplemented()

    def append(self, item):
        _unimplemented()

    def capitalize(self):
        _unimplemented()

    def center(self):
        _unimplemented()

    def clear(self):
        _unimplemented()

    def copy(self):
        _unimplemented()

    def count(self, sub, start=_Unbound, end=_Unbound):
        _unimplemented()

    def decode(self, encoding="utf-8", errors="strict"):
        _unimplemented()

    def endswith(self, suffix, start=_Unbound, end=_Unbound):
        _unimplemented()

    def expandtabs(self, tabsize=8):
        _unimplemented()

    def extend(self, iterable_of_ints):
        _unimplemented()

    def find(self, sub, start=_Unbound, end=_Unbound):
        _unimplemented()

    @classmethod
    def fromhex(cls, string):
        _unimplemented()

    def hex(self) -> str:
        pass

    def index(self, sub, start=_Unbound, end=_Unbound):
        _unimplemented()

    def insert(self, index, item):
        _unimplemented()

    def isalnum(self):
        _unimplemented()

    def isalpha(self):
        _unimplemented()

    def isdigit(self):
        _unimplemented()

    def islower(self):
        _unimplemented()

    def isspace(self):
        _unimplemented()

    def istitle(self):
        _unimplemented()

    def isupper(self):
        _unimplemented()

    def join(self, iterable) -> bytearray:
        if not isinstance(self, bytearray):
            raise TypeError("'join' requires a 'bytearray' object")
        result = _bytearray_join(self, iterable)
        if result is not None:
            return result
        items = [x for x in iterable]
        return _bytearray_join(self, items)

    def ljust(self, width, fillchar=_Unbound):
        _unimplemented()

    def lower(self):
        _unimplemented()

    def lstrip(self, bytes=None):
        _unimplemented()

    @staticmethod
    def maketrans(frm, to) -> bytes:
        return bytes.maketrans(frm, to)

    def partition(self, sep):
        _unimplemented()

    def pop(self, index=-1):
        _unimplemented()

    def remove(self, value):
        _unimplemented()

    def replace(self, old, new, count=-1):
        _unimplemented()

    def reverse(self):
        _unimplemented()

    def rfind(self, sub, start=_Unbound, end=_Unbound):
        _unimplemented()

    def rindex(self, sub, start=_Unbound, end=_Unbound):
        _unimplemented()

    def rjust(self, width, fillchar=_Unbound):
        _unimplemented()

    def rpartition(self, sep):
        _unimplemented()

    def rsplit(self, sep=None, maxsplit=-1):
        _unimplemented()

    def rstrip(self, bytes=None):
        _unimplemented()

    def split(self, sep=None, maxsplit=-1):
        _unimplemented()

    def splitlines(self, keepends=False):
        _unimplemented()

    def startswith(self, prefix, start=_Unbound, end=_Unbound):
        _unimplemented()

    def strip(self, bytes=None):
        _unimplemented()

    def swapcase(self):
        _unimplemented()

    def title(self):
        _unimplemented()

    def translate(self, table, delete=b""):
        _unimplemented()

    def upper(self):
        _unimplemented()

    def zfill(self):
        _unimplemented()


class bytearray_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


class bytes(bootstrap=True):
    def __add__(self, other: bytes) -> bytes:
        pass

    def __contains__(self, key):
        _unimplemented()

    def __eq__(self, other):
        pass

    def __ge__(self, other):
        pass

    def __getitem__(self, key):
        if not isinstance(self, bytes):
            raise TypeError(
                "'__getitem__' requires a 'bytes' object but received a "
                f"'{type(self).__name__}'"
            )
        if isinstance(key, int):
            return _bytes_getitem(self, key)
        if isinstance(key, slice):
            return _bytes_getitem_slice(self, *key.indices(len(self)))
        try:
            return _bytes_getitem(self, _index(key))
        except TypeError:
            raise TypeError(
                f"byte indices must be integers or slice, not {type(key).__name__}"
            )

    def __gt__(self, other):
        pass

    def __hash__(self):
        pass

    def __iter__(self):
        _unimplemented()

    def __le__(self, other):
        pass

    def __len__(self) -> int:
        pass

    def __lt__(self, other):
        pass

    def __mod__(self, n):
        _unimplemented()

    def __mul__(self, n: int) -> bytes:
        pass

    def __ne__(self, other):
        pass

    def __new__(cls, source=_Unbound, encoding=_Unbound, errors=_Unbound):  # noqa: C901
        if not isinstance(cls, type):
            raise TypeError(
                f"bytes.__new__(X): X is not a type object ({type(cls).__name__})"
            )
        if not issubclass(cls, bytes):
            raise TypeError(
                f"bytes.__new__({cls.__name__}): "
                f"{cls.__name__} is not a subtype of bytes"
            )
        if source is _Unbound:
            if encoding is _Unbound and errors is _Unbound:
                # TODO(T36619847): implement bytes subclasses
                return b""
            raise TypeError("encoding or errors without sequence argument")
        if encoding is not _Unbound:
            if not isinstance(source, str):
                raise TypeError("encoding without a string argument")
            _unimplemented()
        if errors is not _Unbound:
            if isinstance(source, str):
                raise TypeError("string argument without an encoding")
            raise TypeError("errors without a string argument")
        if hasattr(source, "__bytes__"):
            result = source.__bytes__()
            if not isinstance(result, bytes):
                raise TypeError(
                    f"__bytes__ returned non-bytes (type {type(result).__name__})"
                )
            return result
        if isinstance(source, str):
            raise TypeError("string argument without an encoding")
        if isinstance(source, int) or hasattr(source, "__index__"):
            # TODO(T36619847): implement bytes subclasses
            return _bytes_repeat(b"\x00", _index(source))
        # TODO(T36619847): implement bytes subclasses
        return _bytes_new(source)

    def __repr__(self) -> str:  # noqa: T484
        pass

    def __rmod__(self, n):
        _unimplemented()

    def __rmul__(self, n: int) -> bytes:
        if not isinstance(self, bytes):
            raise TypeError("'__rmul__' requires a 'bytes' instance")
        return bytes.__mul__(self, n)

    def capitalize(self):
        _unimplemented()

    def center(self):
        _unimplemented()

    def count(self, sub, start=_Unbound, end=_Unbound):
        _unimplemented()

    def decode(self, encoding="utf-8", errors="strict") -> str:
        import _codecs

        return _codecs.decode(self, encoding, errors)

    def endswith(self, suffix, start=_Unbound, end=_Unbound):
        _unimplemented()

    def expandtabs(self, tabsize=8):
        _unimplemented()

    def find(self, sub, start=_Unbound, end=_Unbound):
        _unimplemented()

    @classmethod
    def fromhex(cls, string):
        _unimplemented()

    def hex(self) -> str:
        pass

    def index(self, sub, start=_Unbound, end=_Unbound):
        _unimplemented()

    def isalnum(self):
        _unimplemented()

    def isalpha(self):
        _unimplemented()

    def isdigit(self):
        _unimplemented()

    def islower(self):
        _unimplemented()

    def isspace(self):
        _unimplemented()

    def istitle(self):
        _unimplemented()

    def isupper(self):
        _unimplemented()

    def join(self, iterable) -> bytes:
        if not isinstance(self, bytes):
            raise TypeError("'join' requires a 'bytes' object")
        result = _bytes_join(self, iterable)
        if result is not None:
            return result
        items = [x for x in iterable]
        return _bytes_join(self, items)

    def ljust(self, width, fillchar=_Unbound):
        _unimplemented()

    def lower(self):
        _unimplemented()

    def lstrip(self, bytes=None):
        _unimplemented()

    @staticmethod
    def maketrans(frm, to) -> bytes:
        if not isinstance(frm, bytes) and not isinstance(frm, bytearray):
            raise TypeError(
                f"a bytes-like object is required, not '{type(frm).__name__}'"
            )
        if not isinstance(to, bytes) and not isinstance(to, bytearray):
            raise TypeError(
                f"a bytes-like object is required, not '{type(to).__name__}'"
            )
        if len(frm) != len(to):
            raise ValueError("maketrans arguments must have same length")
        return _bytes_maketrans(frm, to)

    def partition(self, sep):
        _unimplemented()

    def replace(self, old, new, count=-1):
        _unimplemented()

    def rfind(self, sub, start=_Unbound, end=_Unbound):
        _unimplemented()

    def rindex(self, sub, start=_Unbound, end=_Unbound):
        _unimplemented()

    def rjust(self, width, fillchar=_Unbound):
        _unimplemented()

    def rpartition(self, sep):
        _unimplemented()

    def rsplit(self, sep=None, maxsplit=-1):
        _unimplemented()

    def rstrip(self, bytes=None):
        _unimplemented()

    def split(self, sep=None, maxsplit=-1):
        _unimplemented()

    def splitlines(self, keepends=False):
        _unimplemented()

    def startswith(self, prefix, start=_Unbound, end=_Unbound):
        _unimplemented()

    def strip(self, bytes=None):
        _unimplemented()

    def swapcase(self):
        _unimplemented()

    def title(self):
        _unimplemented()

    def translate(self, table, delete=b""):
        _unimplemented()

    def upper(self):
        _unimplemented()

    def zfill(self):
        _unimplemented()


@_patch
def callable(f):
    pass


@_patch
def chr(c):
    pass


@_patch
def compile(source, filename, mode, flags=0, dont_inherit=False, optimize=-1):
    pass


class complex(bootstrap=True):
    def __new__(cls, real=0.0, imag=0.0):
        pass

    def __repr__(self):
        return f"({self.real}+{self.imag}j)"

    @property
    def imag(self):
        return _complex_imag(self)

    @property
    def real(self):
        return _complex_real(self)


class coroutine(bootstrap=True):
    def send(self, value):
        pass


def delattr(obj, name):
    _unimplemented()


class dict(bootstrap=True):
    def __contains__(self, key) -> bool:
        return self.get(key, _Unbound) is not _Unbound  # noqa: T484

    def __delitem__(self, key):
        pass

    def __eq__(self, other):
        pass

    def __getitem__(self, key):
        result = self.get(key, _Unbound)
        if result is _Unbound:
            raise KeyError(key)
        return result

    def __init__(self, *args, **kwargs):
        if len(args) > 1:
            raise TypeError("dict expected at most 1 positional argument, got 2")
        if len(args) == 1:
            dict.update(self, args[0])
        dict.update(self, kwargs)

    def __iter__(self):
        pass

    def __len__(self):
        pass

    def __new__(cls, *args, **kwargs):
        pass

    def __repr__(self):
        if _repr_enter(self):
            return "{...}"
        kwpairs = [f"{key!r}: {self[key]!r}" for key in self.keys()]
        _repr_leave(self)
        return "{" + ", ".join(kwpairs) + "}"

    def __setitem__(self, key, value):
        pass

    def copy(self):
        if not isinstance(self, dict):
            raise TypeError(f"expected 'dict' instance but got {type(self).__name__}")
        return dict(self)

    def get(self, key, default=None):
        pass

    def items(self):
        pass

    def keys(self):
        pass

    def pop(self, key, default=_Unbound):
        value = dict.get(self, key, default)
        if value is _Unbound:
            raise KeyError(key)
        if key in self:
            dict.__delitem__(self, key)
        return value

    def setdefault(self, key, default=None):
        if not isinstance(self, dict):
            raise TypeError("setdefault expected 'dict' but got {type(self).__name__}")
        value = dict.get(self, key, _Unbound)
        if value is _Unbound:
            dict.__setitem__(self, key, default)
            return default
        return value

    def update(self, other):
        pass

    def values(self):
        pass


class dict_itemiterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


class dict_items(bootstrap=True):
    def __iter__(self):
        pass


class dict_keyiterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


class dict_keys(bootstrap=True):
    def __iter__(self):
        pass


class dict_valueiterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


class dict_values(bootstrap=True):
    def __iter__(self):
        pass


def dir(obj=_Unbound):
    if obj is _Unbound:
        names = locals().keys()
    else:
        names = type(obj).__dir__(obj)
    return sorted(names)


@_patch
def divmod(a, b):
    pass


class enumerate:
    def __init__(self, iterable, start=0):
        self.iterator = iter(iterable)
        self.index = start

    def __iter__(self):
        return self

    def __next__(self):
        result = (self.index, next(self.iterator))
        self.index += 1
        return result


def eval(source, globals=None, locals=None):
    _unimplemented()


@_patch
def exec(source, globals=None, locals=None):
    pass


def exit():
    _unimplemented()


class filter:
    """filter(function or None, iterable) --> filter object

    Return an iterator yielding those items of iterable for which function(item)
    is true. If function is None, return the items that are true.
    """

    def __iter__(self):
        return self

    def __new__(cls, function, iterable, **kwargs):
        obj = super(filter, cls).__new__(cls)
        obj.func = (lambda e: e) if function is None else function
        obj.iter = iter(iterable)
        return obj

    def __next__(self):
        func = self.func
        while True:
            item = next(self.iter)
            if func(item):
                return item


class float(bootstrap=True):
    def __abs__(self) -> float:
        pass

    def __add__(self, n: float) -> float:
        pass

    def __bool__(self) -> bool:
        pass

    def __eq__(self, n: float) -> bool:  # noqa: T484
        pass

    def __float__(self) -> float:
        pass

    def __ge__(self, n: float) -> bool:
        pass

    def __gt__(self, n: float) -> bool:
        pass

    def __int__(self, n: float) -> int:
        pass

    def __le__(self, n: float) -> bool:
        pass

    def __lt__(self, n: float) -> bool:
        pass

    def __mul__(self, n: float) -> float:
        pass

    def __ne__(self, n: float) -> float:  # noqa: T484
        if not isinstance(self, float):
            raise TypeError(
                f"'__ne__' requires a 'float' object "
                f"but received a '{self.__class__.__name__}'"
            )
        if not isinstance(n, float) and not isinstance(n, int):
            return NotImplemented
        return not float.__eq__(self, n)  # noqa: T484

    def __neg__(self) -> float:
        pass

    def __new__(cls, arg=0.0) -> float:
        pass

    def __pow__(self, y, z=_Unbound) -> float:
        pass

    def __radd__(self, n: float) -> float:
        # The addition for floating point numbers is commutative:
        # https://en.wikipedia.org/wiki/Floating-point_arithmetic#Accuracy_problems.
        # Note: Handling NaN payloads may vary depending on the hardware, but nobody
        # seems to depend on it due to such variance.
        return float.__add__(self, n)

    def __repr__(self) -> str:  # noqa: T484
        pass

    def __rmul__(self, n: float) -> float:
        # The multiplication for floating point numbers is commutative:
        # https://en.wikipedia.org/wiki/Floating-point_arithmetic#Accuracy_problems
        return float.__mul__(self, n)

    def __rsub__(self, n: float) -> float:
        # n - self == -self + n.
        return float.__neg__(self).__add__(n)

    def __rtruediv__(self, n: float) -> float:
        pass

    def __sub__(self, n: float) -> float:
        pass

    def __truediv__(self, n: float) -> float:
        pass


def format(obj, fmt_spec):
    if not isinstance(fmt_spec, str):
        raise TypeError(
            f"fmt_spec must be str instance, not '{type(fmt_spec).__name__}'"
        )
    result = obj.__format__(fmt_spec)
    if not isinstance(result, str):
        raise TypeError(
            f"__format__ must return str instance, not '{type(result).__name__}'"
        )
    return result


class frozenset(bootstrap=True):
    def __and__(self, other):
        pass

    def __contains__(self, value):
        pass

    def __eq__(self, other):
        pass

    def __ge__(self, other):
        pass

    def __gt__(self, other):
        pass

    def __iter__(self):
        pass

    def __le__(self, other):
        pass

    def __len__(self):
        pass

    def __lt__(self, other):
        pass

    def __ne__(self, other):
        pass

    def __new__(cls, iterable=_Unbound):
        pass

    def copy(self):
        pass

    def intersection(self, other):
        pass

    def isdisjoint(self, other):
        pass


class generator(bootstrap=True):
    def __iter__(self):
        pass

    def __next__(self):
        pass

    def send(self, value):
        pass


@_patch
def getattr(obj, key, default=_Unbound):
    pass


def globals():
    _unimplemented()


@_patch
def hasattr(obj, name):
    pass


def hash(obj):
    dunder_hash = type(obj).__hash__
    try:
        result = dunder_hash(obj)
    except TypeError:
        raise TypeError(f"unhashable type: '{type(obj).__name__}'")
    if not isinstance(result, int):
        raise TypeError("__hash__ method should return an integer")
    # TODO(djang): This needs to be cast to the exact int type.
    return result


def help(obj=_Unbound):
    _unimplemented()


def hex(number):
    _unimplemented()


def id(obj):
    _unimplemented()


def input(prompt=None):
    _unimplemented()


class int(bootstrap=True):
    def __abs__(self) -> int:
        pass

    def __add__(self, n: int) -> int:
        pass

    def __and__(self, n: int) -> int:
        pass

    def __bool__(self) -> bool:
        pass

    def __ceil__(self) -> int:
        pass

    def __divmod__(self, n: int):
        pass

    def __eq__(self, n: int) -> bool:  # noqa: T484
        pass

    def __float__(self) -> float:
        pass

    def __floor__(self) -> int:
        pass

    def __floordiv__(self, n: int) -> int:
        pass

    def __ge__(self, n: int) -> bool:
        pass

    def __gt__(self, n: int) -> bool:
        pass

    def __index__(self) -> int:
        pass

    def __int__(self) -> int:
        pass

    def __invert__(self) -> int:
        pass

    def __le__(self, n: int) -> bool:
        pass

    def __lshift__(self, n: int) -> int:
        pass

    def __lt__(self, n: int) -> bool:
        pass

    def __mod__(self, n: int) -> int:
        pass

    def __mul__(self, n: int) -> int:
        pass

    def __ne__(self, n: int) -> int:  # noqa: T484
        pass

    def __neg__(self) -> int:
        pass

    def __new__(cls, n=0, base=_Unbound):
        pass

    def __or__(self, n: int) -> int:
        pass

    def __pos__(self) -> int:
        pass

    def __pow__(self, power, mod=None) -> int:
        # TODO(T42359066): Re-write this in C++ if we need a speed boost.
        if not isinstance(self, int):
            raise TypeError(
                f"'__pow__' requires an 'int' object but got '{type(self).__name__}'"
            )
        if not isinstance(power, int):
            return NotImplemented
        if mod is not None and not isinstance(mod, int):
            return NotImplemented
        if power < 0:
            if mod is not None:
                raise ValueError(
                    "pow() 2nd argument cannot be negative when 3rd argument specified"
                )
            else:
                return float.__pow__(float(self), power)
        if 0 == power:
            return 1
        if 1 == mod:
            return 0
        result = self
        while int.__gt__(power, 1):
            result = int.__mul__(result, self)
            power = int.__sub__(power, 1)
        if mod is not None:
            result = int.__mod__(result, mod)
        return result

    def __radd__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__radd__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__add__(n, self)

    def __rand__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rand__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__and__(n, self)

    def __rdivmod__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rdivmod__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__divmod__(n, self)  # noqa: T484

    def __repr__(self) -> str:  # noqa: T484
        pass

    def __rfloordiv__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rfloordiv__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__floordiv__(n, self)  # noqa: T484

    def __rlshift__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rlshift__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__lshift__(n, self)

    def __rmod__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rmod__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__mod__(n, self)  # noqa: T484

    def __rmul__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rmul__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__mul__(n, self)

    def __ror__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__ror__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__or__(n, self)

    def __round__(self) -> int:
        pass

    def __rpow__(self, n: int, *, mod=None):
        if not isinstance(self, int):
            raise TypeError("'__rpow__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__pow__(n, self, mod=mod)  # noqa: T484

    def __rrshift__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rrshift__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__rshift__(n, self)

    def __rshift__(self, n: int) -> int:
        pass

    def __rsub__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rsub__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__sub__(n, self)

    def __rtruediv__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rtruediv__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__truediv__(n, self)  # noqa: T484

    def __rxor__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rxor__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__xor__(n, self)

    def __str__(self) -> str:  # noqa: T484
        pass

    def __sub__(self, n: int) -> int:
        pass

    def __truediv__(self, other):
        pass

    def __trunc__(self) -> int:
        pass

    def __xor__(self, n: int) -> int:
        pass

    def bit_length(self) -> int:
        pass

    def conjugate(self) -> int:
        pass

    @property
    def denominator(self) -> int:
        return 1  # noqa: T484

    @property
    def imag(self) -> int:
        return 0  # noqa: T484

    numerator = property(__int__)

    real = property(__int__)

    def to_bytes(self, length, byteorder, signed=False):
        pass


@_patch
def isinstance(obj, ty):
    pass


@_patch
def issubclass(obj, ty):
    pass


def iter(obj, sentinel=None):
    if sentinel is None:
        try:
            dunder_iter = type(obj).__iter__
        except AttributeError:
            raise TypeError(f"'{type(obj).__name__}' object is not iterable")
        return dunder_iter(obj)

    class CallIter:
        def __init__(self, callable, sentinel):
            self.__callable = callable
            self.__sentinel = sentinel

        def __iter__(self):
            return self

        def __next__(self):
            value = self.__callable()
            if self.__sentinel == value:
                raise StopIteration()
            return value

        def __reduce__(self):
            return (iter, (self.__callable, self.__sentinel))

    return CallIter(obj, sentinel)


def len(seq):
    dunder_len = getattr(seq, "__len__", None)
    if dunder_len is None:
        raise TypeError("object has no len()")
    return dunder_len()


class list(bootstrap=True):
    def __add__(self, other):
        pass

    def __contains__(self, value):
        pass

    def __delitem__(self, key):
        pass

    def __eq__(self, other):
        if not isinstance(self, list):
            raise TypeError(
                f"'__eq__' requires 'list' but received a '{type(self).__name__}'"
            )
        if not isinstance(other, list):
            return NotImplemented

        if self is other:
            return True
        length = list.__len__(self)
        if length != list.__len__(other):
            return False
        i = 0
        while i < length:
            lhs = list.__getitem__(self, i)
            rhs = list.__getitem__(other, i)
            if lhs is not rhs and not (lhs == rhs):
                return False
            i += 1
        return True

    def __getitem__(self, key):
        pass

    def __init__(self, iterable=()):
        self.extend(iterable)

    def __iter__(self):
        pass

    def __len__(self):
        pass

    def __mul__(self, other):
        pass

    def __new__(cls, iterable=()):
        pass

    def __repr__(self):
        if _repr_enter(self):
            return "[...]"
        result = "[" + ", ".join([i.__repr__() for i in self]) + "]"
        _repr_leave(self)
        return result

    def __setitem__(self, key, value):
        pass

    def append(self, other):
        pass

    def clear(self):
        pass

    def copy(self):
        if not isinstance(self, list):
            raise TypeError(f"expected 'list' instance but got {type(self).__name__}")
        return list(self)

    def extend(self, other):
        pass

    def insert(self, index, value):
        pass

    def pop(self, index=_Unbound):
        pass

    def remove(self, value):
        pass

    def reverse(self):
        length = len(self)
        if length < 2:
            return
        left = 0
        right = length - 1
        while left < right:
            tmp = list.__getitem__(self, left)
            list.__setitem__(self, left, list.__getitem__(self, right))
            list.__setitem__(self, right, tmp)
            left += 1
            right -= 1

    def sort(self, key=None, reverse=False):
        if not isinstance(self, list):
            raise TypeError(f"sort expected 'list' but got {type(self).__name__}")
        if reverse:
            list.reverse(self)
        if key:
            i = 0
            length = len(self)
            while i < length:
                item = list.__getitem__(self, i)
                list.__setitem__(self, i, (key(item), item))
                i += 1
        _list_sort(self)
        if key:
            i = 0
            while i < length:
                item = list.__getitem__(self, i)
                list.__setitem__(self, i, item[1])
                i += 1
        if reverse:
            list.reverse(self)


class list_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


def locals():
    _unimplemented()


class map:
    def __init__(self, func, *iterables):
        if len(iterables) < 1:
            raise TypeError("map() must have at least two arguments.")
        self.func = func
        self.iters = tuple(iter(iterable) for iterable in iterables)
        self.len_iters = len(self.iters)

    def __iter__(self):
        return self

    def __next__(self):
        func = self.func
        if self.len_iters == 1:
            return func(next(self.iters[0]))
        return func(*[next(iter) for iter in self.iters])


def max(arg1, arg2=_Unbound, *args, key=_Unbound, default=_Unbound):  # noqa: C901
    if arg2 is not _Unbound:
        if default is not _Unbound:
            raise TypeError(
                "Cannot specify a default for max() with multiple positional arguments"
            )
        # compare positional arguments and varargs
        if key is _Unbound:
            # using argument values
            result = arg2 if arg2 > arg1 else arg1
            if args:
                for item in args:
                    result = item if item > result else result
        else:
            # using ordering function values
            key1 = key(arg1)
            key2 = key(arg2)
            key_max = key2 if key2 > key1 else key1
            result = arg1 if key_max is key1 else arg2
            if args:
                for item in args:
                    key_item = key(item)
                    if key_item > key_max:
                        key_max, result = key_item, item
    else:
        # compare iterable items
        result = default
        if key is _Unbound:
            # using iterable item values
            for item in arg1:
                if result is _Unbound or item > result:
                    result = item
        else:
            # using ordering function values
            key_max = _Unbound
            for item in arg1:
                key_item = key(item)
                if key_max is _Unbound or key_item > key_max:
                    key_max = key_item
                    result = item
        if result is _Unbound:
            raise ValueError("max() arg is an empty sequence")
    return result


class memoryview(bootstrap=True):
    def __getitem__(self, index):
        pass

    def __len__(self) -> int:
        pass

    def __new__(cls, object):
        pass

    def cast(self, format: str) -> memoryview:
        pass


def min(arg1, arg2=_Unbound, *args, key=_Unbound, default=_Unbound):  # noqa: C901
    # NOTE: Sync this with max() since min() is copied from max().
    if arg2 is not _Unbound:
        if default is not _Unbound:
            raise TypeError(
                "Cannot specify a default for min() with multiple positional arguments"
            )
        # compare positional arguments and varargs
        if key is _Unbound:
            # using argument values
            result = arg2 if arg2 < arg1 else arg1
            if args:
                for item in args:
                    result = item if item < result else result
        else:
            # using ordering function values
            key1 = key(arg1)
            key2 = key(arg2)
            key_max = key2 if key2 < key1 else key1
            result = arg1 if key_max is key1 else arg2
            if args:
                for item in args:
                    key_item = key(item)
                    if key_item < key_max:
                        key_max, result = key_item, item
    else:
        # compare iterable items
        result = default
        if key is _Unbound:
            # using iterable item values
            for item in arg1:
                if result is _Unbound or item < result:
                    result = item
        else:
            # using ordering function values
            key_max = _Unbound
            for item in arg1:
                key_item = key(item)
                if key_max is _Unbound or key_item < key_max:
                    key_max = key_item
                    result = item
        if result is _Unbound:
            raise ValueError("min() arg is an empty sequence")
    return result


class module(bootstrap=True):
    def __dir__(self):
        if not isinstance(self, module):
            raise TypeError(
                f"'__dir__' requires a 'module' object "
                "but received a '{type(self).__name__}'"
            )
        return list(self.__dict__.keys())

    def __new__(cls, name):
        pass

    def __repr__(self):
        import _frozen_importlib

        return _frozen_importlib._module_repr(self)


def next(iterator, default=_Unbound):
    try:
        dunder_next = _Unbound
        dunder_next = type(iterator).__next__
        return dunder_next(iterator)
    except StopIteration:
        if default is _Unbound:
            raise
        return default
    except AttributeError:
        if dunder_next is _Unbound:
            raise TypeError(f"'{type(iterator).__name__}' object is not iterable")
        raise


def oct(number):
    _unimplemented()


def open(
    file,
    mode="r",
    buffering=-1,
    encoding=None,
    errors=None,
    newline=None,
    closefd=True,
    opener=None,
):
    _unimplemented()


@_patch
def ord(c):
    pass


def pow(x, y, z=None):
    _unimplemented()


def print(*args, sep=" ", end="\n", file=None, flush=False):
    if file is None:
        import sys

        try:
            file = sys.stdout
        except AttributeError:
            raise RuntimeError("lost sys.stdout")
        if file is None:
            return
    if args:
        file.write(str(args[0]))
        length = len(args)
        i = 1
        while i < length:
            file.write(sep)
            file.write(str(args[i]))
            i += 1
    file.write(end)
    if flush:
        file.flush()


def quit():
    _unimplemented()


class range(bootstrap=True):
    def __iter__(self):
        pass

    def __new__(cls, start_or_stop, stop=_Unbound, step=_Unbound):
        pass


class range_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


def repr(obj):
    result = type(obj).__repr__(obj)
    if not isinstance(result, str):
        raise TypeError("__repr__ returned non-string")
    return result


class reversed:
    def __init__(self, sequence):
        _unimplemented()


def round(number, ndigits=_Unbound):
    _unimplemented()


class set(bootstrap=True):
    def __and__(self, other):
        pass

    def __contains__(self, value):
        pass

    def __eq__(self, other):
        pass

    def __ge__(self, other):
        pass

    def __gt__(self, other):
        pass

    def __iand__(self, other):
        pass

    def __init__(self, iterable=()):
        pass

    def __iter__(self):
        pass

    def __le__(self, other):
        pass

    def __len__(self):
        pass

    def __lt__(self, other):
        pass

    def __ne__(self, other):
        pass

    def __new__(cls, iterable=()):
        pass

    def __or__(self, other):
        if not isinstance(self, set) and not isinstance(self, frozenset):
            return NotImplemented
        if not isinstance(other, set) and not isinstance(other, frozenset):
            return NotImplemented
        result = set.copy(self)
        if self is other:
            return result
        set.update(result, other)
        return result

    def __repr__(self):
        if _repr_enter(self):
            return f"{type(self).__name__}(...)"
        if len(self) == 0:
            _repr_leave(self)
            return f"{type(self).__name__}()"
        result = f"{{{', '.join([item.__repr__() for item in self])}}}"
        _repr_leave(self)
        return result

    def add(self, value):
        pass

    def copy(self):
        pass

    def intersection(self, other):
        pass

    def isdisjoint(self, other):
        pass

    def pop(self):
        pass

    def update(self, *args):
        pass


class set_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


@_patch
def setattr(obj, name, value):
    pass


class slice(bootstrap=True):
    def __new__(cls, start_or_stop, stop=_Unbound, step=None):
        pass

    def indices(self, length) -> tuple:  # noqa: C901
        if not isinstance(self, slice):
            raise TypeError(
                "'indices' requires a 'slice' object but received a "
                f"'{type(self).__name__}'"
            )
        length = _index(length)
        if length < 0:
            raise ValueError("length should not be negative")

        # Convert step to an integer; raise for zero step
        if self.step is None:
            step = 1
            negative_step = False
        else:
            step = _slice_index(self.step)
            if step == 0:
                raise ValueError("slice step cannot be zero")
            negative_step = step < 0

        # Bounds for start and stop
        lower = -1 if negative_step else 0
        upper = length + lower if negative_step else length

        # Compute start
        if self.start is None:
            start = upper if negative_step else lower
        else:
            start = _slice_index(self.start)
            if start < 0:
                start += length
                if start < lower:
                    start = lower
            elif start > upper:
                start = upper

        # Compute stop
        if self.stop is None:
            stop = lower if negative_step else upper
        else:
            stop = _slice_index(self.stop)
            if stop < 0:
                stop += length
                if stop < lower:
                    stop = lower
            elif stop > upper:
                stop = upper

        return start, stop, step


def sorted(iterable, *, key=None, reverse=False):
    result = list(iterable)
    result.sort(key=key, reverse=reverse)
    return result


class str(bootstrap=True):
    def __add__(self, other):
        pass

    def __bool__(self):
        pass

    def __contains__(self, other):
        if not isinstance(self, str):
            raise TypeError(f"expected a 'str' instance but got {type(self).__name__}")
        if not isinstance(other, str):
            raise TypeError(f"expected a 'str' instance but got {type(other).__name__}")
        return str.find(self, other) != -1

    def __eq__(self, other):
        pass

    def __ge__(self, other):
        pass

    def __getitem__(self, index):
        pass

    def __gt__(self, other):
        pass

    def __hash__(self):
        pass

    def __iter__(self):
        pass

    def __le__(self, other):
        pass

    def __len__(self) -> int:
        pass

    def __lt__(self, other):
        pass

    def __mod__(self, other):
        pass

    def __mul__(self, n: int) -> str:
        pass

    def __ne__(self, other):
        pass

    def __new__(cls, obj="", encoding=_Unbound, errors=_Unbound):
        if not isinstance(cls, type):
            raise TypeError("cls is not a type object")
        if not issubclass(cls, str):
            raise TypeError("cls is not a subtype of str")
        if cls != str:
            _unimplemented()
        if type(obj) is str and obj == "":
            return obj
        if encoding != _Unbound or errors != _Unbound:
            _unimplemented()
        result = type(obj).__str__(obj)
        if not isinstance(result, str):
            raise TypeError("__str__ returned non-str instance")
        return result

    def __repr__(self):
        pass

    def __rmod__(self, n):
        _unimplemented()

    def __rmul__(self, n: int) -> str:
        if not isinstance(self, str):
            raise TypeError("'__rmul__' requires a 'str' instance")
        return str.__mul__(self, n)

    def __str__(self):
        return self

    def capitalize(self):
        if not isinstance(self, str):
            self_type = type(self).__name__
            raise TypeError(
                f"'capitalize' requires a 'str' instance but got '{self_type}'"
            )
        if not self:
            return self
        first_letter = str.upper(str.__getitem__(self, 0).upper())
        lowercase_str = str.lower(str.__getitem__(self, slice(1, None, None)))
        return first_letter + lowercase_str

    def casefold(self):
        _unimplemented()

    def center(self, width, fillchar=" "):
        _unimplemented()

    def count(self, sub, start=_Unbound, end=_Unbound):
        _unimplemented()

    def encode(self, encoding="utf-8", errors="strict") -> bytes:
        import _codecs

        return _codecs.encode(self, encoding, errors)

    def endswith(self, suffix, start=0, end=None):  # noqa: C901
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

    def expandtabs(self, tabsize=8):
        _unimplemented()

    def find(self, sub, start=None, end=None):
        if not isinstance(self, str):
            raise TypeError(
                f"find requires a 'str' instance but got {type(self).__name__}"
            )
        if not isinstance(sub, str):
            raise TypeError(
                f"find requires a 'str' instance but got {type(sub).__name__}"
            )
        if start is not None:
            start = _index(start)
        if end is not None:
            end = _index(end)
        return _str_find(self, sub, start, end)

    def format(self, *args, **kwargs):
        _unimplemented()

    def format_map(self, mapping):
        _unimplemented()

    def index(self, sub, start=None, end=None):
        res = self.find(sub, start, end)
        if res < 0:
            raise ValueError(f"substring {sub} not found")
        return res

    def isalnum(self):
        # TODO(T41626152): Support non-ASCII
        if not isinstance(self, str):
            raise TypeError(f"isalnum expected 'str' but got {type(self).__name__}")
        if self is "":  # noqa: P202
            return False
        num = range(ord("0"), ord("9") + 1)
        lower = range(ord("a"), ord("z") + 1)
        upper = range(ord("A"), ord("Z") + 1)
        for c in str.__iter__(self):
            i = ord(c)
            if i > 127:
                _unimplemented()
            if i not in num and i not in lower and i not in upper:
                return False
        return True

    def isalpha(self):
        _unimplemented()

    def isdecimal(self):
        _unimplemented()

    def isdigit(self):
        _unimplemented()

    def isidentifier(self):
        if not isinstance(self, str):
            raise TypeError(
                f"'isidentifier' expected 'str' but got '{type(self).__name__}'"
            )
        if not self:
            return False
        is_continue = False
        for ch in str.__iter__(self):
            if "A" <= ch <= "Z" or "a" <= ch <= "z" or ch == "_":
                pass
            elif is_continue and "0" <= ch <= "9":
                pass
            elif ch >= "\x80":
                _unimplemented()
            else:
                return False
            is_continue = True
        return True

    def islower(self):
        # TODO(T42050373): Support non-ASCII
        if not isinstance(self, str):
            raise TypeError(f"islower expected 'str' but got {type(self).__name__}")
        if self is "":  # noqa: P202
            return False
        lower = range(ord("a"), ord("z") + 1)
        for c in str.__iter__(self):
            i = ord(c)
            if i > 127:
                _unimplemented()
            if i not in lower:
                return False
        return True

    def isnumeric(self):
        _unimplemented()

    def isprintable(self):
        _unimplemented()

    def isspace(self):
        if not isinstance(self, str):
            raise TypeError(f"expected 'str' but got {type(self).__name__}")
        if not self:
            return False
        for ch in str.__iter__(self):
            if ch == " " or ch == "\n" or ch == "\t" or ch == "\r":
                continue
            if ch == "\x0B" or ch == "\x0C" or ("\x1C" < ch < "\x1F"):
                continue
            if ch >= "\x80":
                # TODO(T41626183): Support non-ASCII
                _unimplemented()
            return False
        return True

    def istitle(self):
        _unimplemented()

    def isupper(self):
        # TODO(T41626183): Support non-ASCII
        if not isinstance(self, str):
            raise TypeError(f"isupper expected 'str' but got {type(self).__name__}")
        if self is "":  # noqa: P202
            return False
        upper = range(ord("A"), ord("Z") + 1)
        for c in str.__iter__(self):
            i = ord(c)
            if i > 127:
                _unimplemented()
            if i not in upper:
                return False
        return True

    def join(self, items) -> str:
        pass

    def ljust(self, width, fillchar):
        _unimplemented()

    def lower(self):
        pass

    def lstrip(self, other=None):
        pass

    @staticmethod
    def maketrans(x, y=None, z=None):
        _unimplemented()

    def partition(self, sep):
        if not isinstance(self, str):
            raise TypeError(
                f"descriptor 'partition' requires a 'str' object "
                f"but received a {type(self).__name__}"
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

    def replace(self, old, new, count=None):
        if not isinstance(self, str):
            raise TypeError(
                f"replace requires a 'str' instance but got {type(self).__name__}"
            )
        if not isinstance(old, str):
            raise TypeError(
                f"replace requires a 'str' instance but got {type(old).__name__}"
            )
        if not isinstance(new, str):
            raise TypeError(
                f"replace requires a 'str' instance but got {type(new).__name__}"
            )
        if count:
            count = _index(count)
        else:
            count = -1
        result = _str_replace(self, old, new, count)
        return str(result) if self is result else result

    def rfind(self, sub, start=None, end=None):
        if not isinstance(self, str):
            raise TypeError(
                f"rfind requires a 'str' instance but got {type(self).__name__}"
            )
        if not isinstance(sub, str):
            raise TypeError(
                f"rfind requires a 'str' instance but got {type(sub).__name__}"
            )
        if start is not None:
            start = _index(start)
        if end is not None:
            end = _index(end)
        return _str_rfind(self, sub, start, end)

    def rindex(self, sub, start=_Unbound, end=_Unbound):
        _unimplemented()

    def rjust(self, width, fillchar=_Unbound):
        _unimplemented()

    def rpartition(self, sep):
        # TODO(T37438017): Write in C++
        before, itself, after = self[::-1].partition(sep[::-1])[::-1]
        return before[::-1], itself[::-1], after[::-1]

    def rsplit(self, sep=None, maxsplit=-1):
        # TODO(T37437993): Write in C++
        return [s[::-1] for s in self[::-1].split(sep[::-1], maxsplit)[::-1]]

    def rstrip(self, other=None):
        pass

    def split(self, sep=None, maxsplit=-1):  # noqa: C901
        if not isinstance(self, str):
            raise TypeError(f"expected a 'str' instance but got {type(self).__name__}")
        # If the separator is not specified, split on all whitespace characters.
        if sep is None:
            return _str_split_whitespace(self, maxsplit)
        if maxsplit == 0:
            return [self]
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

    def splitlines(self, keepends=False):
        if not isinstance(self, str):
            raise TypeError(
                f"'splitlines' requires a 'str' but got '{type(self).__name__}'"
            )
        if isinstance(keepends, float):
            raise TypeError("integer argument expected, got float")
        if not isinstance(keepends, (bool, int)):
            keepends = int(keepends)
        return _str_splitlines(self, keepends)

    def startswith(self, prefix, start=0, end=None):  # noqa: C901
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

    def strip(self, other=None):
        pass

    def swapcase(self):
        _unimplemented()

    def title(self):
        _unimplemented()

    def translate(self, table):
        _unimplemented()

    def upper(self):
        pass

    def zfill(self, width):
        _unimplemented()


class str_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


def sum(iterable, start=0):
    if isinstance(start, str):
        raise TypeError("sum() can't sum strings [use ''.join(seq) instead]")
    if isinstance(start, bytes):
        raise TypeError("sum() can't sum bytes [use b''.join(seq) instead]")
    if isinstance(start, bytearray):
        raise TypeError("sum() can't sum bytearray [use b''.join(seq) instead]")

    result = start
    for item in iterable:
        result = result + item
    return result


class super(bootstrap=True):
    def __init__(self, type=_Unbound, type_or_obj=_Unbound):
        pass

    def __new__(cls, type=_Unbound, type_or_obj=_Unbound):
        pass


class tuple(bootstrap=True):
    def __add__(self, other):
        pass

    def __contains__(self, key):
        pass

    def __eq__(self, other):
        pass

    def __getitem__(self, index):
        pass

    def __hash__(self):
        pass

    def __iter__(self):
        pass

    def __len__(self):
        pass

    def __lt__(self, other):
        if not isinstance(self, tuple):
            raise TypeError(f"__lt__ expected 'tuple' but got {type(self).__name__}")
        if not isinstance(other, tuple):
            raise TypeError(f"__lt__ expected 'tuple' but got {type(other).__name__}")
        len_self = tuple.__len__(self)
        len_other = tuple.__len__(other)
        # TODO(T42050051): Use builtin.min when it's developed
        min_len = len_self if len_self < len_other else len_other
        # Find the first non-equal item in the tuples
        i = 0
        while i < min_len:
            self_i = tuple.__getitem__(self, i)
            other_i = tuple.__getitem__(other, i)
            if self_i is not other_i and self_i != other_i:
                break
            i += 1
        if i >= min_len:
            # If the items are all up equal up to min_len, compare lengths
            return len_self < len_other
        return self_i < other_i

    def __mul__(self, other):
        pass

    def __new__(cls, iterable=_Unbound):
        pass

    def __repr__(self):
        if _repr_enter(self):
            return "(...)"
        num_elems = len(self)
        output = "("
        i = 0
        while i < num_elems:
            if i != 0:
                output += ", "
            output += repr(self[i])
            i += 1
        _repr_leave(self)
        if num_elems == 1:
            output += ","
        return output + ")"


class tuple_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __length_hint__(self):
        pass

    def __next__(self):
        pass


def vars(*args):
    _unimplemented()


class zip:
    def __init__(self, *iterables):
        _unimplemented()
