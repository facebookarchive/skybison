# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
try:
    a
except Exc as b:
    b
except Exc2 as c:
    b

# Check that capturing vars are properly local
def foo():
    try:
        a
    except Exc as b:
        b
