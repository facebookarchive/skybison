# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
def fun(n):
    [(x, [x + y for y in z]) for x in n]

# Also test lambda to ensure __qualname__'s are right
lambda n: [(x, [x + y for y in z]) for x in n]
