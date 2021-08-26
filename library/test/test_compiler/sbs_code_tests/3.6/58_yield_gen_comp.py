# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
((yield 42) for i in gen())
# EXPECTED:
[
    ...,
    LOAD_CONST(42),
    YIELD_VALUE(0),
    YIELD_VALUE(0),
    ...
]
