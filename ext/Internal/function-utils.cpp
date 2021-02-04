#include "function-utils.h"

#include "capi-trampolines.h"
#include "handles.h"
#include "runtime.h"

namespace py {

RawObject getExtensionFunction(RawObject object) {
  if (!object.isBoundMethod()) return Error::notFound();
  RawObject function_obj = BoundMethod::cast(object).function();
  if (!function_obj.isFunction()) return Error::notFound();
  RawFunction function = Function::cast(function_obj);
  if (!function.isExtension()) return Error::notFound();
  return function;
}

RawObject newCFunction(Thread* thread, PyMethodDef* method, const Object& name,
                       const Object& self, const Object& module_name) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(
      &scope, newExtensionFunction(thread, name,
                                   reinterpret_cast<void*>(method->ml_meth),
                                   method->ml_flags));
  if (method->ml_doc != nullptr) {
    function.setDoc(runtime->newStrFromCStr(method->ml_doc));
  }
  if (runtime->isInstanceOfStr(*module_name)) {
    function.setModuleName(*module_name);
  }
  return runtime->newBoundMethod(function, self);
}

RawObject newClassMethod(Thread* thread, PyMethodDef* method,
                         const Object& name, const Object& type) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(&scope, newExtensionFunction(
                                thread, name, bit_cast<void*>(method->ml_meth),
                                method->ml_flags));
  if (method->ml_doc != nullptr) {
    function.setDoc(runtime->newStrFromCStr(method->ml_doc));
  }
  Object result(
      &scope, thread->invokeFunction2(ID(builtins), ID(_descrclassmethod), type,
                                      function));
  DCHECK(!result.isError(), "failed to create classmethod descriptor");
  return *result;
}

RawObject newExtensionFunction(Thread* thread, const Object& name,
                               void* function, int flags) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function::Entry entry;
  Function::Entry entry_kw;
  Function::Entry entry_ex;
  flags &= ~METH_CLASS & ~METH_STATIC & ~METH_COEXIST;
  switch (flags) {
    case METH_NOARGS:
      entry = methodTrampolineNoArgs;
      entry_kw = methodTrampolineNoArgsKw;
      entry_ex = methodTrampolineNoArgsEx;
      break;
    case METH_O:
      entry = methodTrampolineOneArg;
      entry_kw = methodTrampolineOneArgKw;
      entry_ex = methodTrampolineOneArgEx;
      break;
    case METH_VARARGS:
      entry = methodTrampolineVarArgs;
      entry_kw = methodTrampolineVarArgsKw;
      entry_ex = methodTrampolineVarArgsEx;
      break;
    case METH_VARARGS | METH_KEYWORDS:
      entry = methodTrampolineKeywords;
      entry_kw = methodTrampolineKeywordsKw;
      entry_ex = methodTrampolineKeywordsEx;
      break;
    case METH_FASTCALL:
      entry = methodTrampolineFast;
      entry_kw = methodTrampolineFastKw;
      entry_ex = methodTrampolineFastEx;
      break;
    case METH_FASTCALL | METH_KEYWORDS:
      entry = methodTrampolineFastWithKeywords;
      entry_kw = methodTrampolineFastWithKeywordsKw;
      entry_ex = methodTrampolineFastWithKeywordsEx;
      break;
    default:
      UNIMPLEMENTED("Unsupported MethodDef type");
  }
  Object code(&scope, runtime->newIntFromCPtr(function));
  Object none(&scope, NoneType::object());
  return thread->runtime()->newFunction(
      thread, name, code, /*flags=*/Function::Flags::kExtension,
      /*argcount=*/-1, /*total_args=*/-1,
      /*total_vars=*/-1,
      /*stacksize_or_builtin=*/none, entry, entry_kw, entry_ex);
}

RawObject newMethod(Thread* thread, PyMethodDef* method, const Object& name,
                    const Object& /*type*/) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(&scope, newExtensionFunction(
                                thread, name, bit_cast<void*>(method->ml_meth),
                                method->ml_flags));
  if (method->ml_doc != nullptr) {
    function.setDoc(runtime->newStrFromCStr(method->ml_doc));
  }
  // TODO(T62932301): We currently return the plain function here which means
  // we do not check the `self` parameter to be a proper subtype of `type`.
  // Should we wrap this with a new descriptor type?
  return *function;
}

}  // namespace py
