#pragma once

#include "globals.h"
#include "handles.h"

namespace python {

// Boolean
Object* builtinBooleanBool(Thread* thread, Frame* frame, word nargs);

// ClassMethod
Object* builtinClassMethodNew(Thread* thread, Frame* frame, word nargs);
Object* builtinClassMethodInit(Thread* thread, Frame* frame, word nargs);

// Dictionary
Object* builtinDictionaryEq(Thread* thread, Frame* frame, word nargs);
Object* builtinDictionaryLen(Thread* thread, Frame* frame, word nargs);

// Double
Object* builtinDoubleEq(Thread* thread, Frame* frame, word nargs);
Object* builtinDoubleGe(Thread* thread, Frame* frame, word nargs);
Object* builtinDoubleGt(Thread* thread, Frame* frame, word nargs);
Object* builtinDoubleLe(Thread* thread, Frame* frame, word nargs);
Object* builtinDoubleLt(Thread* thread, Frame* frame, word nargs);
Object* builtinDoubleNe(Thread* thread, Frame* frame, word nargs);

// List
Object* builtinListNew(Thread* thread, Frame* frame, word nargs);
Object* builtinListAppend(Thread* thread, Frame* frame, word nargs);
Object* builtinListInsert(Thread* thread, Frame* frame, word nargs);
Object* builtinListLen(Thread* thread, Frame* frame, word nargs);
Object* builtinListPop(Thread* thread, Frame* frame, word nargs);
Object* builtinListRemove(Thread* thread, Frame* frame, word nargs);

// Object
Object* builtinObjectInit(Thread* thread, Frame* caller, word nargs);
Object* builtinObjectNew(Thread* thread, Frame* caller, word nargs);

// Set
Object* builtinSetLen(Thread* thread, Frame* caller, word nargs);

// SmallInteger
Object* builtinSmallIntegerBool(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerEq(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerGe(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerGt(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerInvert(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerLe(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerLt(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerNe(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerNeg(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerPos(Thread* thread, Frame* frame, word nargs);

// Descriptor
Object* functionDescriptorGet(Thread* thread, Frame* frame, word nargs);
Object* classmethodDescriptorGet(Thread* thread, Frame* frame, word nargs);
Object* staticmethodDescriptorGet(Thread* thread, Frame* frame, word nargs);

// StaticMethod
Object* builtinStaticMethodNew(Thread* thread, Frame* frame, word nargs);
Object* builtinStaticMethodInit(Thread* thread, Frame* frame, word nargs);

// String
Object* builtinStringEq(Thread* thread, Frame* frame, word nargs);
Object* builtinStringGe(Thread* thread, Frame* frame, word nargs);
Object* builtinStringGt(Thread* thread, Frame* frame, word nargs);
Object* builtinStringLe(Thread* thread, Frame* frame, word nargs);
Object* builtinStringLt(Thread* thread, Frame* frame, word nargs);
Object* builtinStringNe(Thread* thread, Frame* frame, word nargs);

// Super
Object* builtinSuperNew(Thread* thread, Frame* frame, word nargs);
Object* builtinSuperInit(Thread* thread, Frame* frame, word nargs);

// Type
Object* builtinTypeCall(Thread* thread, Frame* caller, word nargs)
    __attribute__((aligned(16)));
Object* builtinTypeNew(Thread* thread, Frame* frame, word nargs);
Object* builtinTypeInit(Thread* thread, Frame* frame, word nargs);

// Tuple
Object* builtinTupleEq(Thread* thread, Frame* frame, word nargs);

} // namespace python
