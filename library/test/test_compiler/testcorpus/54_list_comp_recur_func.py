# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
def recur1(a):
    return [recur1(b) for b in a]
