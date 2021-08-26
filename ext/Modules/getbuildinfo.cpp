// Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
#include "cpython-func.h"

#include "globals.h"
#include "version.h"

namespace py {

PY_EXPORT const char* Py_GetBuildInfo() { return kBuildInfo; }

}  // namespace py
