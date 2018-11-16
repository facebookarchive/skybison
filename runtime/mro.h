#pragma once

#include "handles.h"

namespace python {

class Object;
class Class;
class ObjectArray;
class Thread;

// Given the list of immediate parents, compute the complete MRO
// for the klass's ancestors via C3 linearization
Object* computeMro(Thread* thread, const Handle<Class>& klass,
                   const Handle<ObjectArray>& parents);

}  // namespace python
