# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
def foo1(*args):
    func(args)

def foo2(**kwargs):
    func(kwargs)

def foo3(a, *args, **kw):
    func(a, args, kw)
