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
    {ID(_function__annotations), RawFunction::kAnnotationsOffset,
     AttributeFlags::kHidden},
    {ID(_function__argcount), RawFunction::kArgcountOffset,
     AttributeFlags::kHidden},
    {ID(_function__caches), RawFunction::kCachesOffset,
     AttributeFlags::kHidden},
    {ID(_function__closure), RawFunction::kClosureOffset,
     AttributeFlags::kHidden},
    {ID(_function__defaults), RawFunction::kDefaultsOffset,
     AttributeFlags::kHidden},
    {ID(_function__dict), RawFunction::kDictOffset, AttributeFlags::kHidden},
    {ID(_function__entry), RawFunction::kEntryOffset, AttributeFlags::kHidden},
    {ID(_function__entry_ex), RawFunction::kEntryExOffset,
     AttributeFlags::kHidden},
    {ID(_function__entry_kw), RawFunction::kEntryKwOffset,
     AttributeFlags::kHidden},
    {ID(_function__flags), RawFunction::kFlagsOffset, AttributeFlags::kHidden},
    {ID(_function__kw_defaults), RawFunction::kKwDefaultsOffset,
     AttributeFlags::kHidden},
    {ID(_function__original_arguments), RawFunction::kOriginalArgumentsOffset,
     AttributeFlags::kHidden},
    {ID(_function__rewritten_bytecode), RawFunction::kRewrittenBytecodeOffset,
     AttributeFlags::kHidden},
    {ID(_function__stack_size), RawFunction::kStacksizeOffset,
     AttributeFlags::kHidden},
    {ID(_function__total_args), RawFunction::kTotalArgsOffset,
     AttributeFlags::kHidden},
    {ID(_function__total_vars), RawFunction::kTotalVarsOffset,
     AttributeFlags::kHidden},
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
