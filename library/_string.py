#!/usr/bin/env python3


# These values are injected by our boot process. flake8 has no knowledge about
# their definitions and will complain without this circular helper here.
_str_from_str = _str_from_str  # noqa: F821
_str_guard = _str_guard  # noqa: F821
_str_len = _str_len  # noqa: F821
_unimplemented = _unimplemented  # noqa: F821


_empty_iterator = iter(())


def formatter_field_name_split(field_name):
    _str_guard(field_name)

    if "." in field_name or "[" in field_name:
        _unimplemented()

    if field_name.isdigit():
        field_name = int(field_name)
    else:
        field_name = _str_from_str(str, field_name)
    return (field_name, _empty_iterator)


def formatter_parser(string):  # noqa: C901
    _str_guard(string)
    idx = -1
    fragment_begin = 0
    it = str.__iter__(string)
    end_error = None
    try:
        while True:
            idx += 1
            ch = it.__next__()
            if ch == "}":
                end_error = "Single '}' encountered in format string"
                idx += 1
                next_ch = it.__next__()

                if next_ch != "}":
                    raise ValueError(end_error)
                yield (string[fragment_begin:idx], None, None, None)
                end_error = None
                fragment_begin = idx + 1
                continue
            if ch != "{":
                continue

            end_error = "Single '{' encountered in format string"
            idx += 1
            next_ch = it.__next__()

            if next_ch == "{":
                yield (string[fragment_begin:idx], None, None, None)
                end_error = None
                fragment_begin = idx + 1
                continue

            fragment = string[fragment_begin : idx - 1]

            ch = next_ch
            id_begin = idx
            end_error = "expected '}' before end of string"
            while ch != "}" and ch != "!" and ch != ":":
                if ch == "{":
                    raise ValueError("unexpected '{' in field name")
                if ch == "[":
                    while True:
                        idx += 1
                        ch = it.__next__()
                        if ch == "]":
                            break
                idx += 1
                ch = it.__next__()
            ident = string[id_begin:idx]

            if ch == "!":
                end_error = "end of string while looking for conversion specifier"
                idx += 1
                ch = it.__next__()
                end_error = "unmatched '{' in format spec"
                conversion = ch

                idx += 1
                ch = it.__next__()
                if ch != ":" and ch != "}":
                    raise ValueError("expected ':' after conversion specifier")
            else:
                conversion = None

            spec = ""
            if ch == ":":
                spec_begin = idx + 1
                end_error = "unmatched '{' in format spec"
                curly_count = 1
                while True:
                    idx += 1
                    ch = it.__next__()
                    if ch == "{":
                        curly_count += 1
                        continue
                    if ch == "}":
                        curly_count -= 1
                        if curly_count == 0:
                            spec = string[spec_begin:idx]
                            break

            yield (fragment, ident, spec, conversion)
            fragment_begin = idx + 1
            end_error = None
    except StopIteration:
        if end_error is not None:
            raise ValueError(end_error)
        # Make sure everyone called `idx += 1` before `it.__next__()`.
        assert idx == _str_len(string)

    if idx - fragment_begin > 0:
        yield (string[fragment_begin:idx], None, None, None)
