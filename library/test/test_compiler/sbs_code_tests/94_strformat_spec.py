# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
def f(name, args):
    return f"foo.{name:0}"
# EXPECTED:
[
    ...,
    CODE_START('f'),
    LOAD_CONST('foo.'),
    ...,
    LOAD_CONST('0'),
    FORMAT_VALUE(4),
    BUILD_STRING(2),
    ...,
]
