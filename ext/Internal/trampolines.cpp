#include "capi-handles.h"
#include "capi.h"
#include "dict-builtins.h"
#include "runtime.h"
#include "thread.h"

namespace py {

// method no args

static RawObject callMethNoArgs(Thread* thread, const Function& function,
                                const Object& self) {
  HandleScope scope(thread);
  Int address(&scope, function.code());
  binaryfunc method = bit_cast<binaryfunc>(address.asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* pyresult = (*method)(self_obj, nullptr);
  Object result(&scope, ApiHandle::checkFunctionResult(thread, pyresult));
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  return *result;
}

static RawObject raiseTypeErrorMustBeBound(Thread* thread,
                                           const Function& function) {
  HandleScope scope(thread);
  Str function_name(&scope, function.name());
  return thread->raiseWithFmt(
      LayoutId::kTypeError, "'%S' must be bound to an object", &function_name);
}

static RawObject raiseTypeErrorNoArguments(Thread* thread,
                                           const Function& function,
                                           word nargs) {
  HandleScope scope(thread);
  Str function_name(&scope, function.name());
  if (nargs == 0) {
    return raiseTypeErrorMustBeBound(thread, function);
  }
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "'%S' takes no arguments (%w given)",
                              &function_name, nargs - 1);
}

static RawObject raiseTypeErrorNoKeywordArguments(Thread* thread,
                                                  const Function& function) {
  HandleScope scope(thread);
  Str function_name(&scope, function.name());
  return thread->raiseWithFmt(
      LayoutId::kTypeError, "'%S' takes no keyword arguments", &function_name);
}

RawObject methodTrampolineNoArgs(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs));
  if (nargs != 1) {
    RawObject result = raiseTypeErrorNoArguments(thread, function, nargs);
    thread->stackDrop(nargs + 1);
    return result;
  }
  Object self(&scope, thread->stackPeek(0));
  RawObject result = callMethNoArgs(thread, function, self);
  thread->stackDrop(nargs + 1);
  return result;
}

RawObject methodTrampolineNoArgsKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs + 1));
  Tuple kw_names(&scope, thread->stackPeek(0));
  if (kw_names.length() != 0) {
    RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }
  if (nargs != 1) {
    RawObject result = raiseTypeErrorNoArguments(thread, function, nargs);
    thread->stackDrop(nargs + 2);
    return result;
  }
  Object self(&scope, thread->stackPeek(1));
  RawObject result = callMethNoArgs(thread, function, self);
  thread->stackDrop(nargs + 2);
  return result;
}

RawObject methodTrampolineNoArgsEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  Function function(&scope, thread->stackPeek(has_varkeywords + 1));
  Tuple args(&scope, thread->stackPeek(has_varkeywords));
  if (has_varkeywords) {
    Object kw_args(&scope, thread->stackTop());
    if (!kw_args.isDict()) UNIMPLEMENTED("mapping kwargs");
    if (Dict::cast(*kw_args).numItems() != 0) {
      RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
      thread->stackDrop(has_varkeywords + 2);
      return result;
    }
  }
  word args_length = args.length();
  if (args_length != 1) {
    RawObject result = raiseTypeErrorNoArguments(thread, function, args_length);
    thread->stackDrop(has_varkeywords + 2);
    return result;
  }
  Object self(&scope, args.at(0));
  RawObject result = callMethNoArgs(thread, function, self);
  thread->stackDrop(has_varkeywords + 2);
  return result;
}

// method one arg

static RawObject raiseTypeErrorOneArgument(Thread* thread,
                                           const Function& function,
                                           word nargs) {
  HandleScope scope(thread);
  Str function_name(&scope, function.name());
  if (nargs == 0) {
    return raiseTypeErrorMustBeBound(thread, function);
  }
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "'%S' takes exactly one argument (%w given)",
                              &function_name, nargs - 1);
}

static RawObject callMethOneArg(Thread* thread, const Function& function,
                                const Object& self, const Object& arg) {
  HandleScope scope(thread);
  Int address(&scope, function.code());
  binaryfunc method = bit_cast<binaryfunc>(address.asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* arg_obj = ApiHandle::newReference(thread, *arg);
  PyObject* pyresult = (*method)(self_obj, arg_obj);
  Object result(&scope, ApiHandle::checkFunctionResult(thread, pyresult));
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  ApiHandle::fromPyObject(arg_obj)->decref();
  return *result;
}

RawObject methodTrampolineOneArg(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs));
  if (nargs != 2) {
    RawObject result = raiseTypeErrorOneArgument(thread, function, nargs);
    thread->stackDrop(nargs + 1);
    return result;
  }
  Object self(&scope, thread->stackPeek(1));
  Object arg(&scope, thread->stackPeek(0));
  RawObject result = callMethOneArg(thread, function, self, arg);
  thread->stackDrop(nargs + 1);
  return result;
}

RawObject methodTrampolineOneArgKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs + 1));
  Tuple kw_names(&scope, thread->stackPeek(0));
  if (kw_names.length() != 0) {
    RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }
  if (nargs != 2) {
    RawObject result = raiseTypeErrorOneArgument(thread, function, nargs);
    thread->stackDrop(nargs + 2);
    return result;
  }
  Object self(&scope, thread->stackPeek(2));
  Object arg(&scope, thread->stackPeek(1));
  RawObject result = callMethOneArg(thread, function, self, arg);
  thread->stackDrop(nargs + 2);
  return result;
}

RawObject methodTrampolineOneArgEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  Function function(&scope, thread->stackPeek(has_varkeywords + 1));
  if (has_varkeywords) {
    Object kw_args(&scope, thread->stackTop());
    if (!kw_args.isDict()) UNIMPLEMENTED("mapping kwargs");
    if (Dict::cast(*kw_args).numItems() != 0) {
      RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
      thread->stackDrop(has_varkeywords + 2);
      return result;
    }
  }
  Tuple varargs(&scope, thread->stackPeek(has_varkeywords));
  if (varargs.length() != 2) {
    RawObject result =
        raiseTypeErrorOneArgument(thread, function, varargs.length());
    thread->stackDrop(has_varkeywords + 2);
    return result;
  }
  Object self(&scope, varargs.at(0));
  Object arg(&scope, varargs.at(1));
  RawObject result = callMethOneArg(thread, function, self, arg);
  thread->stackDrop(has_varkeywords + 2);
  return result;
}

// callMethVarArgs

static RawObject callMethVarArgs(Thread* thread, const Function& function,
                                 const Object& self, const Object& varargs) {
  HandleScope scope(thread);
  Int address(&scope, function.code());
  binaryfunc method = bit_cast<binaryfunc>(address.asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* varargs_obj = ApiHandle::newReference(thread, *varargs);
  PyObject* pyresult = (*method)(self_obj, varargs_obj);
  Object result(&scope, ApiHandle::checkFunctionResult(thread, pyresult));
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  ApiHandle::fromPyObject(varargs_obj)->decref();
  return *result;
}

RawObject methodTrampolineVarArgs(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs));
  if (nargs == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 1);
    return result;
  }
  Object self(&scope, thread->stackPeek(nargs - 1));
  Tuple varargs(&scope, thread->runtime()->newTuple(nargs - 1));
  for (word i = 0; i < nargs - 1; i++) {
    varargs.atPut(nargs - i - 2, thread->stackPeek(i));
  }
  RawObject result = callMethVarArgs(thread, function, self, varargs);
  thread->stackDrop(nargs + 1);
  return result;
}

RawObject methodTrampolineVarArgsKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs + 1));
  Tuple kw_names(&scope, thread->stackPeek(0));
  if (kw_names.length() != 0) {
    RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }
  if (nargs == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }
  Object self(&scope, thread->stackPeek(nargs));
  Tuple varargs(&scope, thread->runtime()->newTuple(nargs - 1));
  for (word i = 0; i < nargs - 1; i++) {
    varargs.atPut(i, thread->stackPeek(nargs - i - 1));
  }
  RawObject result = callMethVarArgs(thread, function, self, varargs);
  thread->stackDrop(nargs + 2);
  return result;
}

RawObject methodTrampolineVarArgsEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  Function function(&scope, thread->stackPeek(has_varkeywords + 1));
  if (has_varkeywords) {
    Object kw_args(&scope, thread->stackTop());
    if (!kw_args.isDict()) UNIMPLEMENTED("mapping kwargs");
    if (Dict::cast(*kw_args).numItems() != 0) {
      RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
      thread->stackDrop(has_varkeywords + 2);
      return result;
    }
  }
  Tuple args(&scope, thread->stackPeek(has_varkeywords));
  if (args.length() == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(has_varkeywords + 2);
    return result;
  }
  Object self(&scope, args.at(0));
  Object varargs(&scope, thread->runtime()->tupleSubseq(thread, args, 1,
                                                        args.length() - 1));
  RawObject result = callMethVarArgs(thread, function, self, varargs);
  thread->stackDrop(has_varkeywords + 2);
  return result;
}

// callMethKeywordArgs

static RawObject callMethKeywords(Thread* thread, const Function& function,
                                  const Object& self, const Object& args,
                                  const Object& kwargs) {
  HandleScope scope(thread);
  Int address(&scope, function.code());
  ternaryfunc method = bit_cast<ternaryfunc>(address.asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* args_obj = ApiHandle::newReference(thread, *args);
  PyObject* kwargs_obj = nullptr;
  if (*kwargs != NoneType::object()) {
    kwargs_obj = ApiHandle::newReference(thread, *kwargs);
  }
  PyObject* pyresult = (*method)(self_obj, args_obj, kwargs_obj);
  Object result(&scope, ApiHandle::checkFunctionResult(thread, pyresult));
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  ApiHandle::fromPyObject(args_obj)->decref();
  if (kwargs_obj != nullptr) {
    ApiHandle::fromPyObject(kwargs_obj)->decref();
  }
  return *result;
}

RawObject methodTrampolineKeywords(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(&scope, thread->stackPeek(nargs));
  if (nargs == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 1);
    return result;
  }
  Object self(&scope, thread->stackPeek(nargs - 1));
  Tuple varargs(&scope, runtime->newTuple(nargs - 1));
  for (word i = 0; i < nargs - 1; i++) {
    varargs.atPut(nargs - i - 2, thread->stackPeek(i));
  }
  Object keywords(&scope, NoneType::object());
  RawObject result =
      callMethKeywords(thread, function, self, varargs, keywords);
  thread->stackDrop(nargs + 1);
  return result;
}

RawObject methodTrampolineKeywordsKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple kw_names(&scope, thread->stackPeek(0));
  Object kwargs(&scope, NoneType::object());
  word num_keywords = kw_names.length();
  if (num_keywords != 0) {
    Dict dict(&scope, runtime->newDict());
    for (word i = 0; i < num_keywords; i++) {
      Str name(&scope, kw_names.at(i));
      Object value(&scope, thread->stackPeek(num_keywords - i));
      dictAtPutByStr(thread, dict, name, value);
    }
    kwargs = *dict;
  }
  Function function(&scope, thread->stackPeek(nargs + 1));
  if (nargs - num_keywords == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }
  word num_positional = nargs - num_keywords - 1;
  Tuple args(&scope, runtime->newTuple(num_positional));
  for (word i = 0; i < num_positional; i++) {
    args.atPut(i, thread->stackPeek(nargs - i - 1));
  }
  Object self(&scope, thread->stackPeek(nargs));
  RawObject result = callMethKeywords(thread, function, self, args, kwargs);
  thread->stackDrop(nargs + 2);
  return result;
}

RawObject methodTrampolineKeywordsEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  Tuple varargs(&scope, thread->stackPeek(has_varkeywords));
  Object kwargs(&scope, NoneType::object());
  if (has_varkeywords) {
    kwargs = thread->stackTop();
    if (!kwargs.isDict()) UNIMPLEMENTED("mapping kwargs");
  }
  Function function(&scope, thread->stackPeek(has_varkeywords + 1));
  if (varargs.length() == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(has_varkeywords + 2);
    return result;
  }
  Object self(&scope, varargs.at(0));
  Object args(&scope, thread->runtime()->tupleSubseq(thread, varargs, 1,
                                                     varargs.length() - 1));
  RawObject result = callMethKeywords(thread, function, self, args, kwargs);
  thread->stackDrop(has_varkeywords + 2);
  return result;
}

static RawObject callMethFast(Thread* thread, const Function& function,
                              const Object& self, PyObject* const* args,
                              word num_args) {
  _PyCFunctionFast method =
      bit_cast<_PyCFunctionFast>(Int::cast(function.code()).asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* pyresult = (*method)(self_obj, args, num_args);
  RawObject result = ApiHandle::checkFunctionResult(thread, pyresult);
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  return result;
}

RawObject methodTrampolineFast(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs));
  if (nargs == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 1);
    return result;
  }
  Object self(&scope, thread->stackPeek(nargs - 1));

  word num_positional = nargs - 1;
  std::unique_ptr<PyObject*[]> fastcall_args(new PyObject*[num_positional]);
  for (word i = 0; i < num_positional; i++) {
    fastcall_args[nargs - i - 2] =
        ApiHandle::newReference(thread, thread->stackPeek(i));
  }
  Object result(&scope, callMethFast(thread, function, self,
                                     fastcall_args.get(), num_positional));
  for (word i = 0; i < num_positional; i++) {
    ApiHandle::fromPyObject(fastcall_args[nargs - i - 2])->decref();
  }
  thread->stackDrop(nargs + 1);
  return *result;
}

RawObject methodTrampolineFastKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs + 1));
  if (nargs == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }
  Tuple kw_names(&scope, thread->stackPeek(0));
  if (kw_names.length() != 0) {
    RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }

  Object self(&scope, thread->stackPeek(nargs));
  word num_positional = nargs - 1;
  std::unique_ptr<PyObject*[]> fastcall_args(new PyObject*[num_positional]);
  for (word i = 0; i < num_positional; i++) {
    fastcall_args[i] =
        ApiHandle::newReference(thread, thread->stackPeek(nargs - i - 1));
  }
  Object result(&scope, callMethFast(thread, function, self,
                                     fastcall_args.get(), nargs - 1));
  for (word i = 0; i < num_positional; i++) {
    ApiHandle::fromPyObject(fastcall_args[i])->decref();
  }
  thread->stackDrop(nargs + 2);
  return *result;
}

RawObject methodTrampolineFastEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  Function function(&scope, thread->stackPeek(has_varkeywords + 1));

  // Get the keyword arguments
  if (has_varkeywords) {
    Object kw_args_obj(&scope, thread->stackTop());
    if (!kw_args_obj.isDict()) UNIMPLEMENTED("mapping kwargs");
    Dict kw_args(&scope, *kw_args_obj);
    if (kw_args.numItems() != 0) {
      RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
      thread->stackDrop(has_varkeywords + 2);
      return result;
    }
  }

  Tuple args(&scope, thread->stackPeek(has_varkeywords));
  word args_length = args.length();
  if (args_length == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(has_varkeywords + 2);
    return result;
  }
  Object self(&scope, args.at(0));
  word num_positional = args_length - 1;
  std::unique_ptr<PyObject*[]> fastcall_args(new PyObject*[num_positional]);

  // Set the positional arguments
  for (word i = 0; i < num_positional; i++) {
    fastcall_args[i] = ApiHandle::newReference(thread, args.at(i + 1));
  }

  Object result(&scope, callMethFast(thread, function, self,
                                     fastcall_args.get(), num_positional));
  for (word i = 0; i < num_positional; i++) {
    ApiHandle::fromPyObject(fastcall_args[i])->decref();
  }
  thread->stackDrop(has_varkeywords + 2);
  return *result;
}

static RawObject callMethFastWithKeywordsWithKwargs(
    Thread* thread, const Function& function, const Object& self,
    PyObject* const* args, word num_args, const Object& kw_names) {
  _PyCFunctionFastWithKeywords method = bit_cast<_PyCFunctionFastWithKeywords>(
      Int::cast(function.code()).asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  ApiHandle* kw_names_obj = ApiHandle::newReference(thread, *kw_names);
  PyObject* pyresult = (*method)(self_obj, args, num_args, kw_names_obj);
  RawObject result = ApiHandle::checkFunctionResult(thread, pyresult);
  kw_names_obj->decref();
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  return result;
}

static RawObject callMethFastWithKeywords(Thread* thread,
                                          const Function& function,
                                          const Object& self,
                                          PyObject* const* args,
                                          word num_args) {
  _PyCFunctionFastWithKeywords method = bit_cast<_PyCFunctionFastWithKeywords>(
      Int::cast(function.code()).asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* pyresult = (*method)(self_obj, args, num_args, nullptr);
  RawObject result = ApiHandle::checkFunctionResult(thread, pyresult);
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  return result;
}

RawObject methodTrampolineFastWithKeywords(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs));
  if (nargs == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 1);
    return result;
  }
  Object self(&scope, thread->stackPeek(nargs - 1));

  std::unique_ptr<PyObject*[]> fastcall_args(new PyObject*[nargs - 1]);
  for (word i = 0; i < nargs - 1; i++) {
    fastcall_args[nargs - i - 2] =
        ApiHandle::newReference(thread, thread->stackPeek(i));
  }
  word num_positional = nargs - 1;
  Object result(&scope,
                callMethFastWithKeywords(thread, function, self,
                                         fastcall_args.get(), num_positional));
  for (word i = 0; i < nargs - 1; i++) {
    ApiHandle::fromPyObject(fastcall_args[nargs - i - 2])->decref();
  }
  thread->stackDrop(nargs + 1);
  return *result;
}

RawObject methodTrampolineFastWithKeywordsKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs + 1));
  if (nargs == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }
  Object self(&scope, thread->stackPeek(nargs));

  std::unique_ptr<PyObject*[]> fastcall_args(new PyObject*[nargs - 1]);
  for (word i = 0; i < nargs - 1; i++) {
    fastcall_args[i] =
        ApiHandle::newReference(thread, thread->stackPeek(nargs - i - 1));
  }
  Tuple kw_names(&scope, thread->stackPeek(0));
  word num_positional = nargs - kw_names.length() - 1;
  Object result(&scope, callMethFastWithKeywordsWithKwargs(
                            thread, function, self, fastcall_args.get(),
                            num_positional, kw_names));
  for (word i = 0; i < nargs - 1; i++) {
    ApiHandle::fromPyObject(fastcall_args[i])->decref();
  }
  thread->stackDrop(nargs + 2);
  return *result;
}

RawObject methodTrampolineFastWithKeywordsEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  word num_keywords = 0;

  // Get the keyword arguments
  Tuple kw_names(&scope, runtime->emptyTuple());
  if (has_varkeywords) {
    Object kw_args_obj(&scope, thread->stackTop());
    if (!kw_args_obj.isDict()) UNIMPLEMENTED("mapping kwargs");
    Dict kw_args(&scope, *kw_args_obj);
    num_keywords = kw_args.numItems();
  }

  Function function(&scope, thread->stackPeek(has_varkeywords + 1));
  Tuple args(&scope, thread->stackPeek(has_varkeywords));
  word args_length = args.length();
  if (args_length == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(has_varkeywords + 2);
    return result;
  }
  Object self(&scope, args.at(0));
  word num_positional = args_length - 1;
  std::unique_ptr<PyObject*[]> fastcall_args(
      new PyObject*[num_positional + num_keywords]);

  // Set the positional arguments
  for (word i = 0; i < num_positional; i++) {
    fastcall_args[i] = ApiHandle::newReference(thread, args.at(i + 1));
  }

  // Set the keyword arguments
  if (has_varkeywords) {
    Dict kw_args(&scope, thread->stackTop());

    Object key(&scope, NoneType::object());
    Object value(&scope, NoneType::object());
    kw_names = runtime->newTuple(num_keywords);
    for (word dict_i = 0, arg_i = 0;
         dictNextItem(kw_args, &dict_i, &key, &value); arg_i++) {
      kw_names.atPut(arg_i, *key);
      fastcall_args[num_positional + arg_i] =
          ApiHandle::newReference(thread, *value);
    }
  }

  Object result(&scope, NoneType::object());
  if (!has_varkeywords) {
    result = callMethFastWithKeywords(thread, function, self,
                                      fastcall_args.get(), num_positional);
  } else {
    result = callMethFastWithKeywordsWithKwargs(
        thread, function, self, fastcall_args.get(), num_positional, kw_names);
  }
  for (word i = 0; i < num_positional + num_keywords; i++) {
    ApiHandle::fromPyObject(fastcall_args[i])->decref();
  }
  thread->stackDrop(has_varkeywords + 2);
  return *result;
}

}  // namespace py
