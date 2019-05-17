#include "function-builtins.h"

#include "frame.h"
#include "globals.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

RawObject functionFromMethodDef(Thread* thread, const char* c_name, void* meth,
                                const char* c_doc,
                                Function::ExtensionType flags) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(&scope, runtime->newFunction());
  Object name(&scope, runtime->newStrFromCStr(c_name));
  function.setName(*name);
  function.setCode(runtime->newIntFromCPtr(meth));
  if (c_doc != nullptr) {
    Object doc(&scope, runtime->newStrFromCStr(c_doc));
    function.setDoc(*doc);
  }
  switch (flags) {
    case Function::ExtensionType::kMethNoArgs:
      function.setEntry(methodTrampolineNoArgs);
      function.setEntryKw(methodTrampolineNoArgsKw);
      function.setEntryEx(methodTrampolineNoArgsEx);
      break;
    case Function::ExtensionType::kMethO:
      function.setEntry(methodTrampolineOneArg);
      function.setEntryKw(methodTrampolineOneArgKw);
      function.setEntryEx(methodTrampolineOneArgEx);
      break;
    case Function::ExtensionType::kMethVarArgs:
      function.setEntry(methodTrampolineVarArgs);
      function.setEntryKw(methodTrampolineVarArgsKw);
      function.setEntryEx(methodTrampolineVarArgsEx);
      break;
    case Function::ExtensionType::kMethVarArgsAndKeywords:
      function.setEntry(methodTrampolineKeywords);
      function.setEntryKw(methodTrampolineKeywordsKw);
      function.setEntryEx(methodTrampolineKeywordsEx);
      break;
    default:
      UNIMPLEMENTED("Unsupported MethodDef type");
  }
  return *function;
}

RawObject functionGetAttribute(Thread* thread, const Function& function,
                               const Object& name_str) {
  // TODO(T39611261): Figure out a way to skip dict init.
  // Initialize Dict if non-existent
  if (function.dict().isNoneType()) {
    function.setDict(thread->runtime()->newDict());
  }

  return objectGetAttribute(thread, function, name_str);
}

RawObject functionSetAttr(Thread* thread, const Function& function,
                          const Object& name_interned_str,
                          const Object& value) {
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInternedStr(thread, name_interned_str),
         "name must be an interned string");
  // Initialize Dict if non-existent
  HandleScope scope(thread);
  if (function.dict().isNoneType()) {
    function.setDict(runtime->newDict());
  }

  AttributeInfo info;
  Layout layout(&scope, runtime->layoutAt(function.layoutId()));
  if (runtime->layoutFindAttribute(thread, layout, name_interned_str, &info)) {
    // TODO(eelizondo): Handle __dict__ with descriptor
    return objectSetAttr(thread, function, name_interned_str, value);
  }
  Dict function_dict(&scope, function.dict());
  runtime->dictAtPut(thread, function_dict, name_interned_str, value);
  return NoneType::object();
}

const BuiltinMethod FunctionBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderGet, dunderGet},
    {SymbolId::kDunderGetattribute, dunderGetattribute},
    {SymbolId::kDunderSetattr, dunderSetattr},
    {SymbolId::kSentinelId, nullptr},
};

const BuiltinAttribute FunctionBuiltins::kAttributes[] = {
    {SymbolId::kDunderCode, RawFunction::kCodeOffset},
    {SymbolId::kDunderDoc, RawFunction::kDocOffset},
    {SymbolId::kDunderGlobals, RawFunction::kGlobalsOffset,
     AttributeFlags::kReadOnly},
    {SymbolId::kDunderModule, RawFunction::kModuleOffset},
    {SymbolId::kDunderName, RawFunction::kNameOffset},
    {SymbolId::kDunderQualname, RawFunction::kQualnameOffset},
    {SymbolId::kDunderDict, RawFunction::kDictOffset},
    {SymbolId::kSentinelId, -1},
};

void FunctionBuiltins::postInitialize(Runtime*, const Type& new_type) {
  HandleScope scope;
  Layout layout(&scope, new_type.instanceLayout());
  layout.setOverflowAttributes(SmallInt::fromWord(RawFunction::kDictOffset));
}

RawObject FunctionBuiltins::dunderGet(Thread* thread, Frame* frame,
                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!self.isFunction()) {
    return thread->raiseRequiresType(self, SymbolId::kFunction);
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

RawObject FunctionBuiltins::dunderGetattribute(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!self_obj.isFunction()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kFunction);
  }
  Function self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "attribute name must be string, not '%T'", &name);
  }
  Object result(&scope, functionGetAttribute(thread, self, name));
  if (result.isErrorNotFound()) {
    Object function_name(&scope, self.name());
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "function '%S' has no attribute '%S'",
                                &function_name, &name);
  }
  return *result;
}

RawObject FunctionBuiltins::dunderSetattr(Thread* thread, Frame* frame,
                                          word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!self_obj.isFunction()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kFunction);
  }
  Function self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "attribute name must be string, not '%T'", &name);
  }
  if (!name.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  name = runtime->internStr(thread, name);
  Object value(&scope, args.get(2));
  return functionSetAttr(thread, self, name, value);
}

}  // namespace python
