#include "strarray-builtins.h"

#include "runtime.h"
#include "str-builtins.h"

namespace python {

const BuiltinMethod StrArrayBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderStr, dunderStr},
    {SymbolId::kSentinelId, nullptr},
};

RawObject StrArrayBuiltins::dunderInit(Thread* thread, Frame* frame,
                                       word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isStrArray()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kUnderStrArray);
  }
  StrArray self(&scope, *self_obj);
  self.setNumItems(0);
  Object source(&scope, args.get(1));
  if (source.isUnbound()) {
    return NoneType::object();
  }
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*source)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "_strarray can only be initialized with str");
  }
  Str source_str(&scope, strUnderlying(thread, source));
  runtime->strArrayAddStr(thread, self, source_str);
  return NoneType::object();
}

RawObject StrArrayBuiltins::dunderNew(Thread* thread, Frame* frame,
                                      word nargs) {
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  DCHECK(args.get(0) == runtime->typeAt(LayoutId::kStrArray),
         "_strarray.__new__(X): X is not '_strarray'");
  return runtime->newStrArray();
}

RawObject StrArrayBuiltins::dunderStr(Thread* thread, Frame* frame,
                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isStrArray()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kUnderStrArray);
  }
  StrArray self(&scope, *self_obj);
  return thread->runtime()->strFromStrArray(self);
}

}  // namespace python
