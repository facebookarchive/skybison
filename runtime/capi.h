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

struct PyModuleDef;

// TODO(T67311848): Remove this. This is a temporary workaround until we fork
// the readline module into the runtime.
extern "C" char* PyOS_Readline(FILE*, FILE*, const char*);

namespace py {

class HandleVisitor;
class PointerVisitor;
class Runtime;
class Scavenger;
class Thread;

static const word kCAPIStateSize = 256;

extern struct _inittab* PyImport_Inittab;

void disposeExtensionObjects(Runtime* runtime);

void disposeApiHandles(Runtime* runtime);

void finalizeCAPIModules();
void finalizeCAPIState(Runtime* runtime);

void finalizeExtensionObject(Thread* thread, RawObject object);

void freeExtensionModules(Thread* thread);

// Returns `true` if there is a built-in extension module with name `name`.
bool isBuiltinExtensionModule(const Str& name);

void initializeCAPIModules();
void initializeCAPIState(Runtime* runtime);

// Runs the executable functions found in the PyModuleDef
word moduleExecDef(Thread* thread, const Module& module, PyModuleDef* def);

// Initialize built-in extension module `name` if it exists, otherwise
// return `nullptr`.
RawObject moduleInitBuiltinExtension(Thread* thread, const Str& name);

// Load extension module `name` from dynamic library in file `path`.
RawObject moduleLoadDynamicExtension(Thread* thread, const Str& name,
                                     const Str& path);

word numApiHandles(Runtime* runtime);

word numExtensionObjects(Runtime* runtime);

RawObject objectGetMember(Thread* thread, RawObject ptr, RawObject name);

// Check if a borrowed reference to the object has a non-null cache.
// WARNING: This function should only be used in the GC.
bool objectHasHandleCache(Runtime* runtime, RawObject obj);

// Pin a handle for the object until the runtime exits.
// WARNING: This function should only be used in builtins.id()
void* objectNewReference(Runtime* runtime, RawObject obj);

void objectSetMember(Runtime* runtime, RawObject old_ptr, RawObject new_val);

// Return the type's tp_basicsize. Use only with extension types.
uword typeGetBasicSize(const Type& type);

// Return the either computed CPython flags based on Pyro type state or an
// extension type's tp_flags. Use with either managed types or extension types.
uword typeGetFlags(const Type& type);

// Type has a list of type slots attached to it. The type slots are used by the
// C-API emulation layer for C extension types.
bool typeHasSlots(const Type& type);

// Inherit slots defined by a C Extension
RawObject typeInheritSlots(Thread* thread, const Type& type, const Type& base);

// NOTE: THIS FUNCTION IS A HACK. It is slow. Do not use this function. It is
// here to serve Cython modules that occasionally create Python memoryviews
// from buffer protocol objects. It is much better practice to instead use
// builtin types where possible.
//
// Call bf_getbuffer, copy data into a Bytes, and call bf_releasebuffer.
// Assumes the object is not builtin.
// Raises TypeError if slots are not defined.
RawObject newBytesFromBuffer(Thread* thread, const Object& obj);

void visitExtensionObjects(Runtime* runtime, Scavenger* scavenger,
                           PointerVisitor* visitor);

// Calls `visitor->visitHandle` for all `ApiHandle`s.
void visitApiHandles(Runtime* runtime, HandleVisitor* visitor);

// Visits all `ApiHandle`s with `refcount > 1`. `ApiHandle`s with refcount zero
// are ignored here and will be handled by
// `visitNotIncrementedBorrowedApiHandles`.
void visitIncrementedApiHandles(Runtime* runtime, PointerVisitor* visitor);

// Should be called when all GC roots are processed and no gray objects remain.
// This disposes `ApiHandle`s with reference count 0 that are not referenced
// from live objects in the managed heap ("white" objects after GC tri-color
// marking).
void visitNotIncrementedBorrowedApiHandles(Runtime* runtime,
                                           Scavenger* scavenger,
                                           PointerVisitor* visitor);

}  // namespace py
