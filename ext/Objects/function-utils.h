#pragma once

#include "cpython-data.h"
#include "function-builtins.h"

namespace py {

static_assert(static_cast<int>(ExtensionMethodType::kMethVarArgs) ==
                  METH_VARARGS,
              "matching flag values");
static_assert(static_cast<int>(ExtensionMethodType::kMethKeywords) ==
                  METH_KEYWORDS,
              "matching flag values");
static_assert(static_cast<int>(ExtensionMethodType::kMethVarArgsAndKeywords) ==
                  (METH_VARARGS | METH_KEYWORDS),
              "matching flag values");
static_assert(static_cast<int>(ExtensionMethodType::kMethNoArgs) == METH_NOARGS,
              "matching flag values");
static_assert(static_cast<int>(ExtensionMethodType::kMethO) == METH_O,
              "matching flag values");
static_assert(static_cast<int>(ExtensionMethodType::kMethClass) == METH_CLASS,
              "matching flag values");
static_assert(static_cast<int>(ExtensionMethodType::kMethStatic) == METH_STATIC,
              "matching flag values");
static_assert(static_cast<int>(ExtensionMethodType::kMethFastCall) ==
                  METH_FASTCALL,
              "matching flag values");
static_assert(static_cast<int>(ExtensionMethodType::kMethFastCallAndKeywords) ==
                  (METH_FASTCALL | METH_KEYWORDS),
              "matching flag values");

inline ExtensionMethodType methodTypeFromMethodFlags(int flag) {
  if (DCHECK_IS_ON()) {
    int call_flag = flag & ~METH_CLASS & ~METH_STATIC;
    DCHECK(call_flag == METH_NOARGS || call_flag == METH_O ||
               call_flag == METH_VARARGS ||
               call_flag == (METH_VARARGS | METH_KEYWORDS) ||
               call_flag == METH_FASTCALL ||
               call_flag == (METH_FASTCALL | METH_KEYWORDS),
           "unexpected flags %d", flag);
  }
  return static_cast<ExtensionMethodType>(flag);
}

}  // namespace py
