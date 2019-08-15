#!/usr/bin/env python3
"""This is an internal module implementing "str.__mod__" formatting."""

_float = _float  # noqa: F821
_float_check = _float_check  # noqa: F821
_float_format = _float_format  # noqa: F821
_float_signbit = _float_signbit  # noqa: F821
_index = _index  # noqa: F821
_int_check = _int_check  # noqa: F821
_int_format_hexadecimal = _int_format_hexadecimal  # noqa: F821
_int_format_hexadecimal_upcase = _int_format_hexadecimal_upcase  # noqa: F821
_int_format_octal = _int_format_octal  # noqa: F821
_mapping_check = _mapping_check  # noqa: F821
_number_check = _number_check  # noqa: F821
_str_check = _str_check  # noqa: F821
_str_len = _str_len  # noqa: F821
_strarray = _strarray  # noqa: F821
_strarray_iadd = _strarray_iadd  # noqa: F821
_tuple_check = _tuple_check  # noqa: F821
_type = _type  # noqa: F821


_FLAG_LJUST = 1 << 0
_FLAG_ZERO = 1 << 1


def _format_string(result, flags, width, precision, fragment):
    if precision >= 0:
        fragment = fragment[:precision]
    if width <= 0:
        _strarray_iadd(result, fragment)
        return

    padding_len = -1
    padding_len = width - _str_len(fragment)
    if padding_len > 0 and not (flags & _FLAG_LJUST):
        _strarray_iadd(result, " " * padding_len)
        padding_len = 0
    _strarray_iadd(result, fragment)
    if padding_len > 0:
        _strarray_iadd(result, " " * padding_len)


def _format_number(result, flags, width, precision, sign, prefix, fragment):
    if width <= 0 and precision < 0:
        _strarray_iadd(result, sign)
        _strarray_iadd(result, prefix)
        _strarray_iadd(result, fragment)
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
        _strarray_iadd(result, " " * padding_len)
        padding_len = 0
    _strarray_iadd(result, sign)
    _strarray_iadd(result, prefix)
    if num_leading_zeros > 0:
        _strarray_iadd(result, "0" * num_leading_zeros)
    _strarray_iadd(result, fragment)
    if padding_len > 0:
        _strarray_iadd(result, " " * padding_len)


def format(string: str, args) -> str:  # noqa: C901
    args_dict = None
    if _tuple_check(args):
        args_tuple = args
        args_len = len(args_tuple)
    else:
        args_tuple = (args,)
        args_len = 1
    arg_idx = 0

    result = _strarray()
    idx = -1
    begin = 0
    in_specifier = False
    it = str.__iter__(string)
    try:
        while True:
            c = it.__next__()
            idx += 1
            if c != "%":
                continue

            _strarray_iadd(result, string[begin:idx])

            in_specifier = True
            c = it.__next__()
            idx += 1

            # Parse named reference.
            if c == "(":
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
                    if c == ")":
                        pcount -= 1
                    elif c == "(":
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
                if c == "-":
                    flags |= _FLAG_LJUST
                elif c == "+":
                    positive_sign = "+"
                elif c == " ":
                    if positive_sign != "+":
                        positive_sign = " "
                elif c == "#":
                    use_alt_formatting = True
                elif c == "0":
                    flags |= _FLAG_ZERO
                else:
                    break
                c = it.__next__()
                idx += 1

            # Parse width.
            width = -1
            if c == "*":
                if arg_idx >= args_len:
                    raise TypeError("not enough arguments for format string")
                arg = args_tuple[arg_idx]
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
            if c == ".":
                precision = 0
                c = it.__next__()
                idx += 1
                if c == "*":
                    if arg_idx >= args_len:
                        raise TypeError("not enough arguments for format string")
                    arg = args_tuple[arg_idx]
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
            if c != "%":
                if arg_idx >= args_len:
                    raise TypeError("not enough arguments for format string")
                arg = args_tuple[arg_idx]
                arg_idx += 1

            if c == "%":
                _strarray_iadd(result, "%")
            elif c == "s":
                fragment = str(arg)
                _format_string(result, flags, width, precision, fragment)
            elif c == "r":
                fragment = repr(arg)
                _format_string(result, flags, width, precision, fragment)
            elif c == "a":
                fragment = ascii(arg)
                _format_string(result, flags, width, precision, fragment)
            elif c == "c":
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
            elif c == "d" or c == "i" or c == "u":
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
            elif c == "x":
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
            elif c == "X":
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
            elif c == "o":
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
    _strarray_iadd(result, string[begin:])

    if arg_idx < args_len:
        raise TypeError("not all arguments converted during string formatting")
    return result.__str__()
