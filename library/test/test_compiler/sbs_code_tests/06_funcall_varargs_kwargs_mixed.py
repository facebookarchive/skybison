# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
fun(a, kw=1, *c, **d)
# EXPECTED:
[
    ...,
    BUILD_TUPLE(1),
    ...,
    BUILD_TUPLE_UNPACK_WITH_CALL(2),
    ...,
    BUILD_MAP(1),
    ...,
    BUILD_MAP_UNPACK_WITH_CALL(2),
    CALL_FUNCTION_EX(1),
    ...,
]
