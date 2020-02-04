#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

enum class ExtensionMethodType {
  kMethVarArgs = 1 << 0,
  kMethKeywords = 1 << 1,  // only used with METH_VARARGS or METH_FASTCALL
  kMethVarArgsAndKeywords = kMethVarArgs | kMethKeywords,
  kMethNoArgs = 1 << 2,
  kMethO = 1 << 3,
  kMethClass = 1 << 4,
  kMethStatic = 1 << 5,
  // We do not implement METH_COEXIST
  kMethFastCall = 1 << 7,
  kMethFastCallAndKeywords = kMethFastCall | kMethKeywords,
};

// Returns the subset of method types that determine how to call the method.
inline ExtensionMethodType callType(ExtensionMethodType type) {
  return static_cast<ExtensionMethodType>(
      static_cast<int>(type) &
      ~static_cast<int>(ExtensionMethodType::kMethClass) &
      ~static_cast<int>(ExtensionMethodType::kMethStatic));
}

inline bool isClassmethod(ExtensionMethodType type) {
  return static_cast<int>(type) &
         static_cast<int>(ExtensionMethodType::kMethClass);
}

inline bool isStaticmethod(ExtensionMethodType type) {
  return static_cast<int>(type) &
         static_cast<int>(ExtensionMethodType::kMethStatic);
}

RawObject functionFromMethodDef(Thread* thread, const char* c_name, void* meth,
                                const char* c_doc, ExtensionMethodType type);

RawObject functionFromModuleMethodDef(Thread* thread, const char* c_name,
                                      void* meth, const char* c_doc,
                                      ExtensionMethodType type);

class FunctionBuiltins
    : public Builtins<FunctionBuiltins, ID(function), LayoutId::kFunction> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FunctionBuiltins);
};

class BoundMethodBuiltins
    : public Builtins<BoundMethodBuiltins, ID(method), LayoutId::kBoundMethod> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BoundMethodBuiltins);
};

}  // namespace py
