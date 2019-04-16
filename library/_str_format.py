#!/usr/bin/env python3
"""This is an internal module that contains functions used for str.format().
The entry point is format()."""

_unimplemented = _unimplemented  # noqa: F821


def _parse_replacement_fields(s):
    """Parse a string into a list of substrings where replcement fields
    are removed, and also a list of replacement fields.

    For example, given string "a{0}b{{c}}d{1}e{kw}f",
    return (['a', None, 'b{{c}}d', None, 'e', None, 'f'], [0, 1, 'kw']).

    The first list marks replacement fields with None, and the second list
    contains replacement fields of an appropriate type: a positional
    field is converted into an int, and a keyword remains a str.
    """
    fragments = []
    fields = []
    i = 0
    field_start = -1
    field_end = -1
    len_s = len(s)
    while i < len_s:
        c = s[i]
        if c == "{":
            # If { has already started, disallow until it's closed.
            if field_start != -1:
                raise ValueError("unexpected '{' in field name")
            # {{ escape.
            next_c = s[i + 1] if i + 1 < len_s else None
            if next_c == "{":
                fragments.append(s[field_end + 1 : i])
                fragments.append("{")
                field_end = i + 1
                i += 2
                continue
            # { hasn't started yet and not escaped.
            fragments.append(s[field_end + 1 : i])
            field_start = i
            field_end = -1
            i += 1
            continue
        if c == "}":
            # { has started, then eagerly close it.
            if field_start != -1:
                fields.append(s[field_start + 1 : i])
                fragments.append(None)
                field_start = -1
                field_end = i
                i += 1
                continue
            # }} escape.
            next_c = s[i + 1] if i + 1 < len_s else None
            if next_c == "}":
                fragments.append(s[field_end + 1 : i])
                fragments.append("}")
                field_end = i + 1
                i += 2
                continue
            # { hasn't started and the currently } is not escaped.
            raise ValueError("Single '}' encountered in format string")
        i += 1

    if field_start != -1:
        raise ValueError("Single '{' encountered in format string")
    if field_end + 1 < len_s:
        fragments.append(s[field_end + 1 : len_s])

    return fragments, _parse_fields(fields)


def _parse_fields(fields):
    is_auto_num_field = "" in fields
    auto_num_field_index = 0
    for i in range(len(fields)):
        field = fields[i]
        # TODO(T42620257): Implement full format_spec. Currently only
        # {int} and {keyword} are supported.
        if (
            field.__contains__("!")
            or field.__contains__(".")
            or field.__contains__(":")
        ):
            _unimplemented()
        index_field = None
        try:
            index_field = int(field)
            if index_field < 0:
                index_field = None
        except ValueError:
            pass
        if index_field is not None:
            if is_auto_num_field:
                raise ValueError(
                    "cannot switch from automatic field numbering to manual field "
                    "specification"
                )
            fields[i] = index_field
            continue
        if is_auto_num_field and field == "":
            fields[i] = auto_num_field_index
            auto_num_field_index += 1
    return fields


def str_format(s, pos_tuple, keyword_dict):
    res, fields = _parse_replacement_fields(s)
    field_cnt = 0
    for i in range(len(res)):
        if res[i] is not None:
            continue
        field_key = fields[field_cnt]
        field_cnt += 1
        # TODO(djang): A field should be an object that
        # performs formattting itself.
        if isinstance(field_key, int):
            field_value = pos_tuple[field_key]
        else:
            field_value = keyword_dict[field_key]
        res[i] = format(field_value, "")
    return "".join(res)
