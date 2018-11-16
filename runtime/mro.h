#pragma once

#include "handles.h"

namespace python {

class Object;
class Type;
class ObjectArray;
class Thread;

// Given the list of immediate parents, compute the complete MRO
// for the type's ancestors via C3 linearization
Object* computeMro(Thread* thread, const Handle<Type>& type,
                   const Handle<ObjectArray>& parents);

}  // namespace python
