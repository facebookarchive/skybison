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
  HandleScope scope(thread);
  if (nargs < 2 || nargs > 3) {
    return thread->raiseTypeErrorWithCStr("ref() expected 2 or 3 arguments");
  }
  Arguments args(frame, nargs);
  WeakRef ref(&scope, thread->runtime()->newWeakRef());
  ref.setReferent(args.get(1));
  ref.setCallback(args.get(2));

  return *ref;
}

}  // namespace python
