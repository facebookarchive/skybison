#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)

from _builtins import (
    _byteslike_compare_digest,
    _byteslike_guard,
    _str_check,
    _str_compare_digest,
)


def _compare_digest(a, b):
    if _str_check(a) and _str_check(b):
        return _str_compare_digest(a, b)
    _byteslike_guard(a)
    _byteslike_guard(b)
    return _byteslike_compare_digest(a, b)
