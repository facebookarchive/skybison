#pragma once

#include "globals.h"
#include "runtime.h"

namespace python {

// Internal equivalent to PyErr_GivenExceptionMatches(): Return whether or not
// given is a subtype of any of the BaseException subtypes in exc, which may
// contain arbitrarily nested tuples.
bool givenExceptionMatches(Thread* thread, const Object& given,
                           const Object& exc);

// Create an exception of the given type, which should derive from
// BaseException. If value is None, no arguments will be passed to the
// constructor, if value is a tuple, it will be unpacked as arguments, and
// otherwise it will be the single argument.
RawObject createException(Thread* thread, const Type& type,
                          const Object& value);

// Internal equivalent to PyErr_NormalizeException(): If exc is a Type subtype,
// ensure that value is an instance of it (or a subtype). If a new exception
// with a traceback is raised during normalization traceback will be set to the
// new traceback.
void normalizeException(Thread* thread, Object* exc, Object* value,
                        Object* traceback);

class BaseExceptionBuiltins
    : public Builtins<BaseExceptionBuiltins, SymbolId::kBaseException,
                      LayoutId::kBaseException> {
 public:
  static RawObject dunderInit(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BaseExceptionBuiltins);
};

class StopIterationBuiltins
    : public Builtins<StopIterationBuiltins, SymbolId::kStopIteration,
                      LayoutId::kStopIteration, LayoutId::kException> {
 public:
  static RawObject dunderInit(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StopIterationBuiltins);
};

class SystemExitBuiltins
    : public Builtins<SystemExitBuiltins, SymbolId::kSystemExit,
                      LayoutId::kSystemExit, LayoutId::kBaseException> {
 public:
  static RawObject dunderInit(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SystemExitBuiltins);
};

class ImportErrorBuiltins
    : public Builtins<ImportErrorBuiltins, SymbolId::kImportError,
                      LayoutId::kImportError, LayoutId::kException> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ImportErrorBuiltins);
};

}  // namespace python
