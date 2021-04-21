#pragma once

#include "cpython-types.h"

#include "handles-decl.h"

namespace py {

struct ListEntry;

class Runtime;
class Thread;

PyObject* initializeExtensionObject(Thread* thread, PyObject* obj,
                                    PyTypeObject* typeobj,
                                    const Object& instance);

bool trackExtensionObject(Runtime* runtime, ListEntry* entry);

bool untrackExtensionObject(Runtime* runtime, ListEntry* entry);

}  // namespace py
