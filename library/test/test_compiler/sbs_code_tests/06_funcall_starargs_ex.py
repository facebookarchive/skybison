# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
fun(*b, c)
# EXPECTED:
[
    ...,
    BUILD_TUPLE(1),
    BUILD_TUPLE_UNPACK_WITH_CALL(2),
    CALL_FUNCTION_EX(0),
    ...,
]
