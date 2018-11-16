#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinTupleEq(Thread* thread, Frame* frame, word nargs);
Object* builtinTupleGetItem(Thread* thread, Frame* frame, word nargs);
Object* builtinTupleNew(Thread* thread, Frame* frame, word nargs);

Object* tupleSlice(Thread* thread, ObjectArray* tuple, Slice* slice);

}  // namespace python
