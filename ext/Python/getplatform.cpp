// Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
#include "os.h"

namespace py {

PY_EXPORT const char* Py_GetPlatform() { return OS::name(); }

}  // namespace py
