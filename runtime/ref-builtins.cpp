#include "ref-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

const BuiltinAttribute RefBuiltins::kAttributes[] = {
    {ID(_ref__referent), WeakRef::kReferentOffset, AttributeFlags::kHidden},
    {ID(_ref__callback), WeakRef::kCallbackOffset, AttributeFlags::kHidden},
    {ID(_ref__link), WeakRef::kLinkOffset, AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod RefBuiltins::kBuiltinMethods[] = {
    {ID(__new__), dunderNew},
    {ID(__call__), dunderCall},
    {SymbolId::kSentinelId, nullptr},
};

RawObject RefBuiltins::dunderCall(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfWeakRef(*self)) {
    return thread->raiseRequiresType(self, ID(ref));
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

const BuiltinAttribute WeakLinkBuiltins::kAttributes[] = {
    {ID(__weaklink__next), WeakLink::kNextOffset, AttributeFlags::kHidden},
    {ID(__weaklink__prev), WeakLink::kPrevOffset, AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

}  // namespace py
