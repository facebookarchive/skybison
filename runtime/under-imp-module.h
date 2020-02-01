#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

void importAcquireLock(Thread* thread);
bool importReleaseLock(Thread* thread);

RawObject FUNC(_imp, _create_dynamic)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_imp, acquire_lock)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_imp, create_builtin)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_imp, exec_builtin)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_imp, extension_suffixes)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_imp, is_builtin)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_imp, is_frozen)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(_imp, release_lock)(Thread* thread, Frame* frame, word nargs);

class UnderImpModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
};

}  // namespace py
