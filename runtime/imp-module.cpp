#include "imp-module.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace python {

RawObject builtinImpAcquireLock(Thread* /* thread */, Frame* /* frame */,
                                word /* nargs */) {
  UNIMPLEMENTED("acquire_lock");
}

RawObject builtinImpCreateBuiltin(Thread* /* thread */, Frame* /* frame */,
                                  word /* nargs */) {
  UNIMPLEMENTED("create_builtin");
}

RawObject builtinImpExecBuiltin(Thread* /* thread */, Frame* /* frame */,
                                word /* nargs */) {
  UNIMPLEMENTED("exec_builtin");
}

RawObject builtinImpExecDynamic(Thread* /* thread */, Frame* /* frame */,
                                word /* nargs */) {
  UNIMPLEMENTED("exec_dynamic");
}

RawObject builtinImpExtensionSuffixes(Thread* /* thread */, Frame* /* frame */,
                                      word /* nargs */) {
  UNIMPLEMENTED("extension_suffixes");
}

RawObject builtinImpFixCoFilename(Thread* /* thread */, Frame* /* frame */,
                                  word /* nargs */) {
  UNIMPLEMENTED("_fix_co_filename");
}

RawObject builtinImpGetFrozenObject(Thread* /* thread */, Frame* /* frame */,
                                    word /* nargs */) {
  UNIMPLEMENTED("get_frozen_object");
}

RawObject builtinImpIsBuiltin(Thread* /* thread */, Frame* /* frame */,
                              word /* nargs */) {
  UNIMPLEMENTED("is_builtin");
}

RawObject builtinImpIsFrozen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "expected 1 argument, got %ld", nargs - 1));
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object name(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStr(name)) {
    return thread->raiseTypeErrorWithCStr("is_frozen requires a str object");
  }
  // Always return False
  return RawBool::falseObj();
}

RawObject builtinImpIsFrozenPackage(Thread* /* thread */, Frame* /* frame */,
                                    word /* nargs */) {
  UNIMPLEMENTED("is_frozen_package");
}

RawObject builtinImpReleaseLock(Thread* /* thread */, Frame* /* frame */,
                                word /* nargs */) {
  UNIMPLEMENTED("release_lock");
}

}  // namespace python
