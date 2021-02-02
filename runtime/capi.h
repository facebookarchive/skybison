#pragma once

#include "handles-decl.h"
#include "objects.h"

// This file contains all of the functions and data needed from the runtime to
// poke at C-extension internals. Ideally, the extension layer would sit on top
// of the runtime and be neatly insulated from it, but at least right now this
// is not possible. To avoid bringing extension types and internals directly
// into the runtime, we provide a bridge in the form of a small set of APIs.
//
// Please keep this list as small as possible. Think if you can get away with
// instead calling a Python-level function for your use-case, or if you really
// need a C-API bridge.

// from Include/longobject.h
extern "C" const unsigned char _PyLong_DigitValue[];  // NOLINT
// from Include/pyctype.h
extern "C" const unsigned int _Py_ctype_table[];  // NOLINT

struct PyModuleDef;

// TODO(T67311848): Remove this. This is a temporary workaround until we fork
// the readline module into the runtime.
extern "C" char* PyOS_Readline(FILE*, FILE*, const char*);

namespace py {

struct CAPIState;

class PointerVisitor;

class Thread;

class Runtime;

static const word kCAPIStateSize = 256;

extern struct _inittab _PyImport_Inittab[];

// Return the object referenced by the handle.
// WARNING: This function should be called by the garbage collector.
RawObject capiHandleAsObject(void* handle);

// Return true if the extension object at the given handle is being kept alive
// by a reference from another extension object or by a managed reference.
// WARNING: This function should be called by the garbage collector.
bool capiHandleFinalizableReference(void* handle, RawObject** out);

// WARNING: This function should be called by the garbage collector.
void capiHandlesClearNotReferenced(Runtime* runtime);

// WARNING: This function should be called for shutdown.
void capiHandlesDispose(Runtime* runtime);

// WARNING: This function should be called during garbage collection.
void capiHandlesShrink(Runtime* runtime);

void capiStateVisit(CAPIState* state, PointerVisitor* visitor);

void finalizeCAPIModules();
void finalizeCAPIState(Runtime* runtime);

void freeExtensionModules(Thread* thread);

// Returns `true` if there is a built-in extension module with name `name`.
bool isBuiltinExtensionModule(const Str& name);

void initializeCAPIModules();
void initializeCAPIState(CAPIState* state);

// Method trampolines
RawObject methodTrampolineNoArgs(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineNoArgsKw(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineNoArgsEx(Thread* thread, word flags) ALIGN_16;

RawObject methodTrampolineOneArg(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineOneArgKw(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineOneArgEx(Thread* thread, word flags) ALIGN_16;

RawObject methodTrampolineVarArgs(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineVarArgsKw(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineVarArgsEx(Thread* thread, word flags) ALIGN_16;

RawObject methodTrampolineKeywords(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineKeywordsKw(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineKeywordsEx(Thread* thread, word flags) ALIGN_16;

RawObject methodTrampolineFast(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineFastKw(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineFastEx(Thread* thread, word flags) ALIGN_16;

RawObject methodTrampolineFastWithKeywords(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineFastWithKeywordsKw(Thread* thread,
                                             word nargs) ALIGN_16;
RawObject methodTrampolineFastWithKeywordsEx(Thread* thread,
                                             word flags) ALIGN_16;

// Runs the executable functions found in the PyModuleDef
word moduleExecDef(Thread* thread, const Module& module, PyModuleDef* def);

// Initialize built-in extension module `name` if it exists, otherwise
// return `nullptr`.
RawObject moduleInitBuiltinExtension(Thread* thread, const Str& name);

// Load extension module `name` from dynamic library in file `path`.
RawObject moduleLoadDynamicExtension(Thread* thread, const Str& name,
                                     const Str& path);

word numTrackedApiHandles(Runtime* runtime);

// Return a borrowed reference to the object.
void* objectBorrowedReference(Thread* thread, RawObject obj);

RawObject objectGetMember(Thread* thread, RawObject ptr, RawObject name);

// Check if a borrowed reference to the object has a non-null cache.
// WARNING: This function should only be used in the GC.
bool objectHasHandleCache(Thread* thread, RawObject obj);

// Pin a handle for the object until the runtime exits.
// WARNING: This function should only be used in builtins.id()
void* objectNewReference(Thread* thread, RawObject obj);

void objectSetMember(Thread* thread, RawObject old_ptr, RawObject new_val);

// Return the type's tp_basicsize. Use only with extension types.
uword typeGetBasicSize(const Type& type);

// Return the either computed CPython flags based on Pyro type state or an
// extension type's tp_flags. Use with either managed types or extension types.
uword typeGetFlags(const Type& type);

// Type has a list of type slots attached to it. The type slots are used by the
// C-API emulation layer for C extension types.
bool typeHasSlots(const Type& type);

// Inherit slots defined by a C Extension
RawObject typeInheritSlots(Thread* thread, const Type& type);

}  // namespace py
