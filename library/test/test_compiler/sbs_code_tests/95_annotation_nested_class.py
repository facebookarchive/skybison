# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
class F:
    class D:
        x: int = 42
# EXPECTED:
[
    ~SETUP_ANNOTATIONS(0),
    LOAD_CONST('F.D'),
    ...,
    SETUP_ANNOTATIONS(0),
    ...
]
