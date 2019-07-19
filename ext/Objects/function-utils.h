#pragma once

#include "cpython-data.h"
#include "function-builtins.h"

namespace python {

static inline ExtensionMethodType methodTypeFromMethodFlags(int flag) {
  DCHECK(flag == METH_NOARGS || flag == METH_O || flag == METH_VARARGS ||
             flag == (METH_VARARGS | METH_KEYWORDS),
         "unexpected flags");
  static_assert(
      static_cast<int>(ExtensionMethodType::kMethNoArgs) == METH_NOARGS,
      "matching flag values");
  static_assert(static_cast<int>(ExtensionMethodType::kMethO) == METH_O,
                "matchinf flag values");
  static_assert(
      static_cast<int>(ExtensionMethodType::kMethVarArgs) == METH_VARARGS,
      "matching flag values");
  static_assert(
      static_cast<int>(ExtensionMethodType::kMethVarArgsAndKeywords) ==
          (METH_VARARGS | METH_KEYWORDS),
      "matching flag values");
  return static_cast<ExtensionMethodType>(flag);
}

}  // namespace python
