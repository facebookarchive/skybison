#!/usr/bin/env python3
"""This is an internal module implementing "str.__mod__" formatting."""

_float_check = _float_check  # noqa: F821
_int_check = _int_check  # noqa: F821
_str_check = _str_check  # noqa: F821
_str_len = _str_len  # noqa: F821
_strarray = _strarray  # noqa: F821
_strarray_iadd = _strarray_iadd  # noqa: F821
_tuple_check = _tuple_check  # noqa: F821
_unimplemented = _unimplemented  # noqa: F821


def format(string: str, args) -> str:  # noqa: C901
    if _tuple_check(args):
        args_tuple = args
        args_len = len(args_tuple)
    else:
        args_tuple = (args,)
        args_len = 1
    arg_idx = 0

    result = _strarray()
    idx = 0
    begin = 0
    in_specifier = False
    it = str.__iter__(string)
    try:
        while True:
            c = it.__next__()
            idx += 1
            if c != "%":
                continue

            _strarray_iadd(result, string[begin : idx - 1])

            in_specifier = True
            c = it.__next__()
            idx += 1

            # Parse and process format.
            if c != "%":
                if arg_idx >= args_len:
                    raise TypeError("not enough arguments for format string")
                arg = args_tuple[arg_idx]
                arg_idx += 1

            if c == "%":
                _strarray_iadd(result, "%")
            elif c == "s":
                _strarray_iadd(result, str(arg))
            elif c == "r":
                _strarray_iadd(result, repr(arg))
            elif c == "a":
                _strarray_iadd(result, ascii(arg))
            elif c in "d":
                if not _int_check(arg):
                    _unimplemented()
                _strarray_iadd(result, int.__str__(arg))
            elif c == "g":
                if not _float_check(arg):
                    _unimplemented()
                _strarray_iadd(result, float.__str__(arg))
            else:
                # May be a flag, precision or width modifier, an unimplemented
                # format or the user passing an unknown character.
                _unimplemented()

            begin = idx
            in_specifier = False
    except StopIteration:
        # Make sure everyone called `idx += 1` after `it.__next__()`.
        assert idx == _str_len(string)

    if in_specifier:
        raise ValueError("incomplete format")
    _strarray_iadd(result, string[begin:])

    if arg_idx < args_len:
        raise TypeError("not all arguments converted during string formatting")
    return result.__str__()
