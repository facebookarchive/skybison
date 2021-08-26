# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
z: int = 5

# EXPECTED:
[
    SETUP_ANNOTATIONS(0),
    ...,
    LOAD_NAME('int'),
    STORE_ANNOTATION('z'),
    ...,
]
