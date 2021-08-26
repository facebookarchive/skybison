# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
class Outer:
    def f(self):
        class Inner:
            nonlocal __class__
            __class__ = 42
            def f():
                 __class__
