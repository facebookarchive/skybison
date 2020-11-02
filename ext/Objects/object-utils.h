#pragma once

#include "cpython-data.h"

#include "runtime.h"

namespace py {

PyObject* initializeNativeProxy(Thread* thread, PyObject* obj,
                                PyTypeObject* typeobj, const Object& instance);
}  // namespace py
