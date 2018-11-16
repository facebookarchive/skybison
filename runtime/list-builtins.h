#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinListAdd(Thread* thread, Frame* frame, word nargs);
Object* builtinListAppend(Thread* thread, Frame* frame, word nargs);
Object* builtinListExtend(Thread* thread, Frame* frame, word nargs);
Object* builtinListGetItem(Thread* thread, Frame* frame, word nargs);
Object* builtinListInsert(Thread* thread, Frame* frame, word nargs);
Object* builtinListLen(Thread* thread, Frame* frame, word nargs);
Object* builtinListNew(Thread* thread, Frame* frame, word nargs);
Object* builtinListPop(Thread* thread, Frame* frame, word nargs);
Object* builtinListRemove(Thread* thread, Frame* frame, word nargs);

Object* listSlice(Thread* thread, List* list, Slice* slice);

}  // namespace python
