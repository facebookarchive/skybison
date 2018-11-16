#include "imp-module.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"

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

RawObject builtinImpIsFrozen(Thread* /* thread */, Frame* /* frame */,
                             word /* nargs */) {
  UNIMPLEMENTED("is_frozen");
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
