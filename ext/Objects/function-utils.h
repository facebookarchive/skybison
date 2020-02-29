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
static_assert(static_cast<int>(ExtensionMethodType::kMethFastCall) ==
                  METH_FASTCALL,
              "matching flag values");
static_assert(static_cast<int>(ExtensionMethodType::kMethFastCallAndKeywords) ==
                  (METH_FASTCALL | METH_KEYWORDS),
              "matching flag values");

inline ExtensionMethodType methodTypeFromMethodFlags(int flags) {
  int call_flags = flags & ~METH_CLASS & ~METH_STATIC;
  DCHECK(call_flags == METH_NOARGS || call_flags == METH_O ||
             call_flags == METH_VARARGS ||
             call_flags == (METH_VARARGS | METH_KEYWORDS) ||
             call_flags == METH_FASTCALL ||
             call_flags == (METH_FASTCALL | METH_KEYWORDS),
         "unexpected flags %x", flags);
  return static_cast<ExtensionMethodType>(call_flags);
}

RawObject newCFunction(Thread* thread, PyMethodDef* method, const Object& name,
                       const Object& self, const Object& module_name);

RawObject newMethod(Thread* thread, PyMethodDef* method, const Object& name,
                    const Object& type);

RawObject newClassMethod(Thread* thread, PyMethodDef* method,
                         const Object& name, const Object& type);

}  // namespace py
