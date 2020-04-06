#include "ref-builtins.h"

#include "builtins.h"
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

RawObject METH(weakref, __call__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfWeakRef(*self)) {
    return thread->raiseRequiresType(self, ID(weakref));
  }
  WeakRef ref(&scope, weakRefUnderlying(*self));
  return ref.referent();
}

RawObject METH(weakref, __new__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object type_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  Type type(&scope, *type_obj);
  if (type.builtinBase() != LayoutId::kWeakRef) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "not a subtype of weakref");
  }
  Object referent(&scope, args.get(1));
  Object callback(&scope, args.get(2));
  WeakRef result(&scope, runtime->newWeakRef(thread, referent, callback));
  if (type.isBuiltin()) {
    return *result;
  }
  Layout layout(&scope, type.instanceLayout());
  UserWeakRefBase instance(&scope, runtime->newInstance(layout));
  instance.setValue(*result);
  return *instance;
}

const BuiltinAttribute WeakLinkBuiltins::kAttributes[] = {
    {ID(__weaklink__next), WeakLink::kNextOffset, AttributeFlags::kHidden},
    {ID(__weaklink__prev), WeakLink::kPrevOffset, AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

}  // namespace py
