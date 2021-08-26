# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
{1, *[1,2,3], *[4,5,6]}
# EXPECTED:
[
    LOAD_CONST(1),
    BUILD_SET(1),
    ...,
    BUILD_SET_UNPACK(3),
    ...
]
