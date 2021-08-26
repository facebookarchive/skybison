# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
fun(var, 10, a=a, kw=2)
# EXPECTED:
[
    ...,
    LOAD_CONST(('a', 'kw')),
    CALL_FUNCTION_KW(4),
    ...,
]
