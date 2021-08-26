/* Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com) */
#pragma once

#include "globals.h"
#include "objects.h"

namespace py {

class Thread;

word arrayByteLength(RawArray array);

void initializeArrayType(Thread* thread);

}  // namespace py
