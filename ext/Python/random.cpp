#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"

#include "os.h"
#include "runtime.h"

namespace py {

PY_EXPORT int _PyOS_URandom(void* buffer, Py_ssize_t size) {
  // TODO(T41026101): use an interface that trades off not blocking for a
  // potentially higher quality source of random bytes.
  return _PyOS_URandomNonblock(buffer, size);
}

PY_EXPORT int _PyOS_URandomNonblock(void* buffer, Py_ssize_t size) {
  if (size < 0) {
    Thread::current()->raiseWithFmt(LayoutId::kValueError,
                                    "negative argument not allowed");
    return -1;
  }
  bool success = OS::secureRandom(static_cast<byte*>(buffer), size);
  return success ? 0 : -1;
}

}  // namespace py
