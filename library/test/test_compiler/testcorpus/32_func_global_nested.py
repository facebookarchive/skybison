# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
def foo():
    global bar
    def bar():
        pass
