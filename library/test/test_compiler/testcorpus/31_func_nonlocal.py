# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
def foo():
    a = 1

    def bar():
        nonlocal a
        a = 2
