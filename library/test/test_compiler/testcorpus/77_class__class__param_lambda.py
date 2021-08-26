# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
class Outer:
    def x(self):
        def f(__class__):
            lambda: __class__
