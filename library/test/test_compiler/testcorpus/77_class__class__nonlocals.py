# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
class Outer:
    class Inner:
        nonlocal __class__
        __class__ = 42
        def f():
             __class__
