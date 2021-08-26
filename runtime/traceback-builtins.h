/* Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com) */
#pragma once

#include "runtime.h"

namespace py {

void initializeTracebackType(Thread* thread);

RawObject tracebackWrite(Thread* thread, const Traceback& traceback,
                         const Object& file);

}  // namespace py
