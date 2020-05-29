#!/usr/bin/env python3
"""This is an internal module implementing "str.__mod__" formatting."""

from builtins import (
    _float,
    _index,
    _int_format_hexadecimal,
    _int_format_hexadecimal_upcase,
    _int_format_octal,
    _mapping_check,
    _number_check,
    _str_array,
)

from _builtins import (
    _float_format,
    _float_signbit,
    _int_check,
    _str_array_iadd,
    _str_check,
    _str_len,
    _tuple_check,
    _tuple_getitem,
    _tuple_len,
    _type,
)


_FLAG_LJUST = 1 << 0
_FLAG_ZERO = 1 << 1


def _format_string(result, flags, width, precision, fragment):
    if precision >= 0:
        fragment = fragment[:precision]
    if width <= 0:
        _str_array_iadd(result, fragment)
        return

    padding_len = -1
    padding_len = width - _str_len(fragment)
    if padding_len > 0 and not (flags & _FLAG_LJUST):
        _str_array_iadd(result, " " * padding_len)
        padding_len = 0
    _str_array_iadd(result, fragment)
    if padding_len > 0:
        _str_array_iadd(result, " " * padding_len)


def _format_number(result, flags, width, precision, sign, prefix, fragment):
    if width <= 0 and precision < 0:
        _str_array_iadd(result, sign)
        _str_array_iadd(result, prefix)
        _str_array_iadd(result, fragment)
        return

    # Compute a couple values before assembling the result:
    # - `padding_len` the number of spaces around the number
    #    - _FLAG_LJUST determines whether it is before/after
    #    - We compute it by starting with the full width and subtracting the
    #      length of everything else we are going to emit.
    # - `num_leading_zeros` number of extra zeros to print between prefix and
    #    the number.
    fragment_len = _str_len(fragment)
    padding_len = width - fragment_len - _str_len(sign) - _str_len(prefix)

    num_leading_zeros = 0
    if precision >= 0:
        num_leading_zeros = precision - fragment_len
        if num_leading_zeros > 0:
            padding_len -= num_leading_zeros

    if (flags & _FLAG_ZERO) and not (flags & _FLAG_LJUST):
        # Perform padding by increasing precision instead.
        if padding_len > 0:
            num_leading_zeros += padding_len
        padding_len = 0

    # Compose the result.
    if padding_len > 0 and not (flags & _FLAG_LJUST):
        _str_array_iadd(result, " " * padding_len)
        padding_len = 0
    _str_array_iadd(result, sign)
    _str_array_iadd(result, prefix)
    if num_leading_zeros > 0:
        _str_array_iadd(result, "0" * num_leading_zeros)
    _str_array_iadd(result, fragment)
    if padding_len > 0:
        _str_array_iadd(result, " " * padding_len)


def format(string: str, args) -> str:  # noqa: C901
    args_dict = None
    if _tuple_check(args):
        args_tuple = args
        args_len = _tuple_len(args_tuple)
    else:
        args_tuple = (args,)
        args_len = 1
    arg_idx = 0

    result = _str_array()
    idx = -1
    begin = 0
    in_specifier = False
    it = str.__iter__(string)
    try:
        while True:
            c = it.__next__()
            idx += 1
            if c is not "%":  # noqa: F632
                continue

            _str_array_iadd(result, string[begin:idx])

            in_specifier = True
            c = it.__next__()
            idx += 1

            # Parse named reference.
            if c is "(":  # noqa: F632
                # Lazily initialize args_dict.
                if args_dict is None:
                    if (
                        _tuple_check(args)
                        or _str_check(args)
                        or not _mapping_check(args)
                    ):
                        raise TypeError("format requires a mapping")
                    args_dict = args

                pcount = 1
                keystart = idx + 1
                while pcount > 0:
                    c = it.__next__()
                    idx += 1
                    if c is ")":  # noqa: F632
                        pcount -= 1
                    elif c is "(":  # noqa: F632
                        pcount += 1
                key = string[keystart:idx]

                # skip over closing ")"
                c = it.__next__()
                idx += 1

                # lookup parameter in dictionary.
                value = args_dict[key]
                args_tuple = (value,)
                args_len = 1
                arg_idx = 0

            # Parse flags.
            flags = 0
            positive_sign = ""
            use_alt_formatting = False
            while True:
                if c is "-":  # noqa: F632
                    flags |= _FLAG_LJUST
                elif c is "+":  # noqa: F632
                    positive_sign = "+"
                elif c is " ":  # noqa: F632
                    if positive_sign is not "+":  # noqa: F632
                        positive_sign = " "
                elif c is "#":  # noqa: F632
                    use_alt_formatting = True
                elif c is "0":  # noqa: F632
                    flags |= _FLAG_ZERO
                else:
                    break
                c = it.__next__()
                idx += 1

            # Parse width.
            width = -1
            if c is "*":  # noqa: F632
                if arg_idx >= args_len:
                    raise TypeError("not enough arguments for format string")
                arg = _tuple_getitem(args_tuple, arg_idx)
                arg_idx += 1
                if not _int_check(arg):
                    raise TypeError("* wants int")
                width = arg
                if width < 0:
                    flags |= _FLAG_LJUST
                    width = -width
                c = it.__next__()
                idx += 1
            elif "0" <= c <= "9":
                width = 0
                while True:
                    width += ord(c) - ord("0")
                    c = it.__next__()
                    idx += 1
                    if not ("0" <= c <= "9"):
                        break
                    width *= 10

            # Parse precision.
            precision = -1
            if c is ".":  # noqa: F632
                precision = 0
                c = it.__next__()
                idx += 1
                if c is "*":  # noqa: F632
                    if arg_idx >= args_len:
                        raise TypeError("not enough arguments for format string")
                    arg = _tuple_getitem(args_tuple, arg_idx)
                    arg_idx += 1
                    if not _int_check(arg):
                        raise TypeError("* wants int")
                    precision = max(0, arg)
                    c = it.__next__()
                    idx += 1
                elif "0" <= c <= "9":
                    while True:
                        precision += ord(c) - ord("0")
                        c = it.__next__()
                        idx += 1
                        if not ("0" <= c <= "9"):
                            break
                        precision *= 10

            # Parse and process format.
            if c is not "%":  # noqa: F632
                if arg_idx >= args_len:
                    raise TypeError("not enough arguments for format string")
                arg = _tuple_getitem(args_tuple, arg_idx)
                arg_idx += 1

            if c is "%":  # noqa: F632
                _str_array_iadd(result, "%")
            elif c is "s":  # noqa: F632
                fragment = str(arg)
                _format_string(result, flags, width, precision, fragment)
            elif c is "r":  # noqa: F632
                fragment = repr(arg)
                _format_string(result, flags, width, precision, fragment)
            elif c is "a":  # noqa: F632
                fragment = ascii(arg)
                _format_string(result, flags, width, precision, fragment)
            elif c is "c":  # noqa: F632
                if _str_check(arg):
                    if _str_len(arg) != 1:
                        raise TypeError("%c requires int or char")
                    fragment = arg
                else:
                    try:
                        value = _index(arg)
                        fragment = chr(value)
                    except ValueError:
                        import sys

                        max_hex = _int_format_hexadecimal(sys.maxunicode + 1)
                        raise OverflowError(f"%c arg not in range(0x{max_hex})")
                    except Exception:
                        raise TypeError("%c requires int or char")
                _format_string(result, flags, width, precision, fragment)
            elif c is "d" or c is "i" or c is "u":  # noqa: F632
                try:
                    if not _number_check(arg):
                        raise TypeError()
                    value = int(arg)
                except TypeError:
                    tname = _type(arg).__name__
                    raise TypeError(f"%{c} format: a number is required, not {tname}")
                if value < 0:
                    value = -value
                    sign = "-"
                else:
                    sign = positive_sign
                fragment = int.__str__(value)
                _format_number(result, flags, width, precision, sign, "", fragment)
            elif c is "x":  # noqa: F632
                try:
                    if not _number_check(arg):
                        raise TypeError()
                    value = _index(arg)
                except TypeError:
                    tname = _type(arg).__name__
                    raise TypeError(f"%{c} format: an integer is required, not {tname}")
                if value < 0:
                    value = -value
                    sign = "-"
                else:
                    sign = positive_sign
                prefix = "0x" if use_alt_formatting else ""
                fragment = _int_format_hexadecimal(value)
                _format_number(result, flags, width, precision, sign, prefix, fragment)
            elif c is "X":  # noqa: F632
                try:
                    if not _number_check(arg):
                        raise TypeError()
                    value = _index(arg)
                except TypeError:
                    tname = _type(arg).__name__
                    raise TypeError(f"%{c} format: an integer is required, not {tname}")
                if value < 0:
                    value = -value
                    sign = "-"
                else:
                    sign = positive_sign
                prefix = "0X" if use_alt_formatting else ""
                fragment = _int_format_hexadecimal_upcase(value)
                _format_number(result, flags, width, precision, sign, prefix, fragment)
            elif c is "o":  # noqa: F632
                try:
                    if not _number_check(arg):
                        raise TypeError()
                    value = _index(arg)
                except TypeError:
                    tname = _type(arg).__name__
                    raise TypeError(f"%o format: an integer is required, not {tname}")
                if value < 0:
                    value = -value
                    sign = "-"
                else:
                    sign = positive_sign
                prefix = "0o" if use_alt_formatting else ""
                fragment = _int_format_octal(value)
                _format_number(result, flags, width, precision, sign, prefix, fragment)
            elif c in "eEfFgG":
                value = _float(arg)
                if precision < 0:
                    precision = 6
                # The `value != value` test avoids emitting "-nan".
                if _float_signbit(value) and not value != value:
                    sign = "-"
                else:
                    sign = positive_sign
                fragment = _float_format(
                    value, c, precision, True, False, use_alt_formatting
                )
                _format_number(result, flags, width, 0, sign, "", fragment)
            else:
                raise ValueError(
                    f"unsupported format character '{c}' "
                    f"(0x{_int_format_hexadecimal(ord(c))}) "
                    f"at index {idx}"
                )

            begin = idx + 1
            in_specifier = False
    except StopIteration:
        # Make sure everyone called `idx += 1` after `it.__next__()`.
        assert idx + 1 == _str_len(string)

    if in_specifier:
        raise ValueError("incomplete format")
    _str_array_iadd(result, string[begin:])

    if arg_idx < args_len and args_dict is None:
        # Lazily check that the user did not specify an args dictionary and if
        # not raise an error:
        if _tuple_check(args) or _str_check(args) or not _mapping_check(args):
            raise TypeError("not all arguments converted during string formatting")
    return result.__str__()
