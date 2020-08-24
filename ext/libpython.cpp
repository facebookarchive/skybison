#define PyAPI_FUNC_DECL(DECL)                                                  \
  extern "C" __attribute__((visibility("default"))) __attribute__((weak))      \
      DECL {}
#define _Py_NO_RETURN

#include "cpython-func.h"
