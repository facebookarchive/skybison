# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
async def foo():
    l = {k:v  async for k, v in gen()}
    return [i for i in l]

