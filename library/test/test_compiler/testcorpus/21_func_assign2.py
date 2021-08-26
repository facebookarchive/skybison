# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
def foo():
    # This is runtime, not compile-time error
    a
    a = 2
