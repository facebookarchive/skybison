#include "ref-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

const BuiltinAttribute RefBuiltins::kAttributes[] = {
    {SymbolId::kUnderReferent, WeakRef::kReferentOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod RefBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kSentinelId, nullptr},
};

RawObject RefBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 2 || nargs > 3) {
    return thread->raiseTypeErrorWithCStr("ref() expected 2 or 3 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object referent(&scope, args.get(1));
  Object callback(&scope, args.get(2));
  return thread->runtime()->newWeakref(thread, referent, callback);
}

}  // namespace python
