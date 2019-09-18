#include "ref-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

const BuiltinMethod RefBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderCall, dunderCall},
    {SymbolId::kSentinelId, nullptr},
};

RawObject RefBuiltins::dunderCall(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfWeakRef(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kRef);
  }
  return WeakRef::cast(*self).referent();
}

RawObject RefBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object referent(&scope, args.get(1));
  Object callback(&scope, args.get(2));
  return thread->runtime()->newWeakRef(thread, referent, callback);
}

}  // namespace python
