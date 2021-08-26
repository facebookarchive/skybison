# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
def foo():
    ann = None
    def bar(a: ann) -> ann:
        pass
