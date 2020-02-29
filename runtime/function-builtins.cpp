#include "function-builtins.h"

#include "builtins.h"
#include "dict-builtins.h"
#include "frame.h"
#include "globals.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"

namespace py {

const BuiltinAttribute FunctionBuiltins::kAttributes[] = {
    // TODO(T44845145) Support assignment to __code__.
    {ID(__code__), RawFunction::kCodeOffset, AttributeFlags::kReadOnly},
    {ID(__doc__), RawFunction::kDocOffset},
    {ID(__module__), RawFunction::kModuleNameOffset},
    {ID(__module_object__), RawFunction::kModuleObjectOffset},
    {ID(__name__), RawFunction::kNameOffset},
    {ID(__qualname__), RawFunction::kQualnameOffset},
    {ID(_function__dict), RawFunction::kDictOffset, AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

void FunctionBuiltins::postInitialize(Runtime*, const Type& new_type) {
  HandleScope scope;
  Layout layout(&scope, new_type.instanceLayout());
  layout.setDictOverflowOffset(RawFunction::kDictOffset);
}

RawObject METH(function, __get__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!self.isFunction()) {
    return thread->raiseRequiresType(self, ID(function));
  }
  Object instance(&scope, args.get(1));
  // When `instance is None` return the plain function because we are doing a
  // lookup on a class.
  if (instance.isNoneType()) {
    // The unfortunate exception to the rule is looking up a descriptor on the
    // `None` object itself. We make it work by always returning a bound method
    // when `type is type(None)` and special casing the lookup of attributes of
    // `type(None)` to skip `__get__` in `Runtime::classGetAttr()`.
    Type type(&scope, args.get(2));
    if (type.builtinBase() != LayoutId::kNoneType) {
      return *self;
    }
  }
  return thread->runtime()->newBoundMethod(self, instance);
}

const BuiltinAttribute BoundMethodBuiltins::kAttributes[] = {
    {ID(__func__), RawBoundMethod::kFunctionOffset, AttributeFlags::kReadOnly},
    {ID(__self__), RawBoundMethod::kSelfOffset, AttributeFlags::kReadOnly},
    {SymbolId::kSentinelId, 0},
};

}  // namespace py
