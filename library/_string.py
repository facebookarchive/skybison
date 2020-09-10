#!/usr/bin/env python3

from _builtins import _str_from_str, _str_guard, _str_len


_empty_iterator = iter(())


class _SubscriptParser:
    def __init__(self, field_name, idx):
        self.field_name = field_name
        self.idx = idx

    def __iter__(self):
        return self

    def __next__(self):
        field_name = self.field_name
        if not field_name:
            raise StopIteration
        if field_name[0] != "[" and field_name[0] != ".":
            raise ValueError("Only '.' or '[' may follow ']' in format field specifier")
        if field_name[0] == ".":
            right_idx = find_arg_end(field_name[1:])
            subscript = (
                field_name[self.idx + 1 :]
                if right_idx == -1
                else field_name[self.idx + 1 : right_idx + 1]
            )
            if subscript == "":
                raise ValueError("Empty attribute in format string")
            self.field_name = "" if right_idx == -1 else field_name[right_idx + 1 :]
            self.idx = 0
            return (True, subscript)
        right_idx = field_name.find("]")
        if right_idx == -1:
            raise ValueError("Missing ']' in format string")
        subscript = field_name[self.idx + 1 : right_idx]
        if subscript == "":
            raise ValueError("Empty attribute in format string")
        self.field_name = field_name[right_idx + 1 :]
        self.idx = 0
        return (False, int(subscript) if subscript.isdigit() else subscript)


def find_arg_end(field_name):
    """
    Returns the index of the end of an attribute name or element index.
    field_name:   the field being looked up, e.g. "0.name"
                  or "lookup[3]"
    """
    dot_idx = field_name.find(".")
    lbrack_idx = field_name.find("[")
    if dot_idx == -1:
        return lbrack_idx
    if lbrack_idx == -1:
        return dot_idx
    return min(dot_idx, lbrack_idx)


def formatter_field_name_split(field_name):
    _str_guard(field_name)

    idx = find_arg_end(field_name)
    if idx != -1:
        field = field_name[:idx]
        if field.isdigit():
            field = int(field)
        elif not field:
            # In order to accept "[0]" as an implicit "0[0]", return 0 instead
            # of the empty string here. See https://bugs.python.org/issue39985
            # for more detail.
            field = 0
        return (field, _SubscriptParser(field_name[idx:], 0))

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
