#include "interpreter.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>

#include "attributedict.h"
#include "builtins-module.h"
#include "bytes-builtins.h"
#include "complex-builtins.h"
#include "dict-builtins.h"
#include "event.h"
#include "exception-builtins.h"
#include "float-builtins.h"
#include "frame.h"
#include "generator-builtins.h"
#include "ic.h"
#include "int-builtins.h"
#include "interpreter-gen.h"
#include "list-builtins.h"
#include "module-builtins.h"
#include "object-builtins.h"
#include "objects.h"
#include "profiling.h"
#include "runtime.h"
#include "set-builtins.h"
#include "str-builtins.h"
#include "thread.h"
#include "trampolines.h"
#include "tuple-builtins.h"
#include "type-builtins.h"
#include "utils.h"

// TODO(emacs): Figure out why this produces different (more) results than
// using EVENT_ID with the opcode as arg0 and remove EVENT_CACHE.
#define EVENT_CACHE(op) EVENT(InvalidateInlineCache_##op)

namespace py {

using Continue = Interpreter::Continue;

// We want opcode handlers inlined into the interpreter in optimized builds.
// Keep them outlined for nicer debugging in debug builds.
#ifdef NDEBUG
#define HANDLER_INLINE ALWAYS_INLINE __attribute__((used))
#else
#define HANDLER_INLINE __attribute__((noinline))
#endif

Interpreter::~Interpreter() {}

RawObject Interpreter::prepareCallable(Thread* thread, Object* callable,
                                       Object* self) {
  DCHECK(!callable->isFunction(),
         "prepareCallable should only be called on non-function types");
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  for (;;) {
    if (callable->isBoundMethod()) {
      BoundMethod method(&scope, **callable);
      Object maybe_function(&scope, method.function());
      if (maybe_function.isFunction()) {
        // If we have an exact function, unwrap as a fast-path. Otherwise, fall
        // back to __call__.
        *callable = *maybe_function;
        *self = method.self();
        return Bool::trueObj();
      }
    }

    if (callable->isType()) {
      // In case `callable` is a type (e.g., str("value")), this call is
      // resolved via type.__call__(callable, ...). The most common operation
      // performed by such a path is object creation through __init__ and
      // __new__. In case callable.underCtor is explicitly defined, it can
      // perform such instance creation of the exact type `callable` directly
      // without dispatching to `type.__call__` if it exists. Otherwise,
      // callable.underCtor is guaranteed to be same as type.__call__.
      RawType type = Type::cast(**callable);
      RawObject ctor = type.ctor();
      DCHECK(ctor.isFunction(), "ctor is expected to be a function");
      *self = type;
      *callable = ctor;
      return Bool::trueObj();
    }
    // TODO(T44238481): Look into using lookupMethod() once it's fixed.
    Type type(&scope, runtime->typeOf(**callable));
    Object dunder_call(&scope,
                       typeLookupInMroById(thread, *type, ID(__call__)));
    if (!dunder_call.isErrorNotFound()) {
      if (dunder_call.isFunction()) {
        // Avoid calling function.__get__ and creating a short-lived BoundMethod
        // object. Instead, return the unpacked values directly.
        *self = **callable;
        *callable = *dunder_call;
        return Bool::trueObj();
      }
      Type call_type(&scope, runtime->typeOf(*dunder_call));
      if (typeIsNonDataDescriptor(*call_type)) {
        *callable = callDescriptorGet(thread, dunder_call, *callable, type);
        if (callable->isErrorException()) return **callable;
        if (callable->isFunction()) return Bool::falseObj();

        // Retry the lookup using the object returned by the descriptor.
        continue;
      }
      // Update callable for the exception message below.
      *callable = *dunder_call;
    }
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'%T' object is not callable", callable);
  }
}

HANDLER_INLINE USED Interpreter::PrepareCallableResult
Interpreter::prepareCallableCall(Thread* thread, word nargs,
                                 word callable_idx) {
  RawObject callable = thread->stackPeek(callable_idx);
  if (callable.isFunction()) {
    return {callable, nargs};
  }

  if (callable.isBoundMethod()) {
    RawBoundMethod method = BoundMethod::cast(callable);
    RawObject method_function = method.function();
    if (method_function.isFunction()) {
      thread->stackSetAt(callable_idx, method_function);
      thread->stackInsertAt(callable_idx, method.self());
      return {method_function, nargs + 1};
    }
  }
  return prepareCallableCallDunderCall(thread, nargs, callable_idx);
}

NEVER_INLINE
Interpreter::PrepareCallableResult Interpreter::prepareCallableCallDunderCall(
    Thread* thread, word nargs, word callable_idx) {
  HandleScope scope(thread);
  Object callable(&scope, thread->stackPeek(callable_idx));
  Object self(&scope, NoneType::object());
  RawObject prepare_result = prepareCallable(thread, &callable, &self);
  if (prepare_result.isErrorException()) {
    return {prepare_result, nargs};
  }
  thread->stackSetAt(callable_idx, *callable);
  if (prepare_result == Bool::trueObj()) {
    // Shift all arguments on the stack down by 1 and use the unpacked
    // BoundMethod.
    //
    // We don't need to worry too much about the performance overhead for method
    // calls here.
    //
    // Python 3.7 introduces two new opcodes, LOAD_METHOD and CALL_METHOD, that
    // eliminate the need to create a temporary BoundMethod object when
    // performing a method call.
    //
    // The other pattern of bound method usage occurs when someone passes around
    // a reference to a method e.g.:
    //
    //   m = foo.method
    //   m()
    //
    // Our contention is that uses of this pattern are not performance
    // sensitive.
    thread->stackInsertAt(callable_idx, *self);
    return {*callable, nargs + 1};
  }
  return {*callable, nargs};
}

RawObject Interpreter::call(Thread* thread, word nargs) {
  DCHECK(!thread->hasPendingException(), "unhandled exception lingering");
  RawObject* post_call_sp = thread->stackPointer() + nargs + 1;
  PrepareCallableResult prepare_result =
      prepareCallableCall(thread, nargs, nargs);
  RawObject function = prepare_result.function;
  nargs = prepare_result.nargs;
  if (function.isErrorException()) {
    thread->stackDrop(nargs + 1);
    DCHECK(thread->stackPointer() == post_call_sp, "stack not cleaned");
    return function;
  }
  return callFunction(thread, nargs, function);
}

ALWAYS_INLINE RawObject Interpreter::callFunction(Thread* thread, word nargs,
                                                  RawObject function) {
  DCHECK(!thread->hasPendingException(), "unhandled exception lingering");
  RawObject* post_call_sp = thread->stackPointer() + nargs + 1;
  DCHECK(function == thread->stackPeek(nargs),
         "thread->stackPeek(nargs) is expected to be the given function");
  RawObject result = Function::cast(function).entry()(thread, nargs);
  DCHECK(thread->stackPointer() == post_call_sp, "stack not cleaned");
  return result;
}

RawObject Interpreter::callKw(Thread* thread, word nargs) {
  // Top of stack is a tuple of keyword argument names in the order they
  // appear on the stack.
  RawObject* post_call_sp = thread->stackPointer() + nargs + 2;
  PrepareCallableResult prepare_result =
      prepareCallableCall(thread, nargs, nargs + 1);
  RawObject function = prepare_result.function;
  nargs = prepare_result.nargs;
  if (function.isErrorException()) {
    thread->stackDrop(nargs + 2);
    DCHECK(thread->stackPointer() == post_call_sp, "stack not cleaned");
    return function;
  }
  RawObject result = Function::cast(function).entryKw()(thread, nargs);
  DCHECK(thread->stackPointer() == post_call_sp, "stack not cleaned");
  return result;
}

RawObject Interpreter::callEx(Thread* thread, word flags) {
  // Low bit of flags indicates whether var-keyword argument is on TOS.
  // In all cases, var-positional tuple is next, followed by the function
  // pointer.
  word callable_idx = (flags & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1;
  RawObject* post_call_sp = thread->stackPointer() + callable_idx + 1;
  HandleScope scope(thread);
  Object callable(&scope, prepareCallableEx(thread, callable_idx));
  if (callable.isErrorException()) {
    thread->stackDrop(callable_idx + 1);
    DCHECK(thread->stackPointer() == post_call_sp, "stack not cleaned");
    return *callable;
  }
  RawObject result = RawFunction::cast(*callable).entryEx()(thread, flags);
  DCHECK(thread->stackPointer() == post_call_sp, "stack not cleaned");
  return result;
}

RawObject Interpreter::prepareCallableEx(Thread* thread, word callable_idx) {
  HandleScope scope(thread);
  Object callable(&scope, thread->stackPeek(callable_idx));
  word args_idx = callable_idx - 1;
  Object args_obj(&scope, thread->stackPeek(args_idx));
  if (!args_obj.isTuple()) {
    // Make sure the argument sequence is a tuple.
    if (args_obj.isList()) {
      List list(&scope, *args_obj);
      Tuple list_items(&scope, list.items());
      args_obj = thread->runtime()->tupleSubseq(thread, list_items, 0,
                                                list.numItems());
    }
    args_obj = thread->invokeFunction1(ID(builtins), ID(tuple), args_obj);
    if (args_obj.isErrorException()) return *args_obj;
    thread->stackSetAt(args_idx, *args_obj);
  }
  if (!callable.isFunction()) {
    Object self(&scope, NoneType::object());
    Object result(&scope, prepareCallable(thread, &callable, &self));
    if (result.isErrorException()) return *result;
    thread->stackSetAt(callable_idx, *callable);

    if (result == Bool::trueObj()) {
      // Create a new argument tuple with self as the first argument
      Tuple args(&scope, *args_obj);
      MutableTuple new_args(
          &scope, thread->runtime()->newMutableTuple(args.length() + 1));
      new_args.atPut(0, *self);
      new_args.replaceFromWith(1, *args, args.length());
      thread->stackSetAt(args_idx, new_args.becomeImmutable());
    }
  }
  return *callable;
}

static RawObject callDunderHash(Thread* thread, const Object& value) {
  HandleScope scope(thread);
  // TODO(T52406106): This lookup is unfortunately not inline-cached but should
  // eventually be called less and less as code moves to managed.
  Object dunder_hash(&scope,
                     Interpreter::lookupMethod(thread, value, ID(__hash__)));
  if (dunder_hash.isNoneType() || dunder_hash.isError()) {
    if (dunder_hash.isErrorException()) {
      thread->clearPendingException();
    } else {
      DCHECK(dunder_hash.isErrorNotFound() || dunder_hash.isNoneType(),
             "expected Error::notFound() or None");
    }
    return thread->raiseWithFmt(LayoutId::kTypeError, "unhashable type: '%T'",
                                &value);
  }
  Object result(&scope, Interpreter::callMethod1(thread, dunder_hash, value));
  if (result.isErrorException()) return *result;
  if (!thread->runtime()->isInstanceOfInt(*result)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__hash__ method should return an integer");
  }
  Int hash_int(&scope, intUnderlying(*result));
  if (hash_int.isSmallInt()) {
    // cpython always replaces -1 hash values with -2.
    if (hash_int == SmallInt::fromWord(-1)) {
      return SmallInt::fromWord(-2);
    }
    return *hash_int;
  }
  if (hash_int.isBool()) {
    return SmallInt::fromWord(Bool::cast(*hash_int).value() ? 1 : 0);
  }
  // Note that cpython keeps the hash values unaltered as long as they fit into
  // `Py_hash_t` (aka `Py_ssize_t`) while we must return a `SmallInt` here so
  // we have to invoke the large int hashing for 1 bit smaller numbers than
  // cpython.
  return SmallInt::fromWord(largeIntHash(LargeInt::cast(*hash_int)));
}

RawObject Interpreter::hash(Thread* thread, const Object& value) {
  // Directly call into hash functions for all types supported by the marshal
  // code to avoid bootstrapping problems. It also helps performance.
  LayoutId layout_id = value.layoutId();
  word result;
  switch (layout_id) {
    case LayoutId::kBool:
      result = Bool::cast(*value).hash();
      break;
    case LayoutId::kComplex:
      result = complexHash(*value);
      break;
    case LayoutId::kFloat:
      result = floatHash(*value);
      break;
    case LayoutId::kFrozenSet:
      return frozensetHash(thread, value);
    case LayoutId::kSmallInt:
      result = SmallInt::cast(*value).hash();
      break;
    case LayoutId::kLargeBytes:
    case LayoutId::kSmallBytes:
      result = bytesHash(thread, *value);
      break;
    case LayoutId::kLargeInt:
      result = largeIntHash(LargeInt::cast(*value));
      break;
    case LayoutId::kLargeStr:
    case LayoutId::kSmallStr:
      result = strHash(thread, *value);
      break;
    case LayoutId::kTuple: {
      HandleScope scope(thread);
      Tuple value_tuple(&scope, *value);
      return tupleHash(thread, value_tuple);
    }
    case LayoutId::kNoneType:
    case LayoutId::kEllipsis:
    case LayoutId::kStopIteration:
      result = thread->runtime()->hash(*value);
      break;
    default: {
      Runtime* runtime = thread->runtime();
      RawType value_type = runtime->typeOf(*value);
      if (value_type.hasFlag(Type::Flag::kHasObjectDunderHash)) {
        // At this point we already handled all immediate value types, as well
        // as LargeStr and LargeBytes, so we can directly call
        // `Runtime::identityHash` instead of `Runtime::hash`.
        result = runtime->identityHash(*value);
      } else if (value_type.hasFlag(Type::Flag::kHasStrDunderHash) &&
                 runtime->isInstanceOfStr(*value)) {
        result = strHash(thread, strUnderlying(*value));
      } else {
        return callDunderHash(thread, value);
      }
      break;
    }
  }
  return SmallInt::fromWordTruncated(result);
}

RawObject Interpreter::stringJoin(Thread* thread, RawObject* sp, word num) {
  word new_len = 0;
  for (word i = num - 1; i >= 0; i--) {
    if (!sp[i].isStr()) {
      UNIMPLEMENTED("Conversion of non-string values not supported.");
    }
    new_len += Str::cast(sp[i]).length();
  }

  if (new_len <= RawSmallStr::kMaxLength) {
    byte buffer[RawSmallStr::kMaxLength];
    byte* ptr = buffer;
    for (word i = num - 1; i >= 0; i--) {
      RawStr str = Str::cast(sp[i]);
      word len = str.length();
      str.copyTo(ptr, len);
      ptr += len;
    }
    return SmallStr::fromBytes(View<byte>(buffer, new_len));
  }

  HandleScope scope(thread);
  MutableBytes result(&scope,
                      thread->runtime()->newMutableBytesUninitialized(new_len));
  word offset = 0;
  for (word i = num - 1; i >= 0; i--) {
    RawStr str = Str::cast(sp[i]);
    word len = str.length();
    result.replaceFromWithStr(offset, str, len);
    offset += len;
  }
  return result.becomeStr();
}

RawObject Interpreter::callDescriptorGet(Thread* thread,
                                         const Object& descriptor,
                                         const Object& receiver,
                                         const Object& receiver_type) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  switch (descriptor.layoutId()) {
    case LayoutId::kClassMethod: {
      Object method(&scope, ClassMethod::cast(*descriptor).function());
      return runtime->newBoundMethod(method, receiver_type);
    }
    case LayoutId::kFunction: {
      if (receiver.isNoneType()) {
        if (receiver_type.rawCast<RawType>().builtinBase() !=
            LayoutId::kNoneType) {
          // Type lookup.
          return *descriptor;
        }
      }
      return runtime->newBoundMethod(descriptor, receiver);
    }
    case LayoutId::kProperty: {
      Object getter(&scope, Property::cast(*descriptor).getter());
      if (getter.isNoneType()) break;
      if (receiver.isNoneType()) {
        return *descriptor;
      }
      return Interpreter::call1(thread, getter, receiver);
    }
    case LayoutId::kStaticMethod:
      return StaticMethod::cast(*descriptor).function();
    default:
      break;
  }
  Object method(
      &scope, typeLookupInMroById(
                  thread, thread->runtime()->typeOf(*descriptor), ID(__get__)));
  DCHECK(!method.isErrorNotFound(), "no __get__ method found");
  return call3(thread, method, descriptor, receiver, receiver_type);
}

RawObject Interpreter::callDescriptorSet(Thread* thread,
                                         const Object& descriptor,
                                         const Object& receiver,
                                         const Object& value) {
  return thread->invokeMethod3(descriptor, ID(__set__), receiver, value);
}

RawObject Interpreter::callDescriptorDelete(Thread* thread,
                                            const Object& descriptor,
                                            const Object& receiver) {
  return thread->invokeMethod2(descriptor, ID(__delete__), receiver);
}

RawObject Interpreter::lookupMethod(Thread* thread, const Object& receiver,
                                    SymbolId selector) {
  Runtime* runtime = thread->runtime();
  RawType raw_type = runtime->typeOf(*receiver).rawCast<RawType>();
  RawObject raw_method = typeLookupInMroById(thread, raw_type, selector);
  if (raw_method.isFunction() || raw_method.isErrorNotFound()) {
    // Do not create a short-lived bound method object, and propagate
    // exceptions.
    return raw_method;
  }
  HandleScope scope(thread);
  Type type(&scope, raw_type);
  Object method(&scope, raw_method);
  return resolveDescriptorGet(thread, method, receiver, type);
}

RawObject Interpreter::call0(Thread* thread, const Object& callable) {
  thread->stackPush(*callable);
  return call(thread, 0);
}

RawObject Interpreter::call1(Thread* thread, const Object& callable,
                             const Object& arg1) {
  thread->stackPush(*callable);
  thread->stackPush(*arg1);
  return call(thread, 1);
}

RawObject Interpreter::call2(Thread* thread, const Object& callable,
                             const Object& arg1, const Object& arg2) {
  thread->stackPush(*callable);
  thread->stackPush(*arg1);
  thread->stackPush(*arg2);
  return call(thread, 2);
}

RawObject Interpreter::call3(Thread* thread, const Object& callable,
                             const Object& arg1, const Object& arg2,
                             const Object& arg3) {
  thread->stackPush(*callable);
  thread->stackPush(*arg1);
  thread->stackPush(*arg2);
  thread->stackPush(*arg3);
  return call(thread, 3);
}

RawObject Interpreter::call4(Thread* thread, const Object& callable,
                             const Object& arg1, const Object& arg2,
                             const Object& arg3, const Object& arg4) {
  thread->stackPush(*callable);
  thread->stackPush(*arg1);
  thread->stackPush(*arg2);
  thread->stackPush(*arg3);
  thread->stackPush(*arg4);
  return call(thread, 4);
}

RawObject Interpreter::call5(Thread* thread, const Object& callable,
                             const Object& arg1, const Object& arg2,
                             const Object& arg3, const Object& arg4,
                             const Object& arg5) {
  thread->stackPush(*callable);
  thread->stackPush(*arg1);
  thread->stackPush(*arg2);
  thread->stackPush(*arg3);
  thread->stackPush(*arg4);
  thread->stackPush(*arg5);
  return call(thread, 5);
}

RawObject Interpreter::call6(Thread* thread, const Object& callable,
                             const Object& arg1, const Object& arg2,
                             const Object& arg3, const Object& arg4,
                             const Object& arg5, const Object& arg6) {
  thread->stackPush(*callable);
  thread->stackPush(*arg1);
  thread->stackPush(*arg2);
  thread->stackPush(*arg3);
  thread->stackPush(*arg4);
  thread->stackPush(*arg5);
  thread->stackPush(*arg6);
  return call(thread, 6);
}

RawObject Interpreter::callMethod1(Thread* thread, const Object& method,
                                   const Object& self) {
  word nargs = 0;
  thread->stackPush(*method);
  if (method.isFunction()) {
    thread->stackPush(*self);
    return callFunction(thread, nargs + 1, *method);
  }
  return call(thread, nargs);
}

RawObject Interpreter::callMethod2(Thread* thread, const Object& method,
                                   const Object& self, const Object& other) {
  word nargs = 1;
  thread->stackPush(*method);
  if (method.isFunction()) {
    thread->stackPush(*self);
    thread->stackPush(*other);
    return callFunction(thread, nargs + 1, *method);
  }
  thread->stackPush(*other);
  return call(thread, nargs);
}

RawObject Interpreter::callMethod3(Thread* thread, const Object& method,
                                   const Object& self, const Object& arg1,
                                   const Object& arg2) {
  word nargs = 2;
  thread->stackPush(*method);
  if (method.isFunction()) {
    thread->stackPush(*self);
    thread->stackPush(*arg1);
    thread->stackPush(*arg2);
    return callFunction(thread, nargs + 1, *method);
  }
  thread->stackPush(*arg1);
  thread->stackPush(*arg2);
  return call(thread, nargs);
}

RawObject Interpreter::callMethod4(Thread* thread, const Object& method,
                                   const Object& self, const Object& arg1,
                                   const Object& arg2, const Object& arg3) {
  word nargs = 3;
  thread->stackPush(*method);
  if (method.isFunction()) {
    thread->stackPush(*self);
    thread->stackPush(*arg1);
    thread->stackPush(*arg2);
    thread->stackPush(*arg3);
    return callFunction(thread, nargs + 1, *method);
  }
  thread->stackPush(*arg1);
  thread->stackPush(*arg2);
  thread->stackPush(*arg3);
  return call(thread, nargs);
}

HANDLER_INLINE
Continue Interpreter::tailcallMethod1(Thread* thread, RawObject method,
                                      RawObject self) {
  word nargs = 0;
  thread->stackPush(method);
  if (method.isFunction()) {
    thread->stackPush(self);
    nargs++;
    return tailcallFunction(thread, nargs, method);
  }
  return tailcall(thread, nargs);
}

HANDLER_INLINE
Continue Interpreter::tailcallMethod2(Thread* thread, RawObject method,
                                      RawObject self, RawObject arg1) {
  word nargs = 1;
  thread->stackPush(method);
  if (method.isFunction()) {
    thread->stackPush(self);
    nargs++;
    return tailcallFunction(thread, nargs, method);
  }
  thread->stackPush(arg1);
  return tailcall(thread, nargs);
}

HANDLER_INLINE Continue Interpreter::tailcall(Thread* thread, word arg) {
  return handleCall(thread, arg, arg, preparePositionalCall, &Function::entry);
}

static RawObject raiseUnaryOpTypeError(Thread* thread, const Object& object,
                                       SymbolId selector) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(*object));
  Object type_name(&scope, type.name());
  Object op_name(&scope, runtime->symbols()->at(selector));
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "bad operand type for unary '%S': '%S'", &op_name,
                              &type_name);
}

RawObject Interpreter::unaryOperation(Thread* thread, const Object& self,
                                      SymbolId selector) {
  RawObject result = thread->invokeMethod1(self, selector);
  if (result.isErrorNotFound()) {
    return raiseUnaryOpTypeError(thread, self, selector);
  }
  return result;
}

HANDLER_INLINE Continue Interpreter::doUnaryOperation(SymbolId selector,
                                                      Thread* thread) {
  HandleScope scope(thread);
  Object receiver(&scope, thread->stackTop());
  RawObject result = unaryOperation(thread, receiver, selector);
  if (result.isErrorException()) return Continue::UNWIND;
  thread->stackSetTop(result);
  return Continue::NEXT;
}

static RawObject binaryOperationLookupReflected(Thread* thread,
                                                Interpreter::BinaryOp op,
                                                const Object& left,
                                                const Object& right) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  SymbolId swapped_selector = runtime->swappedBinaryOperationSelector(op);
  Object right_reversed_method(
      &scope,
      typeLookupInMroById(thread, runtime->typeOf(*right), swapped_selector));
  if (right_reversed_method.isErrorNotFound()) return *right_reversed_method;

  // Python doesn't bother calling the reverse method when the slot on left and
  // right points to the same method. We compare the reverse methods to get
  // close to this behavior.
  Object left_reversed_method(
      &scope,
      typeLookupInMroById(thread, runtime->typeOf(*left), swapped_selector));
  if (left_reversed_method == right_reversed_method) {
    return Error::notFound();
  }

  return *right_reversed_method;
}

static RawObject executeAndCacheBinaryOp(Thread* thread, const Object& method,
                                         BinaryOpFlags flags,
                                         const Object& left,
                                         const Object& right,
                                         Object* method_out,
                                         BinaryOpFlags* flags_out) {
  if (method.isErrorNotFound()) {
    return NotImplementedType::object();
  }

  if (method_out != nullptr) {
    DCHECK(method.isFunction(), "must be a plain function");
    *method_out = *method;
    *flags_out = flags;
    return Interpreter::binaryOperationWithMethod(thread, *method, flags, *left,
                                                  *right);
  }
  if (flags & kBinaryOpReflected) {
    return Interpreter::callMethod2(thread, method, right, left);
  }
  return Interpreter::callMethod2(thread, method, left, right);
}

RawObject Interpreter::binaryOperationSetMethod(Thread* thread, BinaryOp op,
                                                const Object& left,
                                                const Object& right,
                                                Object* method_out,
                                                BinaryOpFlags* flags_out) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  SymbolId selector = runtime->binaryOperationSelector(op);
  Type left_type(&scope, runtime->typeOf(*left));
  Type right_type(&scope, runtime->typeOf(*right));
  Object left_method(&scope, typeLookupInMroById(thread, *left_type, selector));

  // Figure out whether we want to run the normal or the reverse operation
  // first and set `flags` accordingly.
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags = kBinaryOpNone;
  if (left_type != right_type && (left_method.isErrorNotFound() ||
                                  typeIsSubclass(*right_type, *left_type))) {
    method = binaryOperationLookupReflected(thread, op, left, right);
    if (!method.isErrorNotFound()) {
      flags = kBinaryOpReflected;
      if (!left_method.isErrorNotFound()) {
        flags =
            static_cast<BinaryOpFlags>(flags | kBinaryOpNotImplementedRetry);
      }
      if (!method.isFunction()) {
        method_out = nullptr;
        method = resolveDescriptorGet(thread, method, right, right_type);
        if (method.isErrorException()) return *method;
      }
    }
  }
  if (flags == kBinaryOpNone) {
    flags = kBinaryOpNotImplementedRetry;
    method = *left_method;
    if (!method.isFunction() && !method.isErrorNotFound()) {
      method_out = nullptr;
      method = resolveDescriptorGet(thread, method, left, left_type);
      if (method.isErrorException()) return *method;
    }
  }

  Object result(&scope, executeAndCacheBinaryOp(thread, method, flags, left,
                                                right, method_out, flags_out));
  if (!result.isNotImplementedType()) return *result;

  // Invoke a 2nd method (normal or reverse depends on what we did the first
  // time) or report an error.
  return binaryOperationRetry(thread, op, flags, left, right);
}

RawObject Interpreter::binaryOperation(Thread* thread, BinaryOp op,
                                       const Object& left,
                                       const Object& right) {
  return binaryOperationSetMethod(thread, op, left, right, nullptr, nullptr);
}

HANDLER_INLINE Continue Interpreter::doBinaryOperation(BinaryOp op,
                                                       Thread* thread) {
  HandleScope scope(thread);
  Object other(&scope, thread->stackPop());
  Object self(&scope, thread->stackPop());
  RawObject result = binaryOperation(thread, op, self, other);
  if (result.isErrorException()) return Continue::UNWIND;
  thread->stackPush(result);
  return Continue::NEXT;
}

RawObject Interpreter::inplaceOperationSetMethod(Thread* thread, BinaryOp op,
                                                 const Object& left,
                                                 const Object& right,
                                                 Object* method_out,
                                                 BinaryOpFlags* flags_out) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  SymbolId selector = runtime->inplaceOperationSelector(op);
  Type left_type(&scope, runtime->typeOf(*left));
  Object method(&scope, typeLookupInMroById(thread, *left_type, selector));
  if (!method.isErrorNotFound()) {
    if (method.isFunction()) {
      if (method_out != nullptr) {
        *method_out = *method;
        *flags_out = kInplaceBinaryOpRetry;
      }
    } else {
      method = resolveDescriptorGet(thread, method, left, left_type);
      if (method.isErrorException()) return *method;
    }

    // Make sure we do not put a possible 2nd method call (from
    // binaryOperationSetMethod() down below) into the cache.
    method_out = nullptr;
    Object result(&scope, callMethod2(thread, method, left, right));
    if (result != NotImplementedType::object()) {
      return *result;
    }
  }
  return binaryOperationSetMethod(thread, op, left, right, method_out,
                                  flags_out);
}

RawObject Interpreter::inplaceOperation(Thread* thread, BinaryOp op,
                                        const Object& left,
                                        const Object& right) {
  return inplaceOperationSetMethod(thread, op, left, right, nullptr, nullptr);
}

HANDLER_INLINE Continue Interpreter::doInplaceOperation(BinaryOp op,
                                                        Thread* thread) {
  HandleScope scope(thread);
  Object right(&scope, thread->stackPop());
  Object left(&scope, thread->stackPop());
  RawObject result = inplaceOperation(thread, op, left, right);
  if (result.isErrorException()) return Continue::UNWIND;
  thread->stackPush(result);
  return Continue::NEXT;
}

RawObject Interpreter::compareOperationSetMethod(Thread* thread, CompareOp op,
                                                 const Object& left,
                                                 const Object& right,
                                                 Object* method_out,
                                                 BinaryOpFlags* flags_out) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  SymbolId selector = runtime->comparisonSelector(op);
  Type left_type(&scope, runtime->typeOf(*left));
  Type right_type(&scope, runtime->typeOf(*right));
  Object left_method(&scope, typeLookupInMroById(thread, *left_type, selector));

  // Figure out whether we want to run the normal or the reverse operation
  // first and set `flags` accordingly.
  Object method(&scope, *left_method);
  BinaryOpFlags flags = kBinaryOpNone;
  if (left_type != right_type && (left_method.isErrorNotFound() ||
                                  typeIsSubclass(*right_type, *left_type))) {
    SymbolId reverse_selector = runtime->swappedComparisonSelector(op);
    method = typeLookupInMroById(thread, *right_type, reverse_selector);
    if (!method.isErrorNotFound()) {
      flags = kBinaryOpReflected;
      if (!left_method.isErrorNotFound()) {
        flags =
            static_cast<BinaryOpFlags>(flags | kBinaryOpNotImplementedRetry);
      }
      if (!method.isFunction()) {
        method_out = nullptr;
        method = resolveDescriptorGet(thread, method, right, right_type);
        if (method.isErrorException()) return *method;
      }
    }
  }
  if (flags == kBinaryOpNone) {
    flags = kBinaryOpNotImplementedRetry;
    method = *left_method;
    if (!method.isFunction() && !method.isErrorNotFound()) {
      method_out = nullptr;
      method = resolveDescriptorGet(thread, method, left, left_type);
      if (method.isErrorException()) return *method;
    }
  }

  Object result(&scope, executeAndCacheBinaryOp(thread, method, flags, left,
                                                right, method_out, flags_out));
  if (!result.isNotImplementedType()) return *result;

  return compareOperationRetry(thread, op, flags, left, right);
}

RawObject Interpreter::compareOperationRetry(Thread* thread, CompareOp op,
                                             BinaryOpFlags flags,
                                             const Object& left,
                                             const Object& right) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  if (flags & kBinaryOpNotImplementedRetry) {
    // If we tried reflected first, try normal now.
    if (flags & kBinaryOpReflected) {
      SymbolId selector = runtime->comparisonSelector(op);
      Object method(&scope, lookupMethod(thread, left, selector));
      if (method.isError()) {
        if (method.isErrorException()) return *method;
        DCHECK(method.isErrorNotFound(), "expected not found");
      } else {
        Object result(&scope, callMethod2(thread, method, left, right));
        if (!result.isNotImplementedType()) return *result;
      }
    } else {
      // If we tried normal first, try to find a reflected method and call it.
      SymbolId selector = runtime->swappedComparisonSelector(op);
      Object method(&scope, lookupMethod(thread, right, selector));
      if (!method.isErrorNotFound()) {
        if (!method.isFunction()) {
          Type right_type(&scope, runtime->typeOf(*right));
          method = resolveDescriptorGet(thread, method, right, right_type);
          if (method.isErrorException()) return *method;
        }
        Object result(&scope, callMethod2(thread, method, right, left));
        if (!result.isNotImplementedType()) return *result;
      }
    }
  }

  if (op == CompareOp::EQ) {
    return Bool::fromBool(*left == *right);
  }
  if (op == CompareOp::NE) {
    return Bool::fromBool(*left != *right);
  }

  SymbolId op_symbol = runtime->comparisonSelector(op);
  return thread->raiseUnsupportedBinaryOperation(left, right, op_symbol);
}

HANDLER_INLINE USED RawObject Interpreter::binaryOperationWithMethod(
    Thread* thread, RawObject method, BinaryOpFlags flags, RawObject left,
    RawObject right) {
  DCHECK(method.isFunction(), "function is expected");
  thread->stackPush(method);
  if (flags & kBinaryOpReflected) {
    thread->stackPush(right);
    thread->stackPush(left);
  } else {
    thread->stackPush(left);
    thread->stackPush(right);
  }
  return callFunction(thread, /*nargs=*/2, method);
}

RawObject Interpreter::binaryOperationRetry(Thread* thread, BinaryOp op,
                                            BinaryOpFlags flags,
                                            const Object& left,
                                            const Object& right) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  if (flags & kBinaryOpNotImplementedRetry) {
    // If we tried reflected first, try normal now.
    if (flags & kBinaryOpReflected) {
      SymbolId selector = runtime->binaryOperationSelector(op);
      Object method(&scope, lookupMethod(thread, left, selector));
      if (method.isError()) {
        if (method.isErrorException()) return *method;
        DCHECK(method.isErrorNotFound(), "expected not found");
      } else {
        Object result(&scope, callMethod2(thread, method, left, right));
        if (!result.isNotImplementedType()) return *result;
      }
    } else {
      // If we tried normal first, try to find a reflected method and call it.
      Object method(&scope,
                    binaryOperationLookupReflected(thread, op, left, right));
      if (!method.isErrorNotFound()) {
        if (!method.isFunction()) {
          Type right_type(&scope, runtime->typeOf(*right));
          method = resolveDescriptorGet(thread, method, right, right_type);
          if (method.isErrorException()) return *method;
        }
        Object result(&scope, callMethod2(thread, method, right, left));
        if (!result.isNotImplementedType()) return *result;
      }
    }
  }

  SymbolId op_symbol = runtime->binaryOperationSelector(op);
  return thread->raiseUnsupportedBinaryOperation(left, right, op_symbol);
}

RawObject Interpreter::compareOperation(Thread* thread, CompareOp op,
                                        const Object& left,
                                        const Object& right) {
  return compareOperationSetMethod(thread, op, left, right, nullptr, nullptr);
}

RawObject Interpreter::createIterator(Thread* thread, const Object& iterable) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object dunder_iter(&scope, lookupMethod(thread, iterable, ID(__iter__)));
  if (dunder_iter.isError() || dunder_iter.isNoneType()) {
    if (dunder_iter.isErrorNotFound() &&
        runtime->isSequence(thread, iterable)) {
      return runtime->newSeqIterator(iterable);
    }
    thread->clearPendingException();
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'%T' object is not iterable", &iterable);
  }
  Object iterator(&scope, callMethod1(thread, dunder_iter, iterable));
  if (iterator.isErrorException()) return *iterator;
  if (!runtime->isIterator(thread, iterator)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "iter() returned non-iterator of type '%T'",
                                &iterator);
  }
  return *iterator;
}

RawObject Interpreter::sequenceIterSearch(Thread* thread, const Object& value,
                                          const Object& container) {
  HandleScope scope(thread);
  Object iter(&scope, createIterator(thread, container));
  if (iter.isErrorException()) {
    return *iter;
  }
  Object dunder_next(&scope, lookupMethod(thread, iter, ID(__next__)));
  if (dunder_next.isError()) {
    if (dunder_next.isErrorException()) {
      thread->clearPendingException();
    } else {
      DCHECK(dunder_next.isErrorNotFound(),
             "expected Error::exception() or Error::notFound()");
    }
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__next__ not defined on iterator");
  }
  Object current(&scope, NoneType::object());
  Object compare_result(&scope, NoneType::object());
  Object result(&scope, NoneType::object());
  for (;;) {
    current = callMethod1(thread, dunder_next, iter);
    if (current.isErrorException()) {
      if (thread->hasPendingStopIteration()) {
        thread->clearPendingStopIteration();
        break;
      }
      return *current;
    }
    compare_result = compareOperation(thread, EQ, value, current);
    if (compare_result.isErrorException()) {
      return *compare_result;
    }
    result = isTrue(thread, *compare_result);
    // isTrue can return Error or Bool, and we would want to return on Error or
    // True.
    if (result != Bool::falseObj()) {
      return *result;
    }
  }
  return Bool::falseObj();
}

RawObject Interpreter::sequenceContains(Thread* thread, const Object& value,
                                        const Object& container) {
  return sequenceContainsSetMethod(thread, value, container, nullptr);
}

RawObject Interpreter::sequenceContainsSetMethod(Thread* thread,
                                                 const Object& value,
                                                 const Object& container,
                                                 Object* method_out) {
  HandleScope scope(thread);
  Object method(&scope, lookupMethod(thread, container, ID(__contains__)));
  if (!method.isError()) {
    if (method_out != nullptr && method.isFunction()) *method_out = *method;
    Object result(&scope, callMethod2(thread, method, container, value));
    if (result.isErrorException()) {
      return *result;
    }
    return isTrue(thread, *result);
  }
  if (method.isErrorException()) {
    thread->clearPendingException();
  } else {
    DCHECK(method.isErrorNotFound(),
           "expected Error::exception() or Error::notFound()");
  }
  return sequenceIterSearch(thread, value, container);
}

HANDLER_INLINE USED RawObject Interpreter::isTrue(Thread* thread,
                                                  RawObject value_obj) {
  if (value_obj == Bool::trueObj()) return Bool::trueObj();
  if (value_obj == Bool::falseObj()) return Bool::falseObj();
  return isTrueSlowPath(thread, value_obj);
}

RawObject Interpreter::isTrueSlowPath(Thread* thread, RawObject value_obj) {
  switch (value_obj.layoutId()) {
    case LayoutId::kNoneType:
      return Bool::falseObj();
    case LayoutId::kEllipsis:
    case LayoutId::kFunction:
    case LayoutId::kLargeBytes:
    case LayoutId::kLargeInt:
    case LayoutId::kLargeStr:
    case LayoutId::kModule:
    case LayoutId::kNotImplementedType:
    case LayoutId::kType:
      return Bool::trueObj();
    case LayoutId::kDict:
      return Bool::fromBool(Dict::cast(value_obj).numItems() > 0);
    case LayoutId::kList:
      return Bool::fromBool(List::cast(value_obj).numItems() > 0);
    case LayoutId::kSet:
    case LayoutId::kFrozenSet:
      return Bool::fromBool(RawSetBase::cast(value_obj).numItems() > 0);
    case LayoutId::kSmallBytes:
      return Bool::fromBool(value_obj != Bytes::empty());
    case LayoutId::kSmallInt:
      return Bool::fromBool(value_obj != SmallInt::fromWord(0));
    case LayoutId::kSmallStr:
      return Bool::fromBool(value_obj != Str::empty());
    case LayoutId::kTuple:
      return Bool::fromBool(Tuple::cast(value_obj).length() > 0);
    default:
      break;
  }
  word type_flags =
      thread->runtime()->typeOf(value_obj).rawCast<RawType>().flags();
  if (type_flags & Type::Flag::kHasDunderBool) {
    HandleScope scope(thread);
    Object value(&scope, value_obj);
    Object result(&scope, thread->invokeMethod1(value, ID(__bool__)));
    DCHECK(!result.isErrorNotFound(), "__bool__ is expected to be found");
    if (result.isErrorException()) {
      return *result;
    }
    if (result.isBool()) return *result;
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__bool__ should return bool");
  }
  if (type_flags & Type::Flag::kHasDunderLen) {
    HandleScope scope(thread);
    Object value(&scope, value_obj);
    Object result(&scope, thread->invokeMethod1(value, ID(__len__)));
    DCHECK(!result.isErrorNotFound(), "__len__ is expected to be found");
    if (result.isErrorException()) {
      return *result;
    }
    if (thread->runtime()->isInstanceOfInt(*result)) {
      Int integer(&scope, intUnderlying(*result));
      if (integer.isPositive()) return Bool::trueObj();
      if (integer.isZero()) return Bool::falseObj();
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "__len__() should return >= 0");
    }
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "object cannot be interpreted as an integer");
  }
  return Bool::trueObj();
}

HANDLER_INLINE void Interpreter::raise(Thread* thread, RawObject exc_obj,
                                       RawObject cause_obj) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object exc(&scope, exc_obj);
  Object cause(&scope, cause_obj);
  Object type(&scope, NoneType::object());
  Object value(&scope, NoneType::object());

  if (runtime->isInstanceOfType(*exc) &&
      Type(&scope, *exc).isBaseExceptionSubclass()) {
    // raise was given a BaseException subtype. Use it as the type, and call
    // the type object to create the value.
    type = *exc;
    value = Interpreter::call0(thread, type);
    if (value.isErrorException()) return;
    if (!runtime->isInstanceOfBaseException(*value)) {
      thread->raiseWithFmt(
          LayoutId::kTypeError,
          "calling exception type did not return an instance of BaseException, "
          "but '%T' object",
          &value);
      return;
    }
  } else if (runtime->isInstanceOfBaseException(*exc)) {
    // raise was given an instance of a BaseException subtype. Use it as the
    // value and pull out its type.
    value = *exc;
    type = runtime->typeOf(*value);
  } else {
    // raise was given some other, unexpected value.
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "exceptions must derive from BaseException");
    return;
  }

  // Handle the two-arg form of RAISE_VARARGS, corresponding to "raise x from
  // y". If the cause is a type, call it to create an instance. Either way,
  // attach the cause to the primary exception.
  if (!cause.isErrorNotFound()) {  // TODO(T25860930) use Unbound rather than
                                   // Error.
    if (runtime->isInstanceOfType(*cause) &&
        Type(&scope, *cause).isBaseExceptionSubclass()) {
      cause = Interpreter::call0(thread, cause);
      if (cause.isErrorException()) return;
    } else if (!runtime->isInstanceOfBaseException(*cause) &&
               !cause.isNoneType()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "exception causes must derive from BaseException");
      return;
    }
    BaseException(&scope, *value).setCause(*cause);
  }

  // If we made it here, the process didn't fail with a different exception.
  // Set the pending exception, which is now ready for unwinding. This leaves
  // the VM in a state similar to API functions like PyErr_SetObject(). The
  // main difference is that pendingExceptionValue() will always be an
  // exception instance here, but in the API call case it may be any object
  // (most commonly a str). This discrepancy is cleaned up by
  // normalizeException() in unwind().
  thread->raiseWithType(*type, *value);
}

HANDLER_INLINE void Interpreter::unwindExceptHandler(Thread* thread,
                                                     TryBlock block) {
  // Drop all dead values except for the 3 that are popped into the caught
  // exception state.
  DCHECK(block.kind() == TryBlock::kExceptHandler, "Invalid TryBlock Kind");
  thread->stackDrop(thread->valueStackSize() - block.level() - 3);
  thread->setCaughtExceptionType(thread->stackPop());
  thread->setCaughtExceptionValue(thread->stackPop());
  thread->setCaughtExceptionTraceback(thread->stackPop());
}

NEVER_INLINE static bool handleReturnModes(Thread* thread, word return_mode,
                                           RawObject* retval_ptr) {
  HandleScope scope(thread);
  Object retval(&scope, *retval_ptr);

  if (return_mode & Frame::kProfilerReturn) {
    profiling_return(thread);
  }

  thread->popFrame();
  *retval_ptr = *retval;
  return (return_mode & Frame::kExitRecursiveInterpreter) != 0;
}

RawObject Interpreter::handleReturn(Thread* thread) {
  Frame* frame = thread->currentFrame();
  RawObject retval = thread->stackPop();
  DCHECK(frame->blockStackEmpty(), "block stack should be empty");

  // Check whether we should exit the interpreter loop.
  word return_mode = frame->returnMode();
  if (return_mode == 0) {
    thread->popFrame();
  } else if (return_mode == Frame::kExitRecursiveInterpreter) {
    thread->popFrame();
    return retval;
  } else if (handleReturnModes(thread, return_mode, &retval)) {
    return retval;
  }
  thread->stackPush(retval);
  return Error::error();  // continue interpreter loop.
}

RawObject Interpreter::unwind(Thread* thread) {
  DCHECK(thread->hasPendingException(),
         "unwind() called without a pending exception");
  HandleScope scope(thread);

  Runtime* runtime = thread->runtime();
  Frame* frame = thread->currentFrame();
  Object new_traceback(&scope, NoneType::object());
  Object caught_exc_state(&scope, NoneType::object());
  Object type(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  Object traceback(&scope, NoneType::object());
  for (;;) {
    new_traceback = runtime->newTraceback();
    Traceback::cast(*new_traceback).setFunction(frame->function());
    if (!frame->isNative()) {
      word lasti = frame->virtualPC() - kCodeUnitSize;
      Traceback::cast(*new_traceback).setLasti(SmallInt::fromWord(lasti));
    }
    Traceback::cast(*new_traceback)
        .setNext(thread->pendingExceptionTraceback());
    thread->setPendingExceptionTraceback(*new_traceback);

    while (!frame->blockStackEmpty()) {
      TryBlock block = frame->blockStackPop();
      if (block.kind() == TryBlock::kExceptHandler) {
        unwindExceptHandler(thread, block);
        continue;
      }
      thread->stackDrop(thread->valueStackSize() - block.level());

      if (block.kind() != TryBlock::kFinally) continue;

      // Push a handler block and save the current caught exception, if any.
      frame->blockStackPush(
          TryBlock{TryBlock::kExceptHandler, 0, thread->valueStackSize()});
      caught_exc_state = thread->topmostCaughtExceptionState();
      if (caught_exc_state.isNoneType()) {
        thread->stackPush(NoneType::object());
        thread->stackPush(NoneType::object());
        thread->stackPush(NoneType::object());
      } else {
        thread->stackPush(ExceptionState::cast(*caught_exc_state).traceback());
        thread->stackPush(ExceptionState::cast(*caught_exc_state).value());
        thread->stackPush(ExceptionState::cast(*caught_exc_state).type());
      }

      // Load and normalize the pending exception.
      type = thread->pendingExceptionType();
      value = thread->pendingExceptionValue();
      traceback = thread->pendingExceptionTraceback();
      thread->clearPendingException();
      normalizeException(thread, &type, &value, &traceback);
      BaseException(&scope, *value).setTraceback(*traceback);

      // Promote the normalized exception to caught, push it for the bytecode
      // handler, and jump to the handler.
      thread->setCaughtExceptionType(*type);
      thread->setCaughtExceptionValue(*value);
      thread->setCaughtExceptionTraceback(*traceback);
      thread->stackPush(*traceback);
      thread->stackPush(*value);
      thread->stackPush(*type);
      frame->setVirtualPC(block.handler());
      return Error::error();  // continue interpreter loop.
    }

    word return_mode = frame->returnMode();
    RawObject retval = Error::exception();
    if (return_mode == 0) {
      frame = thread->popFrame();
    } else if (return_mode == Frame::kExitRecursiveInterpreter) {
      thread->popFrame();
      return Error::exception();
    } else if (handleReturnModes(thread, return_mode, &retval)) {
      return retval;
    } else {
      frame = thread->currentFrame();
    }
  }
}

static Bytecode currentBytecode(Thread* thread) {
  Frame* frame = thread->currentFrame();
  word pc = frame->virtualPC() - kCodeUnitSize;
  return static_cast<Bytecode>(frame->bytecode().byteAt(pc));
}

static void rewriteCurrentBytecode(Frame* frame, Bytecode bytecode) {
  word pc = frame->virtualPC() - kCodeUnitSize;
  MutableBytes::cast(frame->bytecode()).byteAtPut(pc, bytecode);
}

HANDLER_INLINE Continue Interpreter::doInvalidBytecode(Thread* thread, word) {
  Bytecode bc = currentBytecode(thread);
  UNREACHABLE("bytecode '%s'", kBytecodeNames[bc]);
}

HANDLER_INLINE Continue Interpreter::doPopTop(Thread* thread, word) {
  thread->stackPop();
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doRotTwo(Thread* thread, word) {
  RawObject peek0 = thread->stackPeek(0);
  RawObject peek1 = thread->stackPeek(1);
  thread->stackSetAt(1, peek0);
  thread->stackSetAt(0, peek1);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doRotThree(Thread* thread, word) {
  RawObject top = thread->stackTop();
  thread->stackSetAt(0, thread->stackPeek(1));
  thread->stackSetAt(1, thread->stackPeek(2));
  thread->stackSetAt(2, top);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doRotFour(Thread* thread, word) {
  RawObject top = thread->stackTop();
  thread->stackSetAt(0, thread->stackPeek(1));
  thread->stackSetAt(1, thread->stackPeek(2));
  thread->stackSetAt(2, thread->stackPeek(3));
  thread->stackSetAt(3, top);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doDupTop(Thread* thread, word) {
  thread->stackPush(thread->stackTop());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doDupTopTwo(Thread* thread, word) {
  RawObject first = thread->stackTop();
  RawObject second = thread->stackPeek(1);
  thread->stackPush(second);
  thread->stackPush(first);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doNop(Thread*, word) {
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doUnaryPositive(Thread* thread, word) {
  return doUnaryOperation(ID(__pos__), thread);
}

HANDLER_INLINE Continue Interpreter::doUnaryNegative(Thread* thread, word) {
  return doUnaryOperation(ID(__neg__), thread);
}

HANDLER_INLINE Continue Interpreter::doUnaryNot(Thread* thread, word) {
  RawObject value = thread->stackTop();
  if (!value.isBool()) {
    value = isTrue(thread, value);
    if (value.isErrorException()) return Continue::UNWIND;
  }
  thread->stackSetTop(RawBool::negate(value));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doUnaryInvert(Thread* thread, word) {
  return doUnaryOperation(ID(__invert__), thread);
}

HANDLER_INLINE Continue Interpreter::doBinaryMatrixMultiply(Thread* thread,
                                                            word) {
  return doBinaryOperation(BinaryOp::MATMUL, thread);
}

HANDLER_INLINE Continue Interpreter::doInplaceMatrixMultiply(Thread* thread,
                                                             word) {
  return doInplaceOperation(BinaryOp::MATMUL, thread);
}

HANDLER_INLINE Continue Interpreter::doBinaryPower(Thread* thread, word) {
  return doBinaryOperation(BinaryOp::POW, thread);
}

HANDLER_INLINE Continue Interpreter::doBinaryMultiply(Thread* thread, word) {
  return doBinaryOperation(BinaryOp::MUL, thread);
}

HANDLER_INLINE Continue Interpreter::doBinaryModulo(Thread* thread, word) {
  return doBinaryOperation(BinaryOp::MOD, thread);
}

HANDLER_INLINE Continue Interpreter::doBinaryAdd(Thread* thread, word) {
  return doBinaryOperation(BinaryOp::ADD, thread);
}

HANDLER_INLINE Continue Interpreter::doBinarySubtract(Thread* thread, word) {
  return doBinaryOperation(BinaryOp::SUB, thread);
}

Continue Interpreter::binarySubscrUpdateCache(Thread* thread, word index) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object container(&scope, thread->stackPeek(1));
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(*container));
  Object getitem(&scope, typeLookupInMroById(thread, *type, ID(__getitem__)));
  if (getitem.isErrorNotFound()) {
    if (runtime->isInstanceOfType(*container)) {
      Type container_as_type(&scope, *container);
      Str dunder_class_getitem_name(
          &scope, runtime->symbols()->at(ID(__class_getitem__)));
      getitem = typeGetAttribute(thread, container_as_type,
                                 dunder_class_getitem_name);
    }
    if (getitem.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "'%T' object is not subscriptable", &container);
      return Continue::UNWIND;
    }
  }
  if (!getitem.isFunction()) {
    getitem = resolveDescriptorGet(thread, getitem, container, type);
    if (getitem.isErrorException()) return Continue::UNWIND;
    thread->stackSetAt(1, *getitem);
    return tailcall(thread, 1);
  }
  if (index >= 0) {
    // TODO(T55274956): Make this into a separate function to be shared.
    MutableTuple caches(&scope, frame->caches());
    Str get_item_name(&scope, runtime->symbols()->at(ID(__getitem__)));
    Function dependent(&scope, frame->function());
    ICState next_cache_state =
        icUpdateAttr(thread, caches, index, container.layoutId(), getitem,
                     get_item_name, dependent);
    rewriteCurrentBytecode(frame, next_cache_state == ICState::kMonomorphic
                                      ? BINARY_SUBSCR_MONOMORPHIC
                                      : BINARY_SUBSCR_POLYMORPHIC);
  }
  thread->stackSetAt(1, *getitem);
  thread->stackInsertAt(1, *container);
  return tailcallFunction(thread, 2, *getitem);
}

HANDLER_INLINE Continue Interpreter::doBinarySubscr(Thread* thread, word) {
  return binarySubscrUpdateCache(thread, -1);
}

HANDLER_INLINE Continue Interpreter::doBinarySubscrDict(Thread* thread,
                                                        word arg) {
  RawObject container = thread->stackPeek(1);
  if (!container.isDict()) {
    EVENT_CACHE(BINARY_SUBSCR_DICT);
    return binarySubscrUpdateCache(thread, arg);
  }
  HandleScope scope(thread);
  Dict dict(&scope, container);
  Object key(&scope, thread->stackPeek(0));
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) {
    return Continue::UNWIND;
  }
  word hash = SmallInt::cast(*hash_obj).value();
  Object result(&scope, dictAt(thread, dict, key, hash));
  if (result.isError()) {
    if (result.isErrorNotFound()) {
      thread->raise(LayoutId::kKeyError, *key);
      return Continue::UNWIND;
    }
    if (result.isErrorException()) {
      return Continue::UNWIND;
    }
    UNREACHABLE("error should be either notFound or errorException");
  }
  thread->stackPop();
  thread->stackSetTop(*result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBinarySubscrList(Thread* thread,
                                                        word arg) {
  RawObject container = thread->stackPeek(1);
  RawObject key = thread->stackPeek(0);
  if (container.isList() && key.isSmallInt()) {
    word index = SmallInt::cast(key).value();
    RawList list = List::cast(container);
    word length = list.numItems();
    if (0 <= index && index < length) {
      thread->stackPop();
      thread->stackSetTop(list.at(index));
      return Continue::NEXT;
    }
  }
  EVENT_CACHE(BINARY_SUBSCR_LIST);
  return binarySubscrUpdateCache(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doBinarySubscrTuple(Thread* thread,
                                                         word arg) {
  RawObject container = thread->stackPeek(1);
  RawObject key = thread->stackPeek(0);
  if (container.isTuple() && key.isSmallInt()) {
    word index = SmallInt::cast(key).value();
    RawTuple tuple = Tuple::cast(container);
    word length = tuple.length();
    if (0 <= index && index < length) {
      thread->stackPop();
      thread->stackSetTop(tuple.at(index));
      return Continue::NEXT;
    }
  }
  EVENT_CACHE(BINARY_SUBSCR_TUPLE);
  return binarySubscrUpdateCache(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doBinarySubscrMonomorphic(Thread* thread,
                                                               word arg) {
  Frame* frame = thread->currentFrame();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  LayoutId receiver_layout_id = thread->stackPeek(1).layoutId();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver_layout_id, &is_found);
  if (!is_found) {
    EVENT_CACHE(BINARY_SUBSCR_MONOMORPHIC);
    return binarySubscrUpdateCache(thread, arg);
  }
  thread->stackInsertAt(2, cached);
  return tailcallFunction(thread, 2, cached);
}

HANDLER_INLINE Continue Interpreter::doBinarySubscrPolymorphic(Thread* thread,
                                                               word arg) {
  Frame* frame = thread->currentFrame();
  LayoutId container_layout_id = thread->stackPeek(1).layoutId();
  bool is_found;
  RawObject cached = icLookupPolymorphic(MutableTuple::cast(frame->caches()),
                                         arg, container_layout_id, &is_found);
  if (!is_found) {
    EVENT_CACHE(BINARY_SUBSCR_POLYMORPHIC);
    return binarySubscrUpdateCache(thread, arg);
  }
  thread->stackInsertAt(2, cached);
  return tailcallFunction(thread, 2, cached);
}

HANDLER_INLINE Continue Interpreter::doBinarySubscrAnamorphic(Thread* thread,
                                                              word arg) {
  Frame* frame = thread->currentFrame();
  RawObject container = thread->stackPeek(1);
  RawObject key = thread->stackPeek(0);
  switch (container.layoutId()) {
    case LayoutId::kDict:
      rewriteCurrentBytecode(frame, BINARY_SUBSCR_DICT);
      return doBinarySubscrDict(thread, arg);
    case LayoutId::kList:
      if (key.isSmallInt()) {
        rewriteCurrentBytecode(frame, BINARY_SUBSCR_LIST);
        return doBinarySubscrList(thread, arg);
      }
      break;
    case LayoutId::kTuple:
      if (key.isSmallInt()) {
        rewriteCurrentBytecode(frame, BINARY_SUBSCR_TUPLE);
        return doBinarySubscrTuple(thread, arg);
      }
      break;
    default:
      break;
  }
  return binarySubscrUpdateCache(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doBinaryFloorDivide(Thread* thread, word) {
  return doBinaryOperation(BinaryOp::FLOORDIV, thread);
}

HANDLER_INLINE Continue Interpreter::doBinaryTrueDivide(Thread* thread, word) {
  return doBinaryOperation(BinaryOp::TRUEDIV, thread);
}

HANDLER_INLINE Continue Interpreter::doInplaceFloorDivide(Thread* thread,
                                                          word) {
  return doInplaceOperation(BinaryOp::FLOORDIV, thread);
}

HANDLER_INLINE Continue Interpreter::doInplaceTrueDivide(Thread* thread, word) {
  return doInplaceOperation(BinaryOp::TRUEDIV, thread);
}

HANDLER_INLINE Continue Interpreter::doGetAiter(Thread* thread, word) {
  HandleScope scope(thread);
  Object obj(&scope, thread->stackPop());
  Object method(&scope, lookupMethod(thread, obj, ID(__aiter__)));
  if (method.isError()) {
    if (method.isErrorException()) {
      thread->clearPendingException();
    } else {
      DCHECK(method.isErrorNotFound(),
             "expected Error::exception() or Error::notFound()");
    }
    thread->raiseWithFmt(
        LayoutId::kTypeError,
        "'async for' requires an object with __aiter__ method");
    return Continue::UNWIND;
  }
  return tailcallMethod1(thread, *method, *obj);
}

HANDLER_INLINE Continue Interpreter::doGetAnext(Thread* thread, word) {
  HandleScope scope(thread);
  Object obj(&scope, thread->stackTop());
  // TODO(T67736679) Add inline caching for this method lookup.
  Object anext(&scope, lookupMethod(thread, obj, ID(__anext__)));
  if (anext.isError()) {
    if (anext.isErrorException()) {
      thread->clearPendingException();
    } else {
      DCHECK(anext.isErrorNotFound(),
             "expected Error::exception() or Error::notFound()");
    }
    thread->raiseWithFmt(
        LayoutId::kTypeError,
        "'async for' requires an iterator with __anext__ method");
    return Continue::UNWIND;
  }
  Object awaitable(&scope, callMethod1(thread, anext, obj));
  if (awaitable.isErrorException()) return Continue::UNWIND;
  thread->stackPush(*awaitable);
  // TODO(T67736679) Add inline caching for the lookupMethod() in
  // awaitableIter.
  Object result(
      &scope,
      awaitableIter(thread,
                    "'async for' received an invalid object from __anext__"));
  if (!result.isError()) return Continue::NEXT;
  thread->raiseWithFmtChainingPendingAsCause(
      LayoutId::kTypeError,
      "'async for' received an invalid object from __anext__");
  return Continue::UNWIND;
}

HANDLER_INLINE Continue Interpreter::doBeginFinally(Thread* thread, word) {
  thread->stackPush(NoneType::object());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBeforeAsyncWith(Thread* thread, word) {
  HandleScope scope(thread);
  Object manager(&scope, thread->stackPop());

  // resolve __aexit__ and push it
  Runtime* runtime = thread->runtime();
  Object exit(&scope, runtime->attributeAtById(thread, manager, ID(__aexit__)));
  if (exit.isErrorException()) {
    return Continue::UNWIND;
  }
  thread->stackPush(*exit);

  // resolve __aenter__ call it and push the return value
  Object enter(&scope, lookupMethod(thread, manager, ID(__aenter__)));
  if (enter.isError()) {
    if (enter.isErrorNotFound()) {
      Object aenter_str(&scope, runtime->newStrFromFmt("__aenter__"));
      objectRaiseAttributeError(thread, manager, aenter_str);
      return Continue::UNWIND;
    }
    if (enter.isErrorException()) {
      return Continue::UNWIND;
    }
  }
  return tailcallMethod1(thread, *enter, *manager);
}

HANDLER_INLINE Continue Interpreter::doInplaceAdd(Thread* thread, word) {
  return doInplaceOperation(BinaryOp::ADD, thread);
}

HANDLER_INLINE Continue Interpreter::doInplaceSubtract(Thread* thread, word) {
  return doInplaceOperation(BinaryOp::SUB, thread);
}

HANDLER_INLINE Continue Interpreter::doInplaceMultiply(Thread* thread, word) {
  return doInplaceOperation(BinaryOp::MUL, thread);
}

HANDLER_INLINE Continue Interpreter::doInplaceModulo(Thread* thread, word) {
  return doInplaceOperation(BinaryOp::MOD, thread);
}

HANDLER_INLINE Continue Interpreter::doStoreSubscr(Thread* thread, word) {
  HandleScope scope(thread);
  Object key(&scope, thread->stackPop());
  Object container(&scope, thread->stackPop());
  Object setitem(&scope, lookupMethod(thread, container, ID(__setitem__)));
  if (setitem.isError()) {
    if (setitem.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "'%T' object does not support item assignment",
                           &container);
    } else {
      DCHECK(setitem.isErrorException(),
             "expected Error::exception() or Error::notFound()");
    }
    return Continue::UNWIND;
  }
  Object value(&scope, thread->stackPop());
  if (callMethod3(thread, setitem, container, key, value).isErrorException()) {
    return Continue::UNWIND;
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreSubscrList(Thread* thread,
                                                       word arg) {
  RawObject container = thread->stackPeek(1);
  RawObject key = thread->stackPeek(0);
  if (container.isList() && key.isSmallInt()) {
    word index = SmallInt::cast(key).value();
    RawList list = List::cast(container);
    word length = list.numItems();
    if (0 <= index && index < length) {
      RawObject value = thread->stackPeek(2);
      list.atPut(index, value);
      thread->stackDrop(3);
      return Continue::NEXT;
    }
  }
  return storeSubscrUpdateCache(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doStoreSubscrDict(Thread* thread,
                                                       word arg) {
  RawObject container = thread->stackPeek(1);
  if (!container.isDict()) {
    return storeSubscrUpdateCache(thread, arg);
  }
  HandleScope scope(thread);
  Dict dict(&scope, container);
  Object key(&scope, thread->stackPeek(0));
  Object value(&scope, thread->stackPeek(2));
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) {
    return Continue::UNWIND;
  }
  word hash = SmallInt::cast(*hash_obj).value();
  if (dictAtPut(thread, dict, key, hash, value).isErrorException()) {
    return Continue::UNWIND;
  }
  thread->stackDrop(3);
  return Continue::NEXT;
}

NEVER_INLINE Continue Interpreter::storeSubscrUpdateCache(Thread* thread,
                                                          word arg) {
  HandleScope scope(thread);
  Object key(&scope, thread->stackPop());
  Object container(&scope, thread->stackPop());
  Object setitem(&scope, lookupMethod(thread, container, ID(__setitem__)));
  if (setitem.isError()) {
    if (setitem.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "'%T' object does not support item assignment",
                           &container);
    } else {
      DCHECK(setitem.isErrorException(),
             "expected Error::exception() or Error::notFound()");
    }
    return Continue::UNWIND;
  }
  if (setitem.isFunction()) {
    Frame* frame = thread->currentFrame();
    MutableTuple caches(&scope, frame->caches());
    Str set_item_name(&scope,
                      thread->runtime()->symbols()->at(ID(__setitem__)));
    Function dependent(&scope, frame->function());
    ICState next_cache_state =
        icUpdateAttr(thread, caches, arg, container.layoutId(), setitem,
                     set_item_name, dependent);
    rewriteCurrentBytecode(frame, next_cache_state == ICState::kMonomorphic
                                      ? STORE_SUBSCR_MONOMORPHIC
                                      : STORE_SUBSCR_POLYMORPHIC);
  }
  Object value(&scope, thread->stackPop());
  if (callMethod3(thread, setitem, container, key, value).isErrorException()) {
    return Continue::UNWIND;
  }
  return Continue::NEXT;
}

ALWAYS_INLINE Continue Interpreter::storeSubscr(Thread* thread,
                                                RawObject set_item_method) {
  DCHECK(set_item_method.isFunction(), "cached should be a function");
  // The shape of the frame before STORE_SUBSCR:
  //   2: value
  //   1: container
  //   0: key
  //
  // The shape of the frame is modified to call __setitem__ as follows:
  //   3: function (__setitem__)
  //   2: container
  //   1: key
  //   0: value
  RawObject value_raw = thread->stackPeek(2);
  thread->stackSetAt(2, set_item_method);
  thread->stackPush(value_raw);

  RawObject result = callFunction(thread, /*nargs=*/3, set_item_method);
  if (result.isErrorException()) {
    return Continue::UNWIND;
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreSubscrMonomorphic(Thread* thread,
                                                              word arg) {
  Frame* frame = thread->currentFrame();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  LayoutId container_layout_id = thread->stackPeek(1).layoutId();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, container_layout_id, &is_found);
  if (!is_found) {
    EVENT_CACHE(STORE_SUBSCR_MONOMORPHIC);
    return storeSubscrUpdateCache(thread, arg);
  }
  return storeSubscr(thread, cached);
}

HANDLER_INLINE Continue Interpreter::doStoreSubscrPolymorphic(Thread* thread,
                                                              word arg) {
  Frame* frame = thread->currentFrame();
  RawObject container_raw = thread->stackPeek(1);
  LayoutId container_layout_id = container_raw.layoutId();
  bool is_found;
  RawObject cached = icLookupPolymorphic(MutableTuple::cast(frame->caches()),
                                         arg, container_layout_id, &is_found);
  if (!is_found) {
    EVENT_CACHE(STORE_SUBSCR_POLYMORPHIC);
    return storeSubscrUpdateCache(thread, arg);
  }
  return storeSubscr(thread, cached);
}

HANDLER_INLINE Continue Interpreter::doStoreSubscrAnamorphic(Thread* thread,
                                                             word arg) {
  RawObject container = thread->stackPeek(1);
  RawObject key = thread->stackPeek(0);
  switch (container.layoutId()) {
    case LayoutId::kDict:
      rewriteCurrentBytecode(thread->currentFrame(), STORE_SUBSCR_DICT);
      return doStoreSubscrDict(thread, arg);
    case LayoutId::kList:
      if (key.isSmallInt()) {
        rewriteCurrentBytecode(thread->currentFrame(), STORE_SUBSCR_LIST);
        return doStoreSubscrList(thread, arg);
      }
      break;
    default:
      break;
  }
  return storeSubscrUpdateCache(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doDeleteSubscr(Thread* thread, word) {
  HandleScope scope(thread);
  Object key(&scope, thread->stackPop());
  Object container(&scope, thread->stackPop());
  Object delitem(&scope, lookupMethod(thread, container, ID(__delitem__)));
  if (delitem.isError()) {
    if (delitem.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "'%T' object does not support item deletion",
                           &container);
    } else {
      DCHECK(delitem.isErrorException(),
             "expected Error::exception() or Error::notFound()");
    }
    return Continue::UNWIND;
  }
  if (callMethod2(thread, delitem, container, key).isErrorException()) {
    return Continue::UNWIND;
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBinaryLshift(Thread* thread, word) {
  return doBinaryOperation(BinaryOp::LSHIFT, thread);
}

HANDLER_INLINE Continue Interpreter::doBinaryRshift(Thread* thread, word) {
  return doBinaryOperation(BinaryOp::RSHIFT, thread);
}

HANDLER_INLINE Continue Interpreter::doBinaryAnd(Thread* thread, word) {
  return doBinaryOperation(BinaryOp::AND, thread);
}

HANDLER_INLINE Continue Interpreter::doBinaryXor(Thread* thread, word) {
  return doBinaryOperation(BinaryOp::XOR, thread);
}

HANDLER_INLINE Continue Interpreter::doBinaryOr(Thread* thread, word) {
  return doBinaryOperation(BinaryOp::OR, thread);
}

HANDLER_INLINE Continue Interpreter::doInplacePower(Thread* thread, word) {
  return doInplaceOperation(BinaryOp::POW, thread);
}

HANDLER_INLINE Continue Interpreter::doGetIter(Thread* thread, word) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object iterable(&scope, thread->stackPop());
  Object iterator(&scope, NoneType::object());
  switch (iterable.layoutId()) {
    case LayoutId::kList:
      iterator = runtime->newListIterator(iterable);
      break;
    case LayoutId::kDict: {
      Dict dict(&scope, *iterable);
      iterator = runtime->newDictKeyIterator(thread, dict);
      break;
    }
    case LayoutId::kGenerator:
      iterator = *iterable;
      break;
    case LayoutId::kTuple: {
      Tuple tuple(&scope, *iterable);
      iterator = runtime->newTupleIterator(tuple, tuple.length());
      break;
    }
    case LayoutId::kRange: {
      Range range(&scope, *iterable);
      Int start_int(&scope, intUnderlying(range.start()));
      Int stop_int(&scope, intUnderlying(range.stop()));
      Int step_int(&scope, intUnderlying(range.step()));
      if (start_int.isLargeInt() || stop_int.isLargeInt() ||
          step_int.isLargeInt()) {
        iterator = runtime->newLongRangeIterator(start_int, stop_int, step_int);
        break;
      }
      word start = start_int.asWord();
      word stop = stop_int.asWord();
      word step = step_int.asWord();
      word length = Slice::length(start, stop, step);
      if (SmallInt::isValid(length)) {
        iterator = runtime->newRangeIterator(start, step, length);
        break;
      }
      iterator = runtime->newLongRangeIterator(start_int, stop_int, step_int);
      break;
    }
    case LayoutId::kStr: {
      Str str(&scope, *iterable);
      iterator = runtime->newStrIterator(str);
      break;
    }
    case LayoutId::kBytearray: {
      Bytearray byte_array(&scope, *iterable);
      iterator = runtime->newBytearrayIterator(thread, byte_array);
      break;
    }
    case LayoutId::kBytes: {
      Bytes bytes(&scope, *iterable);
      iterator = runtime->newBytesIterator(thread, bytes);
      break;
    }
    case LayoutId::kSet: {
      Set set(&scope, *iterable);
      iterator = thread->runtime()->newSetIterator(set);
      break;
    }
    default:
      break;
  }
  if (!iterator.isNoneType()) {
    thread->stackPush(*iterator);
    return Continue::NEXT;
  }
  // TODO(T44729606): Add caching, and turn into a simpler call for builtin
  // types with known iterator creating functions
  iterator = createIterator(thread, iterable);
  if (iterator.isErrorException()) return Continue::UNWIND;
  thread->stackPush(*iterator);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doGetYieldFromIter(Thread* thread, word) {
  HandleScope scope(thread);
  Object iterable(&scope, thread->stackTop());

  if (iterable.isGenerator()) return Continue::NEXT;

  if (iterable.isCoroutine()) {
    Function function(&scope, thread->currentFrame()->function());
    if (!(function.isCoroutine() || function.isIterableCoroutine())) {
      thread->raiseWithFmt(
          LayoutId::kTypeError,
          "cannot 'yield from' a coroutine object in a non-coroutine "
          "generator");
      return Continue::UNWIND;
    }
    return Continue::NEXT;
  }

  thread->stackDrop(1);
  // TODO(T44729661): Add caching, and turn into a simpler call for builtin
  // types with known iterator creating functions
  Object iterator(&scope, createIterator(thread, iterable));
  if (iterator.isErrorException()) return Continue::UNWIND;
  thread->stackPush(*iterator);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doPrintExpr(Thread* thread, word) {
  HandleScope scope(thread);
  Object value(&scope, thread->stackPop());
  ValueCell value_cell(&scope, thread->runtime()->displayHook());
  if (value_cell.isUnbound()) {
    UNIMPLEMENTED("RuntimeError: lost sys.displayhook");
  }
  // TODO(T55021263): Replace with non-recursive call
  Object display_hook(&scope, value_cell.value());
  return callMethod1(thread, display_hook, value).isErrorException()
             ? Continue::UNWIND
             : Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadBuildClass(Thread* thread, word) {
  RawValueCell value_cell = ValueCell::cast(thread->runtime()->buildClass());
  thread->stackPush(value_cell.value());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doYieldFrom(Thread* thread, word) {
  HandleScope scope(thread);

  Object value(&scope, thread->stackPop());
  Object iterator(&scope, thread->stackTop());
  Object result(&scope, NoneType::object());
  if (iterator.isGenerator()) {
    result = generatorSend(thread, iterator, value);
  } else if (iterator.isCoroutine()) {
    result = coroutineSend(thread, iterator, value);
  } else if (!value.isNoneType()) {
    Object send_method(&scope, lookupMethod(thread, iterator, ID(send)));
    if (send_method.isError()) {
      if (send_method.isErrorException()) {
        thread->clearPendingException();
      } else {
        DCHECK(send_method.isErrorNotFound(),
               "expected Error::exception() or Error::notFound()");
      }
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "iter() returned non-iterator");
      return Continue::UNWIND;
    }
    result = callMethod2(thread, send_method, iterator, value);
  } else {
    Object next_method(&scope, lookupMethod(thread, iterator, ID(__next__)));
    if (next_method.isError()) {
      if (next_method.isErrorException()) {
        thread->clearPendingException();
      } else {
        DCHECK(next_method.isErrorNotFound(),
               "expected Error::exception() or Error::notFound()");
      }
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "iter() returned non-iterator");
      return Continue::UNWIND;
    }
    result = callMethod1(thread, next_method, iterator);
  }
  if (result.isErrorException()) {
    if (!thread->hasPendingStopIteration()) return Continue::UNWIND;

    thread->stackSetTop(thread->pendingStopIterationValue());
    thread->clearPendingException();
    return Continue::NEXT;
  }

  // Decrement PC: We want this to re-execute until the subiterator is
  // exhausted.
  Frame* frame = thread->currentFrame();
  frame->setVirtualPC(frame->virtualPC() - kCodeUnitSize);
  thread->stackPush(*result);
  return Continue::YIELD;
}

RawObject Interpreter::awaitableIter(Thread* thread,
                                     const char* invalid_type_message) {
  HandleScope scope(thread);
  Object obj(&scope, thread->stackTop());
  if (obj.isCoroutine() || obj.isAsyncGenerator()) {
    return *obj;
  }
  if (obj.isGenerator()) {
    Generator generator(&scope, *obj);
    GeneratorFrame generator_frame(&scope, generator.generatorFrame());
    Function func(&scope, generator_frame.function());
    if (func.isIterableCoroutine()) {
      return *obj;
    }
    return thread->raiseWithFmt(LayoutId::kTypeError, invalid_type_message);
  }
  thread->stackPop();
  Object await(&scope, lookupMethod(thread, obj, ID(__await__)));
  if (await.isError()) {
    if (await.isErrorException()) {
      thread->clearPendingException();
    } else {
      DCHECK(await.isErrorNotFound(),
             "expected Error::exception() or Error::notFound()");
    }
    return thread->raiseWithFmt(LayoutId::kTypeError, invalid_type_message);
  }
  Object result(&scope, callMethod1(thread, await, obj));
  if (result.isError()) return *result;
  if (result.isGenerator()) {
    Generator gen(&scope, *result);
    GeneratorFrame gen_frame(&scope, gen.generatorFrame());
    Function gen_func(&scope, gen_frame.function());
    if (gen_func.isIterableCoroutine()) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "__await__() returned a coroutine");
    }
  }
  if (result.isCoroutine()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__await__() returned a coroutine");
  }
  // This check is lower priority than for coroutine above which will also fail
  // isIterator() and raise TypeError but with a different string.
  if (!thread->runtime()->isIterator(thread, result)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "__await__() returned non-iterator of type '%T'",
        &result);
  }
  thread->stackPush(*result);
  return *obj;
}

HANDLER_INLINE Continue Interpreter::doGetAwaitable(Thread* thread, word) {
  // TODO(T67736679) Add inline caching for the lookupMethod() in awaitableIter.
  RawObject iter =
      awaitableIter(thread, "object can't be used in 'await' expression");
  if (iter.isError()) {
    return Continue::UNWIND;
  }
  if (iter.isCoroutine()) {
    if (!findYieldFrom(GeneratorBase::cast(iter)).isNoneType()) {
      thread->raiseWithFmt(LayoutId::kRuntimeError,
                           "coroutine is being awaited already");
      return Continue::UNWIND;
    }
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doInplaceLshift(Thread* thread, word) {
  return doInplaceOperation(BinaryOp::LSHIFT, thread);
}

HANDLER_INLINE Continue Interpreter::doInplaceRshift(Thread* thread, word) {
  return doInplaceOperation(BinaryOp::RSHIFT, thread);
}

HANDLER_INLINE Continue Interpreter::doInplaceAnd(Thread* thread, word) {
  return doInplaceOperation(BinaryOp::AND, thread);
}

HANDLER_INLINE Continue Interpreter::doInplaceXor(Thread* thread, word) {
  return doInplaceOperation(BinaryOp::XOR, thread);
}

HANDLER_INLINE Continue Interpreter::doInplaceOr(Thread* thread, word) {
  return doInplaceOperation(BinaryOp::OR, thread);
}

HANDLER_INLINE Continue Interpreter::doWithCleanupStart(Thread* thread, word) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object exc(&scope, thread->stackPop());
  Object value(&scope, NoneType::object());
  Object traceback(&scope, NoneType::object());
  Object exit(&scope, NoneType::object());

  // The stack currently contains a sequence of values understood by
  // END_FINALLY, followed by __exit__ from the context manager. We need to
  // determine the location of __exit__ and remove it from the stack, shifting
  // everything above it down to compensate.
  if (exc.isNoneType()) {
    // The with block exited normally. __exit__ is just below the None.
    exit = thread->stackTop();
    thread->stackSetTop(NoneType::object());
  } else {
    DCHECK(thread->runtime()->isInstanceOfType(*exc) &&
               exc.rawCast<RawType>().isBaseExceptionSubclass(),
           "expected BaseException subclass");
    // The stack contains the caught exception, the previous exception state,
    // then __exit__. Grab __exit__ then shift everything else down.
    exit = thread->stackPeek(5);
    for (word i = 5; i > 0; i--) {
      thread->stackSetAt(i, thread->stackPeek(i - 1));
    }

    // Put exc at the top of the stack and grab value/traceback from below it.
    thread->stackSetTop(*exc);
    value = thread->stackPeek(1);
    traceback = thread->stackPeek(2);

    // We popped __exit__ out from under the depth recorded by the top
    // ExceptHandler block, so adjust it.
    TryBlock block = frame->blockStackPop();
    DCHECK(block.kind() == TryBlock::kExceptHandler,
           "Unexpected TryBlock Kind");
    frame->blockStackPush(
        TryBlock(block.kind(), block.handler(), block.level() - 1));
  }

  // Push exc, to be consumed by WITH_CLEANUP_FINISH.
  thread->stackPush(*exc);

  // Call exit(exc, value, traceback), leaving the result on the stack for
  // WITH_CLEANUP_FINISH.
  thread->stackPush(*exit);
  thread->stackPush(*exc);
  thread->stackPush(*value);
  thread->stackPush(*traceback);
  return tailcall(thread, 3);
}

HANDLER_INLINE Continue Interpreter::doWithCleanupFinish(Thread* thread, word) {
  HandleScope scope(thread);
  Object result(&scope, thread->stackPop());
  Object exc(&scope, thread->stackPop());
  if (exc.isNoneType()) return Continue::NEXT;

  Object is_true(&scope, isTrue(thread, *result));
  if (is_true.isErrorException()) return Continue::UNWIND;
  if (*is_true == Bool::trueObj()) {
    Frame* frame = thread->currentFrame();
    TryBlock block = frame->blockStackPop();
    DCHECK(block.kind() == TryBlock::kExceptHandler, "expected kExceptHandler");
    unwindExceptHandler(thread, block);
    thread->stackPush(NoneType::object());
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doReturnValue(Thread*, word) {
  return Continue::RETURN;
}

HANDLER_INLINE Continue Interpreter::doSetupAnnotations(Thread* thread, word) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Frame* frame = thread->currentFrame();
  Str dunder_annotations(&scope, runtime->symbols()->at(ID(__annotations__)));
  if (frame->implicitGlobals().isNoneType()) {
    // Module body
    Module module(&scope, frame->function().moduleObject());
    if (moduleAt(module, dunder_annotations).isErrorNotFound()) {
      Object annotations(&scope, runtime->newDict());
      moduleAtPut(thread, module, dunder_annotations, annotations);
    }
  } else {
    // Class body
    Object implicit_globals(&scope, frame->implicitGlobals());
    if (implicit_globals.isDict()) {
      Dict implicit_globals_dict(&scope, frame->implicitGlobals());
      word hash = strHash(thread, *dunder_annotations);
      Object include_result(&scope, dictIncludes(thread, implicit_globals_dict,
                                                 dunder_annotations, hash));
      if (include_result.isErrorException()) {
        return Continue::UNWIND;
      }
      if (include_result == Bool::falseObj()) {
        Object annotations(&scope, runtime->newDict());
        if (dictAtPut(thread, implicit_globals_dict, dunder_annotations, hash,
                      annotations)
                .isErrorException()) {
          return Continue::UNWIND;
        }
      }
    } else {
      if (objectGetItem(thread, implicit_globals, dunder_annotations)
              .isErrorException()) {
        if (!thread->pendingExceptionMatches(LayoutId::kKeyError)) {
          return Continue::UNWIND;
        }
        thread->clearPendingException();
        Object annotations(&scope, runtime->newDict());
        if (objectSetItem(thread, implicit_globals, dunder_annotations,
                          annotations)
                .isErrorException()) {
          return Continue::UNWIND;
        }
      }
    }
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doYieldValue(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  // Wrap values directly yielded from asynchronous generator. This
  // distinguishes generator-like yields from async-like yields which propagate
  // from awaitables via `YIELD_FROM`.
  if (Code::cast(frame->code()).isAsyncGenerator()) {
    HandleScope scope(thread);
    Object value(&scope, thread->stackPop());
    Runtime* runtime = thread->runtime();
    Layout async_gen_wrapped_value_layout(
        &scope, runtime->layoutAt(LayoutId::kAsyncGeneratorWrappedValue));
    AsyncGeneratorWrappedValue wrapped_value(
        &scope, runtime->newInstance(async_gen_wrapped_value_layout));
    wrapped_value.setValue(*value);
    thread->stackPush(*wrapped_value);
  }
  return Continue::YIELD;
}

static RawObject implicitGlobalsAtPut(Thread* thread, Frame* frame,
                                      const Object& implicit_globals_obj,
                                      const Str& name, const Object& value) {
  HandleScope scope(thread);
  if (implicit_globals_obj.isNoneType()) {
    Module module(&scope, frame->function().moduleObject());
    moduleAtPut(thread, module, name, value);
    return NoneType::object();
  }
  if (implicit_globals_obj.isDict()) {
    Dict implicit_globals(&scope, *implicit_globals_obj);
    dictAtPutByStr(thread, implicit_globals, name, value);
  } else {
    Object result(&scope,
                  objectSetItem(thread, implicit_globals_obj, name, value));
    if (result.isErrorException()) return *result;
  }
  return NoneType::object();
}

static RawObject callImportAllFrom(Thread* thread, Frame* frame,
                                   const Object& object) {
  HandleScope scope(thread);
  Object implicit_globals(&scope, frame->implicitGlobals());
  if (implicit_globals.isNoneType()) {
    Module module(&scope, frame->function().moduleObject());
    implicit_globals = module.moduleProxy();
  }
  return thread->invokeFunction2(ID(builtins), ID(_import_all_from),
                                 implicit_globals, object);
}

RawObject Interpreter::importAllFrom(Thread* thread, Frame* frame,
                                     const Object& object) {
  // We have a short-cut if `object` is a module and `__all__` does not exist
  // or is a tuple or list; otherwise call `builtins._import_all_from`.
  if (!object.isModule()) {
    return callImportAllFrom(thread, frame, object);
  }

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  bool skip_names_with_underscore_prefix = false;
  Module module(&scope, *object);
  Object dunder_all(&scope, runtime->symbols()->at(ID(__all__)));
  Object all_obj(&scope, moduleGetAttribute(thread, module, dunder_all));
  if (all_obj.isErrorException()) return *all_obj;
  if (all_obj.isErrorNotFound()) {
    all_obj = moduleKeys(thread, module);
    skip_names_with_underscore_prefix = true;
  }
  Tuple all(&scope, runtime->emptyTuple());
  word all_len;
  if (all_obj.isList()) {
    all = List::cast(*all_obj).items();
    all_len = List::cast(*all_obj).numItems();
  } else if (all_obj.isTuple()) {
    all = Tuple::cast(*all_obj);
    all_len = all.length();
  } else {
    return callImportAllFrom(thread, frame, object);
  }

  Object implicit_globals(&scope, frame->implicitGlobals());
  Object name(&scope, NoneType::object());
  Str interned(&scope, Str::empty());
  Object value(&scope, NoneType::object());
  for (word i = 0; i < all_len; i++) {
    name = all.at(i);
    interned = attributeName(thread, name);
    if (interned.isErrorException()) return *interned;
    if (skip_names_with_underscore_prefix && interned.length() > 0 &&
        interned.byteAt(0) == '_') {
      continue;
    }
    value = moduleGetAttribute(thread, module, interned);
    if (value.isErrorNotFound()) {
      return moduleRaiseAttributeError(thread, module, interned);
    }
    if (value.isErrorException()) return *value;
    value =
        implicitGlobalsAtPut(thread, frame, implicit_globals, interned, value);
    if (value.isErrorException()) return *value;
  }
  return NoneType::object();
}

HANDLER_INLINE Continue Interpreter::doImportStar(Thread* thread, word) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();

  // Pre-python3 this used to merge the locals with the locals dict. However,
  // that's not necessary anymore. You can't import * inside a function
  // body anymore.

  Object object(&scope, thread->stackPop());
  if (importAllFrom(thread, frame, object).isErrorException()) {
    return Continue::UNWIND;
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doPopBlock(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  frame->blockStackPop();
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doEndAsyncFor(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  Runtime* runtime = thread->runtime();
  RawObject exc = thread->stackPop();
  DCHECK(runtime->isInstanceOfType(exc) &&
             exc.rawCast<RawType>().isBaseExceptionSubclass(),
         "Expected BaseException subclass");
  // Check if TOS is StopIteration type or a subclass of it.
  if (typeIsSubclass(exc, runtime->typeAt(LayoutId::kStopAsyncIteration))) {
    TryBlock block = frame->blockStackPop();
    unwindExceptHandler(thread, block);
    thread->stackPop();
    frame->setVirtualPC(frame->virtualPC() + arg);
    return Continue::NEXT;
  }

  thread->setPendingExceptionType(exc);
  thread->setPendingExceptionValue(thread->stackPop());
  thread->setPendingExceptionTraceback(thread->stackPop());
  return Continue::UNWIND;
}

HANDLER_INLINE Continue Interpreter::doEndFinally(Thread* thread, word) {
  RawObject top = thread->stackPop();
  if (top.isNoneType()) {
    return Continue::NEXT;
  }
  if (top.isSmallInt()) {
    word value = SmallInt::cast(top).value();
    if (value == -1 && thread->hasPendingException()) {
      return Continue::UNWIND;
    }
    Frame* frame = thread->currentFrame();
    frame->setVirtualPC(value);
    return Continue::NEXT;
  }
  DCHECK(thread->runtime()->isInstanceOfType(top) &&
             top.rawCast<RawType>().isBaseExceptionSubclass(),
         "expected None, SmallInt or BaseException subclass");
  thread->setPendingExceptionType(top);
  thread->setPendingExceptionValue(thread->stackPop());
  thread->setPendingExceptionTraceback(thread->stackPop());
  return Continue::UNWIND;
}

HANDLER_INLINE Continue Interpreter::doPopExcept(Thread* thread, word) {
  Frame* frame = thread->currentFrame();

  TryBlock block = frame->blockStackPop();
  DCHECK(block.kind() == TryBlock::kExceptHandler,
         "popped block is not an except handler");
  word level = block.level();
  word current_level = thread->valueStackSize();
  // The only things left on the stack at this point should be the exc_type,
  // exc_value, exc_traceback values and potentially a result value.
  DCHECK(current_level == level + 3 || current_level == level + 4,
         "unexpected level");
  thread->setCaughtExceptionType(thread->stackPop());
  thread->setCaughtExceptionValue(thread->stackPop());
  thread->setCaughtExceptionTraceback(thread->stackPop());

  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doPopFinally(Thread* thread, word arg) {
  HandleScope scope(thread);
  Object res(&scope, NoneType::object());
  if (arg != 0) {
    res = thread->stackPop();
  }
  Object exc(&scope, thread->stackPop());
  if (exc.isNoneType() || exc.isInt()) {
  } else {
    thread->stackPop();
    thread->stackPop();
    Frame* frame = thread->currentFrame();
    TryBlock block = frame->blockStackPop();
    if (block.kind() != TryBlock::Kind::kExceptHandler) {
      thread->raiseWithFmt(LayoutId::kSystemError,
                           "popped block is not an except handler");
      return Continue::UNWIND;
    }
    thread->setCaughtExceptionType(thread->stackPop());
    thread->setCaughtExceptionValue(thread->stackPop());
    thread->setCaughtExceptionTraceback(thread->stackPop());
  }
  if (arg != 0) {
    thread->stackPush(*res);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doCallFinally(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  word next_pc = frame->virtualPC();
  thread->stackPush(SmallInt::fromWord(next_pc));
  frame->setVirtualPC(next_pc + arg);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreName(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  RawObject names = Code::cast(frame->code()).names();
  Str name(&scope, Tuple::cast(names).at(arg));
  Object value(&scope, thread->stackPop());
  Object implicit_globals(&scope, frame->implicitGlobals());
  if (implicitGlobalsAtPut(thread, frame, implicit_globals, name, value)
          .isErrorException()) {
    return Continue::UNWIND;
  }
  return Continue::NEXT;
}

static Continue raiseUndefinedName(Thread* thread, const Object& name) {
  thread->raiseWithFmt(LayoutId::kNameError, "name '%S' is not defined", &name);
  return Continue::UNWIND;
}

HANDLER_INLINE Continue Interpreter::doDeleteName(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object implicit_globals_obj(&scope, frame->implicitGlobals());
  // Forward to doDeleteGlobal() when implicit globals and globals are the same.
  // This avoids duplicating all the cache invalidation logic here.
  // TODO(T47581831) This should be removed and invalidation should happen when
  // changing the globals dictionary.
  if (implicit_globals_obj.isNoneType()) {
    return doDeleteGlobal(thread, arg);
  }
  RawObject names = Code::cast(frame->code()).names();
  Str name(&scope, Tuple::cast(names).at(arg));
  if (implicit_globals_obj.isDict()) {
    Dict implicit_globals(&scope, *implicit_globals_obj);
    if (dictRemoveByStr(thread, implicit_globals, name).isErrorNotFound()) {
      return raiseUndefinedName(thread, name);
    }
  } else {
    if (objectDelItem(thread, implicit_globals_obj, name).isErrorException()) {
      thread->clearPendingException();
      return raiseUndefinedName(thread, name);
    }
  }
  return Continue::NEXT;
}

static HANDLER_INLINE Continue unpackSequenceWithLength(Thread* thread,
                                                        const Tuple& tuple,
                                                        word count,
                                                        word length) {
  if (length < count) {
    thread->raiseWithFmt(LayoutId::kValueError, "not enough values to unpack");
    return Continue::UNWIND;
  }
  if (length > count) {
    thread->raiseWithFmt(LayoutId::kValueError, "too many values to unpack");
    return Continue::UNWIND;
  }
  for (word i = length - 1; i >= 0; i--) {
    thread->stackPush(tuple.at(i));
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doUnpackSequence(Thread* thread,
                                                      word arg) {
  HandleScope scope(thread);
  Object iterable(&scope, thread->stackPop());
  if (iterable.isTuple()) {
    Tuple tuple(&scope, *iterable);
    return unpackSequenceWithLength(thread, tuple, arg, tuple.length());
  }
  if (iterable.isList()) {
    List list(&scope, *iterable);
    Tuple tuple(&scope, list.items());
    return unpackSequenceWithLength(thread, tuple, arg, list.numItems());
  }
  Object iterator(&scope, createIterator(thread, iterable));
  if (iterator.isErrorException()) return Continue::UNWIND;

  Object next_method(&scope, lookupMethod(thread, iterator, ID(__next__)));
  if (next_method.isError()) {
    if (next_method.isErrorException()) {
      thread->clearPendingException();
    } else {
      DCHECK(next_method.isErrorNotFound(),
             "expected Error::exception() or Error::notFound()");
    }
    thread->raiseWithFmt(LayoutId::kTypeError, "iter() returned non-iterator");
    return Continue::UNWIND;
  }
  word num_pushed = 0;
  Object value(&scope, RawNoneType::object());
  for (;;) {
    value = callMethod1(thread, next_method, iterator);
    if (value.isErrorException()) {
      if (thread->clearPendingStopIteration()) {
        if (num_pushed == arg) break;
        thread->raiseWithFmt(LayoutId::kValueError,
                             "not enough values to unpack");
      }
      return Continue::UNWIND;
    }
    if (num_pushed == arg) {
      thread->raiseWithFmt(LayoutId::kValueError, "too many values to unpack");
      return Continue::UNWIND;
    }
    thread->stackPush(*value);
    ++num_pushed;
  }

  // swap values on the stack
  Object tmp(&scope, NoneType::object());
  for (word i = 0, j = num_pushed - 1, half = num_pushed / 2; i < half;
       ++i, --j) {
    tmp = thread->stackPeek(i);
    thread->stackSetAt(i, thread->stackPeek(j));
    thread->stackSetAt(j, *tmp);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doForIter(Thread* thread, word arg) {
  return forIterUpdateCache(thread, arg, -1);
}

Continue Interpreter::forIterUpdateCache(Thread* thread, word arg, word index) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object iter(&scope, thread->stackTop());
  Type type(&scope, thread->runtime()->typeOf(*iter));
  Object next(&scope, typeLookupInMroById(thread, *type, ID(__next__)));
  if (next.isErrorNotFound()) {
    thread->raiseWithFmt(LayoutId::kTypeError, "iter() returned non-iterator");
    return Continue::UNWIND;
  }

  Object result(&scope, NoneType::object());
  if (next.isFunction()) {
    if (index >= 0) {
      MutableTuple caches(&scope, frame->caches());
      Str next_name(&scope, thread->runtime()->symbols()->at(ID(__next__)));
      Function dependent(&scope, frame->function());
      ICState next_cache_state = icUpdateAttr(
          thread, caches, index, iter.layoutId(), next, next_name, dependent);
      rewriteCurrentBytecode(frame, next_cache_state == ICState::kMonomorphic
                                        ? FOR_ITER_MONOMORPHIC
                                        : FOR_ITER_POLYMORPHIC);
    }
    result = Interpreter::callMethod1(thread, next, iter);
  } else {
    next = resolveDescriptorGet(thread, next, iter, type);
    if (next.isErrorException()) return Continue::UNWIND;
    result = call0(thread, next);
  }

  if (result.isErrorException()) {
    if (thread->clearPendingStopIteration()) {
      thread->stackPop();
      frame->setVirtualPC(frame->virtualPC() + arg);
      return Continue::NEXT;
    }
    return Continue::UNWIND;
  }
  thread->stackPush(*result);
  return Continue::NEXT;
}

static RawObject builtinsAt(Thread* thread, const Module& module,
                            const Object& name) {
  HandleScope scope(thread);
  Object builtins(&scope, moduleAtById(thread, module, ID(__builtins__)));
  Module builtins_module(&scope, *module);
  if (builtins.isModuleProxy()) {
    builtins_module = ModuleProxy::cast(*builtins).module();
  } else if (builtins.isModule()) {
    builtins_module = *builtins;
  } else if (builtins.isErrorNotFound()) {
    return Error::notFound();
  } else {
    return objectGetItem(thread, builtins, name);
  }
  return moduleAt(builtins_module, name);
}

static RawObject globalsAt(Thread* thread, const Module& module,
                           const Object& name) {
  RawObject result = moduleValueCellAt(thread, module, name);
  if (!result.isErrorNotFound() && !ValueCell::cast(result).isPlaceholder()) {
    return ValueCell::cast(result).value();
  }
  return builtinsAt(thread, module, name);
}

ALWAYS_INLINE Continue Interpreter::forIter(Thread* thread,
                                            RawObject next_method, word arg) {
  DCHECK(next_method.isFunction(), "Unexpected next_method value");
  Frame* frame = thread->currentFrame();
  RawObject iter = thread->stackTop();
  thread->stackPush(next_method);
  thread->stackPush(iter);
  RawObject result = callFunction(thread, /*nargs=*/1, next_method);
  if (result.isErrorException()) {
    if (thread->clearPendingStopIteration()) {
      thread->stackPop();
      frame->setVirtualPC(frame->virtualPC() +
                          originalArg(frame->function(), arg));
      return Continue::NEXT;
    }
    return Continue::UNWIND;
  }
  thread->stackPush(result);
  return Continue::NEXT;
}

static Continue retryForIterAnamorphic(Thread* thread, word arg) {
  // Revert the opcode, and retry FOR_ITER_CACHED.
  Frame* frame = thread->currentFrame();
  rewriteCurrentBytecode(frame, FOR_ITER_ANAMORPHIC);
  return Interpreter::doForIterAnamorphic(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doForIterList(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter_obj = thread->stackTop();
  if (!iter_obj.isListIterator()) {
    EVENT_CACHE(FOR_ITER_LIST);
    return retryForIterAnamorphic(thread, arg);
  }
  // NOTE: This should be synced with listIteratorNext in list-builtins.cpp.
  RawListIterator iter = ListIterator::cast(iter_obj);
  word idx = iter.index();
  RawList underlying = iter.iterable().rawCast<RawList>();
  if (idx >= underlying.numItems()) {
    thread->stackPop();
    frame->setVirtualPC(frame->virtualPC() +
                        originalArg(frame->function(), arg));
  } else {
    thread->stackPush(underlying.at(idx));
    iter.setIndex(idx + 1);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doForIterDict(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter_obj = thread->stackTop();
  if (!iter_obj.isDictKeyIterator()) {
    EVENT_CACHE(FOR_ITER_DICT);
    return retryForIterAnamorphic(thread, arg);
  }
  // NOTE: This should be synced with dictKeyIteratorNext in dict-builtins.cpp.
  HandleScope scope(thread);
  DictKeyIterator iter(&scope, DictKeyIterator::cast(iter_obj));
  Dict dict(&scope, iter.iterable());
  word i = iter.index();
  Object key(&scope, NoneType::object());
  if (dictNextKey(dict, &i, &key)) {
    // At this point, we have found a valid index in the buckets.
    iter.setIndex(i);
    iter.setNumFound(iter.numFound() + 1);
    thread->stackPush(*key);
  } else {
    // We hit the end.
    iter.setIndex(i);
    thread->stackPop();
    frame->setVirtualPC(frame->virtualPC() +
                        originalArg(frame->function(), arg));
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doForIterGenerator(Thread* thread,
                                                        word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter_obj = thread->stackTop();
  if (!iter_obj.isGenerator()) {
    EVENT_CACHE(FOR_ITER_GENERATOR);
    return retryForIterAnamorphic(thread, arg);
  }
  HandleScope scope(thread);
  Generator gen(&scope, iter_obj);
  Object value(&scope, NoneType::object());
  Object result(&scope, resumeGenerator(thread, gen, value));
  if (result.isErrorException()) {
    if (thread->clearPendingStopIteration()) {
      thread->stackPop();
      frame->setVirtualPC(frame->virtualPC() +
                          originalArg(frame->function(), arg));
      return Continue::NEXT;
    }
    return Continue::UNWIND;
  }
  thread->stackPush(*result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doForIterTuple(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter_obj = thread->stackTop();
  if (!iter_obj.isTupleIterator()) {
    EVENT_CACHE(FOR_ITER_TUPLE);
    return retryForIterAnamorphic(thread, arg);
  }
  // NOTE: This should be synced with tupleIteratorNext in tuple-builtins.cpp.
  RawTupleIterator iter = TupleIterator::cast(iter_obj);
  word idx = iter.index();
  if (idx == iter.length()) {
    thread->stackPop();
    frame->setVirtualPC(frame->virtualPC() +
                        originalArg(frame->function(), arg));
  } else {
    RawTuple underlying = iter.iterable().rawCast<RawTuple>();
    RawObject item = underlying.at(idx);
    iter.setIndex(idx + 1);
    thread->stackPush(item);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doForIterRange(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter_obj = thread->stackTop();
  if (!iter_obj.isRangeIterator()) {
    EVENT_CACHE(FOR_ITER_RANGE);
    return retryForIterAnamorphic(thread, arg);
  }
  // NOTE: This should be synced with rangeIteratorNext in range-builtins.cpp.
  RawRangeIterator iter = RangeIterator::cast(iter_obj);
  word length = iter.length();
  if (length == 0) {
    thread->stackPop();
    frame->setVirtualPC(frame->virtualPC() +
                        originalArg(frame->function(), arg));
  } else {
    iter.setLength(length - 1);
    word next = iter.next();
    if (length > 1) {
      word step = iter.step();
      iter.setNext(next + step);
    }
    thread->stackPush(SmallInt::fromWord(next));
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doForIterStr(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter_obj = thread->stackTop();
  if (!iter_obj.isStrIterator()) {
    EVENT_CACHE(FOR_ITER_STR);
    return retryForIterAnamorphic(thread, arg);
  }
  // NOTE: This should be synced with strIteratorNext in str-builtins.cpp.
  RawStrIterator iter = StrIterator::cast(iter_obj);
  word byte_offset = iter.index();
  RawStr underlying = iter.iterable().rawCast<RawStr>();
  if (byte_offset == underlying.length()) {
    thread->stackPop();
    frame->setVirtualPC(frame->virtualPC() +
                        originalArg(frame->function(), arg));
  } else {
    word num_bytes = 0;
    word code_point = underlying.codePointAt(byte_offset, &num_bytes);
    iter.setIndex(byte_offset + num_bytes);
    thread->stackPush(RawSmallStr::fromCodePoint(code_point));
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doForIterMonomorphic(Thread* thread,
                                                          word arg) {
  Frame* frame = thread->currentFrame();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  LayoutId iter_layout_id = thread->stackTop().layoutId();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, iter_layout_id, &is_found);
  if (!is_found) {
    EVENT_CACHE(FOR_ITER_MONOMORPHIC);
    return forIterUpdateCache(thread, originalArg(frame->function(), arg), arg);
  }
  return forIter(thread, cached, arg);
}

HANDLER_INLINE Continue Interpreter::doForIterPolymorphic(Thread* thread,
                                                          word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter = thread->stackTop();
  LayoutId iter_layout_id = iter.layoutId();
  bool is_found;
  RawObject cached = icLookupPolymorphic(MutableTuple::cast(frame->caches()),
                                         arg, iter_layout_id, &is_found);
  if (!is_found) {
    EVENT_CACHE(FOR_ITER_POLYMORPHIC);
    return forIterUpdateCache(thread, originalArg(frame->function(), arg), arg);
  }
  return forIter(thread, cached, arg);
}

HANDLER_INLINE Continue Interpreter::doForIterAnamorphic(Thread* thread,
                                                         word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter = thread->stackTop();
  LayoutId iter_layout_id = iter.layoutId();
  switch (iter_layout_id) {
    case LayoutId::kListIterator:
      rewriteCurrentBytecode(frame, FOR_ITER_LIST);
      return doForIterList(thread, arg);
    case LayoutId::kDictKeyIterator:
      rewriteCurrentBytecode(frame, FOR_ITER_DICT);
      return doForIterDict(thread, arg);
    case LayoutId::kTupleIterator:
      rewriteCurrentBytecode(frame, FOR_ITER_TUPLE);
      return doForIterTuple(thread, arg);
    case LayoutId::kRangeIterator:
      rewriteCurrentBytecode(frame, FOR_ITER_RANGE);
      return doForIterRange(thread, arg);
    case LayoutId::kStrIterator:
      rewriteCurrentBytecode(frame, FOR_ITER_STR);
      return doForIterStr(thread, arg);
    case LayoutId::kGenerator:
      rewriteCurrentBytecode(frame, FOR_ITER_GENERATOR);
      return doForIterGenerator(thread, arg);
    default:
      break;
  }
  return forIterUpdateCache(thread, originalArg(frame->function(), arg), arg);
}

HANDLER_INLINE Continue Interpreter::doUnpackEx(Thread* thread, word arg) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object iterable(&scope, thread->stackPop());
  Object iterator(&scope, createIterator(thread, iterable));
  if (iterator.isErrorException()) return Continue::UNWIND;

  Object next_method(&scope, lookupMethod(thread, iterator, ID(__next__)));
  if (next_method.isError()) {
    if (next_method.isErrorException()) {
      thread->clearPendingException();
    } else {
      DCHECK(next_method.isErrorNotFound(),
             "expected Error::exception() or Error::notFound()");
    }
    thread->raiseWithFmt(LayoutId::kTypeError, "iter() returned non-iterator");
    return Continue::UNWIND;
  }

  word before = arg & kMaxByte;
  word after = (arg >> kBitsPerByte) & kMaxByte;
  word num_pushed = 0;
  Object value(&scope, RawNoneType::object());
  for (; num_pushed < before; ++num_pushed) {
    value = callMethod1(thread, next_method, iterator);
    if (value.isErrorException()) {
      if (thread->clearPendingStopIteration()) break;
      return Continue::UNWIND;
    }
    thread->stackPush(*value);
  }

  if (num_pushed < before) {
    thread->raiseWithFmt(LayoutId::kValueError, "not enough values to unpack");
    return Continue::UNWIND;
  }

  List list(&scope, runtime->newList());
  for (;;) {
    value = callMethod1(thread, next_method, iterator);
    if (value.isErrorException()) {
      if (thread->clearPendingStopIteration()) break;
      return Continue::UNWIND;
    }
    runtime->listAdd(thread, list, value);
  }

  thread->stackPush(*list);
  num_pushed++;

  if (list.numItems() < after) {
    thread->raiseWithFmt(LayoutId::kValueError, "not enough values to unpack");
    return Continue::UNWIND;
  }

  if (after > 0) {
    // Pop elements off the list and set them on the stack
    for (word i = list.numItems() - after, j = list.numItems(); i < j;
         ++i, ++num_pushed) {
      thread->stackPush(list.at(i));
      list.atPut(i, NoneType::object());
    }
    list.setNumItems(list.numItems() - after);
  }

  // swap values on the stack
  Object tmp(&scope, NoneType::object());
  for (word i = 0, j = num_pushed - 1, half = num_pushed / 2; i < half;
       ++i, --j) {
    tmp = thread->stackPeek(i);
    thread->stackSetAt(i, thread->stackPeek(j));
    thread->stackSetAt(j, *tmp);
  }
  return Continue::NEXT;
}

void Interpreter::storeAttrWithLocation(Thread* thread, RawObject receiver,
                                        RawObject location, RawObject value) {
  word offset = SmallInt::cast(location).value();
  RawInstance instance = Instance::cast(receiver);
  if (offset >= 0) {
    instance.instanceVariableAtPut(offset, value);
    return;
  }

  RawLayout layout = Layout::cast(thread->runtime()->layoutOf(receiver));
  RawTuple overflow =
      Tuple::cast(instance.instanceVariableAt(layout.overflowOffset()));
  overflow.atPut(-offset - 1, value);
}

RawObject Interpreter::storeAttrSetLocation(Thread* thread,
                                            const Object& object,
                                            const Object& name,
                                            const Object& value,
                                            Object* location_out) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Type type(&scope, runtime->typeOf(*object));
  Object dunder_setattr(&scope,
                        typeLookupInMroById(thread, *type, ID(__setattr__)));
  if (dunder_setattr == runtime->objectDunderSetattr()) {
    return objectSetAttrSetLocation(thread, object, name, value, location_out);
  }
  Object result(&scope,
                thread->invokeMethod3(object, ID(__setattr__), name, value));
  return *result;
}

Continue Interpreter::storeAttrUpdateCache(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  word original_arg = originalArg(frame->function(), arg);
  HandleScope scope(thread);
  Object receiver(&scope, thread->stackPop());
  Str name(&scope,
           Tuple::cast(Code::cast(frame->code()).names()).at(original_arg));
  Object value(&scope, thread->stackPop());

  Object location(&scope, NoneType::object());
  LayoutId saved_layout_id = receiver.layoutId();
  Object result(&scope,
                storeAttrSetLocation(thread, receiver, name, value, &location));
  if (result.isErrorException()) return Continue::UNWIND;
  if (location.isNoneType()) return Continue::NEXT;
  DCHECK(location.isSmallInt(), "unexpected location");
  bool is_in_object = SmallInt::cast(*location).value() >= 0;

  MutableTuple caches(&scope, frame->caches());
  ICState ic_state = icCurrentState(*caches, arg);
  Function dependent(&scope, frame->function());
  LayoutId receiver_layout_id = receiver.layoutId();
  // TODO(T59400994): Clean up when storeAttrSetLocation can return a
  // StoreAttrKind.
  if (ic_state == ICState::kAnamorphic) {
    if (saved_layout_id == receiver_layout_id) {
      // No layout transition.
      if (is_in_object) {
        rewriteCurrentBytecode(frame, STORE_ATTR_INSTANCE);
        icUpdateAttr(thread, caches, arg, saved_layout_id, location, name,
                     dependent);
      } else {
        rewriteCurrentBytecode(frame, STORE_ATTR_INSTANCE_OVERFLOW);
        icUpdateAttr(thread, caches, arg, saved_layout_id, location, name,
                     dependent);
      }
    } else {
      // Layout transition.
      word offset = SmallInt::cast(*location).value();
      if (offset < 0) offset = -offset - 1;
      DCHECK(offset < (1 << Header::kLayoutIdBits), "offset doesn't fit");
      word new_layout_id = static_cast<word>(receiver_layout_id);
      SmallInt layout_offset(
          &scope,
          SmallInt::fromWord(offset << Header::kLayoutIdBits | new_layout_id));
      if (is_in_object) {
        rewriteCurrentBytecode(frame, STORE_ATTR_INSTANCE_UPDATE);
        icUpdateAttr(thread, caches, arg, saved_layout_id, layout_offset, name,
                     dependent);
      } else {
        rewriteCurrentBytecode(frame, STORE_ATTR_INSTANCE_OVERFLOW_UPDATE);
        icUpdateAttr(thread, caches, arg, saved_layout_id, layout_offset, name,
                     dependent);
      }
    }
  } else {
    DCHECK(currentBytecode(thread) == STORE_ATTR_INSTANCE ||
               currentBytecode(thread) == STORE_ATTR_INSTANCE_OVERFLOW ||
               currentBytecode(thread) == STORE_ATTR_POLYMORPHIC,
           "unexpected opcode");
    if (saved_layout_id == receiver_layout_id) {
      rewriteCurrentBytecode(frame, STORE_ATTR_POLYMORPHIC);
      icUpdateAttr(thread, caches, arg, saved_layout_id, location, name,
                   dependent);
    }
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreAttrAnamorphic(Thread* thread,
                                                           word arg) {
  return storeAttrUpdateCache(thread, arg);
}

// This code cleans-up a monomorphic cache and prepares it for its potential
// use as a polymorphic cache.  This code should be removed when we change the
// structure of our caches directly accessible from a function to be monomophic
// and to allocate the relatively uncommon polymorphic caches in a separate
// object.
static Continue retryStoreAttrCached(Thread* thread, word arg) {
  // Revert the opcode, clear the cache, and retry the attribute lookup.
  Frame* frame = thread->currentFrame();
  rewriteCurrentBytecode(frame, STORE_ATTR_ANAMORPHIC);
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  word index = arg * kIcPointersPerEntry;
  caches.atPut(index + kIcEntryKeyOffset, NoneType::object());
  caches.atPut(index + kIcEntryValueOffset, NoneType::object());
  return Interpreter::doStoreAttrAnamorphic(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doStoreAttrInstance(Thread* thread,
                                                         word arg) {
  Frame* frame = thread->currentFrame();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  RawObject receiver = thread->stackTop();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    EVENT_CACHE(STORE_ATTR_INSTANCE);
    return storeAttrUpdateCache(thread, arg);
  }
  word offset = SmallInt::cast(cached).value();
  DCHECK(offset >= 0, "unexpected offset");
  RawInstance instance = Instance::cast(receiver);
  instance.instanceVariableAtPut(offset, thread->stackPeek(1));
  thread->stackDrop(2);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreAttrInstanceOverflow(Thread* thread,
                                                                 word arg) {
  Frame* frame = thread->currentFrame();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  RawObject receiver = thread->stackTop();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    EVENT_CACHE(STORE_ATTR_INSTANCE_OVERFLOW);
    return storeAttrUpdateCache(thread, arg);
  }
  word offset = SmallInt::cast(cached).value();
  DCHECK(offset < 0, "unexpected offset");
  RawInstance instance = Instance::cast(receiver);
  RawLayout layout = Layout::cast(thread->runtime()->layoutOf(receiver));
  RawTuple overflow =
      Tuple::cast(instance.instanceVariableAt(layout.overflowOffset()));
  overflow.atPut(-offset - 1, thread->stackPeek(1));
  thread->stackDrop(2);
  return Continue::NEXT;
}

HANDLER_INLINE Continue
Interpreter::doStoreAttrInstanceOverflowUpdate(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  RawObject receiver = thread->stackTop();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    EVENT_CACHE(STORE_ATTR_INSTANCE_OVERFLOW_UPDATE);
    return retryStoreAttrCached(thread, arg);
  }
  // Set the value in an overflow tuple that needs expansion.
  word offset_and_new_offset_id = SmallInt::cast(cached).value();
  LayoutId new_layout_id =
      static_cast<LayoutId>(offset_and_new_offset_id & Header::kLayoutIdMask);
  word offset = offset_and_new_offset_id >> Header::kLayoutIdBits;

  HandleScope scope(thread);
  Instance instance(&scope, receiver);
  Layout layout(&scope, thread->runtime()->layoutOf(receiver));
  Tuple overflow(&scope, instance.instanceVariableAt(layout.overflowOffset()));
  Object value(&scope, thread->stackPeek(1));
  if (offset >= overflow.length()) {
    instanceGrowOverflow(thread, instance, offset + 1);
    overflow = instance.instanceVariableAt(layout.overflowOffset());
  }
  instance.setLayoutId(new_layout_id);
  overflow.atPut(offset, *value);
  thread->stackDrop(2);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreAttrInstanceUpdate(Thread* thread,
                                                               word arg) {
  Frame* frame = thread->currentFrame();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  RawObject receiver = thread->stackTop();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    EVENT_CACHE(STORE_ATTR_INSTANCE_UPDATE);
    return retryStoreAttrCached(thread, arg);
  }
  // Set the value in object at offset.
  // TODO(T59462341): Encapsulate this in a function.
  word offset_and_new_offset_id = SmallInt::cast(cached).value();
  LayoutId new_layout_id =
      static_cast<LayoutId>(offset_and_new_offset_id & Header::kLayoutIdMask);
  word offset = offset_and_new_offset_id >> Header::kLayoutIdBits;
  DCHECK(offset >= 0, "unexpected offset");
  RawInstance instance = Instance::cast(receiver);
  instance.instanceVariableAtPut(offset, thread->stackPeek(1));
  instance.setLayoutId(new_layout_id);
  thread->stackDrop(2);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreAttrPolymorphic(Thread* thread,
                                                            word arg) {
  Frame* frame = thread->currentFrame();
  RawObject receiver = thread->stackTop();
  LayoutId layout_id = receiver.layoutId();
  bool is_found;
  RawObject cached = icLookupPolymorphic(MutableTuple::cast(frame->caches()),
                                         arg, layout_id, &is_found);
  if (!is_found) {
    EVENT_CACHE(STORE_ATTR_POLYMORPHIC);
    return storeAttrUpdateCache(thread, arg);
  }
  RawObject value = thread->stackPeek(1);
  thread->stackDrop(2);
  storeAttrWithLocation(thread, receiver, cached, value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreAttr(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object receiver(&scope, thread->stackPop());
  Tuple names(&scope, Code::cast(frame->code()).names());
  Str name(&scope, names.at(arg));
  Object value(&scope, thread->stackPop());
  if (thread->invokeMethod3(receiver, ID(__setattr__), name, value)
          .isErrorException()) {
    return Continue::UNWIND;
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doDeleteAttr(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object receiver(&scope, thread->stackPop());
  Tuple names(&scope, Code::cast(frame->code()).names());
  Str name(&scope, names.at(arg));
  if (thread->runtime()
          ->attributeDel(thread, receiver, name)
          .isErrorException()) {
    return Continue::UNWIND;
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreGlobal(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Tuple names(&scope, Code::cast(frame->code()).names());
  Str name(&scope, names.at(arg));
  Object value(&scope, thread->stackPop());
  Module module(&scope, frame->function().moduleObject());
  Function function(&scope, frame->function());
  ValueCell module_result(&scope, moduleAtPut(thread, module, name, value));
  icUpdateGlobalVar(thread, function, arg, module_result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreGlobalCached(Thread* thread,
                                                         word arg) {
  Frame* frame = thread->currentFrame();
  RawObject cached =
      icLookupGlobalVar(MutableTuple::cast(frame->caches()), arg);
  ValueCell::cast(cached).setValue(thread->stackPop());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doDeleteGlobal(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Module module(&scope, frame->function().moduleObject());
  Tuple names(&scope, Code::cast(frame->code()).names());
  Str name(&scope, names.at(arg));
  if (moduleRemove(thread, module, name).isErrorNotFound()) {
    return raiseUndefinedName(thread, name);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadConst(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject consts = Code::cast(frame->code()).consts();
  thread->stackPush(Tuple::cast(consts).at(arg));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadImmediate(Thread* thread, word arg) {
  thread->stackPush(objectFromOparg(arg));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadName(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object names(&scope, Code::cast(frame->code()).names());
  Str name(&scope, Tuple::cast(*names).at(arg));
  Object implicit_globals_obj(&scope, frame->implicitGlobals());
  if (!implicit_globals_obj.isNoneType()) {
    // Give implicit_globals_obj a higher priority than globals.
    if (implicit_globals_obj.isDict()) {
      // Shortcut for the common case of implicit_globals being a dict.
      Dict implicit_globals(&scope, *implicit_globals_obj);
      Object result(&scope, dictAtByStr(thread, implicit_globals, name));
      DCHECK(!result.isError() || result.isErrorNotFound(),
             "expected value or not found");
      if (!result.isErrorNotFound()) {
        thread->stackPush(*result);
        return Continue::NEXT;
      }
    } else {
      Object result(&scope, objectGetItem(thread, implicit_globals_obj, name));
      if (!result.isErrorException()) {
        thread->stackPush(*result);
        return Continue::NEXT;
      }
      if (!thread->pendingExceptionMatches(LayoutId::kKeyError)) {
        return Continue::UNWIND;
      }
      thread->clearPendingException();
    }
  }
  Module module(&scope, frame->function().moduleObject());
  Object result(&scope, globalsAt(thread, module, name));
  if (result.isError()) {
    if (result.isErrorNotFound()) return raiseUndefinedName(thread, name);
    DCHECK(result.isErrorException(), "Expected ErrorException");
    return Continue::UNWIND;
  }
  thread->stackPush(*result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildTuple(Thread* thread, word arg) {
  if (arg == 0) {
    thread->stackPush(thread->runtime()->emptyTuple());
    return Continue::NEXT;
  }
  HandleScope scope(thread);
  MutableTuple tuple(&scope, thread->runtime()->newMutableTuple(arg));
  for (word i = arg - 1; i >= 0; i--) {
    tuple.atPut(i, thread->stackPop());
  }
  thread->stackPush(tuple.becomeImmutable());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildList(Thread* thread, word arg) {
  Runtime* runtime = thread->runtime();
  if (arg == 0) {
    thread->stackPush(runtime->newList());
    return Continue::NEXT;
  }
  HandleScope scope(thread);
  MutableTuple array(&scope, runtime->newMutableTuple(arg));
  for (word i = arg - 1; i >= 0; i--) {
    array.atPut(i, thread->stackPop());
  }
  RawList list = List::cast(runtime->newList());
  list.setItems(*array);
  list.setNumItems(array.length());
  thread->stackPush(list);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildSet(Thread* thread, word arg) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Set set(&scope, runtime->newSet());
  Object value(&scope, NoneType::object());
  Object hash_obj(&scope, NoneType::object());
  for (word i = arg - 1; i >= 0; i--) {
    value = thread->stackPop();
    hash_obj = hash(thread, value);
    if (hash_obj.isErrorException()) return Continue::UNWIND;
    word hash = SmallInt::cast(*hash_obj).value();
    setAdd(thread, set, value, hash);
  }
  thread->stackPush(*set);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildMap(Thread* thread, word arg) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Dict dict(&scope, runtime->newDictWithSize(arg));
  Object value(&scope, NoneType::object());
  Object key(&scope, NoneType::object());
  Object hash_obj(&scope, NoneType::object());
  for (word i = 0; i < arg; i++) {
    value = thread->stackPop();
    key = thread->stackPop();
    hash_obj = hash(thread, key);
    if (hash_obj.isErrorException()) return Continue::UNWIND;
    word hash = SmallInt::cast(*hash_obj).value();
    if (dictAtPut(thread, dict, key, hash, value).isErrorException()) {
      return Continue::UNWIND;
    }
  }
  thread->stackPush(*dict);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadAttr(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object receiver(&scope, thread->stackTop());
  Tuple names(&scope, Code::cast(frame->code()).names());
  Str name(&scope, names.at(arg));
  RawObject result = thread->runtime()->attributeAt(thread, receiver, name);
  if (result.isErrorException()) return Continue::UNWIND;
  thread->stackSetTop(result);
  return Continue::NEXT;
}

Continue Interpreter::loadAttrUpdateCache(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  word original_arg = originalArg(frame->function(), arg);
  Object receiver(&scope, thread->stackTop());
  Str name(&scope,
           Tuple::cast(Code::cast(frame->code()).names()).at(original_arg));

  Object location(&scope, NoneType::object());
  LoadAttrKind kind;
  Object result(&scope, thread->runtime()->attributeAtSetLocation(
                            thread, receiver, name, &kind, &location));
  if (result.isErrorException()) return Continue::UNWIND;
  if (location.isNoneType()) {
    thread->stackSetTop(*result);
    return Continue::NEXT;
  }

  // Cache the attribute load
  MutableTuple caches(&scope, frame->caches());
  ICState ic_state = icCurrentState(*caches, arg);
  Function dependent(&scope, frame->function());
  LayoutId receiver_layout_id = receiver.layoutId();
  if (ic_state == ICState::kAnamorphic) {
    switch (kind) {
      case LoadAttrKind::kInstanceOffset:
        rewriteCurrentBytecode(frame, LOAD_ATTR_INSTANCE);
        icUpdateAttr(thread, caches, arg, receiver_layout_id, location, name,
                     dependent);
        break;
      case LoadAttrKind::kInstanceFunction:
        rewriteCurrentBytecode(frame, LOAD_ATTR_INSTANCE_TYPE_BOUND_METHOD);
        icUpdateAttr(thread, caches, arg, receiver_layout_id, location, name,
                     dependent);
        break;
      case LoadAttrKind::kInstanceProperty:
        rewriteCurrentBytecode(frame, LOAD_ATTR_INSTANCE_PROPERTY);
        icUpdateAttr(thread, caches, arg, receiver_layout_id, location, name,
                     dependent);
        break;
      case LoadAttrKind::kInstanceSlotDescr:
        rewriteCurrentBytecode(frame, LOAD_ATTR_INSTANCE_SLOT_DESCR);
        icUpdateAttr(thread, caches, arg, receiver_layout_id, location, name,
                     dependent);
        break;
      case LoadAttrKind::kInstanceType:
        rewriteCurrentBytecode(frame, LOAD_ATTR_INSTANCE_TYPE);
        icUpdateAttr(thread, caches, arg, receiver_layout_id, location, name,
                     dependent);
        break;
      case LoadAttrKind::kInstanceTypeDescr:
        rewriteCurrentBytecode(frame, LOAD_ATTR_INSTANCE_TYPE_DESCR);
        icUpdateAttr(thread, caches, arg, receiver_layout_id, location, name,
                     dependent);
        break;
      case LoadAttrKind::kModule: {
        ValueCell value_cell(&scope, *location);
        DCHECK(location.isValueCell(), "location must be ValueCell");
        icUpdateAttrModule(thread, caches, arg, receiver, value_cell,
                           dependent);
      } break;
      case LoadAttrKind::kType:
        icUpdateAttrType(thread, caches, arg, receiver, name, location,
                         dependent);
        break;
      default:
        UNREACHABLE("kinds should have been handled before");
    }
  } else {
    DCHECK(
        currentBytecode(thread) == LOAD_ATTR_INSTANCE ||
            currentBytecode(thread) == LOAD_ATTR_INSTANCE_TYPE_BOUND_METHOD ||
            currentBytecode(thread) == LOAD_ATTR_POLYMORPHIC,
        "unexpected opcode");
    switch (kind) {
      case LoadAttrKind::kInstanceOffset:
      case LoadAttrKind::kInstanceFunction:
        rewriteCurrentBytecode(frame, LOAD_ATTR_POLYMORPHIC);
        icUpdateAttr(thread, caches, arg, receiver_layout_id, location, name,
                     dependent);
        break;
      default:
        break;
    }
  }
  thread->stackSetTop(*result);
  return Continue::NEXT;
}

HANDLER_INLINE USED RawObject Interpreter::loadAttrWithLocation(
    Thread* thread, RawObject receiver, RawObject location) {
  if (location.isFunction()) {
    HandleScope scope(thread);
    Object self(&scope, receiver);
    Object function(&scope, location);
    return thread->runtime()->newBoundMethod(function, self);
  }

  word offset = SmallInt::cast(location).value();

  DCHECK(receiver.isHeapObject(), "expected heap object");
  RawInstance instance = Instance::cast(receiver);
  if (offset >= 0) {
    return instance.instanceVariableAt(offset);
  }

  RawLayout layout = Layout::cast(thread->runtime()->layoutOf(receiver));
  RawTuple overflow =
      Tuple::cast(instance.instanceVariableAt(layout.overflowOffset()));
  return overflow.at(-offset - 1);
}

HANDLER_INLINE Continue Interpreter::doLoadAttrAnamorphic(Thread* thread,
                                                          word arg) {
  return loadAttrUpdateCache(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doLoadAttrInstance(Thread* thread,
                                                        word arg) {
  Frame* frame = thread->currentFrame();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  RawObject receiver = thread->stackTop();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    EVENT_CACHE(LOAD_ATTR_INSTANCE);
    return Interpreter::loadAttrUpdateCache(thread, arg);
  }
  RawObject result = loadAttrWithLocation(thread, receiver, cached);
  thread->stackSetTop(result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue
Interpreter::doLoadAttrInstanceTypeBoundMethod(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  RawObject receiver = thread->stackTop();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    EVENT_CACHE(LOAD_ATTR_INSTANCE_TYPE_BOUND_METHOD);
    return Interpreter::loadAttrUpdateCache(thread, arg);
  }
  HandleScope scope(thread);
  Object self(&scope, receiver);
  Object function(&scope, cached);
  thread->stackSetTop(thread->runtime()->newBoundMethod(function, self));
  return Continue::NEXT;
}

// This code cleans-up a monomorphic cache and prepares it for its potential
// use as a polymorphic cache.  This code should be removed when we change the
// structure of our caches directly accessible from a function to be monomophic
// and to allocate the relatively uncommon polymorphic caches in a separate
// object.
NEVER_INLINE Continue Interpreter::retryLoadAttrCached(Thread* thread,
                                                       word arg) {
  // Revert the opcode, clear the cache, and retry the attribute lookup.
  Frame* frame = thread->currentFrame();
  rewriteCurrentBytecode(frame, LOAD_ATTR_ANAMORPHIC);
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  word index = arg * kIcPointersPerEntry;
  caches.atPut(index + kIcEntryKeyOffset, NoneType::object());
  caches.atPut(index + kIcEntryValueOffset, NoneType::object());
  return Interpreter::loadAttrUpdateCache(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doLoadAttrInstanceProperty(Thread* thread,
                                                                word arg) {
  Frame* frame = thread->currentFrame();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  RawObject receiver = thread->stackTop();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    EVENT_CACHE(LOAD_ATTR_INSTANCE_PROPERTY);
    return retryLoadAttrCached(thread, arg);
  }
  thread->stackPush(receiver);
  thread->stackSetAt(1, cached);
  return tailcallFunction(thread, 1, cached);
}

HANDLER_INLINE Continue Interpreter::doLoadAttrInstanceSlotDescr(Thread* thread,
                                                                 word arg) {
  Frame* frame = thread->currentFrame();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  RawObject receiver = thread->stackTop();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    EVENT_CACHE(LOAD_ATTR_INSTANCE_SLOT_DESCR);
    return retryLoadAttrCached(thread, arg);
  }
  word offset = SmallInt::cast(cached).value();
  RawObject value = Instance::cast(receiver).instanceVariableAt(offset);
  if (!value.isUnbound()) {
    thread->stackSetTop(value);
    return Continue::NEXT;
  }
  // If the value is unbound, we remove the cached slot descriptor.
  EVENT_CACHE(LOAD_ATTR_INSTANCE_SLOT_DESCR);
  return retryLoadAttrCached(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doLoadAttrInstanceTypeDescr(Thread* thread,
                                                                 word arg) {
  Frame* frame = thread->currentFrame();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  RawObject receiver = thread->stackTop();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    EVENT_CACHE(LOAD_ATTR_INSTANCE_TYPE_DESCR);
    return retryLoadAttrCached(thread, arg);
  }
  HandleScope scope(thread);
  Object descr(&scope, cached);
  Object self(&scope, receiver);
  Type self_type(&scope, thread->runtime()->typeAt(self.layoutId()));
  Object result(&scope,
                Interpreter::callDescriptorGet(thread, descr, self, self_type));
  if (result.isError()) return Continue::UNWIND;
  thread->stackSetTop(*result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadAttrInstanceType(Thread* thread,
                                                            word arg) {
  Frame* frame = thread->currentFrame();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  RawObject receiver = thread->stackTop();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    EVENT_CACHE(LOAD_ATTR_INSTANCE_TYPE);
    return retryLoadAttrCached(thread, arg);
  }
  thread->stackSetTop(cached);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadAttrModule(Thread* thread,
                                                      word arg) {
  Frame* frame = thread->currentFrame();
  RawObject receiver = thread->stackTop();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  word index = arg * kIcPointersPerEntry;
  RawObject cache_key = caches.at(index + kIcEntryKeyOffset);
  // isInstanceOfModule() should be just as fast as isModule() in the common
  // case. If code size or quality is an issue we can adjust this as needed
  // based on the types that actually flow through here.
  if (thread->runtime()->isInstanceOfModule(receiver) &&
      // Use rawCast() to support subclasses without the overhead of a
      // handle.
      SmallInt::fromWord(receiver.rawCast<RawModule>().id()) == cache_key) {
    RawObject result = caches.at(index + kIcEntryValueOffset);
    DCHECK(result.isValueCell(), "cached value is not a value cell");
    thread->stackSetTop(ValueCell::cast(result).value());
    return Continue::NEXT;
  }
  EVENT_CACHE(LOAD_ATTR_MODULE);
  return retryLoadAttrCached(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doLoadAttrPolymorphic(Thread* thread,
                                                           word arg) {
  Frame* frame = thread->currentFrame();
  RawObject receiver = thread->stackTop();
  LayoutId layout_id = receiver.layoutId();
  bool is_found;
  RawObject cached = icLookupPolymorphic(MutableTuple::cast(frame->caches()),
                                         arg, layout_id, &is_found);
  if (!is_found) {
    EVENT_CACHE(LOAD_ATTR_POLYMORPHIC);
    return loadAttrUpdateCache(thread, arg);
  }
  RawObject result = loadAttrWithLocation(thread, receiver, cached);
  thread->stackSetTop(result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadAttrType(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject receiver = thread->stackTop();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  word index = arg * kIcPointersPerEntry;
  RawObject layout_id = caches.at(index + kIcEntryKeyOffset);
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfType(receiver)) {
    word id = static_cast<word>(receiver.rawCast<RawType>().instanceLayoutId());
    if (SmallInt::fromWord(id) == layout_id) {
      RawObject result = caches.at(index + kIcEntryValueOffset);
      DCHECK(result.isValueCell(), "cached value is not a value cell");
      thread->stackSetTop(ValueCell::cast(result).value());
      return Continue::NEXT;
    }
  }
  EVENT_CACHE(LOAD_ATTR_TYPE);
  return retryLoadAttrCached(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doLoadBool(Thread* thread, word arg) {
  DCHECK(arg == 0x80 || arg == 0, "unexpected arg");
  thread->stackPush(Bool::fromBool(arg));
  return Continue::NEXT;
}

static RawObject excMatch(Thread* thread, const Object& left,
                          const Object& right) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  static const char* cannot_catch_msg =
      "catching classes that do not inherit from BaseException is not allowed";
  if (runtime->isInstanceOfTuple(*right)) {
    Tuple tuple(&scope, tupleUnderlying(*right));
    for (word i = 0, length = tuple.length(); i < length; i++) {
      Object obj(&scope, tuple.at(i));
      if (!(runtime->isInstanceOfType(*obj) &&
            Type(&scope, *obj).isBaseExceptionSubclass())) {
        return thread->raiseWithFmt(LayoutId::kTypeError, cannot_catch_msg);
      }
    }
  } else if (!(runtime->isInstanceOfType(*right) &&
               Type(&scope, *right).isBaseExceptionSubclass())) {
    return thread->raiseWithFmt(LayoutId::kTypeError, cannot_catch_msg);
  }

  return Bool::fromBool(givenExceptionMatches(thread, left, right));
}

HANDLER_INLINE Continue Interpreter::doCompareIs(Thread* thread, word) {
  RawObject right = thread->stackPop();
  RawObject left = thread->stackPop();
  thread->stackPush(Bool::fromBool(left == right));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doCompareIsNot(Thread* thread, word) {
  RawObject right = thread->stackPop();
  RawObject left = thread->stackPop();
  thread->stackPush(Bool::fromBool(left != right));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doCompareOp(Thread* thread, word arg) {
  HandleScope scope(thread);
  Object right(&scope, thread->stackPop());
  Object left(&scope, thread->stackPop());
  CompareOp op = static_cast<CompareOp>(arg);
  RawObject result = NoneType::object();
  if (op == IS) {
    result = Bool::fromBool(*left == *right);
  } else if (op == IS_NOT) {
    result = Bool::fromBool(*left != *right);
  } else if (op == IN) {
    result = sequenceContains(thread, left, right);
  } else if (op == NOT_IN) {
    result = sequenceContains(thread, left, right);
    if (result.isBool()) result = Bool::negate(result);
  } else if (op == EXC_MATCH) {
    result = excMatch(thread, left, right);
  } else {
    result = compareOperation(thread, op, left, right);
  }

  if (result.isErrorException()) return Continue::UNWIND;
  thread->stackPush(result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doImportName(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Code code(&scope, frame->code());
  Object name(&scope, Tuple::cast(code.names()).at(arg));
  Object fromlist(&scope, thread->stackPop());
  Object level(&scope, thread->stackPop());
  Module module(&scope, frame->function().moduleObject());
  Object globals(&scope, module.moduleProxy());
  // TODO(T41634372) Pass in a dict that is similar to what `builtins.locals`
  // returns. Use `None` for now since the default importlib behavior is to
  // ignore the value and this only matters if `__import__` is replaced.
  Object locals(&scope, NoneType::object());

  // Call __builtins__.__import__(name, globals, locals, fromlist, level).
  Runtime* runtime = thread->runtime();
  Object dunder_import_name(&scope, runtime->symbols()->at(ID(__import__)));
  Object dunder_import(&scope, builtinsAt(thread, module, dunder_import_name));
  if (dunder_import.isErrorNotFound()) {
    thread->raiseWithFmt(LayoutId::kImportError, "__import__ not found");
    return Continue::UNWIND;
  }

  thread->stackPush(*dunder_import);
  thread->stackPush(*name);
  thread->stackPush(*globals);
  thread->stackPush(*locals);
  thread->stackPush(*fromlist);
  thread->stackPush(*level);
  return tailcall(thread, 5);
}

static RawObject tryImportFromSysModules(Thread* thread, const Object& from,
                                         const Object& name) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object fully_qualified_name(
      &scope, runtime->attributeAtById(thread, from, ID(__name__)));
  if (fully_qualified_name.isErrorException() ||
      !runtime->isInstanceOfStr(*fully_qualified_name)) {
    thread->clearPendingException();
    return Error::notFound();
  }
  Object module_name(
      &scope, runtime->newStrFromFmt("%S.%S", &fully_qualified_name, &name));
  Object result(&scope, runtime->findModule(module_name));
  if (result.isNoneType()) {
    return Error::notFound();
  }
  return *result;
}

HANDLER_INLINE Continue Interpreter::doImportFrom(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Code code(&scope, frame->code());
  Str name(&scope, Tuple::cast(code.names()).at(arg));
  Object from(&scope, thread->stackTop());

  Object value(&scope, NoneType::object());
  if (from.isModule()) {
    // Common case of a lookup done on the built-in module type.
    Module from_module(&scope, *from);
    value = moduleGetAttribute(thread, from_module, name);
  } else {
    // Do a generic attribute lookup.
    value = thread->runtime()->attributeAt(thread, from, name);
  }

  if (value.isErrorException()) {
    if (!thread->pendingExceptionMatches(LayoutId::kAttributeError)) {
      return Continue::UNWIND;
    }
    thread->clearPendingException();
    value = Error::notFound();
  }

  if (value.isErrorNotFound()) {
    // in case this failed because of a circular relative import, try to
    // fallback on reading the module directly from sys.modules.
    // See cpython bpo-17636.
    value = tryImportFromSysModules(thread, from, name);
    if (value.isErrorNotFound()) {
      Runtime* runtime = thread->runtime();
      if (runtime->isInstanceOfModule(*from)) {
        Module from_module(&scope, *from);
        Object module_name(&scope, from_module.name());
        if (runtime->isInstanceOfStr(*module_name)) {
          thread->raiseWithFmt(LayoutId::kImportError,
                               "cannot import name '%S' from '%S'", &name,
                               &module_name);
          return Continue::UNWIND;
        }
      }
      thread->raiseWithFmt(LayoutId::kImportError, "cannot import name '%S'",
                           &name);
      return Continue::UNWIND;
    }
  }
  thread->stackPush(*value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doJumpForward(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  frame->setVirtualPC(frame->virtualPC() + arg);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doJumpIfFalseOrPop(Thread* thread,
                                                        word arg) {
  Frame* frame = thread->currentFrame();
  RawObject value = thread->stackTop();
  value = isTrue(thread, value);
  if (LIKELY(value == Bool::falseObj())) {
    frame->setVirtualPC(arg);
    return Continue::NEXT;
  }
  if (value == Bool::trueObj()) {
    thread->stackPop();
    return Continue::NEXT;
  }
  DCHECK(value.isErrorException(), "value must be error");
  return Continue::UNWIND;
}

HANDLER_INLINE Continue Interpreter::doJumpIfTrueOrPop(Thread* thread,
                                                       word arg) {
  Frame* frame = thread->currentFrame();
  RawObject value = thread->stackTop();
  value = isTrue(thread, value);
  if (LIKELY(value == Bool::trueObj())) {
    frame->setVirtualPC(arg);
    return Continue::NEXT;
  }
  if (value == Bool::falseObj()) {
    thread->stackPop();
    return Continue::NEXT;
  }
  DCHECK(value.isErrorException(), "value must be error");
  return Continue::UNWIND;
}

HANDLER_INLINE Continue Interpreter::doJumpAbsolute(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  frame->setVirtualPC(arg);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doPopJumpIfFalse(Thread* thread,
                                                      word arg) {
  Frame* frame = thread->currentFrame();
  RawObject value = thread->stackPop();
  value = isTrue(thread, value);
  if (LIKELY(value == Bool::falseObj())) {
    frame->setVirtualPC(arg);
    return Continue::NEXT;
  }
  if (value == Bool::trueObj()) {
    return Continue::NEXT;
  }
  DCHECK(value.isErrorException(), "value must be error");
  return Continue::UNWIND;
}

HANDLER_INLINE Continue Interpreter::doPopJumpIfTrue(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject value = thread->stackPop();
  value = isTrue(thread, value);
  if (LIKELY(value == Bool::trueObj())) {
    frame->setVirtualPC(arg);
    return Continue::NEXT;
  }
  if (value == Bool::falseObj()) {
    return Continue::NEXT;
  }
  DCHECK(value.isErrorException(), "value must be error");
  return Continue::UNWIND;
}

HANDLER_INLINE Continue Interpreter::doLoadGlobal(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Tuple names(&scope, Code::cast(frame->code()).names());
  Str name(&scope, names.at(arg));
  Function function(&scope, frame->function());
  Module module(&scope, function.moduleObject());

  Object module_result(&scope, moduleValueCellAt(thread, module, name));
  if (!module_result.isErrorNotFound() &&
      !ValueCell::cast(*module_result).isPlaceholder()) {
    ValueCell value_cell(&scope, *module_result);
    icUpdateGlobalVar(thread, function, arg, value_cell);
    thread->stackPush(value_cell.value());
    return Continue::NEXT;
  }
  Object builtins(&scope, moduleAtById(thread, module, ID(__builtins__)));
  Module builtins_module(&scope, *module);
  if (builtins.isModuleProxy()) {
    builtins_module = ModuleProxy::cast(*builtins).module();
  } else if (builtins.isModule()) {
    builtins_module = *builtins;
  } else if (builtins.isErrorNotFound()) {
    return raiseUndefinedName(thread, name);
  } else {
    Object result(&scope, objectGetItem(thread, builtins, name));
    if (result.isErrorException()) return Continue::UNWIND;
    thread->stackPush(*result);
    return Continue::NEXT;
  }
  Object builtins_result(&scope,
                         moduleValueCellAt(thread, builtins_module, name));
  if (builtins_result.isErrorNotFound()) {
    return raiseUndefinedName(thread, name);
  }
  ValueCell value_cell(&scope, *builtins_result);
  if (value_cell.isPlaceholder()) {
    return raiseUndefinedName(thread, name);
  }
  icUpdateGlobalVar(thread, function, arg, value_cell);
  // Set up a placeholder in module to signify that a builtin entry under
  // the same name is cached.
  attributeValueCellAtPut(thread, module, name);
  thread->stackPush(value_cell.value());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadGlobalCached(Thread* thread,
                                                        word arg) {
  Frame* frame = thread->currentFrame();
  RawObject cached =
      icLookupGlobalVar(MutableTuple::cast(frame->caches()), arg);
  DCHECK(cached.isValueCell(), "cached value must be a ValueCell");
  DCHECK(!ValueCell::cast(cached).isPlaceholder(),
         "cached ValueCell must not be a placeholder");
  thread->stackPush(ValueCell::cast(cached).value());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doSetupFinally(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  word stack_depth = thread->valueStackSize();
  word handler_pc = frame->virtualPC() + arg;
  frame->blockStackPush(TryBlock(TryBlock::kFinally, handler_pc, stack_depth));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadFast(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject value = frame->local(arg);
  // TODO(T66255738): Remove this once we can statically prove local variable
  // are always bound.
  if (UNLIKELY(value.isErrorNotFound())) {
    HandleScope scope(thread);
    Str name(&scope, Tuple::cast(Code::cast(frame->code()).varnames()).at(arg));
    thread->raiseWithFmt(LayoutId::kUnboundLocalError,
                         "local variable '%S' referenced before assignment",
                         &name);
    return Continue::UNWIND;
  }
  thread->stackPush(value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadFastReverse(Thread* thread,
                                                       word arg) {
  Frame* frame = thread->currentFrame();
  RawObject value = frame->localWithReverseIndex(arg);
  if (UNLIKELY(value.isErrorNotFound())) {
    HandleScope scope(thread);
    Code code(&scope, frame->code());
    word name_idx = code.nlocals() - arg - 1;
    Str name(&scope, Tuple::cast(code.varnames()).at(name_idx));
    thread->raiseWithFmt(LayoutId::kUnboundLocalError,
                         "local variable '%S' referenced before assignment",
                         &name);
    return Continue::UNWIND;
  }
  thread->stackPush(value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadFastReverseUnchecked(Thread* thread,
                                                                word arg) {
  RawObject value = thread->currentFrame()->localWithReverseIndex(arg);
  DCHECK(!value.isErrorNotFound(), "no value assigned yet");
  thread->stackPush(value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreFast(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject value = thread->stackPop();
  frame->setLocal(arg, value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreFastReverse(Thread* thread,
                                                        word arg) {
  Frame* frame = thread->currentFrame();
  RawObject value = thread->stackPop();
  frame->setLocalWithReverseIndex(arg, value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doDeleteFast(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  // TODO(T66255738): Remove this once we can statically prove local variable
  // are always bound.
  if (UNLIKELY(frame->local(arg).isErrorNotFound())) {
    HandleScope scope(thread);
    Object name(&scope,
                Tuple::cast(Code::cast(frame->code()).varnames()).at(arg));
    thread->raiseWithFmt(LayoutId::kUnboundLocalError,
                         "local variable '%S' referenced before assignment",
                         &name);
    return Continue::UNWIND;
  }
  frame->setLocal(arg, Error::notFound());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreAnnotation(Thread* thread,
                                                       word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object names(&scope, Code::cast(frame->code()).names());
  Object value(&scope, thread->stackPop());
  Str name(&scope, Tuple::cast(*names).at(arg));
  Object annotations(&scope, NoneType::object());
  Object dunder_annotations(&scope,
                            runtime->symbols()->at(ID(__annotations__)));
  if (frame->implicitGlobals().isNoneType()) {
    // Module body
    Module module(&scope, frame->function().moduleObject());
    annotations = moduleAt(module, dunder_annotations);
  } else {
    // Class body
    Object implicit_globals(&scope, frame->implicitGlobals());
    if (implicit_globals.isDict()) {
      Dict implicit_globals_dict(&scope, *implicit_globals);
      annotations =
          dictAtByStr(thread, implicit_globals_dict, dunder_annotations);
    } else {
      annotations = objectGetItem(thread, implicit_globals, dunder_annotations);
      if (annotations.isErrorException()) {
        return Continue::UNWIND;
      }
    }
  }
  if (annotations.isDict()) {
    Dict annotations_dict(&scope, *annotations);
    dictAtPutByStr(thread, annotations_dict, name, value);
  } else {
    if (objectSetItem(thread, annotations, name, value).isErrorException()) {
      return Continue::UNWIND;
    }
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doRaiseVarargs(Thread* thread, word arg) {
  DCHECK(arg >= 0, "Negative argument to RAISE_VARARGS");
  DCHECK(arg <= 2, "Argument to RAISE_VARARGS too large");

  if (arg == 0) {
    // Re-raise the caught exception.
    HandleScope scope(thread);
    Object caught_exc_state_obj(&scope, thread->topmostCaughtExceptionState());
    if (caught_exc_state_obj.isNoneType()) {
      thread->raiseWithFmt(LayoutId::kRuntimeError,
                           "No active exception to reraise");
    } else {
      ExceptionState caught_exc_state(&scope, *caught_exc_state_obj);
      thread->setPendingExceptionType(caught_exc_state.type());
      thread->setPendingExceptionValue(caught_exc_state.value());
      thread->setPendingExceptionTraceback(caught_exc_state.traceback());
    }
  } else {
    RawObject cause = (arg >= 2) ? thread->stackPop() : Error::notFound();
    RawObject exn = (arg >= 1) ? thread->stackPop() : NoneType::object();
    raise(thread, exn, cause);
  }

  return Continue::UNWIND;
}

HANDLER_INLINE
Continue Interpreter::callTrampoline(Thread* thread, Function::Entry entry,
                                     word nargs, RawObject* post_call_sp) {
  RawObject result = entry(thread, nargs);
  DCHECK(thread->stackPointer() == post_call_sp, "stack not cleaned");
  if (result.isErrorException()) return Continue::UNWIND;
  thread->stackPush(result);
  return Continue::NEXT;
}

static HANDLER_INLINE Continue
callInterpretedImpl(Thread* thread, word nargs, RawFunction function,
                    RawObject* post_call_sp, PrepareCallFunc prepare_args) {
  // Warning: This code is using `RawXXX` variables for performance! This is
  // despite the fact that we call functions that do potentially perform memory
  // allocations. This is legal here because we always rely on the functions
  // returning an up-to-date address and we make sure to never access any value
  // produce before a call after that call. Be careful not to break this
  // invariant if you change the code!

  RawObject result = prepare_args(thread, nargs, function);
  if (result.isErrorException()) {
    DCHECK(thread->stackPointer() == post_call_sp, "stack not cleaned");
    return Continue::UNWIND;
  }
  function = RawFunction::cast(result);

  bool has_freevars_or_cellvars = function.hasFreevarsOrCellvars();
  Frame* callee_frame = thread->pushCallFrame(function);
  if (UNLIKELY(callee_frame == nullptr)) {
    return Continue::UNWIND;
  }
  if (has_freevars_or_cellvars) {
    processFreevarsAndCellvars(thread, callee_frame);
  }
  return Continue::NEXT;
}

Continue Interpreter::callInterpreted(Thread* thread, word nargs,
                                      RawFunction function) {
  RawObject* post_call_sp = thread->stackPointer() + nargs + 1;
  return callInterpretedImpl(thread, nargs, function, post_call_sp,
                             preparePositionalCall);
}

HANDLER_INLINE Continue Interpreter::handleCall(
    Thread* thread, word nargs, word callable_idx, PrepareCallFunc prepare_args,
    Function::Entry (RawFunction::*get_entry)() const) {
  // Warning: This code is using `RawXXX` variables for performance! This is
  // despite the fact that we call functions that do potentially perform memory
  // allocations. This is legal here because we always rely on the functions
  // returning an up-to-date address and we make sure to never access any value
  // produce before a call after that call. Be careful not to break this
  // invariant if you change the code!

  RawObject* post_call_sp = thread->stackPointer() + callable_idx + 1;
  PrepareCallableResult prepare_result =
      prepareCallableCall(thread, nargs, callable_idx);
  nargs = prepare_result.nargs;
  if (prepare_result.function.isErrorException()) {
    thread->stackDrop(nargs + 1);
    DCHECK(thread->stackPointer() == post_call_sp, "stack not cleaned");
    return Continue::UNWIND;
  }
  RawFunction function = RawFunction::cast(prepare_result.function);

  IntrinsicFunction intrinsic =
      reinterpret_cast<IntrinsicFunction>(function.intrinsic());
  if (intrinsic != nullptr) {
    // Executes the function at the given symbol without pushing a new frame.
    // If the call succeeds, pops the arguments off of the caller's frame, sets
    // the top value to the result, and returns true. If the call fails, leaves
    // the stack unchanged and returns false.
    if ((*intrinsic)(thread)) {
      DCHECK(thread->stackPointer() == post_call_sp - 1, "stack not cleaned");
      return Continue::NEXT;
    }
  }

  if (!function.isInterpreted()) {
    return callTrampoline(thread, (function.*get_entry)(), nargs, post_call_sp);
  }

  return callInterpretedImpl(thread, nargs, function, post_call_sp,
                             prepare_args);
}

ALWAYS_INLINE Continue Interpreter::tailcallFunction(Thread* thread, word nargs,
                                                     RawObject function_obj) {
  RawObject* post_call_sp = thread->stackPointer() + nargs + 1;
  DCHECK(function_obj == thread->stackPeek(nargs),
         "thread->stackPeek(nargs) is expected to be the given function");
  RawFunction function = Function::cast(function_obj);
  IntrinsicFunction intrinsic =
      reinterpret_cast<IntrinsicFunction>(function.intrinsic());
  if (intrinsic != nullptr) {
    // Executes the function at the given symbol without pushing a new frame.
    // If the call succeeds, pops the arguments off of the caller's frame, sets
    // the top value to the result, and returns true. If the call fails, leaves
    // the stack unchanged and returns false.
    if ((*intrinsic)(thread)) {
      DCHECK(thread->stackPointer() == post_call_sp - 1, "stack not cleaned");
      return Continue::NEXT;
    }
  }
  if (!function.isInterpreted()) {
    return callTrampoline(thread, function.entry(), nargs, post_call_sp);
  }
  return callInterpretedImpl(thread, nargs, function, post_call_sp,
                             preparePositionalCall);
}

HANDLER_INLINE Continue Interpreter::doCallFunction(Thread* thread, word arg) {
  return handleCall(thread, arg, arg, preparePositionalCall, &Function::entry);
}

HANDLER_INLINE Continue Interpreter::doMakeFunction(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object qualname(&scope, thread->stackPop());
  Code code(&scope, thread->stackPop());
  Module module(&scope, frame->function().moduleObject());
  Runtime* runtime = thread->runtime();
  Function function(
      &scope, runtime->newFunctionWithCode(thread, qualname, code, module));
  if (arg & MakeFunctionFlag::CLOSURE) {
    function.setClosure(thread->stackPop());
    DCHECK(runtime->isInstanceOfTuple(function.closure()), "expected tuple");
  }
  if (arg & MakeFunctionFlag::ANNOTATION_DICT) {
    function.setAnnotations(thread->stackPop());
    DCHECK(runtime->isInstanceOfDict(function.annotations()), "expected dict");
  }
  if (arg & MakeFunctionFlag::DEFAULT_KW) {
    function.setKwDefaults(thread->stackPop());
    DCHECK(runtime->isInstanceOfDict(function.kwDefaults()), "expected dict");
  }
  if (arg & MakeFunctionFlag::DEFAULT) {
    function.setDefaults(thread->stackPop());
    DCHECK(runtime->isInstanceOfTuple(function.defaults()), "expected tuple");
  }
  thread->stackPush(*function);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildSlice(Thread* thread, word arg) {
  RawObject step = arg == 3 ? thread->stackPop() : NoneType::object();
  RawObject stop = thread->stackPop();
  RawObject start = thread->stackTop();
  Runtime* runtime = thread->runtime();
  if (start.isNoneType() && stop.isNoneType() && step.isNoneType()) {
    thread->stackSetTop(runtime->emptySlice());
  } else {
    HandleScope scope(thread);
    Object start_obj(&scope, start);
    Object stop_obj(&scope, stop);
    Object step_obj(&scope, step);
    thread->stackSetTop(runtime->newSlice(start_obj, stop_obj, step_obj));
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadClosure(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawCode code = Code::cast(frame->code());
  thread->stackPush(frame->local(code.nlocals() + arg));
  return Continue::NEXT;
}

static RawObject raiseUnboundCellFreeVar(Thread* thread, const Code& code,
                                         word idx) {
  HandleScope scope(thread);
  Object names_obj(&scope, NoneType::object());
  const char* fmt;
  if (idx < code.numCellvars()) {
    names_obj = code.cellvars();
    fmt = "local variable '%S' referenced before assignment";
  } else {
    idx -= code.numCellvars();
    names_obj = code.freevars();
    fmt =
        "free variable '%S' referenced before assignment in enclosing "
        "scope";
  }
  Tuple names(&scope, *names_obj);
  Str name(&scope, names.at(idx));
  return thread->raiseWithFmt(LayoutId::kUnboundLocalError, fmt, &name);
}

HANDLER_INLINE Continue Interpreter::doLoadDeref(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Code code(&scope, frame->code());
  Cell cell(&scope, frame->local(code.nlocals() + arg));
  Object value(&scope, cell.value());
  if (value.isUnbound()) {
    raiseUnboundCellFreeVar(thread, code, arg);
    return Continue::UNWIND;
  }
  thread->stackPush(*value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreDeref(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawCode code = Code::cast(frame->code());
  Cell::cast(frame->local(code.nlocals() + arg)).setValue(thread->stackPop());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doDeleteDeref(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawCode code = Code::cast(frame->code());
  Cell::cast(frame->local(code.nlocals() + arg)).setValue(Unbound::object());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doCallFunctionKw(Thread* thread,
                                                      word arg) {
  return handleCall(thread, arg, arg + 1, prepareKeywordCall,
                    &Function::entryKw);
}

HANDLER_INLINE Continue Interpreter::doCallFunctionEx(Thread* thread,
                                                      word arg) {
  word callable_idx = (arg & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1;
  RawObject* post_call_sp = thread->stackPointer() + callable_idx + 1;
  HandleScope scope(thread);
  Object callable(&scope, prepareCallableEx(thread, callable_idx));
  if (callable.isErrorException()) {
    thread->stackDrop(callable_idx + 1);
    DCHECK(thread->stackPointer() == post_call_sp, "stack not cleaned");
    return Continue::UNWIND;
  }

  Function function(&scope, *callable);
  if (!function.isInterpreted()) {
    return callTrampoline(thread, function.entryEx(), arg, post_call_sp);
  }

  if (prepareExplodeCall(thread, arg, *function).isErrorException()) {
    DCHECK(thread->stackPointer() == post_call_sp, "stack not cleaned");
    return Continue::UNWIND;
  }

  bool has_freevars_or_cellvars = function.hasFreevarsOrCellvars();
  Frame* callee_frame = thread->pushCallFrame(*function);
  if (UNLIKELY(callee_frame == nullptr)) {
    return Continue::UNWIND;
  }
  if (has_freevars_or_cellvars) {
    processFreevarsAndCellvars(thread, callee_frame);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doSetupWith(Thread* thread, word arg) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object mgr(&scope, thread->stackTop());
  Type mgr_type(&scope, runtime->typeOf(*mgr));
  Object enter(&scope, typeLookupInMroById(thread, *mgr_type, ID(__enter__)));
  if (enter.isError()) {
    if (enter.isErrorNotFound()) {
      thread->raise(LayoutId::kAttributeError,
                    runtime->symbols()->at(ID(__enter__)));
    } else {
      DCHECK(enter.isErrorException(),
             "expected Error::exception() or Error::notFound()");
    }
    return Continue::UNWIND;
  }

  Object exit(&scope, typeLookupInMroById(thread, *mgr_type, ID(__exit__)));
  if (exit.isError()) {
    if (exit.isErrorNotFound()) {
      thread->raise(LayoutId::kAttributeError,
                    runtime->symbols()->at(ID(__exit__)));
    } else {
      DCHECK(exit.isErrorException(),
             "expected Error::exception() or Error::notFound()");
    }
    return Continue::UNWIND;
  }
  Object exit_bound(&scope,
                    exit.isFunction()
                        ? runtime->newBoundMethod(exit, mgr)
                        : resolveDescriptorGet(thread, exit, mgr, mgr_type));
  thread->stackSetTop(*exit_bound);

  Object result(&scope, NoneType::object());
  if (enter.isFunction()) {
    result = callMethod1(thread, enter, mgr);
  } else {
    thread->stackPush(resolveDescriptorGet(thread, enter, mgr, mgr_type));
    result = call(thread, 0);
  }
  if (result.isErrorException()) return Continue::UNWIND;

  word stack_depth = thread->valueStackSize();
  Frame* frame = thread->currentFrame();
  word handler_pc = frame->virtualPC() + arg;
  frame->blockStackPush(TryBlock(TryBlock::kFinally, handler_pc, stack_depth));
  thread->stackPush(*result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doListAppend(Thread* thread, word arg) {
  HandleScope scope(thread);
  Object value(&scope, thread->stackPop());
  List list(&scope, thread->stackPeek(arg - 1));
  thread->runtime()->listAdd(thread, list, value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doSetAdd(Thread* thread, word arg) {
  HandleScope scope(thread);
  Object value(&scope, thread->stackPop());
  Object hash_obj(&scope, hash(thread, value));
  if (hash_obj.isErrorException()) {
    return Continue::UNWIND;
  }
  word hash = SmallInt::cast(*hash_obj).value();
  Set set(&scope, Set::cast(thread->stackPeek(arg - 1)));
  setAdd(thread, set, value, hash);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doMapAdd(Thread* thread, word arg) {
  HandleScope scope(thread);
  Object value(&scope, thread->stackPop());
  Object key(&scope, thread->stackPop());
  Dict dict(&scope, Dict::cast(thread->stackPeek(arg - 1)));
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) return Continue::UNWIND;
  word hash = SmallInt::cast(*hash_obj).value();
  Object result(&scope, dictAtPut(thread, dict, key, hash, value));
  if (result.isErrorException()) return Continue::UNWIND;
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadClassDeref(Thread* thread,
                                                      word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Code code(&scope, frame->code());
  word idx = arg - code.numCellvars();
  Str name(&scope, Tuple::cast(code.freevars()).at(idx));
  Object result(&scope, NoneType::object());
  if (frame->implicitGlobals().isNoneType()) {
    // Module body
    Module module(&scope, frame->function().moduleObject());
    result = moduleAt(module, name);
  } else {
    // Class body
    Object implicit_globals(&scope, frame->implicitGlobals());
    if (implicit_globals.isDict()) {
      Dict implicit_globals_dict(&scope, *implicit_globals);
      result = dictAtByStr(thread, implicit_globals_dict, name);
    } else {
      result = objectGetItem(thread, implicit_globals, name);
      if (result.isErrorException()) {
        if (!thread->pendingExceptionMatches(LayoutId::kKeyError)) {
          return Continue::UNWIND;
        }
        thread->clearPendingException();
      }
    }
  }

  if (result.isErrorNotFound()) {
    Cell cell(&scope, frame->local(code.nlocals() + arg));
    if (cell.isUnbound()) {
      UNIMPLEMENTED("unbound free var %s", Str::cast(*name).toCStr());
    }
    thread->stackPush(cell.value());
  } else {
    thread->stackPush(*result);
  }

  return Continue::NEXT;
}

static RawObject listUnpack(Thread* thread, const List& list,
                            const Object& iterable, Tuple* src_handle) {
  word src_length;
  if (iterable.isList()) {
    *src_handle = List::cast(*iterable).items();
    src_length = List::cast(*iterable).numItems();
  } else if (iterable.isTuple()) {
    *src_handle = *iterable;
    src_length = src_handle->length();
  } else {
    return thread->invokeMethodStatic2(LayoutId::kList, ID(extend), list,
                                       iterable);
  }
  listExtend(thread, list, *src_handle, src_length);
  return NoneType::object();
}

HANDLER_INLINE Continue Interpreter::doBuildListUnpack(Thread* thread,
                                                       word arg) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  List list(&scope, runtime->newList());
  Object iterable(&scope, NoneType::object());
  Tuple src_handle(&scope, runtime->emptyTuple());
  for (word i = arg - 1; i >= 0; i--) {
    iterable = thread->stackPeek(i);
    if (listUnpack(thread, list, iterable, &src_handle).isErrorException()) {
      return Continue::UNWIND;
    }
  }
  thread->stackDrop(arg - 1);
  thread->stackSetTop(*list);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildMapUnpack(Thread* thread,
                                                      word arg) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Dict dict(&scope, runtime->newDict());
  Object obj(&scope, NoneType::object());
  for (word i = arg - 1; i >= 0; i--) {
    obj = thread->stackPeek(i);
    if (dictMergeOverride(thread, dict, obj).isErrorException()) {
      if (thread->pendingExceptionType() ==
          runtime->typeAt(LayoutId::kAttributeError)) {
        thread->clearPendingException();
        thread->raiseWithFmt(LayoutId::kTypeError,
                             "'%T' object is not a mapping", &obj);
      }
      return Continue::UNWIND;
    }
  }
  thread->stackDrop(arg - 1);
  thread->stackSetTop(*dict);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildMapUnpackWithCall(Thread* thread,
                                                              word arg) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Dict dict(&scope, runtime->newDict());
  Object obj(&scope, NoneType::object());
  for (word i = arg - 1; i >= 0; i--) {
    obj = thread->stackPeek(i);
    if (dictMergeError(thread, dict, obj).isErrorException()) {
      if (thread->pendingExceptionType() ==
          runtime->typeAt(LayoutId::kAttributeError)) {
        thread->clearPendingException();
        thread->raiseWithFmt(LayoutId::kTypeError,
                             "'%T' object is not a mapping", &obj);
      } else if (thread->pendingExceptionType() ==
                 runtime->typeAt(LayoutId::kKeyError)) {
        Object value(&scope, thread->pendingExceptionValue());
        thread->clearPendingException();
        if (runtime->isInstanceOfStr(*value)) {
          thread->raiseWithFmt(LayoutId::kTypeError,
                               "got multiple values for keyword argument '%S'",
                               &value);
        } else {
          thread->raiseWithFmt(LayoutId::kTypeError,
                               "keywords must be strings");
        }
      }
      return Continue::UNWIND;
    }
  }
  thread->stackDrop(arg - 1);
  thread->stackSetTop(*dict);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildTupleUnpack(Thread* thread,
                                                        word arg) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  List list(&scope, runtime->newList());
  Object iterable(&scope, NoneType::object());
  Tuple src_handle(&scope, runtime->emptyTuple());
  for (word i = arg - 1; i >= 0; i--) {
    iterable = thread->stackPeek(i);
    if (listUnpack(thread, list, iterable, &src_handle).isErrorException()) {
      return Continue::UNWIND;
    }
  }
  Tuple items(&scope, list.items());
  Tuple tuple(&scope, runtime->tupleSubseq(thread, items, 0, list.numItems()));
  thread->stackDrop(arg - 1);
  thread->stackSetTop(*tuple);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildSetUnpack(Thread* thread,
                                                      word arg) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Set set(&scope, runtime->newSet());
  Object obj(&scope, NoneType::object());
  for (word i = 0; i < arg; i++) {
    obj = thread->stackPeek(i);
    if (setUpdate(thread, set, obj).isErrorException()) return Continue::UNWIND;
  }
  thread->stackDrop(arg - 1);
  thread->stackSetTop(*set);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doSetupAsyncWith(Thread* thread,
                                                      word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object result(&scope, thread->stackPop());
  word stack_depth = thread->valueStackSize();
  word handler_pc = frame->virtualPC() + arg;
  frame->blockStackPush(TryBlock(TryBlock::kFinally, handler_pc, stack_depth));
  thread->stackPush(*result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doFormatValue(Thread* thread, word arg) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object fmt_spec(&scope, Str::empty());
  if (arg & kFormatValueHasSpecBit) {
    fmt_spec = thread->stackPop();
  }
  Object value(&scope, thread->stackPop());
  switch (static_cast<FormatValueConv>(arg & kFormatValueConvMask)) {
    case FormatValueConv::kStr: {
      if (!value.isStr()) {
        value = thread->invokeMethod1(value, ID(__str__));
        DCHECK(!value.isErrorNotFound(), "`__str__` should always exist");
        if (value.isErrorException()) return Continue::UNWIND;
        if (!runtime->isInstanceOfStr(*value)) {
          thread->raiseWithFmt(LayoutId::kTypeError,
                               "__str__ returned non-string (type %T)", &value);
          return Continue::UNWIND;
        }
      }
      break;
    }
    case FormatValueConv::kRepr: {
      value = thread->invokeMethod1(value, ID(__repr__));
      DCHECK(!value.isErrorNotFound(), "`__repr__` should always exist");
      if (value.isErrorException()) return Continue::UNWIND;
      if (!runtime->isInstanceOfStr(*value)) {
        thread->raiseWithFmt(LayoutId::kTypeError,
                             "__repr__ returned non-string (type %T)", &value);
        return Continue::UNWIND;
      }
      break;
    }
    case FormatValueConv::kAscii: {
      value = thread->invokeMethod1(value, ID(__repr__));
      DCHECK(!value.isErrorNotFound(), "`__repr__` should always exist");
      if (value.isErrorException()) return Continue::UNWIND;
      if (!runtime->isInstanceOfStr(*value)) {
        thread->raiseWithFmt(LayoutId::kTypeError,
                             "__repr__ returned non-string (type %T)", &value);
        return Continue::UNWIND;
      }
      value = strEscapeNonASCII(thread, value);
      break;
    }
    case FormatValueConv::kNone:
      break;
  }

  if (fmt_spec != Str::empty() || !value.isStr()) {
    value = thread->invokeMethod2(value, ID(__format__), fmt_spec);
    if (value.isErrorException()) return Continue::UNWIND;
    if (!runtime->isInstanceOfStr(*value)) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "__format__ must return a str, not %T", &value);
      return Continue::UNWIND;
    }
  }
  thread->stackPush(*value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildConstKeyMap(Thread* thread,
                                                        word arg) {
  HandleScope scope(thread);
  Tuple keys(&scope, thread->stackTop());
  Dict dict(&scope, thread->runtime()->newDictWithSize(keys.length()));
  Object key(&scope, NoneType::object());
  Object hash_obj(&scope, NoneType::object());
  for (word i = 0; i < arg; i++) {
    key = keys.at(i);
    hash_obj = Interpreter::hash(thread, key);
    if (hash_obj.isErrorException()) return Continue::UNWIND;
    word hash = SmallInt::cast(*hash_obj).value();
    Object value(&scope, thread->stackPeek(arg - i));
    if (dictAtPut(thread, dict, key, hash, value).isErrorException()) {
      return Continue::UNWIND;
    }
  }
  thread->stackDrop(arg + 1);
  thread->stackPush(*dict);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildString(Thread* thread, word arg) {
  switch (arg) {
    case 0:  // empty
      thread->stackPush(Str::empty());
      break;
    case 1:  // no-op
      break;
    default: {  // concat
      RawObject res = stringJoin(thread, thread->stackPointer(), arg);
      thread->stackDrop(arg - 1);
      thread->stackSetTop(res);
      break;
    }
  }
  return Continue::NEXT;
}

// LOAD_METHOD shapes the stack as follows:
//
//     receiver or unbound
//     callable <- Top of stack / lower memory addresses
//
// LOAD_METHOD is paired with a CALL_METHOD, and the matching CALL_METHOD
// falls back to the behavior of CALL_FUNCTION in this shape of the stack.
HANDLER_INLINE Continue Interpreter::doLoadMethod(Thread* thread, word arg) {
  thread->stackInsertAt(1, Unbound::object());
  return doLoadAttr(thread, arg);
}

Continue Interpreter::loadMethodUpdateCache(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  word original_arg = originalArg(frame->function(), arg);
  Object receiver(&scope, thread->stackTop());
  Str name(&scope,
           Tuple::cast(Code::cast(frame->code()).names()).at(original_arg));

  Object location(&scope, NoneType::object());
  LoadAttrKind kind;
  Object result(&scope, thread->runtime()->attributeAtSetLocation(
                            thread, receiver, name, &kind, &location));
  if (result.isErrorException()) return Continue::UNWIND;
  if (kind != LoadAttrKind::kInstanceFunction) {
    thread->stackPush(*result);
    thread->stackSetAt(1, Unbound::object());
    return Continue::NEXT;
  }

  // Cache the attribute load.
  MutableTuple caches(&scope, frame->caches());
  Function dependent(&scope, frame->function());
  ICState next_ic_state = icUpdateAttr(thread, caches, arg, receiver.layoutId(),
                                       location, name, dependent);

  switch (next_ic_state) {
    case ICState::kMonomorphic:
      rewriteCurrentBytecode(frame, LOAD_METHOD_INSTANCE_FUNCTION);
      break;
    case ICState::kPolymorphic:
      rewriteCurrentBytecode(frame, LOAD_METHOD_POLYMORPHIC);
      break;
    case ICState::kAnamorphic:
      UNREACHABLE("next_ic_state cannot be anamorphic");
      break;
  }
  thread->stackInsertAt(1, *location);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadMethodAnamorphic(Thread* thread,
                                                            word arg) {
  return loadMethodUpdateCache(thread, arg);
}

HANDLER_INLINE Continue
Interpreter::doLoadMethodInstanceFunction(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawMutableTuple caches = MutableTuple::cast(frame->caches());
  RawObject receiver = thread->stackTop();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    EVENT_CACHE(LOAD_METHOD_INSTANCE_FUNCTION);
    return loadMethodUpdateCache(thread, arg);
  }
  DCHECK(cached.isFunction(), "cached is expected to be a function");
  thread->stackInsertAt(1, cached);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadMethodPolymorphic(Thread* thread,
                                                             word arg) {
  Frame* frame = thread->currentFrame();
  RawObject receiver = thread->stackTop();
  bool is_found;
  RawObject cached = icLookupPolymorphic(MutableTuple::cast(frame->caches()),
                                         arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    EVENT_CACHE(LOAD_METHOD_POLYMORPHIC);
    return loadMethodUpdateCache(thread, arg);
  }
  DCHECK(cached.isFunction(), "cached is expected to be a function");
  thread->stackInsertAt(1, cached);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doCallMethod(Thread* thread, word arg) {
  RawObject maybe_method = thread->stackPeek(arg + 1);
  if (maybe_method.isUnbound()) {
    thread->stackRemoveAt(arg + 1);
    return handleCall(thread, arg, arg, preparePositionalCall,
                      &Function::entry);
  }
  // Add one to bind receiver to the self argument. See doLoadMethod()
  // for details on the stack's shape.
  return tailcallFunction(thread, arg + 1, maybe_method);
}

NEVER_INLINE
Continue Interpreter::compareInUpdateCache(Thread* thread, word arg) {
  HandleScope scope(thread);
  Object container(&scope, thread->stackPop());
  Object value(&scope, thread->stackPop());
  Object method(&scope, NoneType::object());
  Object result(&scope,
                sequenceContainsSetMethod(thread, value, container, &method));
  if (method.isFunction()) {
    Frame* frame = thread->currentFrame();
    MutableTuple caches(&scope, frame->caches());
    Str dunder_contains_name(
        &scope, thread->runtime()->symbols()->at(ID(__contains__)));
    Function dependent(&scope, frame->function());
    ICState next_cache_state =
        icUpdateAttr(thread, caches, arg, container.layoutId(), method,
                     dunder_contains_name, dependent);
    rewriteCurrentBytecode(frame, next_cache_state == ICState::kMonomorphic
                                      ? COMPARE_IN_MONOMORPHIC
                                      : COMPARE_IN_POLYMORPHIC);
  }
  if (result.isErrorException()) return Continue::UNWIND;
  thread->stackPush(*result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doCompareInAnamorphic(Thread* thread,
                                                           word arg) {
  Frame* frame = thread->currentFrame();
  RawObject container = thread->stackPeek(0);
  switch (container.layoutId()) {
    case LayoutId::kSmallStr:
    case LayoutId::kLargeStr:
      if (thread->stackPeek(1).isStr()) {
        rewriteCurrentBytecode(frame, COMPARE_IN_STR);
        return doCompareInStr(thread, arg);
      }
      return compareInUpdateCache(thread, arg);
    case LayoutId::kTuple:
      rewriteCurrentBytecode(frame, COMPARE_IN_TUPLE);
      return doCompareInTuple(thread, arg);
    case LayoutId::kDict:
      rewriteCurrentBytecode(frame, COMPARE_IN_DICT);
      return doCompareInDict(thread, arg);
    case LayoutId::kList:
      rewriteCurrentBytecode(frame, COMPARE_IN_LIST);
      return doCompareInList(thread, arg);
    default:
      return compareInUpdateCache(thread, arg);
  }
}

HANDLER_INLINE Continue Interpreter::doCompareInDict(Thread* thread, word arg) {
  RawObject container = thread->stackPeek(0);
  if (!container.isDict()) {
    EVENT_CACHE(COMPARE_IN_DICT);
    return compareInUpdateCache(thread, arg);
  }
  HandleScope scope(thread);
  Dict dict(&scope, container);
  Object key(&scope, thread->stackPeek(1));
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) return Continue::UNWIND;
  word hash = SmallInt::cast(*hash_obj).value();
  RawObject result = dictAt(thread, dict, key, hash);
  DCHECK(!result.isErrorException(), "dictAt raised an exception");
  thread->stackDrop(2);
  if (result.isErrorNotFound()) {
    thread->stackPush(Bool::falseObj());
  } else {
    thread->stackPush(Bool::trueObj());
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doCompareInList(Thread* thread, word arg) {
  RawObject container = thread->stackPeek(0);
  if (!container.isList()) {
    EVENT_CACHE(COMPARE_IN_LIST);
    return compareInUpdateCache(thread, arg);
  }
  HandleScope scope(thread);
  List list(&scope, container);
  Object key(&scope, thread->stackPeek(1));
  Object result(&scope, listContains(thread, list, key));
  if (result.isErrorException()) return Continue::UNWIND;
  DCHECK(result.isBool(), "bool is unexpected");
  thread->stackDrop(2);
  thread->stackPush(*result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doCompareInStr(Thread* thread, word arg) {
  RawObject container = thread->stackPeek(0);
  RawObject value = thread->stackPeek(1);
  if (!(container.isStr() && value.isStr())) {
    EVENT_CACHE(COMPARE_IN_STR);
    return compareInUpdateCache(thread, arg);
  }
  HandleScope scope(thread);
  Str haystack(&scope, container);
  Str needle(&scope, value);
  thread->stackDrop(2);
  thread->stackPush(Bool::fromBool(strFind(haystack, needle) != -1));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doCompareInTuple(Thread* thread,
                                                      word arg) {
  RawObject container = thread->stackPeek(0);
  if (!container.isTuple()) {
    EVENT_CACHE(COMPARE_IN_TUPLE);
    return compareInUpdateCache(thread, arg);
  }
  HandleScope scope(thread);
  Tuple tuple(&scope, container);
  Object value(&scope, thread->stackPeek(1));
  RawObject result = tupleContains(thread, tuple, value);
  if (result.isErrorException()) {
    return Continue::UNWIND;
  }
  thread->stackDrop(2);
  thread->stackPush(result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doCompareInMonomorphic(Thread* thread,
                                                            word arg) {
  Frame* frame = thread->currentFrame();
  RawObject container = thread->stackPeek(0);
  RawObject value = thread->stackPeek(1);
  LayoutId container_layout_id = container.layoutId();
  bool is_found;
  RawObject cached = icLookupMonomorphic(MutableTuple::cast(frame->caches()),
                                         arg, container_layout_id, &is_found);
  if (!is_found) {
    EVENT_CACHE(COMPARE_IN_MONOMORPHIC);
    return compareInUpdateCache(thread, arg);
  }
  thread->stackDrop(2);
  thread->stackPush(cached);
  thread->stackPush(container);
  thread->stackPush(value);
  // A recursive call is needed to coerce the return value to bool.
  RawObject result = call(thread, 2);
  if (result.isErrorException()) return Continue::UNWIND;
  thread->stackPush(isTrue(thread, result));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doCompareInPolymorphic(Thread* thread,
                                                            word arg) {
  Frame* frame = thread->currentFrame();
  RawObject container = thread->stackPeek(0);
  RawObject value = thread->stackPeek(1);
  LayoutId container_layout_id = container.layoutId();
  bool is_found;
  RawObject cached = icLookupPolymorphic(MutableTuple::cast(frame->caches()),
                                         arg, container_layout_id, &is_found);
  if (!is_found) {
    EVENT_CACHE(COMPARE_IN_POLYMORPHIC);
    return compareInUpdateCache(thread, arg);
  }
  thread->stackDrop(2);
  thread->stackPush(cached);
  thread->stackPush(container);
  thread->stackPush(value);
  // Should use a recursive call to convert it return type to bool.
  RawObject result = call(thread, 2);
  if (result.isError()) return Continue::UNWIND;
  thread->stackPush(isTrue(thread, result));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::cachedBinaryOpImpl(
    Thread* thread, word arg, OpcodeHandler update_cache,
    BinaryOpFallbackHandler fallback) {
  Frame* frame = thread->currentFrame();
  RawObject left_raw = thread->stackPeek(1);
  RawObject right_raw = thread->stackPeek(0);
  LayoutId left_layout_id = left_raw.layoutId();
  LayoutId right_layout_id = right_raw.layoutId();
  BinaryOpFlags flags = kBinaryOpNone;
  RawObject method =
      icLookupBinOpPolymorphic(MutableTuple::cast(frame->caches()), arg,
                               left_layout_id, right_layout_id, &flags);
  if (method.isErrorNotFound()) {
    return update_cache(thread, arg);
  }

  // Fast-path: Call cached method and return if possible.
  RawObject result =
      binaryOperationWithMethod(thread, method, flags, left_raw, right_raw);
  if (result.isErrorException()) return Continue::UNWIND;
  if (!result.isNotImplementedType()) {
    thread->stackDrop(1);
    thread->stackSetTop(result);
    return Continue::NEXT;
  }

  return fallback(thread, arg, flags);
}

Continue Interpreter::compareOpUpdateCache(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object right(&scope, thread->stackPop());
  Object left(&scope, thread->stackPop());
  Function function(&scope, frame->function());
  CompareOp op = static_cast<CompareOp>(originalArg(*function, arg));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  RawObject result =
      compareOperationSetMethod(thread, op, left, right, &method, &flags);
  if (result.isErrorException()) return Continue::UNWIND;
  if (!method.isNoneType()) {
    MutableTuple caches(&scope, frame->caches());
    LayoutId left_layout_id = left.layoutId();
    LayoutId right_layout_id = right.layoutId();
    ICState next_cache_state = icUpdateBinOp(
        thread, caches, arg, left_layout_id, right_layout_id, method, flags);
    icInsertCompareOpDependencies(thread, function, left_layout_id,
                                  right_layout_id, op);
    rewriteCurrentBytecode(frame, next_cache_state == ICState::kMonomorphic
                                      ? COMPARE_OP_MONOMORPHIC
                                      : COMPARE_OP_POLYMORPHIC);
  }
  thread->stackPush(result);
  return Continue::NEXT;
}

Continue Interpreter::compareOpFallback(Thread* thread, word arg,
                                        BinaryOpFlags flags) {
  // Slow-path: We may need to call the reversed op when the first method
  // returned `NotImplemented`.
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  CompareOp op = static_cast<CompareOp>(originalArg(frame->function(), arg));
  Object right(&scope, thread->stackPop());
  Object left(&scope, thread->stackPop());
  Object result(&scope, compareOperationRetry(thread, op, flags, left, right));
  if (result.isErrorException()) return Continue::UNWIND;
  thread->stackPush(*result);
  return Continue::NEXT;
}

HANDLER_INLINE
Continue Interpreter::doCompareEqSmallInt(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    thread->stackDrop(1);
    thread->stackSetTop(Bool::fromBool(left_value == right_value));
    return Continue::NEXT;
  }
  EVENT_CACHE(COMPARE_EQ_SMALLINT);
  return compareOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doCompareGtSmallInt(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    thread->stackDrop(1);
    thread->stackSetTop(Bool::fromBool(left_value > right_value));
    return Continue::NEXT;
  }
  EVENT_CACHE(COMPARE_GT_SMALLINT);
  return compareOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doCompareLtSmallInt(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    thread->stackDrop(1);
    thread->stackSetTop(Bool::fromBool(left_value < right_value));
    return Continue::NEXT;
  }
  EVENT_CACHE(COMPARE_LT_SMALLINT);
  return compareOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doCompareGeSmallInt(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    thread->stackDrop(1);
    thread->stackSetTop(Bool::fromBool(left_value >= right_value));
    return Continue::NEXT;
  }
  EVENT_CACHE(COMPARE_GE_SMALLINT);
  return compareOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doCompareNeSmallInt(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    thread->stackDrop(1);
    thread->stackSetTop(Bool::fromBool(left_value != right_value));
    return Continue::NEXT;
  }
  EVENT_CACHE(COMPARE_NE_SMALLINT);
  return compareOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doCompareLeSmallInt(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    thread->stackDrop(1);
    thread->stackSetTop(Bool::fromBool(left_value <= right_value));
    return Continue::NEXT;
  }
  EVENT_CACHE(COMPARE_LE_SMALLINT);
  return compareOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doCompareEqStr(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isStr() && right.isStr()) {
    thread->stackDrop(1);
    thread->stackSetTop(Bool::fromBool(Str::cast(left).equals(right)));
    return Continue::NEXT;
  }
  EVENT_CACHE(COMPARE_EQ_STR);
  return compareOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doCompareNeStr(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isStr() && right.isStr()) {
    thread->stackDrop(1);
    thread->stackSetTop(Bool::fromBool(!Str::cast(left).equals(right)));
    return Continue::NEXT;
  }
  EVENT_CACHE(COMPARE_NE_STR);
  return compareOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doCompareOpMonomorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left_raw = thread->stackPeek(1);
  RawObject right_raw = thread->stackPeek(0);
  LayoutId left_layout_id = left_raw.layoutId();
  LayoutId right_layout_id = right_raw.layoutId();
  BinaryOpFlags flags = kBinaryOpNone;
  RawObject method =
      icLookupBinOpMonomorphic(MutableTuple::cast(frame->caches()), arg,
                               left_layout_id, right_layout_id, &flags);
  if (method.isErrorNotFound()) {
    EVENT_CACHE(COMPARE_OP_MONOMORPHIC);
    return compareOpUpdateCache(thread, arg);
  }
  return binaryOp(thread, arg, method, flags, left_raw, right_raw,
                  compareOpFallback);
}

HANDLER_INLINE
Continue Interpreter::doCompareOpPolymorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left_raw = thread->stackPeek(1);
  RawObject right_raw = thread->stackPeek(0);
  LayoutId left_layout_id = left_raw.layoutId();
  LayoutId right_layout_id = right_raw.layoutId();
  BinaryOpFlags flags = kBinaryOpNone;
  RawObject method =
      icLookupBinOpPolymorphic(MutableTuple::cast(frame->caches()), arg,
                               left_layout_id, right_layout_id, &flags);
  if (method.isErrorNotFound()) {
    EVENT_CACHE(COMPARE_OP_POLYMORPHIC);
    return compareOpUpdateCache(thread, arg);
  }
  return binaryOp(thread, arg, method, flags, left_raw, right_raw,
                  compareOpFallback);
}

HANDLER_INLINE
Continue Interpreter::doCompareOpAnamorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    switch (static_cast<CompareOp>(originalArg(frame->function(), arg))) {
      case CompareOp::EQ:
        rewriteCurrentBytecode(frame, COMPARE_EQ_SMALLINT);
        return doCompareEqSmallInt(thread, arg);
      case CompareOp::GT:
        rewriteCurrentBytecode(frame, COMPARE_GT_SMALLINT);
        return doCompareGtSmallInt(thread, arg);
      case CompareOp::LT:
        rewriteCurrentBytecode(frame, COMPARE_LT_SMALLINT);
        return doCompareLtSmallInt(thread, arg);
      case CompareOp::GE:
        rewriteCurrentBytecode(frame, COMPARE_GE_SMALLINT);
        return doCompareGeSmallInt(thread, arg);
      case CompareOp::NE:
        rewriteCurrentBytecode(frame, COMPARE_NE_SMALLINT);
        return doCompareNeSmallInt(thread, arg);
      case CompareOp::LE:
        rewriteCurrentBytecode(frame, COMPARE_LE_SMALLINT);
        return doCompareLeSmallInt(thread, arg);
      default:
        return compareOpUpdateCache(thread, arg);
    }
  }
  if (left.isStr() && right.isStr()) {
    switch (static_cast<CompareOp>(originalArg(frame->function(), arg))) {
      case CompareOp::EQ:
        rewriteCurrentBytecode(frame, COMPARE_EQ_STR);
        return doCompareEqStr(thread, arg);
      case CompareOp::NE:
        rewriteCurrentBytecode(frame, COMPARE_NE_STR);
        return doCompareNeStr(thread, arg);
      default:
        return compareOpUpdateCache(thread, arg);
    }
  }
  return compareOpUpdateCache(thread, arg);
}

Continue Interpreter::inplaceOpUpdateCache(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object right(&scope, thread->stackPop());
  Object left(&scope, thread->stackPop());
  Function function(&scope, frame->function());
  BinaryOp op = static_cast<BinaryOp>(originalArg(*function, arg));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  RawObject result =
      inplaceOperationSetMethod(thread, op, left, right, &method, &flags);
  if (!method.isNoneType()) {
    MutableTuple caches(&scope, frame->caches());
    LayoutId left_layout_id = left.layoutId();
    LayoutId right_layout_id = right.layoutId();
    ICState next_cache_state = icUpdateBinOp(
        thread, caches, arg, left_layout_id, right_layout_id, method, flags);
    icInsertInplaceOpDependencies(thread, function, left_layout_id,
                                  right_layout_id, op);
    rewriteCurrentBytecode(frame, next_cache_state == ICState::kMonomorphic
                                      ? INPLACE_OP_MONOMORPHIC
                                      : INPLACE_OP_POLYMORPHIC);
  }
  if (result.isErrorException()) return Continue::UNWIND;
  thread->stackPush(result);
  return Continue::NEXT;
}

Continue Interpreter::inplaceOpFallback(Thread* thread, word arg,
                                        BinaryOpFlags flags) {
  // Slow-path: We may need to try other ways to resolve things when the first
  // call returned `NotImplemented`.
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  BinaryOp op = static_cast<BinaryOp>(originalArg(frame->function(), arg));
  Object right(&scope, thread->stackPop());
  Object left(&scope, thread->stackPop());
  Object result(&scope, NoneType::object());
  if (flags & kInplaceBinaryOpRetry) {
    // The cached operation was an in-place operation we have to try to the
    // usual binary operation mechanics now.
    result = binaryOperation(thread, op, left, right);
  } else {
    // The cached operation was already a binary operation (e.g. __add__ or
    // __radd__) so we have to invoke `binaryOperationRetry`.
    result = binaryOperationRetry(thread, op, flags, left, right);
  }
  if (result.isErrorException()) return Continue::UNWIND;
  thread->stackPush(*result);
  return Continue::NEXT;
}

HANDLER_INLINE
Continue Interpreter::doInplaceOpMonomorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left_raw = thread->stackPeek(1);
  RawObject right_raw = thread->stackPeek(0);
  LayoutId left_layout_id = left_raw.layoutId();
  LayoutId right_layout_id = right_raw.layoutId();
  BinaryOpFlags flags = kBinaryOpNone;
  RawObject method =
      icLookupBinOpMonomorphic(MutableTuple::cast(frame->caches()), arg,
                               left_layout_id, right_layout_id, &flags);
  if (method.isErrorNotFound()) {
    EVENT_CACHE(INPLACE_OP_MONOMORPHIC);
    return inplaceOpUpdateCache(thread, arg);
  }
  return binaryOp(thread, arg, method, flags, left_raw, right_raw,
                  inplaceOpFallback);
}

HANDLER_INLINE
Continue Interpreter::doInplaceOpPolymorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left_raw = thread->stackPeek(1);
  RawObject right_raw = thread->stackPeek(0);
  LayoutId left_layout_id = left_raw.layoutId();
  LayoutId right_layout_id = right_raw.layoutId();
  BinaryOpFlags flags = kBinaryOpNone;
  RawObject method =
      icLookupBinOpPolymorphic(MutableTuple::cast(frame->caches()), arg,
                               left_layout_id, right_layout_id, &flags);
  if (method.isErrorNotFound()) {
    EVENT_CACHE(INPLACE_OP_POLYMORPHIC);
    return inplaceOpUpdateCache(thread, arg);
  }
  return binaryOp(thread, arg, method, flags, left_raw, right_raw,
                  inplaceOpFallback);
}

HANDLER_INLINE
Continue Interpreter::doInplaceAddSmallInt(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    word result_value = left_value + right_value;
    if (SmallInt::isValid(result_value)) {
      thread->stackDrop(1);
      thread->stackSetTop(SmallInt::fromWord(result_value));
      return Continue::NEXT;
    }
  }
  EVENT_CACHE(INPLACE_ADD_SMALLINT);
  return inplaceOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doInplaceSubSmallInt(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    word result_value = left_value - right_value;
    if (SmallInt::isValid(result_value)) {
      thread->stackDrop(1);
      thread->stackSetTop(SmallInt::fromWord(result_value));
      return Continue::NEXT;
    }
  }
  return inplaceOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doInplaceOpAnamorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  if (thread->stackPeek(0).isSmallInt() && thread->stackPeek(1).isSmallInt()) {
    switch (static_cast<BinaryOp>(originalArg(frame->function(), arg))) {
      case BinaryOp::ADD:
        rewriteCurrentBytecode(frame, INPLACE_ADD_SMALLINT);
        return doInplaceAddSmallInt(thread, arg);
      case BinaryOp::SUB:
        rewriteCurrentBytecode(frame, INPLACE_SUB_SMALLINT);
        return doInplaceSubSmallInt(thread, arg);
      default:
        return inplaceOpUpdateCache(thread, arg);
    }
  }
  return inplaceOpUpdateCache(thread, arg);
}

Continue Interpreter::binaryOpUpdateCache(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object right(&scope, thread->stackPop());
  Object left(&scope, thread->stackPop());
  Function function(&scope, frame->function());
  BinaryOp op = static_cast<BinaryOp>(originalArg(*function, arg));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  Object result(&scope, binaryOperationSetMethod(thread, op, left, right,
                                                 &method, &flags));
  if (!method.isNoneType()) {
    MutableTuple caches(&scope, frame->caches());
    LayoutId left_layout_id = left.layoutId();
    LayoutId right_layout_id = right.layoutId();
    ICState next_cache_state = icUpdateBinOp(
        thread, caches, arg, left_layout_id, right_layout_id, method, flags);
    icInsertBinaryOpDependencies(thread, function, left_layout_id,
                                 right_layout_id, op);
    rewriteCurrentBytecode(frame, next_cache_state == ICState::kMonomorphic
                                      ? BINARY_OP_MONOMORPHIC
                                      : BINARY_OP_POLYMORPHIC);
  }
  if (result.isErrorException()) return Continue::UNWIND;
  thread->stackPush(*result);
  return Continue::NEXT;
}

Continue Interpreter::binaryOpFallback(Thread* thread, word arg,
                                       BinaryOpFlags flags) {
  // Slow-path: We may need to call the reversed op when the first method
  // returned `NotImplemented`.
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  BinaryOp op = static_cast<BinaryOp>(originalArg(frame->function(), arg));
  Object right(&scope, thread->stackPop());
  Object left(&scope, thread->stackPop());
  Object result(&scope, binaryOperationRetry(thread, op, flags, left, right));
  if (result.isErrorException()) return Continue::UNWIND;
  thread->stackPush(*result);
  return Continue::NEXT;
}

ALWAYS_INLINE Continue Interpreter::binaryOp(Thread* thread, word arg,
                                             RawObject method,
                                             BinaryOpFlags flags,
                                             RawObject left, RawObject right,
                                             BinaryOpFallbackHandler fallback) {
  DCHECK(method.isFunction(), "method is expected to be a function");
  RawObject result =
      binaryOperationWithMethod(thread, method, flags, left, right);
  if (result.isErrorException()) return Continue::UNWIND;
  if (!result.isNotImplementedType()) {
    thread->stackDrop(1);
    thread->stackSetTop(result);
    return Continue::NEXT;
  }
  return fallback(thread, arg, flags);
}

HANDLER_INLINE
Continue Interpreter::doBinaryOpMonomorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left_raw = thread->stackPeek(1);
  RawObject right_raw = thread->stackPeek(0);
  LayoutId left_layout_id = left_raw.layoutId();
  LayoutId right_layout_id = right_raw.layoutId();
  BinaryOpFlags flags = kBinaryOpNone;
  RawObject method =
      icLookupBinOpMonomorphic(MutableTuple::cast(frame->caches()), arg,
                               left_layout_id, right_layout_id, &flags);
  if (method.isErrorNotFound()) {
    EVENT_CACHE(BINARY_OP_MONOMORPHIC);
    return binaryOpUpdateCache(thread, arg);
  }
  return binaryOp(thread, arg, method, flags, left_raw, right_raw,
                  binaryOpFallback);
}

HANDLER_INLINE
Continue Interpreter::doBinaryOpPolymorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left_raw = thread->stackPeek(1);
  RawObject right_raw = thread->stackPeek(0);
  LayoutId left_layout_id = left_raw.layoutId();
  LayoutId right_layout_id = right_raw.layoutId();
  BinaryOpFlags flags = kBinaryOpNone;
  RawObject method =
      icLookupBinOpPolymorphic(MutableTuple::cast(frame->caches()), arg,
                               left_layout_id, right_layout_id, &flags);
  if (method.isErrorNotFound()) {
    EVENT_CACHE(BINARY_OP_POLYMORPHIC);
    return binaryOpUpdateCache(thread, arg);
  }
  return binaryOp(thread, arg, method, flags, left_raw, right_raw,
                  binaryOpFallback);
}

HANDLER_INLINE
Continue Interpreter::doBinaryAddSmallInt(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    word result_value = left_value + right_value;
    if (SmallInt::isValid(result_value)) {
      thread->stackDrop(1);
      thread->stackSetTop(SmallInt::fromWord(result_value));
      return Continue::NEXT;
    }
  }
  EVENT_CACHE(BINARY_ADD_SMALLINT);
  return binaryOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doBinaryAndSmallInt(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    word result_value = left_value & right_value;
    DCHECK(SmallInt::isValid(result_value), "result should be a SmallInt");
    thread->stackDrop(1);
    thread->stackSetTop(SmallInt::fromWord(result_value));
    return Continue::NEXT;
  }
  EVENT_CACHE(BINARY_AND_SMALLINT);
  return binaryOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doBinaryMulSmallInt(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word result;
    if (!__builtin_mul_overflow(SmallInt::cast(left).value(),
                                SmallInt::cast(right).value(), &result) &&
        SmallInt::isValid(result)) {
      thread->stackDrop(1);
      thread->stackSetTop(SmallInt::fromWord(result));
      return Continue::NEXT;
    }
  }
  EVENT_CACHE(BINARY_MUL_SMALLINT);
  return binaryOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doBinaryFloordivSmallInt(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    if (right_value == 0) {
      thread->raiseWithFmt(LayoutId::kZeroDivisionError,
                           "integer division or modulo by zero");
      return Continue::UNWIND;
    }
    word result_value = left_value / right_value;
    DCHECK(SmallInt::isValid(result_value), "result should be a SmallInt");
    thread->stackDrop(1);
    thread->stackSetTop(SmallInt::fromWord(result_value));
    return Continue::NEXT;
  }
  EVENT_CACHE(BINARY_FLOORDIV_SMALLINT);
  return binaryOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doBinarySubSmallInt(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    word result_value = left_value - right_value;
    if (SmallInt::isValid(result_value)) {
      thread->stackDrop(1);
      thread->stackSetTop(SmallInt::fromWord(result_value));
      return Continue::NEXT;
    }
  }
  EVENT_CACHE(BINARY_SUB_SMALLINT);
  return binaryOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doBinaryOrSmallInt(Thread* thread, word arg) {
  RawObject left = thread->stackPeek(1);
  RawObject right = thread->stackPeek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    word result_value = left_value | right_value;
    DCHECK(SmallInt::isValid(result_value), "result should be a SmallInt");
    thread->stackDrop(1);
    thread->stackSetTop(SmallInt::fromWord(result_value));
    return Continue::NEXT;
  }
  EVENT_CACHE(BINARY_OR_SMALLINT);
  return binaryOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doBinaryOpAnamorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  if (thread->stackPeek(0).isSmallInt() && thread->stackPeek(1).isSmallInt()) {
    switch (static_cast<BinaryOp>(originalArg(frame->function(), arg))) {
      case BinaryOp::ADD:
        rewriteCurrentBytecode(frame, BINARY_ADD_SMALLINT);
        return doBinaryAddSmallInt(thread, arg);
      case BinaryOp::AND:
        rewriteCurrentBytecode(frame, BINARY_AND_SMALLINT);
        return doBinaryAndSmallInt(thread, arg);
      case BinaryOp::MUL:
        rewriteCurrentBytecode(frame, BINARY_MUL_SMALLINT);
        return doBinaryMulSmallInt(thread, arg);
      case BinaryOp::FLOORDIV:
        rewriteCurrentBytecode(frame, BINARY_FLOORDIV_SMALLINT);
        return doBinaryFloordivSmallInt(thread, arg);
      case BinaryOp::SUB:
        rewriteCurrentBytecode(frame, BINARY_SUB_SMALLINT);
        return doBinarySubSmallInt(thread, arg);
      case BinaryOp::OR:
        rewriteCurrentBytecode(frame, BINARY_OR_SMALLINT);
        return doBinaryOrSmallInt(thread, arg);
      default:
        return binaryOpUpdateCache(thread, arg);
    }
  }
  return binaryOpUpdateCache(thread, arg);
}

RawObject Interpreter::execute(Thread* thread) {
  DCHECK(!thread->hasPendingException(), "unhandled exception lingering");
  return thread->interpreterFunc()(thread);
}

static RawObject resumeGeneratorImpl(Thread* thread,
                                     const GeneratorBase& generator,
                                     const GeneratorFrame& generator_frame,
                                     const ExceptionState& exc_state) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  DCHECK((frame->returnMode() & Frame::kExitRecursiveInterpreter) != 0,
         "expected kExitRecursiveInterpreter return mode");
  generator.setRunning(Bool::trueObj());
  Object result(&scope, Interpreter::execute(thread));
  generator.setRunning(Bool::falseObj());
  thread->setCaughtExceptionState(exc_state.previous());
  exc_state.setPrevious(NoneType::object());

  // Did generator end with yield?
  if (thread->currentFrame() == frame) {
    thread->popFrameToGeneratorFrame(generator_frame);
    return *result;
  }
  generator_frame.setVirtualPC(Frame::kFinishedGeneratorPC);

  // Return now if generator ended with exception.
  if (result.isErrorException()) {
    if (thread->pendingExceptionMatches(LayoutId::kStopIteration)) {
      thread->clearPendingException();
      return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                  generator.isAsyncGenerator()
                                      ? "async generator raised StopIteration"
                                      : "coroutine raised StopIteration");
    }
    if (generator.isAsyncGenerator() &&
        thread->pendingExceptionMatches(LayoutId::kStopAsyncIteration)) {
      thread->clearPendingException();
      return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                  "async generator raised StopAsyncIteration");
    }
    return *result;
  }
  // Process generator return value.
  if (generator.isAsyncGenerator()) {
    // The Python compiler should disallow non-None return from asynchronous
    // generators.
    CHECK(result.isNoneType(), "Asynchronous generators cannot return values");
    return thread->raiseStopAsyncIteration();
  }
  return thread->raiseStopIterationWithValue(result);
}

RawObject Interpreter::resumeGenerator(Thread* thread,
                                       const GeneratorBase& generator,
                                       const Object& send_value) {
  if (generator.running() == Bool::trueObj()) {
    return thread->raiseWithFmt(LayoutId::kValueError, "%T already executing",
                                &generator);
  }
  HandleScope scope(thread);
  GeneratorFrame generator_frame(&scope, generator.generatorFrame());
  word pc = generator_frame.virtualPC();
  if (pc == Frame::kFinishedGeneratorPC) {
    if (generator.isCoroutine()) {
      return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                  "cannot reuse already awaited coroutine");
    }
    return thread->raise(generator.isAsyncGenerator()
                             ? LayoutId::kStopAsyncIteration
                             : LayoutId::kStopIteration,
                         NoneType::object());
  }
  Frame* frame = thread->pushGeneratorFrame(generator_frame);
  if (frame == nullptr) {
    return Error::exception();
  }
  if (pc != 0) {
    thread->stackPush(*send_value);
  } else if (!send_value.isNoneType()) {
    thread->popFrame();
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "can't send non-None value to a just-started %T",
        &generator);
  }

  // TODO(T38009294): Improve the compiler to avoid this exception state
  // overhead on every generator entry.
  ExceptionState exc_state(&scope, generator.exceptionState());
  exc_state.setPrevious(thread->caughtExceptionState());
  thread->setCaughtExceptionState(*exc_state);
  return resumeGeneratorImpl(thread, generator, generator_frame, exc_state);
}

RawObject Interpreter::resumeGeneratorWithRaise(Thread* thread,
                                                const GeneratorBase& generator,
                                                const Object& type,
                                                const Object& value,
                                                const Object& traceback) {
  if (generator.running() == Bool::trueObj()) {
    return thread->raiseWithFmt(LayoutId::kValueError, "%T already executing",
                                &generator);
  }
  HandleScope scope(thread);
  GeneratorFrame generator_frame(&scope, generator.generatorFrame());
  Frame* frame = thread->pushGeneratorFrame(generator_frame);
  if (frame == nullptr) {
    return Error::exception();
  }
  if (generator.isCoroutine() &&
      frame->virtualPC() == Frame::kFinishedGeneratorPC) {
    thread->popFrame();
    return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                "cannot reuse already awaited coroutine");
  }

  // TODO(T38009294): Improve the compiler to avoid this exception state
  // overhead on every generator entry.
  ExceptionState exc_state(&scope, generator.exceptionState());
  exc_state.setPrevious(thread->caughtExceptionState());
  thread->setCaughtExceptionState(*exc_state);
  thread->setPendingExceptionType(*type);
  thread->setPendingExceptionValue(*value);
  thread->setPendingExceptionTraceback(*traceback);
  DCHECK((frame->returnMode() & Frame::kExitRecursiveInterpreter) != 0,
         "expected kExitRecursiveInterpreter return mode");
  RawObject result = Interpreter::unwind(thread);
  if (!result.isErrorError()) {
    // Exception was not caught; stop generator.
    thread->setCaughtExceptionState(exc_state.previous());
    exc_state.setPrevious(NoneType::object());
    if (thread->currentFrame() != frame) {
      generator_frame.setVirtualPC(Frame::kFinishedGeneratorPC);
    }
    return Error::exception();
  }
  if (frame->virtualPC() == Frame::kFinishedGeneratorPC) {
    thread->popFrame();
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return resumeGeneratorImpl(thread, generator, generator_frame, exc_state);
}

// TODO(T69575746): Reduce the number of lookups by storing current generator
// state as it changes.
RawObject Interpreter::findYieldFrom(RawGeneratorBase gen) {
  if (gen.running() == Bool::trueObj()) return NoneType::object();
  RawGeneratorFrame gf = GeneratorFrame::cast(gen.generatorFrame());
  word pc = gf.virtualPC();
  if (pc == Frame::kFinishedGeneratorPC) return NoneType::object();
  RawFunction function = Function::cast(gf.function());
  RawMutableBytes bytecode = MutableBytes::cast(function.rewrittenBytecode());
  if (bytecode.byteAt(pc) != Bytecode::YIELD_FROM) return NoneType::object();
  return gf.valueStackTop()[0];
}

namespace {

class CppInterpreter : public Interpreter {
 public:
  ~CppInterpreter() override;
  void setupThread(Thread* thread) override;
  void* entryAsm(const Function& function) override;
  void setOpcodeCounting(bool) override;

 private:
  static RawObject interpreterLoop(Thread* thread);
};

CppInterpreter::~CppInterpreter() {}

void CppInterpreter::setupThread(Thread* thread) {
  thread->setInterpreterFunc(interpreterLoop);
}

void* CppInterpreter::entryAsm(const Function&) { return nullptr; }

void CppInterpreter::setOpcodeCounting(bool) {
  UNIMPLEMENTED("opcode counting not supported by C++ interpreter");
}

RawObject CppInterpreter::interpreterLoop(Thread* thread) {
  // Silence warnings about computed goto
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

  static const void* const dispatch_table[] = {
#define OP(name, id, handler)                                                  \
  name == EXTENDED_ARG ? &&extendedArg : &&handle##name,
      FOREACH_BYTECODE(OP)
#undef OP
  };

  Frame* frame = thread->currentFrame();
  frame->addReturnMode(Frame::kExitRecursiveInterpreter);

  Bytecode bc;
  int32_t arg;
  Continue cont;
  auto next_label = [&]() __attribute__((always_inline)) {
    Frame* current_frame = thread->currentFrame();
    word pc = current_frame->virtualPC();
    static_assert(endian::native == endian::little, "big endian unsupported");
    static_assert(kCodeUnitSize == sizeof(uint16_t), "matching type");
    arg = current_frame->bytecode().uint16At(pc);
    current_frame->setVirtualPC(pc + kCodeUnitSize);
    bc = static_cast<Bytecode>(arg & 0xFF);
    arg >>= 8;
    return dispatch_table[bc];
  };

  goto* next_label();

extendedArg:
  do {
    Frame* current_frame = thread->currentFrame();
    word pc = current_frame->virtualPC();
    static_assert(endian::native == endian::little, "big endian unsupported");
    static_assert(kCodeUnitSize == sizeof(uint16_t), "matching type");
    uint16_t bytes_at = current_frame->bytecode().uint16At(pc);
    current_frame->setVirtualPC(pc + kCodeUnitSize);
    bc = static_cast<Bytecode>(bytes_at & 0xFF);
    arg = (arg << 8) | (bytes_at >> 8);
  } while (bc == EXTENDED_ARG);
  goto* dispatch_table[bc];

#define OP(name, id, handler)                                                  \
  handle##name : cont = handler(thread, arg);                                  \
  if (LIKELY(cont == Continue::NEXT)) goto* next_label();                      \
  goto handle_return_or_unwind;
  FOREACH_BYTECODE(OP)
#undef OP

handle_return_or_unwind:
  if (cont == Continue::UNWIND) {
    RawObject result = unwind(thread);
    if (!result.isErrorError()) {
      return result;
    }
  } else if (cont == Continue::RETURN) {
    RawObject result = handleReturn(thread);
    if (!result.isErrorError()) {
      return result;
    }
  } else {
    DCHECK(cont == Continue::YIELD, "expected RETURN, UNWIND or YIELD");
    return thread->stackPop();
  }
  goto* next_label();
#pragma GCC diagnostic pop
}

}  // namespace

Interpreter* createCppInterpreter() { return new CppInterpreter; }

}  // namespace py
