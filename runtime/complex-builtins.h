/* Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com) */
#pragma once

#include "objects.h"

namespace py {

class Thread;

word complexHash(RawObject value);

void initializeComplexType(Thread* thread);

}  // namespace py
