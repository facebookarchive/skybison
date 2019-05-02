#!/usr/bin/env python3
"""The internal _namedtuple module, used in the Pyro port of the collections
module."""


# flake8 has no knowledge about these functions' definitions and will complain
# without this gross circular helper here.
_str_check = _str_check  # noqa: F821
_structseq_setattr = _structseq_setattr  # noqa: F821
_structseq_field = _structseq_field  # noqa: F821


def _iskeyword(s):
    return s in [
        "False",
        "class",
        "finally",
        "is",
        "return",
        "None",
        "continue",
        "for",
        "lambda",
        "try",
        "True",
        "def",
        "from",
        "nonlocal",
        "while",
        "and",
        "del",
        "global",
        "not",
        "with",
        "as",
        "elif",
        "if",
        "or",
        "yield",
        "assert",
        "else",
        "import",
        "pass",
        "break",
        "except",
        "in",
        "raise",
    ]


def _namedtuple_new(cls, *sequence, **kwargs):  # noqa B006
    seq_tuple = tuple(sequence)
    seq_len = len(seq_tuple) + len(kwargs)
    if seq_len != cls.n_fields:
        raise TypeError(
            f"{cls.__name__} needs a {cls.n_fields}-sequence ({seq_len}-sequence given)"
        )

    # Create the tuple of size min_len
    kwarg_names = tuple(dict.keys(kwargs))
    kwarg_values = tuple(dict.get(kwargs, k) for k in kwarg_names)
    structseq = tuple.__new__(cls, seq_tuple + kwarg_values)

    # Fill the remaining from the dict
    for i in range(len(kwarg_names)):
        key = kwarg_names[i]
        value = kwarg_values[i]
        _structseq_setattr(structseq, key, value)

    return structseq


def validate_field_names(typename, field_names, rename):
    # Validate the field names.  At the user's option, either generate an error
    # message or automatically replace the field name with a valid name.
    if _str_check(field_names):
        field_names = field_names.replace(",", " ").split()
    field_names = list(map(str, field_names))
    typename = str(typename)
    if rename:
        seen = set()
        for index, name in enumerate(field_names):
            if (
                not name.isidentifier()
                or _iskeyword(name)
                or name.startswith("_")
                or name in seen
            ):
                field_names[index] = f"_{index}"
            seen.add(name)
    for name in [typename] + field_names:
        if type(name) is not str:
            raise TypeError("Type names and field names must be strings")
        if not name.isidentifier():
            raise ValueError(
                f"Type names and field names must be valid identifiers: {name!r}"
            )
        if _iskeyword(name):
            raise ValueError(
                f"Type names and field names cannot be a keyword: {name!r}"
            )
    seen = set()
    for name in field_names:
        if name.startswith("_") and not rename:
            raise ValueError(f"Field names cannot start with an underscore: {name!r}")
        if name in seen:
            raise ValueError(f"Encountered duplicate field name: {name!r}")
        seen.add(name)
    return typename, field_names


def namedtuple(typename, field_names, *, verbose=False, rename=False, module=None):
    """Returns a new subclass of tuple with named fields.

    >>> Point = namedtuple('Point', ['x', 'y'])
    >>> Point.__doc__                   # docstring for the new class
    'Point(x, y)'
    >>> p = Point(11, y=22)             # instantiate with positional args or keywords
    >>> p[0] + p[1]                     # indexable like a plain tuple
    33
    >>> x, y = p                        # unpack like a regular tuple
    >>> x, y
    (11, 22)
    >>> p.x + p.y                       # fields also accessible by name
    33
    >>> d = p._asdict()                 # convert to a dictionary
    >>> d['x']
    11
    >>> Point(**d)                      # convert from a dictionary
    Point(x=11, y=22)
    >>> p._replace(x=100)               # _replace() is like str.replace()...
    Point(x=100, y=22)
    >>>                                 # ...but targets named fields

    """

    typename, field_names = validate_field_names(typename, field_names, rename)
    field_names = tuple(field_names)
    cls = type.__new__(type, typename, (tuple,), {})
    num_fields = tuple.__len__(field_names)
    # TODO(emacs): Make sure all the runtime field names begin with
    # underscores.
    cls._fields = field_names
    cls.n_fields = num_fields
    cls.n_sequence_fields = num_fields
    cls.n_unnamed_fields = 0
    cls.__new__ = _namedtuple_new
    cls._structseq_field_names = field_names

    def _repr(self):
        fields = ", ".join(
            [f"{field}={getattr(self,field)!r}" for field in self._fields]
        )
        return f"{typename}({fields})"

    cls.__repr__ = _repr

    def _make(iterable, new=tuple.__new__, len=len):
        result = new(cls, iterable)
        if len(result) != num_fields:
            raise TypeError(f"Expected {num_fields} arguments, got %d" % len(result))
        return result

    cls._make = _make

    def _replace(self, **kwds):
        print(self, kwds)
        result = _make(map(kwds.pop, field_names, self))
        if kwds:
            raise ValueError(f"Got unexpected field names: {kwds}")
        return result

    cls._replace = _replace
    index = 0
    for name in tuple.__iter__(field_names):
        setattr(cls, name, _structseq_field(name, index))
        index += 1
    return cls
