#!/usr/bin/env python3
_byteslike_compare_digest = _byteslike_compare_digest  # noqa: F821
_byteslike_guard = _byteslike_guard  # noqa: F821
_str_check = _str_check  # noqa: F821
_str_compare_digest = _str_compare_digest  # noqa: F821
_type = _type  # noqa: F821
_unimplemented = _unimplemented  # noqa: F821


def _compare_digest(a, b):
    if _str_check(a) and _str_check(b):
        return _str_compare_digest(a, b)
    _byteslike_guard(a)
    _byteslike_guard(b)
    return _byteslike_compare_digest(a, b)
