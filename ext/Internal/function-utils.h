#pragma once

#include "cpython-data.h"

#include "runtime.h"

namespace py {

RawObject getExtensionFunction(RawObject object);

RawObject newCFunction(Thread* thread, PyMethodDef* method, const Object& name,
                       const Object& self, const Object& module_name);

RawObject newClassMethod(Thread* thread, PyMethodDef* method,
                         const Object& name, const Object& type);

RawObject newExtensionFunction(Thread* thread, const Object& name,
                               void* function, int flags);

RawObject newMethod(Thread* thread, PyMethodDef* method, const Object& name,
                    const Object& type);

}  // namespace py
