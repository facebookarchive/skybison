# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
async def foo():
    l = [i async for i in gen()]
    return [i for i in l]

