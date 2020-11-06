#include "ref-builtins.h"

#include "builtins.h"
#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kRefAttributes[] = {
    {ID(_ref__referent), RawWeakRef::kReferentOffset, AttributeFlags::kHidden},
    {ID(_ref__callback), RawWeakRef::kCallbackOffset, AttributeFlags::kHidden},
    {ID(_ref__link), RawWeakRef::kLinkOffset, AttributeFlags::kHidden},
    {ID(_ref__hash), RawWeakRef::kHashOffset, AttributeFlags::kHidden},
};

static const BuiltinAttribute kWeakLinkAttributes[] = {
    {ID(__weaklink__referent), RawWeakLink::kReferentOffset,
     AttributeFlags::kHidden},
    {ID(__weaklink__callback), RawWeakLink::kCallbackOffset,
     AttributeFlags::kHidden},
    {ID(__weaklink__link), RawWeakLink::kLinkOffset, AttributeFlags::kHidden},
    {ID(__weaklink__hash), RawWeakLink::kHashOffset, AttributeFlags::kHidden},
    {ID(__weaklink__next), RawWeakLink::kNextOffset, AttributeFlags::kHidden},
    {ID(__weaklink__prev), RawWeakLink::kPrevOffset, AttributeFlags::kHidden},
};

static const BuiltinAttribute kWeakCallableProxyAttributes[] = {
    {ID(ref_obj), RawWeakCallableProxy::kReferentOffset},
};

static const BuiltinAttribute kWeakProxyAttributes[] = {
    {ID(ref_obj), RawWeakProxy::kReferentOffset},
};

void initializeRefTypes(Thread* thread) {
  addBuiltinType(thread, ID(weakref), LayoutId::kWeakRef,
                 /*superclass_id=*/LayoutId::kObject, kRefAttributes,
                 WeakRef::kSize, /*basetype=*/true);

  addBuiltinType(thread, ID(weakcallableproxy), LayoutId::kWeakCallableProxy,
                 /*superclass_id=*/LayoutId::kObject,
                 kWeakCallableProxyAttributes, WeakCallableProxy::kSize,
                 /*basetype=*/false);

  addBuiltinType(thread, ID(weakproxy), LayoutId::kWeakProxy,
                 /*superclass_id=*/LayoutId::kObject, kWeakProxyAttributes,
                 WeakProxy::kSize, /*basetype=*/false);

  addBuiltinType(thread, ID(_weaklink), LayoutId::kWeakLink,
                 /*superclass_id=*/LayoutId::kObject, kWeakLinkAttributes,
                 WeakLink::kSize, /*basetype=*/false);
}

RawObject METH(weakref, __call__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfWeakRef(*self)) {
    return thread->raiseRequiresType(self, ID(weakref));
  }
  WeakRef ref(&scope, weakRefUnderlying(*self));
  return ref.referent();
}

RawObject METH(weakref, __new__)(Thread* thread, Arguments args) {
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
  WeakRef result(&scope, runtime->newWeakRef(thread, referent));
  if (type.isBuiltin()) {
    if (callback.isNoneType()) {
      result.setCallback(*callback);
      return *result;
    }
    result.setCallback(runtime->newBoundMethod(callback, result));
    return *result;
  }
  Layout layout(&scope, type.instanceLayout());
  UserWeakRefBase instance(&scope, runtime->newInstance(layout));
  instance.setValue(*result);
  if (callback.isNoneType()) {
    result.setCallback(*callback);
    return *instance;
  }
  result.setCallback(runtime->newBoundMethod(callback, instance));
  return *instance;
}
}  // namespace py
