# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
fun(a)
# EXPECTED:
[
    LOAD_NAME('fun'),
    LOAD_NAME('a'),
    CALL_FUNCTION( 1),
    ...,
]
