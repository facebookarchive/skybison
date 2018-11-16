#pragma once

#include <iostream>

#include "globals.h"
#include "handles.h"

namespace python {

class Frame;
class Object;
class Thread;

// TODO: Remove this along with the iostream include once we have file-like
// objects. This is a side channel that allows us to override print's stdout
// during tests.
extern std::ostream* builtinPrintStream;

Object* builtinBuildClass(Thread* thread, Frame* frame, word nargs);
Object* builtinIsinstance(Thread* thread, Frame* frame, word nargs);

// CALL_FUNCTION entry point for print()
Object* builtinPrint(Thread* thread, Frame* frame, word nargs);
Object* builtinOrd(Thread* thread, Frame* frame, word nargs);
Object* builtinChr(Thread* thread, Frame* frame, word nargs);
Object* builtinLen(Thread* thread, Frame* frame, word nargs);

// CALL_FUNCTION_KW entry pointer for print()
Object* builtinPrintKw(Thread* thread, Frame* frame, word nargs);

Object* builtinRange(Thread* thread, Frame* frame, word nargs);

Object* builtinGenericNew(Thread* thread, Frame* frame, word nargs);

// List
Object* listOrDelegate(Thread* thread, const Handle<Object>& instance);
Object* builtinListNew(Thread* thread, Frame* frame, word nargs);
Object* builtinListAppend(Thread* thread, Frame* frame, word nargs);
Object* builtinListInsert(Thread* thread, Frame* frame, word nargs);
Object* builtinListPop(Thread* thread, Frame* frame, word nargs);
Object* builtinListRemove(Thread* thread, Frame* frame, word nargs);

// Descriptor
Object* functionDescriptorGet(
    Thread* thread,
    const Handle<Object>& self,
    const Handle<Object>& instance,
    const Handle<Object>& /* owner */) __attribute__((aligned(16)));

Object* classmethodDescriptorGet(
    Thread* thread,
    const Handle<Object>& self,
    const Handle<Object>& instance,
    const Handle<Object>& /* owner */) __attribute__((aligned(16)));

// ClassMethod
Object* builtinClassMethodNew(Thread* thread, Frame* frame, word nargs);
Object* builtinClassMethodInit(Thread* thread, Frame* frame, word nargs);

// Super
Object* builtinSuperNew(Thread* thread, Frame* frame, word nargs);
Object* builtinSuperInit(Thread* thread, Frame* frame, word nargs);

// Object
Object* builtinObjectInit(Thread*, Frame*, word);

// "sys" module
Object* builtinSysExit(Thread* thread, Frame* frame, word nargs);

} // namespace python
