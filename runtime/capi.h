#pragma once

#include "handles-decl.h"

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

// TODO(T67311848): Remove this. This is a temporary workaround until we fork
// the readline module into the runtime.
extern "C" char* PyOS_Readline(FILE*, FILE*, const char*);

namespace py {

struct CAPIState;

class IdentityDict;

class PointerVisitor;

class Runtime;

static const word kCAPIStateSize = 128;

extern struct _inittab _PyImport_Inittab[];

IdentityDict* capiCaches(Runtime* runtime);
IdentityDict* capiHandles(Runtime* runtime);

void capiStateVisit(CAPIState* state, PointerVisitor* visitor);

void finalizeCAPIModules();
void finalizeCAPIState(Runtime* runtime);

// Returns `true` if there is a built-in extension module with name `name`.
bool isBuiltinExtensionModule(const Str& name);

void initializeCAPIModules();
void initializeCAPIState(Runtime* runtime);

// Initialize built-in extension module `name` if it exists, otherwise
// return `nullptr`.
RawObject moduleInitBuiltinExtension(Thread* thread, const Str& name);

// Load extension module `name` from dynamic library in file `path`.
RawObject moduleLoadDynamicExtension(Thread* thread, const Str& name,
                                     const Str& path);

word numTrackedApiHandles(Runtime* runtime);

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
