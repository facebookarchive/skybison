# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
class F:
    async def f(x):
        x: int = 42
# EXPECTED:
[
    ~SETUP_ANNOTATIONS(0),
]
