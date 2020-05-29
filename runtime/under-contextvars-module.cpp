#include "under-contextvars-module.h"

#include "builtins.h"
#include "dict-builtins.h"
#include "type-builtins.h"

namespace py {

static const BuiltinType kUnderContextvarsBuiltinTypes[] = {
    {ID(Context), LayoutId::kContext},
    {ID(ContextVar), LayoutId::kContextVar},
    {ID(Token), LayoutId::kToken},
};

void FUNC(_contextvars, __init_module__)(Thread* thread, const Module& module,
                                         View<byte> bytecode) {
  moduleAddBuiltinTypes(thread, module, kUnderContextvarsBuiltinTypes);
  executeFrozenModule(thread, module, bytecode);
}

static const BuiltinAttribute kContextVarAttributes[] = {
    {ID(name), RawContextVar::kNameOffset, AttributeFlags::kReadOnly},
};

static const BuiltinAttribute kTokenAttributes[] = {
    {ID(var), RawToken::kVarOffset, AttributeFlags::kReadOnly},
    {ID(old_value), RawToken::kOldValueOffset},
};

void initializeUnderContextvarsTypes(Thread* thread) {
  addEmptyBuiltinType(thread, ID(Context), LayoutId::kContext,
                      /*superclass_id=*/LayoutId::kObject);

  addBuiltinType(thread, ID(ContextVar), LayoutId::kContextVar,
                 /*superclass_id=*/LayoutId::kObject, kContextVarAttributes);

  addBuiltinType(thread, ID(Token), LayoutId::kToken,
                 /*superclass_id=*/LayoutId::kObject, kTokenAttributes);
}

RawObject FUNC(_contextvars, _ContextVar_default_value)(Thread* thread,
                                                        Frame* frame,
                                                        word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object ctxvar_obj(&scope, args.get(0));
  if (!ctxvar_obj.isContextVar()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "'_contextvar__default_value_get' requires a 'ContextVar', "
        "but received a '%T'",
        &ctxvar_obj);
  }
  ContextVar ctxvar(&scope, *ctxvar_obj);
  return ctxvar.defaultValue();
}

RawObject FUNC(_contextvars, _ContextVar_name)(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object ctxvar_obj(&scope, args.get(0));
  if (!ctxvar_obj.isContextVar()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "'_contextvar__name_get' requires a 'ContextVar', "
        "but received a '%T'",
        &ctxvar_obj);
  }
  ContextVar ctxvar(&scope, *ctxvar_obj);
  return ctxvar.name();
}

RawObject FUNC(_contextvars, _Token_used)(Thread* thread, Frame* frame,
                                          word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object token_obj(&scope, args.get(0));
  if (!token_obj.isToken()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'_Token_used' requires a 'Token', "
                                "but received a '%T'",
                                &token_obj);
  }
  Token token(&scope, *token_obj);
  return Bool::fromBool(token.used());
}

RawObject FUNC(_contextvars, _Token_var)(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object token_obj(&scope, args.get(0));
  if (!token_obj.isToken()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'_Token_var' requires a 'Token', "
                                "but received a '%T'",
                                &token_obj);
  }
  Token token(&scope, *token_obj);
  return token.var();
}

static RawObject contextForThread(Thread* thread) {
  HandleScope scope(thread);
  Object ctx_obj(&scope, thread->contextvarsContext());
  if (ctx_obj.isNoneType()) {
    Runtime* runtime = thread->runtime();
    Dict data(&scope, runtime->newDict());
    Context ctx(&scope, runtime->newContext(data));
    thread->setContextvarsContext(*ctx);
    return *ctx;
  }
  return *ctx_obj;
}

RawObject FUNC(_contextvars, _thread_context)(Thread* thread, Frame*, word) {
  return contextForThread(thread);
}

static RawObject dataDictFromContext(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isContext()) {
    return thread->raiseRequiresType(self_obj, ID(Context));
  }
  Context self(&scope, *self_obj);
  return self.data();
}

static RawObject lookupVarInContext(Thread* thread, Frame* frame, word nargs,
                                    bool contains_mode) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object var_obj(&scope, args.get(1));
  if (!var_obj.isContextVar()) {
    return thread->raiseRequiresType(var_obj, ID(ContextVar));
  }
  ContextVar var(&scope, *var_obj);
  Object data_obj(&scope, dataDictFromContext(thread, frame, nargs));
  if (data_obj.isError()) return *data_obj;
  Dict data(&scope, *data_obj);
  Object var_hash_obj(&scope, Interpreter::hash(thread, var));
  if (var_hash_obj.isError()) return *var_hash_obj;
  word var_hash = SmallInt::cast(*var_hash_obj).value();
  return contains_mode ? dictIncludes(thread, data, var, var_hash)
                       : dictAt(thread, data, var, var_hash);
}

RawObject METH(Context, __contains__)(Thread* thread, Frame* frame,
                                      word nargs) {
  return lookupVarInContext(thread, frame, nargs, true);
}

RawObject METH(Context, __eq__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);

  Object data_obj(&scope, dataDictFromContext(thread, frame, nargs));
  if (data_obj.isError()) return *data_obj;
  Dict data(&scope, *data_obj);

  Arguments args(frame, nargs);
  Object other_ctx_obj(&scope, args.get(1));
  if (!other_ctx_obj.isContext()) {
    return NotImplementedType::object();
  }
  Context other_ctx(&scope, *other_ctx_obj);
  Dict other_data(&scope, other_ctx.data());

  return dictEq(thread, data, other_data);
}

RawObject METH(Context, __getitem__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Object result(&scope, lookupVarInContext(thread, frame, nargs, false));
  if (result.isErrorNotFound()) {
    return thread->raise(LayoutId::kKeyError, NoneType::object());
  }
  return *result;
}

RawObject METH(Context, __iter__)(Thread* thread, Frame* frame, word nargs) {
  return METH(Context, keys)(thread, frame, nargs);
}

RawObject METH(Context, __new__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  if (args.get(0) != runtime->typeAt(LayoutId::kContext)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "Context.__new__(X): X is not 'Context'");
  }
  Dict data(&scope, runtime->newDict());
  Context ctx(&scope, runtime->newContext(data));
  return *ctx;
}

RawObject METH(Context, __len__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Object data_obj(&scope, dataDictFromContext(thread, frame, nargs));
  if (data_obj.isError()) return *data_obj;
  Dict data(&scope, *data_obj);
  return SmallInt::fromWord(data.numItems());
}

RawObject METH(Context, copy)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Object data_obj(&scope, dataDictFromContext(thread, frame, nargs));
  if (data_obj.isError()) return *data_obj;
  Dict data(&scope, *data_obj);
  return thread->runtime()->newContext(data);
}

RawObject METH(Context, get)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object val(&scope, lookupVarInContext(thread, frame, nargs, false));
  if (val.isErrorNotFound()) {
    return args.get(2);
  }
  return *val;
}

RawObject METH(Context, items)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Object data_obj(&scope, dataDictFromContext(thread, frame, nargs));
  if (data_obj.isError()) return *data_obj;
  Dict data(&scope, *data_obj);
  return thread->runtime()->newDictItemIterator(thread, data);
}

RawObject METH(Context, keys)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Object data_obj(&scope, dataDictFromContext(thread, frame, nargs));
  if (data_obj.isError()) return *data_obj;
  Dict data(&scope, *data_obj);
  return thread->runtime()->newDictKeyIterator(thread, data);
}

RawObject METH(Context, run)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isContext()) {
    return thread->raiseRequiresType(self_obj, ID(Context));
  }
  Context self(&scope, *self_obj);

  // Set Context.prev_context to current thread-global Context
  if (!self.prevContext().isNoneType()) {
    Str self_repr(&scope, thread->invokeMethod1(self, ID(__repr__)));
    return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                "cannot enter context: %S is already entered",
                                &self_repr);
  }
  Context ctx(&scope, contextForThread(thread));
  self.setPrevContext(*ctx);

  thread->setContextvarsContext(*self);

  // Call callable forwarding all args
  frame->pushValue(args.get(1));  // callable
  frame->pushValue(args.get(2));  // *args
  frame->pushValue(args.get(3));  // **kwargs
  Object call_result(
      &scope,
      Interpreter::callEx(thread, frame, CallFunctionExFlag::VAR_KEYWORDS));

  // Always restore the thread's previous Context even if call above failed
  thread->setContextvarsContext(self.prevContext());
  self.setPrevContext(NoneType::object());

  return *call_result;
}

RawObject METH(Context, values)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Object data_obj(&scope, dataDictFromContext(thread, frame, nargs));
  if (data_obj.isError()) return *data_obj;
  Dict data(&scope, *data_obj);
  return thread->runtime()->newDictValueIterator(thread, data);
}

RawObject METH(ContextVar, __new__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  if (args.get(0) != runtime->typeAt(LayoutId::kContextVar)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "ContextVar.__new__(X): X is not 'ContextVar'");
  }

  Object name_obj(&scope, args.get(1));
  if (!name_obj.isStr()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "context variable name must be a str");
  }
  Str name(&scope, *name_obj);

  Object default_value(&scope, args.get(2));

  return runtime->newContextVar(name, default_value);
}

RawObject METH(ContextVar, get)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isContextVar()) {
    return thread->raiseRequiresType(self_obj, ID(ContextVar));
  }
  ContextVar self(&scope, *self_obj);

  // Check for value held in thread-global Context
  Context ctx(&scope, contextForThread(thread));
  Dict ctx_data(&scope, ctx.data());
  Object self_hash_obj(&scope, Interpreter::hash(thread, self));
  if (self_hash_obj.isError()) return *self_hash_obj;
  word self_hash = SmallInt::cast(*self_hash_obj).value();
  Object result(&scope, dictAt(thread, ctx_data, self, self_hash));
  if (!result.isError() || !result.isErrorNotFound()) {
    return *result;
  }

  // No data in thread-global Context, check default argument
  Object arg_default(&scope, args.get(1));
  if (!arg_default.isUnbound()) {
    return *arg_default;
  }

  // No default argument, check ContextVar default
  Object default_value(&scope, self.defaultValue());
  if (!default_value.isUnbound()) {
    return *default_value;
  }

  return thread->raise(LayoutId::kLookupError, NoneType::object());
}

RawObject METH(ContextVar, reset)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isContextVar()) {
    return thread->raiseRequiresType(self_obj, ID(ContextVar));
  }
  ContextVar self(&scope, *self_obj);
  Object token_obj(&scope, args.get(1));
  if (!token_obj.isToken()) {
    return thread->raiseRequiresType(self_obj, ID(Token));
  }
  Token token(&scope, *token_obj);

  if (token.used()) {
    return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                "Token has already been used once");
  }

  if (token.var() != self) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "Token was created by a different ContextVar");
  }

  Context ctx(&scope, contextForThread(thread));
  if (token.context() != ctx) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "Token was created in a different Context");
  }

  // Copy thread-global Context data for update
  Dict ctx_data(&scope, ctx.data());
  Object self_hash_obj(&scope, Interpreter::hash(thread, self));
  if (self_hash_obj.isError()) return *self_hash_obj;
  word self_hash = SmallInt::cast(*self_hash_obj).value();
  Object ctx_data_copy_obj(&scope, dictCopy(thread, ctx_data));
  if (ctx_data_copy_obj.isError()) return *ctx_data_copy_obj;
  Dict ctx_data_copy(&scope, *ctx_data_copy_obj);

  // Update thread-global Context data based on Token.old_value
  Object dict_op_res(&scope, NoneType::object());
  Object old_value(&scope, token.oldValue());
  if (old_value.isUnbound()) {
    dict_op_res = dictRemove(thread, ctx_data_copy, self, self_hash);
  } else {
    dict_op_res = dictAtPut(thread, ctx_data_copy, self, self_hash, old_value);
  }
  if (dict_op_res.isError()) return *dict_op_res;
  ctx.setData(*ctx_data_copy);

  token.setUsed(true);

  return NoneType::object();
}

RawObject METH(ContextVar, set)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isContextVar()) {
    return thread->raiseRequiresType(self_obj, ID(ContextVar));
  }
  ContextVar self(&scope, *self_obj);

  // Get thread-global Context and its data dict.
  Context ctx(&scope, contextForThread(thread));
  Dict ctx_data(&scope, ctx.data());
  Object self_hash_obj(&scope, Interpreter::hash(thread, self));
  if (self_hash_obj.isError()) return *self_hash_obj;
  word self_hash = SmallInt::cast(*self_hash_obj).value();

  // Get any oldvalue from the thread-global Context or Token.MISSING
  Object old_value(&scope, dictAt(thread, ctx_data, self, self_hash));
  if (old_value.isError()) {
    if (old_value.isErrorNotFound()) {
      old_value = Unbound::object();
    } else {
      return *old_value;
    }
  }

  // Update thread-global Context data by copying the dict and updating it.
  Object ctx_data_copy_obj(&scope, dictCopy(thread, ctx_data));
  if (ctx_data_copy_obj.isError()) return *ctx_data_copy_obj;
  Dict ctx_data_copy(&scope, *ctx_data_copy_obj);
  Object value(&scope, args.get(1));
  Object ctx_data_copy_put_result(
      &scope, dictAtPut(thread, ctx_data_copy, self, self_hash, value));
  if (ctx_data_copy_put_result.isError()) return *ctx_data_copy_put_result;
  ctx.setData(*ctx_data_copy);

  return thread->runtime()->newToken(ctx, self, old_value);
}

}  // namespace py
