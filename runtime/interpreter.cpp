#include "interpreter.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>

#include "builtins-module.h"
#include "bytes-builtins.h"
#include "complex-builtins.h"
#include "dict-builtins.h"
#include "exception-builtins.h"
#include "float-builtins.h"
#include "frame.h"
#include "generator-builtins.h"
#include "ic.h"
#include "int-builtins.h"
#include "interpreter-gen.h"
#include "intrinsic.h"
#include "list-builtins.h"
#include "module-builtins.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "set-builtins.h"
#include "str-builtins.h"
#include "thread.h"
#include "trampolines.h"
#include "tuple-builtins.h"
#include "type-builtins.h"
#include "utils.h"

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

RawObject Interpreter::prepareCallable(Thread* thread, Frame* frame,
                                       Object* callable, Object* self) {
  DCHECK(!callable->isFunction(),
         "prepareCallable should only be called on non-function types");
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  for (;;) {
    if (callable->isBoundMethod()) {
      BoundMethod method(&scope, **callable);
      *callable = method.function();
      *self = method.self();
      return Bool::trueObj();
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
    Object dunder_call(&scope, typeLookupInMroById(thread, type, ID(__call__)));
    if (!dunder_call.isErrorNotFound()) {
      if (dunder_call.isFunction()) {
        // Avoid calling function.__get__ and creating a short-lived BoundMethod
        // object. Instead, return the unpacked values directly.
        *self = **callable;
        *callable = *dunder_call;
        return Bool::trueObj();
      }
      Type call_type(&scope, runtime->typeOf(*dunder_call));
      if (typeIsNonDataDescriptor(thread, call_type)) {
        *callable =
            callDescriptorGet(thread, frame, dunder_call, *callable, type);
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
Interpreter::prepareCallableCall(Thread* thread, Frame* frame, word nargs,
                                 word callable_idx) {
  RawObject callable = frame->peek(callable_idx);
  if (callable.isFunction()) {
    return {callable, nargs};
  }

  if (callable.isBoundMethod()) {
    RawBoundMethod method = BoundMethod::cast(callable);
    RawObject method_function = method.function();
    frame->setValueAt(method_function, callable_idx);
    frame->insertValueAt(method.self(), callable_idx);
    return {method_function, nargs + 1};
  }
  return prepareCallableCallDunderCall(thread, frame, nargs, callable_idx);
}

NEVER_INLINE
Interpreter::PrepareCallableResult Interpreter::prepareCallableCallDunderCall(
    Thread* thread, Frame* frame, word nargs, word callable_idx) {
  HandleScope scope(thread);
  Object callable(&scope, frame->peek(callable_idx));
  Object self(&scope, NoneType::object());
  RawObject prepare_result = prepareCallable(thread, frame, &callable, &self);
  if (prepare_result.isErrorException()) {
    return {prepare_result, 0};
  }
  frame->setValueAt(*callable, callable_idx);
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
    frame->insertValueAt(*self, callable_idx);
    return {*callable, nargs + 1};
  }
  return {*callable, nargs};
}

RawObject Interpreter::call(Thread* thread, Frame* frame, word nargs) {
  DCHECK(!thread->hasPendingException(), "unhandled exception lingering");
  RawObject* sp = frame->valueStackTop() + nargs + 1;
  PrepareCallableResult prepare_result =
      prepareCallableCall(thread, frame, nargs, nargs);
  RawObject function = prepare_result.function;
  nargs = prepare_result.nargs;
  if (function.isErrorException()) {
    frame->setValueStackTop(sp);
    return function;
  }
  RawObject result = Function::cast(function).entry()(thread, frame, nargs);
  // Clear the stack of the function object and return.
  frame->setValueStackTop(sp);
  return result;
}

ALWAYS_INLINE RawObject Interpreter::callPreparedFunction(Thread* thread,
                                                          Frame* frame,
                                                          RawObject function,
                                                          word nargs) {
  DCHECK(!thread->hasPendingException(), "unhandled exception lingering");
  RawObject* sp = frame->valueStackTop() + nargs + 1;
  DCHECK(function == frame->peek(nargs),
         "frame->peek(nargs) is expected to be the given function");
  RawObject result = Function::cast(function).entry()(thread, frame, nargs);
  // Clear the stack of the function object and return.
  frame->setValueStackTop(sp);
  return result;
}

RawObject Interpreter::callKw(Thread* thread, Frame* frame, word nargs) {
  // Top of stack is a tuple of keyword argument names in the order they
  // appear on the stack.
  RawObject* sp = frame->valueStackTop() + nargs + 2;
  PrepareCallableResult prepare_result =
      prepareCallableCall(thread, frame, nargs, nargs + 1);
  RawObject function = prepare_result.function;
  nargs = prepare_result.nargs;
  if (function.isErrorException()) {
    frame->setValueStackTop(sp);
    return function;
  }
  RawObject result = Function::cast(function).entryKw()(thread, frame, nargs);
  frame->setValueStackTop(sp);
  return result;
}

RawObject Interpreter::callEx(Thread* thread, Frame* frame, word flags) {
  // Low bit of flags indicates whether var-keyword argument is on TOS.
  // In all cases, var-positional tuple is next, followed by the function
  // pointer.
  word callable_idx = (flags & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1;
  RawObject* post_call_sp = frame->valueStackTop() + callable_idx + 1;
  HandleScope scope(thread);
  Object callable(&scope, prepareCallableEx(thread, frame, callable_idx));
  if (callable.isErrorException()) return *callable;
  Object result(&scope,
                RawFunction::cast(*callable).entryEx()(thread, frame, flags));
  frame->setValueStackTop(post_call_sp);
  return *result;
}

RawObject Interpreter::prepareCallableEx(Thread* thread, Frame* frame,
                                         word callable_idx) {
  HandleScope scope(thread);
  Object callable(&scope, frame->peek(callable_idx));
  word args_idx = callable_idx - 1;
  Object args_obj(&scope, frame->peek(args_idx));
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
    frame->setValueAt(*args_obj, args_idx);
  }
  if (!callable.isFunction()) {
    Object self(&scope, NoneType::object());
    Object result(&scope, prepareCallable(thread, frame, &callable, &self));
    if (result.isErrorException()) return *result;
    frame->setValueAt(*callable, callable_idx);

    if (result == Bool::trueObj()) {
      // Create a new argument tuple with self as the first argument
      Tuple args(&scope, *args_obj);
      MutableTuple new_args(
          &scope, thread->runtime()->newMutableTuple(args.length() + 1));
      new_args.atPut(0, *self);
      new_args.replaceFromWith(1, *args, args.length());
      frame->setValueAt(new_args.becomeImmutable(), args_idx);
    }
  }
  return *callable;
}

static RawObject callDunderHash(Thread* thread, const Object& value) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  // TODO(T52406106): This lookup is unfortunately not inline-cached but should
  // eventually be called less and less as code moves to managed.
  Object dunder_hash(
      &scope, Interpreter::lookupMethod(thread, frame, value, ID(__hash__)));
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
  Object result(&scope,
                Interpreter::callMethod1(thread, frame, dunder_hash, value));
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
    default:
      return callDunderHash(thread, value);
  }
  return SmallInt::fromWordTruncated(result);
}

RawObject Interpreter::stringJoin(Thread* thread, RawObject* sp, word num) {
  word new_len = 0;
  for (word i = num - 1; i >= 0; i--) {
    if (!sp[i].isStr()) {
      UNIMPLEMENTED("Conversion of non-string values not supported.");
    }
    new_len += Str::cast(sp[i]).charLength();
  }

  if (new_len <= RawSmallStr::kMaxLength) {
    byte buffer[RawSmallStr::kMaxLength];
    byte* ptr = buffer;
    for (word i = num - 1; i >= 0; i--) {
      RawStr str = Str::cast(sp[i]);
      word len = str.charLength();
      str.copyTo(ptr, len);
      ptr += len;
    }
    return SmallStr::fromBytes(View<byte>(buffer, new_len));
  }

  HandleScope scope(thread);
  LargeStr result(&scope, thread->runtime()->heap()->createLargeStr(new_len));
  word offset = RawLargeStr::kDataOffset;
  for (word i = num - 1; i >= 0; i--) {
    RawStr str = Str::cast(sp[i]);
    word len = str.charLength();
    str.copyTo(reinterpret_cast<byte*>(result.address() + offset), len);
    offset += len;
  }
  return *result;
}

RawObject Interpreter::callDescriptorGet(Thread* thread, Frame* frame,
                                         const Object& descriptor,
                                         const Object& receiver,
                                         const Object& receiver_type) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type descriptor_type(&scope, runtime->typeOf(*descriptor));
  Object method(&scope,
                typeLookupInMroById(thread, descriptor_type, ID(__get__)));
  DCHECK(!method.isErrorNotFound(), "no __get__ method found");
  return callMethod3(thread, frame, method, descriptor, receiver,
                     receiver_type);
}

RawObject Interpreter::callDescriptorSet(Thread* thread, Frame* frame,
                                         const Object& descriptor,
                                         const Object& receiver,
                                         const Object& value) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type descriptor_type(&scope, runtime->typeOf(*descriptor));
  Object method(&scope,
                typeLookupInMroById(thread, descriptor_type, ID(__set__)));
  DCHECK(!method.isErrorNotFound(), "no __set__ method found");
  return callMethod3(thread, frame, method, descriptor, receiver, value);
}

RawObject Interpreter::callDescriptorDelete(Thread* thread, Frame* frame,
                                            const Object& descriptor,
                                            const Object& receiver) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type descriptor_type(&scope, runtime->typeOf(*descriptor));
  Object method(&scope,
                typeLookupInMroById(thread, descriptor_type, ID(__delete__)));
  DCHECK(!method.isErrorNotFound(), "no __delete__ method found");
  return callMethod2(thread, frame, method, descriptor, receiver);
}

RawObject Interpreter::lookupMethod(Thread* thread, Frame* /* frame */,
                                    const Object& receiver, SymbolId selector) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(*receiver));
  Object method(&scope, typeLookupInMroById(thread, type, selector));
  if (method.isFunction() || method.isErrorNotFound()) {
    // Do not create a short-lived bound method object, and propagate
    // exceptions.
    return *method;
  }
  return resolveDescriptorGet(thread, method, receiver, type);
}

RawObject Interpreter::callFunction0(Thread* thread, Frame* frame,
                                     const Object& func) {
  frame->pushValue(*func);
  return call(thread, frame, 0);
}

RawObject Interpreter::callFunction1(Thread* thread, Frame* frame,
                                     const Object& func, const Object& arg1) {
  frame->pushValue(*func);
  frame->pushValue(*arg1);
  return call(thread, frame, 1);
}

RawObject Interpreter::callFunction2(Thread* thread, Frame* frame,
                                     const Object& func, const Object& arg1,
                                     const Object& arg2) {
  frame->pushValue(*func);
  frame->pushValue(*arg1);
  frame->pushValue(*arg2);
  return call(thread, frame, 2);
}

RawObject Interpreter::callFunction3(Thread* thread, Frame* frame,
                                     const Object& func, const Object& arg1,
                                     const Object& arg2, const Object& arg3) {
  frame->pushValue(*func);
  frame->pushValue(*arg1);
  frame->pushValue(*arg2);
  frame->pushValue(*arg3);
  return call(thread, frame, 3);
}

RawObject Interpreter::callFunction4(Thread* thread, Frame* frame,
                                     const Object& func, const Object& arg1,
                                     const Object& arg2, const Object& arg3,
                                     const Object& arg4) {
  frame->pushValue(*func);
  frame->pushValue(*arg1);
  frame->pushValue(*arg2);
  frame->pushValue(*arg3);
  frame->pushValue(*arg4);
  return call(thread, frame, 4);
}

RawObject Interpreter::callFunction5(Thread* thread, Frame* frame,
                                     const Object& func, const Object& arg1,
                                     const Object& arg2, const Object& arg3,
                                     const Object& arg4, const Object& arg5) {
  frame->pushValue(*func);
  frame->pushValue(*arg1);
  frame->pushValue(*arg2);
  frame->pushValue(*arg3);
  frame->pushValue(*arg4);
  frame->pushValue(*arg5);
  return call(thread, frame, 5);
}

RawObject Interpreter::callFunction6(Thread* thread, Frame* frame,
                                     const Object& func, const Object& arg1,
                                     const Object& arg2, const Object& arg3,
                                     const Object& arg4, const Object& arg5,
                                     const Object& arg6) {
  frame->pushValue(*func);
  frame->pushValue(*arg1);
  frame->pushValue(*arg2);
  frame->pushValue(*arg3);
  frame->pushValue(*arg4);
  frame->pushValue(*arg5);
  frame->pushValue(*arg6);
  return call(thread, frame, 6);
}

RawObject Interpreter::callFunction(Thread* thread, Frame* frame,
                                    const Object& func, const Tuple& args) {
  frame->pushValue(*func);
  word length = args.length();
  for (word i = 0; i < length; i++) {
    frame->pushValue(args.at(i));
  }
  return call(thread, frame, length);
}

RawObject Interpreter::callMethod1(Thread* thread, Frame* frame,
                                   const Object& method, const Object& self) {
  word nargs = 0;
  frame->pushValue(*method);
  if (method.isFunction()) {
    frame->pushValue(*self);
    return callPreparedFunction(thread, frame, *method, nargs + 1);
  }
  return call(thread, frame, nargs);
}

RawObject Interpreter::callMethod2(Thread* thread, Frame* frame,
                                   const Object& method, const Object& self,
                                   const Object& other) {
  word nargs = 1;
  frame->pushValue(*method);
  if (method.isFunction()) {
    frame->pushValue(*self);
    frame->pushValue(*other);
    return callPreparedFunction(thread, frame, *method, nargs + 1);
  }
  frame->pushValue(*other);
  return call(thread, frame, nargs);
}

RawObject Interpreter::callMethod3(Thread* thread, Frame* frame,
                                   const Object& method, const Object& self,
                                   const Object& arg1, const Object& arg2) {
  word nargs = 2;
  frame->pushValue(*method);
  if (method.isFunction()) {
    frame->pushValue(*self);
    frame->pushValue(*arg1);
    frame->pushValue(*arg2);
    return callPreparedFunction(thread, frame, *method, nargs + 1);
  }
  frame->pushValue(*arg1);
  frame->pushValue(*arg2);
  return call(thread, frame, nargs);
}

RawObject Interpreter::callMethod4(Thread* thread, Frame* frame,
                                   const Object& method, const Object& self,
                                   const Object& arg1, const Object& arg2,
                                   const Object& arg3) {
  word nargs = 3;
  frame->pushValue(*method);
  if (method.isFunction()) {
    frame->pushValue(*self);
    frame->pushValue(*arg1);
    frame->pushValue(*arg2);
    frame->pushValue(*arg3);
    return callPreparedFunction(thread, frame, *method, nargs + 1);
  }
  frame->pushValue(*arg1);
  frame->pushValue(*arg2);
  frame->pushValue(*arg3);
  return call(thread, frame, nargs);
}

HANDLER_INLINE
Continue Interpreter::tailcallMethod1(Thread* thread, RawObject method,
                                      RawObject self) {
  word nargs = 0;
  Frame* frame = thread->currentFrame();
  frame->pushValue(method);
  if (method.isFunction()) {
    frame->pushValue(self);
    nargs++;
    return tailcallPreparedFunction(thread, frame, method, nargs);
  }
  return tailcallFunction(thread, nargs);
}

HANDLER_INLINE
Continue Interpreter::tailcallMethod2(Thread* thread, RawObject method,
                                      RawObject self, RawObject arg1) {
  word nargs = 1;
  Frame* frame = thread->currentFrame();
  frame->pushValue(method);
  if (method.isFunction()) {
    frame->pushValue(self);
    nargs++;
    return tailcallPreparedFunction(thread, frame, method, nargs);
  }
  frame->pushValue(arg1);
  return tailcallFunction(thread, nargs);
}

HANDLER_INLINE Continue Interpreter::tailcallFunction(Thread* thread,
                                                      word arg) {
  return handleCall(thread, arg, arg, 0, preparePositionalCall,
                    &Function::entry);
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
  Frame* frame = thread->currentFrame();
  Object receiver(&scope, frame->topValue());
  RawObject result = unaryOperation(thread, receiver, selector);
  if (result.isErrorException()) return Continue::UNWIND;
  frame->setTopValue(result);
  return Continue::NEXT;
}

static RawObject binaryOperationLookupReflected(Thread* thread,
                                                Interpreter::BinaryOp op,
                                                const Object& left,
                                                const Object& right) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  SymbolId swapped_selector = runtime->swappedBinaryOperationSelector(op);
  Type right_type(&scope, runtime->typeOf(*right));
  Object right_reversed_method(
      &scope, typeLookupInMroById(thread, right_type, swapped_selector));
  if (right_reversed_method.isErrorNotFound()) return *right_reversed_method;

  // Python doesn't bother calling the reverse method when the slot on left and
  // right points to the same method. We compare the reverse methods to get
  // close to this behavior.
  Type left_type(&scope, runtime->typeOf(*left));
  Object left_reversed_method(
      &scope, typeLookupInMroById(thread, left_type, swapped_selector));
  if (left_reversed_method == right_reversed_method) {
    return Error::notFound();
  }

  return *right_reversed_method;
}

static RawObject executeAndCacheBinaryOp(
    Thread* thread, Frame* frame, const Object& method, BinaryOpFlags flags,
    const Object& left, const Object& right, Object* method_out,
    BinaryOpFlags* flags_out) {
  if (method.isErrorNotFound()) {
    return NotImplementedType::object();
  }

  if (method_out != nullptr) {
    DCHECK(method.isFunction(), "must be a plain function");
    *method_out = *method;
    *flags_out = flags;
    return Interpreter::binaryOperationWithMethod(thread, frame, *method, flags,
                                                  *left, *right);
  }
  if (flags & kBinaryOpReflected) {
    return Interpreter::callMethod2(thread, frame, method, right, left);
  }
  return Interpreter::callMethod2(thread, frame, method, left, right);
}

RawObject Interpreter::binaryOperationSetMethod(Thread* thread, Frame* frame,
                                                BinaryOp op, const Object& left,
                                                const Object& right,
                                                Object* method_out,
                                                BinaryOpFlags* flags_out) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  SymbolId selector = runtime->binaryOperationSelector(op);
  Type left_type(&scope, runtime->typeOf(*left));
  Type right_type(&scope, runtime->typeOf(*right));
  Object left_method(&scope, typeLookupInMroById(thread, left_type, selector));

  // Figure out whether we want to run the normal or the reverse operation
  // first and set `flags` accordingly.
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags = kBinaryOpNone;
  if (left_type != right_type && (left_method.isErrorNotFound() ||
                                  typeIsSubclass(right_type, left_type))) {
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

  Object result(
      &scope, executeAndCacheBinaryOp(thread, frame, method, flags, left, right,
                                      method_out, flags_out));
  if (!result.isNotImplementedType()) return *result;

  // Invoke a 2nd method (normal or reverse depends on what we did the first
  // time) or report an error.
  return binaryOperationRetry(thread, frame, op, flags, left, right);
}

RawObject Interpreter::binaryOperation(Thread* thread, Frame* frame,
                                       BinaryOp op, const Object& left,
                                       const Object& right) {
  return binaryOperationSetMethod(thread, frame, op, left, right, nullptr,
                                  nullptr);
}

HANDLER_INLINE Continue Interpreter::doBinaryOperation(BinaryOp op,
                                                       Thread* thread) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object other(&scope, frame->popValue());
  Object self(&scope, frame->popValue());
  RawObject result = binaryOperation(thread, frame, op, self, other);
  if (result.isErrorException()) return Continue::UNWIND;
  frame->pushValue(result);
  return Continue::NEXT;
}

RawObject Interpreter::inplaceOperationSetMethod(
    Thread* thread, Frame* frame, BinaryOp op, const Object& left,
    const Object& right, Object* method_out, BinaryOpFlags* flags_out) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  SymbolId selector = runtime->inplaceOperationSelector(op);
  Type left_type(&scope, runtime->typeOf(*left));
  Object method(&scope, typeLookupInMroById(thread, left_type, selector));
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
    Object result(&scope, callMethod2(thread, frame, method, left, right));
    if (result != NotImplementedType::object()) {
      return *result;
    }
  }
  return binaryOperationSetMethod(thread, frame, op, left, right, method_out,
                                  flags_out);
}

RawObject Interpreter::inplaceOperation(Thread* thread, Frame* frame,
                                        BinaryOp op, const Object& left,
                                        const Object& right) {
  return inplaceOperationSetMethod(thread, frame, op, left, right, nullptr,
                                   nullptr);
}

HANDLER_INLINE Continue Interpreter::doInplaceOperation(BinaryOp op,
                                                        Thread* thread) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object right(&scope, frame->popValue());
  Object left(&scope, frame->popValue());
  RawObject result = inplaceOperation(thread, frame, op, left, right);
  if (result.isErrorException()) return Continue::UNWIND;
  frame->pushValue(result);
  return Continue::NEXT;
}

RawObject Interpreter::compareOperationSetMethod(
    Thread* thread, Frame* frame, CompareOp op, const Object& left,
    const Object& right, Object* method_out, BinaryOpFlags* flags_out) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  SymbolId selector = runtime->comparisonSelector(op);
  Type left_type(&scope, runtime->typeOf(*left));
  Type right_type(&scope, runtime->typeOf(*right));
  Object left_method(&scope, typeLookupInMroById(thread, left_type, selector));

  // Figure out whether we want to run the normal or the reverse operation
  // first and set `flags` accordingly.
  Object method(&scope, *left_method);
  BinaryOpFlags flags = kBinaryOpNone;
  if (left_type != right_type && (left_method.isErrorNotFound() ||
                                  typeIsSubclass(right_type, left_type))) {
    SymbolId reverse_selector = runtime->swappedComparisonSelector(op);
    method = typeLookupInMroById(thread, right_type, reverse_selector);
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

  Object result(
      &scope, executeAndCacheBinaryOp(thread, frame, method, flags, left, right,
                                      method_out, flags_out));
  if (!result.isNotImplementedType()) return *result;

  return compareOperationRetry(thread, frame, op, flags, left, right);
}

RawObject Interpreter::compareOperationRetry(Thread* thread, Frame* frame,
                                             CompareOp op, BinaryOpFlags flags,
                                             const Object& left,
                                             const Object& right) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  if (flags & kBinaryOpNotImplementedRetry) {
    // If we tried reflected first, try normal now.
    if (flags & kBinaryOpReflected) {
      SymbolId selector = runtime->comparisonSelector(op);
      Object method(&scope, lookupMethod(thread, frame, left, selector));
      if (method.isError()) {
        if (method.isErrorException()) return *method;
        DCHECK(method.isErrorNotFound(), "expected not found");
      } else {
        Object result(&scope, callMethod2(thread, frame, method, left, right));
        if (!result.isNotImplementedType()) return *result;
      }
    } else {
      // If we tried normal first, try to find a reflected method and call it.
      SymbolId selector = runtime->swappedComparisonSelector(op);
      Object method(&scope, lookupMethod(thread, frame, right, selector));
      if (!method.isErrorNotFound()) {
        if (!method.isFunction()) {
          Type right_type(&scope, runtime->typeOf(*right));
          method = resolveDescriptorGet(thread, method, right, right_type);
          if (method.isErrorException()) return *method;
        }
        Object result(&scope, callMethod2(thread, frame, method, right, left));
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
    Thread* thread, Frame* frame, RawObject method, BinaryOpFlags flags,
    RawObject left, RawObject right) {
  DCHECK(method.isFunction(), "function is expected");
  frame->pushValue(method);
  if (flags & kBinaryOpReflected) {
    frame->pushValue(right);
    frame->pushValue(left);
  } else {
    frame->pushValue(left);
    frame->pushValue(right);
  }
  RawObject* sp = frame->valueStackTop() + /*nargs=*/2 + /*callable=*/1;
  RawObject result = Function::cast(method).entry()(thread, frame, 2);
  frame->setValueStackTop(sp);
  return result;
}

RawObject Interpreter::binaryOperationRetry(Thread* thread, Frame* frame,
                                            BinaryOp op, BinaryOpFlags flags,
                                            const Object& left,
                                            const Object& right) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  if (flags & kBinaryOpNotImplementedRetry) {
    // If we tried reflected first, try normal now.
    if (flags & kBinaryOpReflected) {
      SymbolId selector = runtime->binaryOperationSelector(op);
      Object method(&scope, lookupMethod(thread, frame, left, selector));
      if (method.isError()) {
        if (method.isErrorException()) return *method;
        DCHECK(method.isErrorNotFound(), "expected not found");
      } else {
        Object result(&scope, callMethod2(thread, frame, method, left, right));
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
        Object result(&scope, callMethod2(thread, frame, method, right, left));
        if (!result.isNotImplementedType()) return *result;
      }
    }
  }

  SymbolId op_symbol = runtime->binaryOperationSelector(op);
  return thread->raiseUnsupportedBinaryOperation(left, right, op_symbol);
}

RawObject Interpreter::compareOperation(Thread* thread, Frame* frame,
                                        CompareOp op, const Object& left,
                                        const Object& right) {
  return compareOperationSetMethod(thread, frame, op, left, right, nullptr,
                                   nullptr);
}

RawObject Interpreter::createIterator(Thread* thread, Frame* frame,
                                      const Object& iterable) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object dunder_iter(&scope,
                     lookupMethod(thread, frame, iterable, ID(__iter__)));
  if (dunder_iter.isError() || dunder_iter.isNoneType()) {
    if (dunder_iter.isErrorNotFound() &&
        runtime->isSequence(thread, iterable)) {
      return runtime->newSeqIterator(iterable);
    }
    thread->clearPendingException();
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'%T' object is not iterable", &iterable);
  }
  Object iterator(&scope, callMethod1(thread, frame, dunder_iter, iterable));
  if (iterator.isErrorException()) return *iterator;
  if (!runtime->isIterator(thread, iterator)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "iter() returned non-iterator of type '%T'",
                                &iterator);
  }
  return *iterator;
}

RawObject Interpreter::sequenceIterSearch(Thread* thread, Frame* frame,
                                          const Object& value,
                                          const Object& container) {
  HandleScope scope(thread);
  Object iter(&scope, createIterator(thread, frame, container));
  if (iter.isErrorException()) {
    return *iter;
  }
  Object dunder_next(&scope, lookupMethod(thread, frame, iter, ID(__next__)));
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
    current = callMethod1(thread, frame, dunder_next, iter);
    if (current.isErrorException()) {
      if (thread->hasPendingStopIteration()) {
        thread->clearPendingStopIteration();
        break;
      }
      return *current;
    }
    compare_result = compareOperation(thread, frame, EQ, value, current);
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

RawObject Interpreter::sequenceContains(Thread* thread, Frame* frame,
                                        const Object& value,
                                        const Object& container) {
  HandleScope scope(thread);
  Object method(&scope,
                lookupMethod(thread, frame, container, ID(__contains__)));
  if (!method.isError()) {
    Object result(&scope, callMethod2(thread, frame, method, container, value));
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
  return sequenceIterSearch(thread, frame, value, container);
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
  HandleScope scope(thread);
  Object value(&scope, value_obj);
  Object result(&scope, thread->invokeMethod1(value, ID(__bool__)));
  if (!result.isError()) {
    if (result.isBool()) return *result;
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__bool__ should return bool");
  }
  if (result.isErrorException()) {
    return *result;
  }
  DCHECK(result.isErrorNotFound(), "expected error not found");

  result = thread->invokeMethod1(value, ID(__len__));
  if (!result.isError()) {
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
  if (result.isErrorException()) {
    return *result;
  }
  DCHECK(result.isErrorNotFound(), "expected error not found");
  return Bool::trueObj();
}

HANDLER_INLINE void Interpreter::raise(Thread* thread, RawObject exc_obj,
                                       RawObject cause_obj) {
  Frame* frame = thread->currentFrame();
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
    value = Interpreter::callFunction0(thread, frame, type);
    if (value.isErrorException()) return;
    if (!runtime->isInstanceOfBaseException(*value)) {
      // TODO(bsimmers): Include relevant types here once we have better string
      // formatting.
      thread->raiseWithFmt(
          LayoutId::kTypeError,
          "calling exception type did not return an instance of BaseException");
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
  // attach the cause the the primary exception.
  if (!cause.isErrorNotFound()) {  // TODO(T25860930) use Unbound rather than
                                   // Error.
    if (runtime->isInstanceOfType(*cause) &&
        Type(&scope, *cause).isBaseExceptionSubclass()) {
      cause = Interpreter::callFunction0(thread, frame, cause);
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
                                                     Frame* frame,
                                                     TryBlock block) {
  // Drop all dead values except for the 3 that are popped into the caught
  // exception state.
  DCHECK(block.kind() == TryBlock::kExceptHandler, "Invalid TryBlock Kind");
  frame->dropValues(frame->valueStackSize() - block.level() - 3);
  thread->setCaughtExceptionType(frame->popValue());
  thread->setCaughtExceptionValue(frame->popValue());
  thread->setCaughtExceptionTraceback(frame->popValue());
}

bool Interpreter::popBlock(Thread* thread, TryBlock::Why why, RawObject value) {
  Frame* frame = thread->currentFrame();
  DCHECK(frame->blockStack()->depth() > 0,
         "Tried to pop from empty blockstack");
  DCHECK(why != TryBlock::Why::kException, "Unsupported Why");

  TryBlock block = frame->blockStack()->peek();
  if (block.kind() == TryBlock::kLoop && why == TryBlock::Why::kContinue) {
    frame->setVirtualPC(SmallInt::cast(value).value());
    return true;
  }

  frame->blockStack()->pop();
  if (block.kind() == TryBlock::kExceptHandler) {
    unwindExceptHandler(thread, frame, block);
    return false;
  }
  frame->dropValues(frame->valueStackSize() - block.level());

  if (block.kind() == TryBlock::kLoop) {
    if (why == TryBlock::Why::kBreak) {
      frame->setVirtualPC(block.handler());
      return true;
    }
    return false;
  }

  if (block.kind() == TryBlock::kExcept) {
    // Exception unwinding is handled in Interpreter::unwind() and doesn't come
    // through here. Ignore the Except block.
    return false;
  }

  DCHECK(block.kind() == TryBlock::kFinally, "Unexpected TryBlock kind");
  if (why == TryBlock::Why::kReturn || why == TryBlock::Why::kContinue) {
    frame->pushValue(value);
  }
  frame->pushValue(SmallInt::fromWord(static_cast<word>(why)));
  frame->setVirtualPC(block.handler());
  return true;
}

RawObject Interpreter::handleReturn(Thread* thread, Frame* entry_frame) {
  Frame* frame = thread->currentFrame();
  RawObject retval = frame->popValue();
  while (frame->blockStack()->depth() > 0) {
    if (popBlock(thread, TryBlock::Why::kReturn, retval)) {
      return Error::error();  // continue interpreter loop.
    }
  }

  // Check whether we should exit the interpreter loop.
  if (frame == entry_frame) {
    thread->popFrame();
    return retval;
  }

  frame = thread->popFrame();
  frame->pushValue(retval);
  return Error::error();  // continue interpreter loop.
}

HANDLER_INLINE void Interpreter::handleLoopExit(Thread* thread,
                                                TryBlock::Why why,
                                                RawObject retval) {
  for (;;) {
    if (popBlock(thread, why, retval)) return;
  }
}

// TODO(T39919701): This is a temporary, off-by-default (in Release builds) hack
// until we have proper traceback support. It has no mapping to actual
// tracebacks as understood by Python code; see its usage in
// Interpreter::unwind() below for details.
#ifdef NDEBUG
const bool kRecordTracebacks = std::getenv("PYRO_RECORD_TRACEBACKS") != nullptr;
#else
const bool kRecordTracebacks = true;
#endif

bool Interpreter::unwind(Thread* thread, Frame* entry_frame) {
  DCHECK(thread->hasPendingException(),
         "unwind() called without a pending exception");
  HandleScope scope(thread);

  if (UNLIKELY(kRecordTracebacks) &&
      thread->pendingExceptionTraceback().isNoneType()) {
    std::ostringstream tb;
    Utils::printTraceback(&tb);
    thread->setPendingExceptionTraceback(
        thread->runtime()->newStrFromCStr(tb.str().c_str()));
  }

  Frame* frame = thread->currentFrame();
  for (;;) {
    BlockStack* stack = frame->blockStack();

    while (stack->depth() > 0) {
      TryBlock block = stack->pop();
      if (block.kind() == TryBlock::kExceptHandler) {
        unwindExceptHandler(thread, frame, block);
        continue;
      }
      frame->dropValues(frame->valueStackSize() - block.level());

      if (block.kind() == TryBlock::kLoop) continue;
      DCHECK(block.kind() == TryBlock::kExcept ||
                 block.kind() == TryBlock::kFinally,
             "Unexpected TryBlock::Kind");

      // Push a handler block and save the current caught exception, if any.
      stack->push(
          TryBlock{TryBlock::kExceptHandler, 0, frame->valueStackSize()});
      frame->pushValue(thread->caughtExceptionTraceback());
      frame->pushValue(thread->caughtExceptionValue());
      frame->pushValue(thread->caughtExceptionType());

      // Load and normalize the pending exception.
      Object type(&scope, thread->pendingExceptionType());
      Object value(&scope, thread->pendingExceptionValue());
      Object traceback(&scope, thread->pendingExceptionTraceback());
      thread->clearPendingException();
      normalizeException(thread, &type, &value, &traceback);
      BaseException(&scope, *value).setTraceback(*traceback);

      // Promote the normalized exception to caught, push it for the bytecode
      // handler, and jump to the handler.
      thread->setCaughtExceptionType(*type);
      thread->setCaughtExceptionValue(*value);
      thread->setCaughtExceptionTraceback(*traceback);
      frame->pushValue(*traceback);
      frame->pushValue(*value);
      frame->pushValue(*type);
      frame->setVirtualPC(block.handler());
      return false;  // continue interpreter loop.
    }

    if (frame == entry_frame) break;
    frame = thread->popFrame();
  }

  thread->popFrame();
  return true;
}

static Bytecode currentBytecode(Thread* thread) {
  Frame* frame = thread->currentFrame();
  word pc = frame->virtualPC() - kCodeUnitSize;
  return static_cast<Bytecode>(frame->bytecode().byteAt(pc));
}

HANDLER_INLINE Continue Interpreter::doInvalidBytecode(Thread* thread, word) {
  Bytecode bc = currentBytecode(thread);
  UNREACHABLE("bytecode '%s'", kBytecodeNames[bc]);
}

HANDLER_INLINE Continue Interpreter::doPopTop(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  frame->popValue();
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doRotTwo(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  RawObject* sp = frame->valueStackTop();
  RawObject top = *sp;
  *sp = *(sp + 1);
  *(sp + 1) = top;
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doRotThree(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  RawObject* sp = frame->valueStackTop();
  RawObject top = *sp;
  *sp = *(sp + 1);
  *(sp + 1) = *(sp + 2);
  *(sp + 2) = top;
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doDupTop(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  frame->pushValue(frame->topValue());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doDupTopTwo(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  RawObject first = frame->topValue();
  RawObject second = frame->peek(1);
  frame->pushValue(second);
  frame->pushValue(first);
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
  Frame* frame = thread->currentFrame();
  RawObject value = frame->topValue();
  if (!value.isBool()) {
    value = isTrue(thread, value);
    if (value.isErrorException()) return Continue::UNWIND;
  }
  frame->setTopValue(RawBool::negate(value));
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
  Object container(&scope, frame->peek(1));
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(*container));
  Object getitem(&scope, typeLookupInMroById(thread, type, ID(__getitem__)));
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
    frame->setValueAt(*getitem, 1);
    return tailcallFunction(thread, 1);
  }
  if (index >= 0) {
    // TODO(T55274956): Make this into a separate function to be shared.
    Tuple caches(&scope, frame->caches());
    Str get_item_name(&scope, runtime->symbols()->at(ID(__getitem__)));
    Function dependent(&scope, frame->function());
    ICState next_cache_state =
        icUpdateAttr(thread, caches, index, container.layoutId(), getitem,
                     get_item_name, dependent);
    word pc = frame->currentPC();
    RawMutableBytes bytecode = frame->bytecode();
    bytecode.byteAtPut(pc, next_cache_state == ICState::kMonomorphic
                               ? BINARY_SUBSCR_MONOMORPHIC
                               : BINARY_SUBSCR_POLYMORPHIC);
  }
  frame->setValueAt(*getitem, 1);
  frame->insertValueAt(*container, 1);
  return tailcallPreparedFunction(thread, frame, *getitem, 2);
}

HANDLER_INLINE Continue Interpreter::doBinarySubscr(Thread* thread, word) {
  return binarySubscrUpdateCache(thread, -1);
}

HANDLER_INLINE Continue Interpreter::doBinarySubscrMonomorphic(Thread* thread,
                                                               word arg) {
  Frame* frame = thread->currentFrame();
  RawTuple caches = frame->caches();
  LayoutId receiver_layout_id = frame->peek(1).layoutId();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver_layout_id, &is_found);
  if (!is_found) {
    return binarySubscrUpdateCache(thread, arg);
  }
  frame->insertValueAt(cached, 2);
  return tailcallPreparedFunction(thread, frame, cached, 2);
}

HANDLER_INLINE Continue Interpreter::doBinarySubscrPolymorphic(Thread* thread,
                                                               word arg) {
  Frame* frame = thread->currentFrame();
  LayoutId container_layout_id = frame->peek(1).layoutId();
  bool is_found;
  RawObject cached =
      icLookupPolymorphic(frame->caches(), arg, container_layout_id, &is_found);
  if (!is_found) {
    return binarySubscrUpdateCache(thread, arg);
  }
  frame->insertValueAt(cached, 2);
  return tailcallPreparedFunction(thread, frame, cached, 2);
}

HANDLER_INLINE Continue Interpreter::doBinarySubscrAnamorphic(Thread* thread,
                                                              word arg) {
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
  Frame* frame = thread->currentFrame();
  Object obj(&scope, frame->popValue());
  Object method(&scope, lookupMethod(thread, frame, obj, ID(__aiter__)));
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
  Frame* frame = thread->currentFrame();
  Object obj(&scope, frame->popValue());
  Object anext(&scope, lookupMethod(thread, frame, obj, ID(__anext__)));
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
  Object awaitable(&scope, callMethod1(thread, frame, anext, obj));
  if (awaitable.isErrorException()) return Continue::UNWIND;

  // TODO(T33628943): Check if `awaitable` is a native or generator-based
  // coroutine and if it is, no need to call __await__
  Object await(&scope, lookupMethod(thread, frame, obj, ID(__await__)));
  if (await.isError()) {
    if (await.isErrorException()) {
      thread->clearPendingException();
    } else {
      DCHECK(await.isErrorNotFound(),
             "expected Error::exception() or Error::notFound()");
    }
    thread->raiseWithFmt(
        LayoutId::kTypeError,
        "'async for' received an invalid object from __anext__");
    return Continue::UNWIND;
  }
  return tailcallMethod1(thread, *await, *obj);
}

HANDLER_INLINE Continue Interpreter::doBeforeAsyncWith(Thread* thread, word) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object manager(&scope, frame->popValue());

  // resolve __aexit__ an push it
  Runtime* runtime = thread->runtime();
  Object exit(&scope, runtime->attributeAtById(thread, manager, ID(__aexit__)));
  if (exit.isErrorException()) {
    UNIMPLEMENTED("throw TypeError");
  }
  frame->pushValue(*exit);

  // resolve __aenter__ call it and push the return value
  Object enter(&scope, lookupMethod(thread, frame, manager, ID(__aenter__)));
  if (enter.isError()) {
    UNIMPLEMENTED("throw TypeError");
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
  Frame* frame = thread->currentFrame();
  Object key(&scope, frame->popValue());
  Object container(&scope, frame->popValue());
  Object setitem(&scope,
                 lookupMethod(thread, frame, container, ID(__setitem__)));
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
  Object value(&scope, frame->popValue());
  if (callMethod3(thread, frame, setitem, container, key, value)
          .isErrorException()) {
    return Continue::UNWIND;
  }
  return Continue::NEXT;
}

NEVER_INLINE Continue Interpreter::storeSubscrUpdateCache(Thread* thread,
                                                          word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object key(&scope, frame->popValue());
  Object container(&scope, frame->popValue());
  Object setitem(&scope,
                 lookupMethod(thread, frame, container, ID(__setitem__)));
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
    Tuple caches(&scope, frame->caches());
    Str set_item_name(&scope,
                      thread->runtime()->symbols()->at(ID(__setitem__)));
    Function dependent(&scope, frame->function());
    ICState next_cache_state =
        icUpdateAttr(thread, caches, arg, container.layoutId(), setitem,
                     set_item_name, dependent);
    word pc = frame->currentPC();
    RawMutableBytes bytecode = frame->bytecode();
    bytecode.byteAtPut(pc, next_cache_state == ICState::kMonomorphic
                               ? STORE_SUBSCR_MONOMORPHIC
                               : STORE_SUBSCR_POLYMORPHIC);
  }
  Object value(&scope, frame->popValue());
  if (callMethod3(thread, frame, setitem, container, key, value)
          .isErrorException()) {
    return Continue::UNWIND;
  }
  return Continue::NEXT;
}

ALWAYS_INLINE Continue Interpreter::storeSubscr(Thread* thread,
                                                RawObject set_item_method) {
  DCHECK(set_item_method.isFunction(), "cached should be a function");
  Frame* frame = thread->currentFrame();
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
  RawObject value_raw = frame->peek(2);
  frame->setValueAt(set_item_method, 2);
  frame->pushValue(value_raw);

  word nargs = 3;
  RawObject* sp = frame->valueStackTop() + nargs + 1;
  RawObject result =
      Function::cast(set_item_method).entry()(thread, frame, nargs);
  // Clear the stack of the function object and return.
  frame->setValueStackTop(sp);
  if (result.isErrorException()) {
    return Continue::UNWIND;
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreSubscrMonomorphic(Thread* thread,
                                                              word arg) {
  Frame* frame = thread->currentFrame();
  RawTuple caches = frame->caches();
  LayoutId container_layout_id = frame->peek(1).layoutId();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, container_layout_id, &is_found);
  if (!is_found) {
    return storeSubscrUpdateCache(thread, arg);
  }
  return storeSubscr(thread, cached);
}

HANDLER_INLINE Continue Interpreter::doStoreSubscrPolymorphic(Thread* thread,
                                                              word arg) {
  Frame* frame = thread->currentFrame();
  RawObject container_raw = frame->peek(1);
  LayoutId container_layout_id = container_raw.layoutId();
  bool is_found;
  RawObject cached =
      icLookupPolymorphic(frame->caches(), arg, container_layout_id, &is_found);
  if (!is_found) {
    return storeSubscrUpdateCache(thread, arg);
  }
  return storeSubscr(thread, cached);
}

HANDLER_INLINE Continue Interpreter::doStoreSubscrAnamorphic(Thread* thread,
                                                             word arg) {
  return storeSubscrUpdateCache(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doDeleteSubscr(Thread* thread, word) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object key(&scope, frame->popValue());
  Object container(&scope, frame->popValue());
  Object delitem(&scope,
                 lookupMethod(thread, frame, container, ID(__delitem__)));
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
  if (callMethod2(thread, frame, delitem, container, key).isErrorException()) {
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
  Frame* frame = thread->currentFrame();
  Object iterable(&scope, frame->popValue());
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
    case LayoutId::kByteArray: {
      ByteArray byte_array(&scope, *iterable);
      iterator = runtime->newByteArrayIterator(thread, byte_array);
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
    frame->pushValue(*iterator);
    return Continue::NEXT;
  }
  // TODO(T44729606): Add caching, and turn into a simpler call for builtin
  // types with known iterator creating functions
  iterator = createIterator(thread, frame, iterable);
  if (iterator.isErrorException()) return Continue::UNWIND;
  frame->pushValue(*iterator);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doGetYieldFromIter(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object iterable(&scope, frame->topValue());

  if (iterable.isGenerator()) return Continue::NEXT;

  if (iterable.isCoroutine()) {
    Function function(&scope, frame->function());
    if (function.isCoroutine() || function.isIterableCoroutine()) {
      thread->raiseWithFmt(
          LayoutId::kTypeError,
          "cannot 'yield from' a coroutine object in a non-coroutine "
          "generator");
      return Continue::UNWIND;
    }
    return Continue::NEXT;
  }

  frame->dropValues(1);
  // TODO(T44729661): Add caching, and turn into a simpler call for builtin
  // types with known iterator creating functions
  Object iterator(&scope, createIterator(thread, frame, iterable));
  if (iterator.isErrorException()) return Continue::UNWIND;
  frame->pushValue(*iterator);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doPrintExpr(Thread* thread, word) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object value(&scope, frame->popValue());
  ValueCell value_cell(&scope, thread->runtime()->displayHook());
  if (value_cell.isUnbound()) {
    UNIMPLEMENTED("RuntimeError: lost sys.displayhook");
  }
  // TODO(T55021263): Replace with non-recursive call
  Object display_hook(&scope, value_cell.value());
  return callMethod1(thread, frame, display_hook, value).isErrorException()
             ? Continue::UNWIND
             : Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadBuildClass(Thread* thread, word) {
  RawValueCell value_cell = ValueCell::cast(thread->runtime()->buildClass());
  Frame* frame = thread->currentFrame();
  frame->pushValue(value_cell.value());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doYieldFrom(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);

  Object value(&scope, frame->popValue());
  Object iterator(&scope, frame->topValue());
  Object result(&scope, NoneType::object());
  if (value.isNoneType()) {
    Object next_method(&scope,
                       lookupMethod(thread, frame, iterator, ID(__next__)));
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
    result = callMethod1(thread, frame, next_method, iterator);
  } else {
    Object send_method(&scope, lookupMethod(thread, frame, iterator, ID(send)));
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
    result = callMethod2(thread, frame, send_method, iterator, value);
  }
  if (result.isErrorException()) {
    if (!thread->hasPendingStopIteration()) return Continue::UNWIND;

    frame->setTopValue(thread->pendingStopIterationValue());
    thread->clearPendingException();
    return Continue::NEXT;
  }

  // Decrement PC: We want this to re-execute until the subiterator is
  // exhausted.
  frame->setVirtualPC(frame->virtualPC() - kCodeUnitSize);
  frame->pushValue(*result);
  return Continue::YIELD;
}

HANDLER_INLINE Continue Interpreter::doGetAwaitable(Thread* thread, word) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object obj(&scope, frame->popValue());

  // TODO(T33628943): Check if `obj` is a native or generator-based
  // coroutine and if it is, no need to call __await__
  Object await(&scope, lookupMethod(thread, frame, obj, ID(__await__)));
  if (await.isError()) {
    if (await.isErrorException()) {
      thread->clearPendingException();
    } else {
      DCHECK(await.isErrorNotFound(),
             "expected Error::exception() or Error::notFound()");
    }
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "object can't be used in 'await' expression");
    return Continue::UNWIND;
  }
  return tailcallMethod1(thread, *await, *obj);
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

HANDLER_INLINE Continue Interpreter::doBreakLoop(Thread* thread, word) {
  handleLoopExit(thread, TryBlock::Why::kBreak, NoneType::object());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doWithCleanupStart(Thread* thread, word) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object exc(&scope, frame->popValue());
  Object value(&scope, NoneType::object());
  Object traceback(&scope, NoneType::object());
  Object exit(&scope, NoneType::object());

  // The stack currently contains a sequence of values understood by
  // END_FINALLY, followed by __exit__ from the context manager. We need to
  // determine the location of __exit__ and remove it from the stack, shifting
  // everything above it down to compensate.
  if (exc.isNoneType()) {
    // The with block exited normally. __exit__ is just below the None.
    exit = frame->topValue();
    frame->setTopValue(*exc);
  } else if (exc.isSmallInt()) {
    // The with block exited for a return, continue, or break. __exit__ will be
    // below 'why' and an optional return value (depending on 'why').
    auto why = static_cast<TryBlock::Why>(SmallInt::cast(*exc).value());
    if (why == TryBlock::Why::kReturn || why == TryBlock::Why::kContinue) {
      exit = frame->peek(1);
      frame->setValueAt(frame->peek(0), 1);
    } else {
      exit = frame->topValue();
    }
    frame->setTopValue(*exc);
    exc = NoneType::object();
  } else {
    // The stack contains the caught exception, the previous exception state,
    // then __exit__. Grab __exit__ then shift everything else down.
    exit = frame->peek(5);
    for (word i = 5; i > 0; i--) {
      frame->setValueAt(frame->peek(i - 1), i);
    }

    // Put exc at the top of the stack and grab value/traceback from below it.
    frame->setTopValue(*exc);
    value = frame->peek(1);
    traceback = frame->peek(2);

    // We popped __exit__ out from under the depth recorded by the top
    // ExceptHandler block, so adjust it.
    TryBlock block = frame->blockStack()->pop();
    DCHECK(block.kind() == TryBlock::kExceptHandler,
           "Unexpected TryBlock Kind");
    frame->blockStack()->push(
        TryBlock(block.kind(), block.handler(), block.level() - 1));
  }

  // Push exc, to be consumed by WITH_CLEANUP_FINISH.
  frame->pushValue(*exc);

  // Call exit(exc, value, traceback), leaving the result on the stack for
  // WITH_CLEANUP_FINISH.
  frame->pushValue(*exit);
  frame->pushValue(*exc);
  frame->pushValue(*value);
  frame->pushValue(*traceback);
  return tailcallFunction(thread, 3);
}

HANDLER_INLINE Continue Interpreter::doWithCleanupFinish(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object result(&scope, frame->popValue());
  Object exc(&scope, frame->popValue());
  if (!exc.isNoneType()) {
    Object is_true(&scope, isTrue(thread, *result));
    if (is_true.isErrorException()) return Continue::UNWIND;
    if (*is_true == Bool::trueObj()) {
      frame->pushValue(
          SmallInt::fromWord(static_cast<int>(TryBlock::Why::kSilenced)));
    }
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
    if (moduleAt(thread, module, dunder_annotations).isErrorNotFound()) {
      Object annotations(&scope, runtime->newDict());
      moduleAtPut(thread, module, dunder_annotations, annotations);
    }
  } else {
    // Class body
    Object implicit_globals(&scope, frame->implicitGlobals());
    if (implicit_globals.isDict()) {
      Dict implicit_globals_dict(&scope, frame->implicitGlobals());
      if (!dictIncludesByStr(thread, implicit_globals_dict,
                             dunder_annotations)) {
        Object annotations(&scope, runtime->newDict());
        dictAtPutByStr(thread, implicit_globals_dict, dunder_annotations,
                       annotations);
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

HANDLER_INLINE Continue Interpreter::doYieldValue(Thread*, word) {
  return Continue::YIELD;
}

static Continue implicitGlobalsAtPut(Thread* thread, Frame* frame,
                                     const Object& implicit_globals_obj,
                                     const Str& name, const Object& value) {
  HandleScope scope(thread);
  if (implicit_globals_obj.isNoneType()) {
    Module module(&scope, frame->function().moduleObject());
    moduleAtPut(thread, module, name, value);
    return Continue::NEXT;
  }
  if (implicit_globals_obj.isDict()) {
    Dict implicit_globals(&scope, *implicit_globals_obj);
    dictAtPutByStr(thread, implicit_globals, name, value);
  } else {
    if (objectSetItem(thread, implicit_globals_obj, name, value)
            .isErrorException()) {
      return Continue::UNWIND;
    }
  }
  return Continue::NEXT;
}

Continue Interpreter::moduleImportAllFrom(Thread* thread, Frame* frame,
                                          const Module& module,
                                          const Object& implicit_globals) {
  HandleScope scope(thread);
  List module_keys(&scope, moduleKeys(thread, module));
  Str symbol_name(&scope, Str::empty());
  Object value(&scope, NoneType::object());
  for (word i = 0; i < module_keys.numItems(); i++) {
    CHECK(module_keys.at(i).isStr(), "Symbol is not a String");
    symbol_name = module_keys.at(i);
    // Load all the symbols not starting with '_'
    if (symbol_name.charAt(0) != '_') {
      value = moduleAt(thread, module, symbol_name);
      DCHECK(!value.isErrorNotFound(), "value must not be ErrorNotFound");
      if (implicitGlobalsAtPut(thread, frame, implicit_globals, symbol_name,
                               value) == Continue::UNWIND) {
        return Continue::UNWIND;
      }
    }
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doImportStar(Thread* thread, word) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();

  // Pre-python3 this used to merge the locals with the locals dict. However,
  // that's not necessary anymore. You can't import * inside a function
  // body anymore.

  Object implicit_globals(&scope, frame->implicitGlobals());
  Module module(&scope, frame->popValue());
  return moduleImportAllFrom(thread, frame, module, implicit_globals);
}

HANDLER_INLINE Continue Interpreter::doPopBlock(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  TryBlock block = frame->blockStack()->pop();
  frame->setValueStackTop(frame->valueStackBase() - block.level());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doEndFinally(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);

  Object status(&scope, frame->popValue());
  if (status.isSmallInt()) {
    auto why = static_cast<TryBlock::Why>(SmallInt::cast(*status).value());
    switch (why) {
      case TryBlock::Why::kReturn:
        return Continue::RETURN;
      case TryBlock::Why::kContinue:
        handleLoopExit(thread, why, frame->popValue());
        return Continue::NEXT;
      case TryBlock::Why::kBreak:
      case TryBlock::Why::kYield:
      case TryBlock::Why::kException:
        handleLoopExit(thread, why, NoneType::object());
        return Continue::NEXT;
      case TryBlock::Why::kSilenced:
        unwindExceptHandler(thread, frame, frame->blockStack()->pop());
        return Continue::NEXT;
    }
    UNIMPLEMENTED("unexpected why value");
  }
  if (thread->runtime()->isInstanceOfType(*status) &&
      Type(&scope, *status).isBaseExceptionSubclass()) {
    thread->setPendingExceptionType(*status);
    thread->setPendingExceptionValue(frame->popValue());
    thread->setPendingExceptionTraceback(frame->popValue());
    return Continue::UNWIND;
  }
  if (!status.isNoneType()) {
    thread->raiseWithFmt(LayoutId::kSystemError,
                         "Bad exception given to 'finally'");
    return Continue::UNWIND;
  }

  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doPopExcept(Thread* thread, word) {
  Frame* frame = thread->currentFrame();

  TryBlock block = frame->blockStack()->pop();
  if (block.kind() != TryBlock::kExceptHandler) {
    thread->raiseWithFmt(LayoutId::kSystemError,
                         "popped block is not an except handler");
    return Continue::UNWIND;
  }

  unwindExceptHandler(thread, frame, block);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreName(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  RawObject names = Code::cast(frame->code()).names();
  Str name(&scope, Tuple::cast(names).at(arg));
  Object value(&scope, frame->popValue());
  Object implicit_globals(&scope, frame->implicitGlobals());
  return implicitGlobalsAtPut(thread, frame, implicit_globals, name, value);
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

static HANDLER_INLINE Continue unpackSequenceWithLength(
    Thread* thread, Frame* frame, const Tuple& tuple, word count, word length) {
  if (length < count) {
    thread->raiseWithFmt(LayoutId::kValueError, "not enough values to unpack");
    return Continue::UNWIND;
  }
  if (length > count) {
    thread->raiseWithFmt(LayoutId::kValueError, "too many values to unpack");
    return Continue::UNWIND;
  }
  for (word i = length - 1; i >= 0; i--) {
    frame->pushValue(tuple.at(i));
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doUnpackSequence(Thread* thread,
                                                      word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object iterable(&scope, frame->popValue());
  if (iterable.isTuple()) {
    Tuple tuple(&scope, *iterable);
    return unpackSequenceWithLength(thread, frame, tuple, arg, tuple.length());
  }
  if (iterable.isList()) {
    List list(&scope, *iterable);
    Tuple tuple(&scope, list.items());
    return unpackSequenceWithLength(thread, frame, tuple, arg, list.numItems());
  }
  Object iterator(&scope, createIterator(thread, frame, iterable));
  if (iterator.isErrorException()) return Continue::UNWIND;

  Object next_method(&scope,
                     lookupMethod(thread, frame, iterator, ID(__next__)));
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
    value = callMethod1(thread, frame, next_method, iterator);
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
    frame->pushValue(*value);
    ++num_pushed;
  }

  // swap values on the stack
  Object tmp(&scope, NoneType::object());
  for (word i = 0, j = num_pushed - 1, half = num_pushed / 2; i < half;
       ++i, --j) {
    tmp = frame->peek(i);
    frame->setValueAt(frame->peek(j), i);
    frame->setValueAt(*tmp, j);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doForIter(Thread* thread, word arg) {
  return forIterUpdateCache(thread, arg, -1);
}

Continue Interpreter::forIterUpdateCache(Thread* thread, word arg, word index) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object iter(&scope, frame->topValue());
  Type type(&scope, thread->runtime()->typeOf(*iter));
  Object next(&scope, typeLookupInMroById(thread, type, ID(__next__)));
  if (next.isErrorNotFound()) {
    thread->raiseWithFmt(LayoutId::kTypeError, "iter() returned non-iterator");
    return Continue::UNWIND;
  }

  Object result(&scope, NoneType::object());
  if (next.isFunction()) {
    if (index >= 0) {
      Tuple caches(&scope, frame->caches());
      Str next_name(&scope, thread->runtime()->symbols()->at(ID(__next__)));
      Function dependent(&scope, frame->function());
      ICState next_cache_state = icUpdateAttr(
          thread, caches, index, iter.layoutId(), next, next_name, dependent);
      word pc = frame->currentPC();
      RawMutableBytes bytecode = frame->bytecode();
      bytecode.byteAtPut(pc, next_cache_state == ICState::kMonomorphic
                                 ? FOR_ITER_MONOMORPHIC
                                 : FOR_ITER_POLYMORPHIC);
    }
    result = Interpreter::callMethod1(thread, frame, next, iter);
  } else {
    next = resolveDescriptorGet(thread, next, iter, type);
    if (next.isErrorException()) return Continue::UNWIND;
    result = callFunction0(thread, frame, next);
  }

  if (result.isErrorException()) {
    if (thread->clearPendingStopIteration()) {
      frame->popValue();
      frame->setVirtualPC(frame->virtualPC() + arg);
      return Continue::NEXT;
    }
    return Continue::UNWIND;
  }
  frame->pushValue(*result);
  return Continue::NEXT;
}

static RawObject builtinsModule(Thread* thread, const Module& module) {
  HandleScope scope(thread);
  Object builtins_obj(&scope, moduleAtById(thread, module, ID(__builtins__)));
  if (builtins_obj.isErrorNotFound()) {
    return Error::notFound();
  }
  CHECK(thread->runtime()->isInstanceOfModule(*builtins_obj),
        "expected builtins to be a module");
  return *builtins_obj;
}

RawObject Interpreter::globalsAt(Thread* thread, const Module& module,
                                 const Object& name, const Function& function,
                                 word cache_index) {
  HandleScope scope(thread);
  Object module_result(&scope, moduleValueCellAt(thread, module, name));
  if (module_result.isValueCell()) {
    ValueCell value_cell(&scope, *module_result);
    icUpdateGlobalVar(thread, function, cache_index, value_cell);
    return value_cell.value();
  }
  Module builtins(&scope, builtinsModule(thread, module));
  Object builtins_result(&scope, moduleValueCellAt(thread, builtins, name));
  if (builtins_result.isValueCell()) {
    ValueCell value_cell(&scope, *builtins_result);
    icUpdateGlobalVar(thread, function, cache_index, value_cell);
    // Set up a placeholder in module to signify that a builtin entry under
    // the same name is cached.
    Dict module_dict(&scope, module.dict());
    NoneType none(&scope, NoneType::object());
    ValueCell module_value_cell(
        &scope, dictAtPutInValueCellByStr(thread, module_dict, name, none));
    module_value_cell.makePlaceholder();
    return value_cell.value();
  }
  return Error::notFound();
}

void Interpreter::globalsAtPut(Thread* thread, const Module& module,
                               const Object& name, const Object& value,
                               const Function& function, word cache_index) {
  HandleScope scope(thread);
  ValueCell module_result(&scope, moduleAtPut(thread, module, name, value));
  icUpdateGlobalVar(thread, function, cache_index, module_result);
}

ALWAYS_INLINE Continue Interpreter::forIter(Thread* thread,
                                            RawObject next_method, word arg) {
  DCHECK(next_method.isFunction(), "Unexpected next_method value");
  Frame* frame = thread->currentFrame();
  RawObject iter = frame->topValue();
  RawObject* sp = frame->valueStackTop();
  frame->pushValue(next_method);
  frame->pushValue(iter);
  RawObject result =
      Function::cast(Function::cast(next_method)).entry()(thread, frame, 1);
  frame->setValueStackTop(sp);
  if (result.isErrorException()) {
    if (thread->clearPendingStopIteration()) {
      frame->popValue();
      // TODO(bsimmers): originalArg() is only meant for slow paths, but we
      // currently have no other way of getting this information.
      frame->setVirtualPC(frame->virtualPC() +
                          originalArg(frame->function(), arg));
      return Continue::NEXT;
    }
    return Continue::UNWIND;
  }
  frame->pushValue(result);
  return Continue::NEXT;
}

static Continue retryForIterAnamorphic(Thread* thread, word arg) {
  // Revert the opcode, and retry FOR_ITER_CACHED.
  Frame* frame = thread->currentFrame();
  word pc = frame->currentPC();
  RawMutableBytes bytecode = frame->bytecode();
  bytecode.byteAtPut(pc, FOR_ITER_ANAMORPHIC);
  return Interpreter::doForIterAnamorphic(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doForIterList(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter_obj = frame->topValue();
  if (!iter_obj.isListIterator()) {
    return retryForIterAnamorphic(thread, arg);
  }
  // NOTE: This should be synced with listIteratorNext in list-builtins.cpp.
  RawListIterator iter = ListIterator::cast(iter_obj);
  word idx = iter.index();
  RawList underlying = iter.iterable().rawCast<RawList>();
  if (idx >= underlying.numItems()) {
    frame->popValue();
    frame->setVirtualPC(frame->virtualPC() +
                        originalArg(frame->function(), arg));
  } else {
    frame->pushValue(underlying.at(idx));
    iter.setIndex(idx + 1);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doForIterDict(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter_obj = frame->topValue();
  if (!iter_obj.isDictKeyIterator()) {
    return retryForIterAnamorphic(thread, arg);
  }
  // NOTE: This should be synced with dictKeyIteratorNext in dict-builtins.cpp.
  RawDictKeyIterator iter = DictKeyIterator::cast(iter_obj);
  RawDict dict = iter.iterable().rawCast<RawDict>();
  RawTuple buckets = Tuple::cast(dict.data());
  word i = iter.index();
  if (Dict::Bucket::nextItem(buckets, &i)) {
    // At this point, we have found a valid index in the buckets.
    iter.setIndex(i);
    iter.setNumFound(iter.numFound() + 1);
    frame->pushValue(Dict::Bucket::key(buckets, i));
  } else {
    // We hit the end.
    iter.setIndex(i);
    frame->popValue();
    frame->setVirtualPC(frame->virtualPC() +
                        originalArg(frame->function(), arg));
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doForIterTuple(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter_obj = frame->topValue();
  if (!iter_obj.isTupleIterator()) {
    return retryForIterAnamorphic(thread, arg);
  }
  // NOTE: This should be synced with tupleIteratorNext in tuple-builtins.cpp.
  RawTupleIterator iter = TupleIterator::cast(iter_obj);
  word idx = iter.index();
  if (idx == iter.tupleLength()) {
    frame->popValue();
    frame->setVirtualPC(frame->virtualPC() +
                        originalArg(frame->function(), arg));
  } else {
    RawTuple underlying = iter.iterable().rawCast<RawTuple>();
    RawObject item = underlying.at(idx);
    iter.setIndex(idx + 1);
    frame->pushValue(item);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doForIterRange(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter_obj = frame->topValue();
  if (!iter_obj.isRangeIterator()) {
    return retryForIterAnamorphic(thread, arg);
  }
  // NOTE: This should be synced with rangeIteratorNext in range-builtins.cpp.
  RawRangeIterator iter = RangeIterator::cast(iter_obj);
  word length = iter.length();
  if (length == 0) {
    frame->popValue();
    frame->setVirtualPC(frame->virtualPC() +
                        originalArg(frame->function(), arg));
  } else {
    iter.setLength(length - 1);
    word next = iter.next();
    if (length > 1) {
      word step = iter.step();
      iter.setNext(next + step);
    }
    frame->pushValue(SmallInt::fromWord(next));
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doForIterStr(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter_obj = frame->topValue();
  if (!iter_obj.isStrIterator()) {
    return retryForIterAnamorphic(thread, arg);
  }
  // NOTE: This should be synced with strIteratorNext in str-builtins.cpp.
  RawStrIterator iter = StrIterator::cast(iter_obj);
  word byte_offset = iter.index();
  RawStr underlying = iter.iterable().rawCast<RawStr>();
  if (byte_offset == underlying.charLength()) {
    frame->popValue();
    frame->setVirtualPC(frame->virtualPC() +
                        originalArg(frame->function(), arg));
  } else {
    word num_bytes = 0;
    word code_point = underlying.codePointAt(byte_offset, &num_bytes);
    iter.setIndex(byte_offset + num_bytes);
    frame->pushValue(RawSmallStr::fromCodePoint(code_point));
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doForIterMonomorphic(Thread* thread,
                                                          word arg) {
  Frame* frame = thread->currentFrame();
  RawTuple caches = frame->caches();
  LayoutId iter_layout_id = frame->topValue().layoutId();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, iter_layout_id, &is_found);
  if (!is_found) {
    return forIterUpdateCache(thread, originalArg(frame->function(), arg), arg);
  }
  return forIter(thread, cached, arg);
}

HANDLER_INLINE Continue Interpreter::doForIterPolymorphic(Thread* thread,
                                                          word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter = frame->topValue();
  LayoutId iter_layout_id = iter.layoutId();
  bool is_found;
  RawObject cached =
      icLookupPolymorphic(frame->caches(), arg, iter_layout_id, &is_found);
  if (!is_found) {
    return forIterUpdateCache(thread, originalArg(frame->function(), arg), arg);
  }
  return forIter(thread, cached, arg);
}

HANDLER_INLINE Continue Interpreter::doForIterAnamorphic(Thread* thread,
                                                         word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter = frame->topValue();
  LayoutId iter_layout_id = iter.layoutId();
  word pc = frame->virtualPC() - kCodeUnitSize;
  RawMutableBytes bytecode = frame->bytecode();
  switch (iter_layout_id) {
    case LayoutId::kListIterator:
      bytecode.byteAtPut(pc, FOR_ITER_LIST);
      return doForIterList(thread, arg);
    case LayoutId::kDictKeyIterator:
      bytecode.byteAtPut(pc, FOR_ITER_DICT);
      return doForIterDict(thread, arg);
    case LayoutId::kTupleIterator:
      bytecode.byteAtPut(pc, FOR_ITER_TUPLE);
      return doForIterTuple(thread, arg);
    case LayoutId::kRangeIterator:
      bytecode.byteAtPut(pc, FOR_ITER_RANGE);
      return doForIterRange(thread, arg);
    case LayoutId::kStrIterator:
      bytecode.byteAtPut(pc, FOR_ITER_STR);
      return doForIterStr(thread, arg);
    default:
      break;
  }
  return forIterUpdateCache(thread, originalArg(frame->function(), arg), arg);
}

HANDLER_INLINE Continue Interpreter::doUnpackEx(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object iterable(&scope, frame->popValue());
  Object iterator(&scope, createIterator(thread, frame, iterable));
  if (iterator.isErrorException()) return Continue::UNWIND;

  Object next_method(&scope,
                     lookupMethod(thread, frame, iterator, ID(__next__)));
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
    value = callMethod1(thread, frame, next_method, iterator);
    if (value.isErrorException()) {
      if (thread->clearPendingStopIteration()) break;
      return Continue::UNWIND;
    }
    frame->pushValue(*value);
  }

  if (num_pushed < before) {
    thread->raiseWithFmt(LayoutId::kValueError, "not enough values to unpack");
    return Continue::UNWIND;
  }

  List list(&scope, runtime->newList());
  for (;;) {
    value = callMethod1(thread, frame, next_method, iterator);
    if (value.isErrorException()) {
      if (thread->clearPendingStopIteration()) break;
      return Continue::UNWIND;
    }
    runtime->listAdd(thread, list, value);
  }

  frame->pushValue(*list);
  num_pushed++;

  if (list.numItems() < after) {
    thread->raiseWithFmt(LayoutId::kValueError, "not enough values to unpack");
    return Continue::UNWIND;
  }

  if (after > 0) {
    // Pop elements off the list and set them on the stack
    for (word i = list.numItems() - after, j = list.numItems(); i < j;
         ++i, ++num_pushed) {
      frame->pushValue(list.at(i));
      list.atPut(i, NoneType::object());
    }
    list.setNumItems(list.numItems() - after);
  }

  // swap values on the stack
  Object tmp(&scope, NoneType::object());
  for (word i = 0, j = num_pushed - 1, half = num_pushed / 2; i < half;
       ++i, --j) {
    tmp = frame->peek(i);
    frame->setValueAt(frame->peek(j), i);
    frame->setValueAt(*tmp, j);
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
                        typeLookupInMroById(thread, type, ID(__setattr__)));
  if (dunder_setattr == runtime->objectDunderSetattr()) {
    return objectSetAttrSetLocation(thread, object, name, value, location_out);
  }
  Object result(&scope,
                thread->invokeMethod3(object, ID(__setattr__), name, value));
  return *result;
}

Continue Interpreter::storeAttrUpdateCache(Thread* thread, word arg,
                                           ICState ic_state) {
  Frame* frame = thread->currentFrame();
  word original_arg = originalArg(frame->function(), arg);
  HandleScope scope(thread);
  Object receiver(&scope, frame->popValue());
  Str name(&scope,
           Tuple::cast(Code::cast(frame->code()).names()).at(original_arg));
  Object value(&scope, frame->popValue());

  Object location(&scope, NoneType::object());
  LayoutId saved_layout_id = receiver.layoutId();
  Object result(&scope,
                storeAttrSetLocation(thread, receiver, name, value, &location));
  if (result.isErrorException()) return Continue::UNWIND;
  if (location.isNoneType()) return Continue::NEXT;
  DCHECK(location.isSmallInt(), "unexpected location");
  bool is_in_object = SmallInt::cast(*location).value() >= 0;

  Tuple caches(&scope, frame->caches());
  Function dependent(&scope, frame->function());
  LayoutId receiver_layout_id = receiver.layoutId();
  word pc = frame->currentPC();
  RawMutableBytes bytecode = frame->bytecode();
  // TODO(T59400994): Clean up when storeAttrSetLocation can return a
  // StoreAttrKind.
  if (ic_state == ICState::kAnamorphic) {
    if (saved_layout_id == receiver_layout_id) {
      // No layout transition.
      if (is_in_object) {
        bytecode.byteAtPut(pc, STORE_ATTR_INSTANCE);
        icUpdateAttr(thread, caches, arg, saved_layout_id, location, name,
                     dependent);
      } else {
        bytecode.byteAtPut(pc, STORE_ATTR_INSTANCE_OVERFLOW);
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
        bytecode.byteAtPut(pc, STORE_ATTR_INSTANCE_UPDATE);
        icUpdateAttr(thread, caches, arg, saved_layout_id, layout_offset, name,
                     dependent);
      } else {
        bytecode.byteAtPut(pc, STORE_ATTR_INSTANCE_OVERFLOW_UPDATE);
        icUpdateAttr(thread, caches, arg, saved_layout_id, layout_offset, name,
                     dependent);
      }
    }
  } else {
    DCHECK(bytecode.byteAt(pc) == STORE_ATTR_INSTANCE ||
               bytecode.byteAt(pc) == STORE_ATTR_INSTANCE_OVERFLOW ||
               bytecode.byteAt(pc) == STORE_ATTR_POLYMORPHIC,
           "unexpected opcode");
    if (saved_layout_id == receiver_layout_id) {
      bytecode.byteAtPut(pc, STORE_ATTR_POLYMORPHIC);
      icUpdateAttr(thread, caches, arg, saved_layout_id, location, name,
                   dependent);
    }
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreAttrAnamorphic(Thread* thread,
                                                           word arg) {
  return storeAttrUpdateCache(thread, arg, ICState::kAnamorphic);
}

// This code cleans-up a monomorphic cache and prepares it for its potential
// use as a polymorphic cache.  This code should be removed when we change the
// structure of our caches directly accessible from a function to be monomophic
// and to allocate the relatively uncommon polymorphic caches in a separate
// object.
static Continue retryStoreAttrCached(Thread* thread, word arg) {
  // Revert the opcode, clear the cache, and retry the attribute lookup.
  Frame* frame = thread->currentFrame();
  RawTuple caches = frame->caches();
  word pc = frame->virtualPC() - kCodeUnitSize;
  word index = arg * kIcPointersPerEntry;
  RawMutableBytes bytecode = frame->bytecode();
  bytecode.byteAtPut(pc, STORE_ATTR_ANAMORPHIC);
  caches.atPut(index + kIcEntryKeyOffset, NoneType::object());
  caches.atPut(index + kIcEntryValueOffset, NoneType::object());
  return Interpreter::doStoreAttrAnamorphic(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doStoreAttrInstance(Thread* thread,
                                                         word arg) {
  Frame* frame = thread->currentFrame();
  RawTuple caches = frame->caches();
  RawObject receiver = frame->topValue();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    return storeAttrUpdateCache(thread, arg, ICState::kMonomorphic);
  }
  word offset = SmallInt::cast(cached).value();
  DCHECK(offset >= 0, "unexpected offset");
  RawInstance instance = Instance::cast(receiver);
  instance.instanceVariableAtPut(offset, frame->peek(1));
  frame->dropValues(2);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreAttrInstanceOverflow(Thread* thread,
                                                                 word arg) {
  Frame* frame = thread->currentFrame();
  RawTuple caches = frame->caches();
  RawObject receiver = frame->topValue();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    return storeAttrUpdateCache(thread, arg, ICState::kMonomorphic);
  }
  word offset = SmallInt::cast(cached).value();
  DCHECK(offset < 0, "unexpected offset");
  RawInstance instance = Instance::cast(receiver);
  RawLayout layout = Layout::cast(thread->runtime()->layoutOf(receiver));
  RawTuple overflow =
      Tuple::cast(instance.instanceVariableAt(layout.overflowOffset()));
  overflow.atPut(-offset - 1, frame->peek(1));
  frame->dropValues(2);
  return Continue::NEXT;
}

HANDLER_INLINE Continue
Interpreter::doStoreAttrInstanceOverflowUpdate(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawTuple caches = frame->caches();
  RawObject receiver = frame->topValue();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
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
  Object value(&scope, frame->peek(1));
  if (offset >= overflow.length()) {
    instanceGrowOverflow(thread, instance, offset + 1);
    overflow = instance.instanceVariableAt(layout.overflowOffset());
  }
  instance.setLayoutId(new_layout_id);
  overflow.atPut(offset, *value);
  frame->dropValues(2);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreAttrInstanceUpdate(Thread* thread,
                                                               word arg) {
  Frame* frame = thread->currentFrame();
  RawTuple caches = frame->caches();
  RawObject receiver = frame->topValue();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
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
  instance.instanceVariableAtPut(offset, frame->peek(1));
  instance.setLayoutId(new_layout_id);
  frame->dropValues(2);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreAttrPolymorphic(Thread* thread,
                                                            word arg) {
  Frame* frame = thread->currentFrame();
  RawObject receiver = frame->topValue();
  LayoutId layout_id = receiver.layoutId();
  bool is_found;
  RawObject cached =
      icLookupPolymorphic(frame->caches(), arg, layout_id, &is_found);
  if (!is_found) {
    return storeAttrUpdateCache(thread, arg, ICState::kPolymorphic);
  }
  RawObject value = frame->peek(1);
  frame->dropValues(2);
  storeAttrWithLocation(thread, receiver, cached, value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreAttr(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object receiver(&scope, frame->popValue());
  Tuple names(&scope, Code::cast(frame->code()).names());
  Str name(&scope, names.at(arg));
  Object value(&scope, frame->popValue());
  if (thread->invokeMethod3(receiver, ID(__setattr__), name, value)
          .isErrorException()) {
    return Continue::UNWIND;
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doDeleteAttr(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object receiver(&scope, frame->popValue());
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
  Str key(&scope, names.at(arg));
  Object value(&scope, frame->popValue());
  Module module(&scope, frame->function().moduleObject());
  Function function(&scope, frame->function());
  globalsAtPut(thread, module, key, value, function, arg);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreGlobalCached(Thread* thread,
                                                         word arg) {
  Frame* frame = thread->currentFrame();
  RawObject cached = icLookupGlobalVar(frame->caches(), arg);
  ValueCell::cast(cached).setValue(frame->popValue());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doDeleteGlobal(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Module module(&scope, frame->function().moduleObject());
  Tuple names(&scope, Code::cast(frame->code()).names());
  Str name(&scope, names.at(arg));
  word hash = strHash(thread, *name);
  if (moduleRemove(thread, module, name, hash).isErrorNotFound()) {
    return raiseUndefinedName(thread, name);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadConst(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject consts = Code::cast(frame->code()).consts();
  frame->pushValue(Tuple::cast(consts).at(arg));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadImmediate(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  frame->pushValue(objectFromOparg(arg));
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
        frame->pushValue(*result);
        return Continue::NEXT;
      }
    } else {
      Object result(&scope, objectGetItem(thread, implicit_globals_obj, name));
      if (!result.isErrorException()) {
        frame->pushValue(*result);
        return Continue::NEXT;
      }
      if (!thread->pendingExceptionMatches(LayoutId::kKeyError)) {
        return Continue::UNWIND;
      }
      thread->clearPendingException();
    }
  }
  Module module(&scope, frame->function().moduleObject());
  Object module_result(&scope, moduleAt(thread, module, name));
  if (!module_result.isErrorNotFound()) {
    frame->pushValue(*module_result);
    return Continue::NEXT;
  }
  Module builtins(&scope, builtinsModule(thread, module));
  Object builtins_result(&scope, moduleAt(thread, builtins, name));
  if (!builtins_result.isErrorNotFound()) {
    frame->pushValue(*builtins_result);
    return Continue::NEXT;
  }
  return raiseUndefinedName(thread, name);
}

HANDLER_INLINE Continue Interpreter::doBuildTuple(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Tuple tuple(&scope, thread->runtime()->newTuple(arg));
  for (word i = arg - 1; i >= 0; i--) {
    tuple.atPut(i, frame->popValue());
  }
  frame->pushValue(*tuple);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildList(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  Runtime* runtime = thread->runtime();
  if (arg == 0) {
    frame->pushValue(runtime->newList());
    return Continue::NEXT;
  }
  HandleScope scope(thread);
  MutableTuple array(&scope, runtime->newMutableTuple(arg));
  for (word i = arg - 1; i >= 0; i--) {
    array.atPut(i, frame->popValue());
  }
  RawList list = List::cast(runtime->newList());
  list.setItems(*array);
  list.setNumItems(array.length());
  frame->pushValue(list);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildSet(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Runtime* runtime = thread->runtime();
  Set set(&scope, runtime->newSet());
  Object value(&scope, NoneType::object());
  Object hash_obj(&scope, NoneType::object());
  for (word i = arg - 1; i >= 0; i--) {
    value = frame->popValue();
    hash_obj = hash(thread, value);
    if (hash_obj.isErrorException()) return Continue::UNWIND;
    word hash = SmallInt::cast(*hash_obj).value();
    setAdd(thread, set, value, hash);
  }
  frame->pushValue(*set);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildMap(Thread* thread, word arg) {
  Runtime* runtime = thread->runtime();
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Dict dict(&scope, runtime->newDictWithSize(arg));
  Object value(&scope, NoneType::object());
  Object key(&scope, NoneType::object());
  Object hash_obj(&scope, NoneType::object());
  for (word i = 0; i < arg; i++) {
    value = frame->popValue();
    key = frame->popValue();
    hash_obj = hash(thread, key);
    if (hash_obj.isErrorException()) return Continue::UNWIND;
    word hash = SmallInt::cast(*hash_obj).value();
    dictAtPut(thread, dict, key, hash, value);
  }
  frame->pushValue(*dict);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadAttr(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object receiver(&scope, frame->topValue());
  Tuple names(&scope, Code::cast(frame->code()).names());
  Str name(&scope, names.at(arg));
  RawObject result = thread->runtime()->attributeAt(thread, receiver, name);
  if (result.isErrorException()) return Continue::UNWIND;
  frame->setTopValue(result);
  return Continue::NEXT;
}

RawObject Interpreter::loadAttrSetLocation(Thread* thread,
                                           const Object& receiver,
                                           const Object& name,
                                           LoadAttrKind* kind,
                                           Object* location_out) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(*receiver));
  Object dunder_getattribute(
      &scope, typeLookupInMroById(thread, type, ID(__getattribute__)));
  *kind = LoadAttrKind::kUnknown;
  if (dunder_getattribute == runtime->objectDunderGetattribute()) {
    Object result(&scope, objectGetAttributeSetLocation(thread, receiver, name,
                                                        location_out, kind));
    if (result.isErrorNotFound()) {
      result = thread->invokeMethod2(receiver, ID(__getattr__), name);
      if (result.isErrorNotFound()) {
        return objectRaiseAttributeError(thread, receiver, name);
      }
    }
    return *result;
  }
  if (dunder_getattribute == runtime->moduleDunderGetattribute() &&
      runtime->isInstanceOfModule(*receiver)) {
    Module module(&scope, *receiver);
    Object result(&scope, moduleGetAttributeSetLocation(thread, module, name,
                                                        location_out));
    if (!result.isErrorNotFound()) {
      // We have a result that can be cached.
      *kind = LoadAttrKind::kModule;
    } else {
      // Try again
      result = thread->invokeMethod2(receiver, ID(__getattr__), name);
      if (result.isErrorNotFound()) {
        return moduleRaiseAttributeError(thread, module, name);
      }
    }
    return *result;
  }
  if (dunder_getattribute == runtime->typeDunderGetattribute() &&
      runtime->isInstanceOfType(*receiver)) {
    Type object_as_type(&scope, *receiver);
    Object result(&scope, typeGetAttributeSetLocation(thread, object_as_type,
                                                      name, location_out));
    if (!result.isErrorNotFound()) {
      // We have a result that can be cached.
      if (location_out->isValueCell()) {
        *kind = LoadAttrKind::kType;
      }
    } else {
      // Try again
      result = thread->invokeMethod2(receiver, ID(__getattr__), name);
      if (result.isErrorNotFound()) {
        return objectRaiseAttributeError(thread, receiver, name);
      }
    }
    return *result;
  }
  return thread->runtime()->attributeAt(thread, receiver, name);
}

Continue Interpreter::loadAttrUpdateCache(Thread* thread, word arg,
                                          ICState ic_state) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  word original_arg = originalArg(frame->function(), arg);
  Object receiver(&scope, frame->topValue());
  Str name(&scope,
           Tuple::cast(Code::cast(frame->code()).names()).at(original_arg));

  Object location(&scope, NoneType::object());
  LoadAttrKind kind;
  Object result(&scope,
                loadAttrSetLocation(thread, receiver, name, &kind, &location));
  if (result.isErrorException()) return Continue::UNWIND;
  if (location.isNoneType()) {
    frame->setTopValue(*result);
    return Continue::NEXT;
  }

  // Cache the attribute load
  Tuple caches(&scope, frame->caches());
  Function dependent(&scope, frame->function());
  LayoutId receiver_layout_id = receiver.layoutId();
  word pc = frame->currentPC();
  RawMutableBytes bytecode = frame->bytecode();
  if (ic_state == ICState::kAnamorphic) {
    switch (kind) {
      case LoadAttrKind::kInstanceOffset:
        bytecode.byteAtPut(pc, LOAD_ATTR_INSTANCE);
        icUpdateAttr(thread, caches, arg, receiver_layout_id, location, name,
                     dependent);
        break;
      case LoadAttrKind::kInstanceFunction:
        bytecode.byteAtPut(pc, LOAD_ATTR_INSTANCE_TYPE_BOUND_METHOD);
        icUpdateAttr(thread, caches, arg, receiver_layout_id, location, name,
                     dependent);
        break;
      case LoadAttrKind::kInstanceProperty:
        bytecode.byteAtPut(pc, LOAD_ATTR_INSTANCE_PROPERTY);
        icUpdateAttr(thread, caches, arg, receiver_layout_id, location, name,
                     dependent);
        break;
      case LoadAttrKind::kInstanceType:
        bytecode.byteAtPut(pc, LOAD_ATTR_INSTANCE_TYPE);
        icUpdateAttr(thread, caches, arg, receiver_layout_id, location, name,
                     dependent);
        break;
      case LoadAttrKind::kInstanceTypeDescr:
        bytecode.byteAtPut(pc, LOAD_ATTR_INSTANCE_TYPE_DESCR);
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
    DCHECK(bytecode.byteAt(pc) == LOAD_ATTR_INSTANCE ||
               bytecode.byteAt(pc) == LOAD_ATTR_INSTANCE_TYPE_BOUND_METHOD ||
               bytecode.byteAt(pc) == LOAD_ATTR_POLYMORPHIC,
           "unexpected opcode");
    switch (kind) {
      case LoadAttrKind::kInstanceOffset:
      case LoadAttrKind::kInstanceFunction:
        bytecode.byteAtPut(pc, LOAD_ATTR_POLYMORPHIC);
        icUpdateAttr(thread, caches, arg, receiver_layout_id, location, name,
                     dependent);
        break;
      default:
        break;
    }
  }
  frame->setTopValue(*result);
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
  return loadAttrUpdateCache(thread, arg, ICState::kAnamorphic);
}

HANDLER_INLINE Continue Interpreter::doLoadAttrInstance(Thread* thread,
                                                        word arg) {
  Frame* frame = thread->currentFrame();
  RawTuple caches = frame->caches();
  RawObject receiver = frame->topValue();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    return Interpreter::loadAttrUpdateCache(thread, arg, ICState::kMonomorphic);
  }
  RawObject result = loadAttrWithLocation(thread, receiver, cached);
  frame->setTopValue(result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue
Interpreter::doLoadAttrInstanceTypeBoundMethod(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawTuple caches = frame->caches();
  RawObject receiver = frame->topValue();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    return Interpreter::loadAttrUpdateCache(thread, arg, ICState::kMonomorphic);
  }
  HandleScope scope(thread);
  Object self(&scope, receiver);
  Object function(&scope, cached);
  frame->setTopValue(thread->runtime()->newBoundMethod(function, self));
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
  RawTuple caches = frame->caches();
  word pc = frame->virtualPC() - kCodeUnitSize;
  word index = arg * kIcPointersPerEntry;
  RawMutableBytes bytecode = frame->bytecode();
  bytecode.byteAtPut(pc, LOAD_ATTR_ANAMORPHIC);
  caches.atPut(index + kIcEntryKeyOffset, NoneType::object());
  caches.atPut(index + kIcEntryValueOffset, NoneType::object());
  return Interpreter::loadAttrUpdateCache(thread, arg, ICState::kAnamorphic);
}

HANDLER_INLINE Continue Interpreter::doLoadAttrInstanceProperty(Thread* thread,
                                                                word arg) {
  Frame* frame = thread->currentFrame();
  RawTuple caches = frame->caches();
  RawObject receiver = frame->topValue();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    return retryLoadAttrCached(thread, arg);
  }
  frame->pushValue(receiver);
  frame->setValueAt(cached, 1);
  return tailcallPreparedFunction(thread, frame, cached, 1);
}

HANDLER_INLINE Continue Interpreter::doLoadAttrInstanceTypeDescr(Thread* thread,
                                                                 word arg) {
  Frame* frame = thread->currentFrame();
  RawTuple caches = frame->caches();
  RawObject receiver = frame->topValue();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    return retryLoadAttrCached(thread, arg);
  }
  HandleScope scope(thread);
  Object descr(&scope, cached);
  Object self(&scope, receiver);
  Type self_type(&scope, thread->runtime()->typeAt(self.layoutId()));
  Object result(&scope,
                Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                               descr, self, self_type));
  if (result.isError()) return Continue::UNWIND;
  frame->setTopValue(*result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadAttrInstanceType(Thread* thread,
                                                            word arg) {
  Frame* frame = thread->currentFrame();
  RawTuple caches = frame->caches();
  RawObject receiver = frame->topValue();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    return retryLoadAttrCached(thread, arg);
  }
  frame->setTopValue(cached);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadAttrModule(Thread* thread,
                                                      word arg) {
  Frame* frame = thread->currentFrame();
  RawObject receiver = frame->topValue();
  RawTuple caches = frame->caches();
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
    frame->setTopValue(ValueCell::cast(result).value());
    return Continue::NEXT;
  }
  return retryLoadAttrCached(thread, arg);
}

HANDLER_INLINE Continue Interpreter::doLoadAttrPolymorphic(Thread* thread,
                                                           word arg) {
  Frame* frame = thread->currentFrame();
  RawObject receiver = frame->topValue();
  LayoutId layout_id = receiver.layoutId();
  bool is_found;
  RawObject cached =
      icLookupPolymorphic(frame->caches(), arg, layout_id, &is_found);
  if (!is_found) {
    return loadAttrUpdateCache(thread, arg, ICState::kPolymorphic);
  }
  RawObject result = loadAttrWithLocation(thread, receiver, cached);
  frame->setTopValue(result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadAttrType(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject receiver = frame->topValue();
  RawTuple caches = frame->caches();
  word index = arg * kIcPointersPerEntry;
  RawObject type = caches.at(index + kIcEntryKeyOffset);
  if (receiver == type) {
    RawObject result = caches.at(index + kIcEntryValueOffset);
    DCHECK(result.isValueCell(), "cached value is not a value cell");
    frame->setTopValue(ValueCell::cast(result).value());
    return Continue::NEXT;
  }
  return retryLoadAttrCached(thread, arg);
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
  Frame* frame = thread->currentFrame();
  RawObject right = frame->popValue();
  RawObject left = frame->popValue();
  frame->pushValue(Bool::fromBool(left == right));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doCompareIsNot(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  RawObject right = frame->popValue();
  RawObject left = frame->popValue();
  frame->pushValue(Bool::fromBool(left != right));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doCompareOp(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object right(&scope, frame->popValue());
  Object left(&scope, frame->popValue());
  CompareOp op = static_cast<CompareOp>(arg);
  RawObject result = NoneType::object();
  if (op == IS) {
    result = Bool::fromBool(*left == *right);
  } else if (op == IS_NOT) {
    result = Bool::fromBool(*left != *right);
  } else if (op == IN) {
    result = sequenceContains(thread, frame, left, right);
  } else if (op == NOT_IN) {
    result = sequenceContains(thread, frame, left, right);
    if (result.isBool()) result = Bool::negate(result);
  } else if (op == EXC_MATCH) {
    result = excMatch(thread, left, right);
  } else {
    result = compareOperation(thread, frame, op, left, right);
  }

  if (result.isErrorException()) return Continue::UNWIND;
  frame->pushValue(result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doImportName(Thread* thread, word arg) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Frame* frame = thread->currentFrame();
  Code code(&scope, frame->code());
  Object name(&scope, Tuple::cast(code.names()).at(arg));
  Object fromlist(&scope, frame->popValue());
  Object level(&scope, frame->popValue());
  Module module(&scope, frame->function().moduleObject());
  Object globals(&scope, module.moduleProxy());
  // TODO(T41634372) Pass in a dict that is similar to what `builtins.locals`
  // returns. Use `None` for now since the default importlib behavior is to
  // ignore the value and this only matters if `__import__` is replaced.
  Object locals(&scope, NoneType::object());

  // Call builtins.__import__(name, globals, locals, fromlist, level).
  ValueCell dunder_import_cell(&scope, runtime->dunderImport());
  DCHECK(!dunder_import_cell.isUnbound(), "builtins module not initialized");
  Object dunder_import(&scope, dunder_import_cell.value());

  frame->pushValue(*dunder_import);
  frame->pushValue(*name);
  frame->pushValue(*globals);
  frame->pushValue(*locals);
  frame->pushValue(*fromlist);
  frame->pushValue(*level);
  return tailcallFunction(thread, 5);
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
  Object from(&scope, frame->topValue());

  Object value(&scope, NoneType::object());
  if (from.isModule()) {
    // Common case of a lookup done on the built-in module type.
    Module from_module(&scope, *from);
    value = moduleGetAttribute(thread, from_module, name);
  } else {
    // Do a generic attribute lookup.
    value = thread->runtime()->attributeAt(thread, from, name);
    if (value.isErrorException()) {
      if (!thread->pendingExceptionMatches(LayoutId::kAttributeError)) {
        return Continue::UNWIND;
      }
      thread->clearPendingException();
      value = Error::notFound();
    }
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
  frame->pushValue(*value);
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
  RawObject value = frame->topValue();
  value = isTrue(thread, value);
  if (LIKELY(value == Bool::falseObj())) {
    frame->setVirtualPC(arg);
    return Continue::NEXT;
  }
  if (value == Bool::trueObj()) {
    frame->popValue();
    return Continue::NEXT;
  }
  DCHECK(value.isErrorException(), "value must be error");
  return Continue::UNWIND;
}

HANDLER_INLINE Continue Interpreter::doJumpIfTrueOrPop(Thread* thread,
                                                       word arg) {
  Frame* frame = thread->currentFrame();
  RawObject value = frame->topValue();
  value = isTrue(thread, value);
  if (LIKELY(value == Bool::trueObj())) {
    frame->setVirtualPC(arg);
    return Continue::NEXT;
  }
  if (value == Bool::falseObj()) {
    frame->popValue();
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
  RawObject value = frame->popValue();
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
  RawObject value = frame->popValue();
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
  Str key(&scope, names.at(arg));
  Module module(&scope, frame->function().moduleObject());
  Function function(&scope, frame->function());
  Object result(&scope, globalsAt(thread, module, key, function, arg));
  if (result.isErrorNotFound()) {
    return raiseUndefinedName(thread, key);
  }
  frame->pushValue(*result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadGlobalCached(Thread* thread,
                                                        word arg) {
  Frame* frame = thread->currentFrame();
  RawObject cached = icLookupGlobalVar(frame->caches(), arg);
  DCHECK(cached.isValueCell(), "cached value must be a ValueCell");
  DCHECK(!ValueCell::cast(cached).isPlaceholder(),
         "cached ValueCell must not be a placeholder");
  frame->pushValue(ValueCell::cast(cached).value());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doContinueLoop(Thread* thread, word arg) {
  handleLoopExit(thread, TryBlock::Why::kContinue, SmallInt::fromWord(arg));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doSetupLoop(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  word stack_depth = frame->valueStackBase() - frame->valueStackTop();
  BlockStack* block_stack = frame->blockStack();
  word handler_pc = frame->virtualPC() + arg;
  block_stack->push(TryBlock(TryBlock::kLoop, handler_pc, stack_depth));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doSetupExcept(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  word stack_depth = frame->valueStackSize();
  BlockStack* block_stack = frame->blockStack();
  word handler_pc = frame->virtualPC() + arg;
  block_stack->push(TryBlock(TryBlock::kExcept, handler_pc, stack_depth));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doSetupFinally(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  word stack_depth = frame->valueStackBase() - frame->valueStackTop();
  BlockStack* block_stack = frame->blockStack();
  word handler_pc = frame->virtualPC() + arg;
  block_stack->push(TryBlock(TryBlock::kFinally, handler_pc, stack_depth));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadFast(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject value = frame->local(arg);
  if (UNLIKELY(value.isErrorNotFound())) {
    HandleScope scope(thread);
    Str name(&scope, Tuple::cast(Code::cast(frame->code()).varnames()).at(arg));
    thread->raiseWithFmt(LayoutId::kUnboundLocalError,
                         "local variable '%S' referenced before assignment",
                         &name);
    return Continue::UNWIND;
  }
  frame->pushValue(value);
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
  frame->pushValue(value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreFast(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject value = frame->popValue();
  frame->setLocal(arg, value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreFastReverse(Thread* thread,
                                                        word arg) {
  Frame* frame = thread->currentFrame();
  RawObject value = frame->popValue();
  frame->setLocalWithReverseIndex(arg, value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doDeleteFast(Thread* thread, word arg) {
  // TODO(T32821785): use another immediate value than Error to signal unbound
  // local
  Frame* frame = thread->currentFrame();
  if (UNLIKELY(frame->local(arg).isErrorNotFound())) {
    RawObject name = Tuple::cast(Code::cast(frame->code()).varnames()).at(arg);
    UNIMPLEMENTED("unbound local %s", Str::cast(name).toCStr());
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
  Object value(&scope, frame->popValue());
  Str name(&scope, Tuple::cast(*names).at(arg));
  Object annotations(&scope, NoneType::object());
  Object dunder_annotations(&scope,
                            runtime->symbols()->at(ID(__annotations__)));
  if (frame->implicitGlobals().isNoneType()) {
    // Module body
    Module module(&scope, frame->function().moduleObject());
    annotations = moduleAt(thread, module, dunder_annotations);
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
    if (thread->hasCaughtException()) {
      thread->setPendingExceptionType(thread->caughtExceptionType());
      thread->setPendingExceptionValue(thread->caughtExceptionValue());
      thread->setPendingExceptionTraceback(thread->caughtExceptionTraceback());
    } else {
      thread->raiseWithFmt(LayoutId::kRuntimeError,
                           "No active exception to reraise");
    }
  } else {
    Frame* frame = thread->currentFrame();
    RawObject cause = (arg >= 2) ? frame->popValue() : Error::notFound();
    RawObject exn = (arg >= 1) ? frame->popValue() : NoneType::object();
    raise(thread, exn, cause);
  }

  return Continue::UNWIND;
}

HANDLER_INLINE
Continue Interpreter::callTrampoline(Thread* thread, Function::Entry entry,
                                     word nargs, RawObject* post_call_sp) {
  Frame* frame = thread->currentFrame();
  RawObject result = entry(thread, frame, nargs);
  if (result.isErrorException()) return Continue::UNWIND;
  frame->setValueStackTop(post_call_sp);
  frame->pushValue(result);
  return Continue::NEXT;
}

static HANDLER_INLINE Continue callInterpretedImpl(
    Thread* thread, word nargs, Frame* caller_frame, RawFunction function,
    RawObject* post_call_sp, PrepareCallFunc prepare_args) {
  // Warning: This code is using `RawXXX` variables for performance! This is
  // despite the fact that we call functions that do potentially perform memory
  // allocations. This is legal here because we always rely on the functions
  // returning an up-to-date address and we make sure to never access any value
  // produce before a call after that call. Be careful not to break this
  // invariant if you change the code!

  RawObject result = prepare_args(thread, function, caller_frame, nargs);
  if (result.isErrorException()) return Continue::UNWIND;
  function = RawFunction::cast(result);

  bool has_freevars_or_cellvars = function.hasFreevarsOrCellvars();
  Frame* callee_frame = thread->pushCallFrame(function);
  if (UNLIKELY(callee_frame == nullptr)) {
    return Continue::UNWIND;
  }
  caller_frame->setValueStackTop(post_call_sp);
  if (has_freevars_or_cellvars) {
    processFreevarsAndCellvars(thread, callee_frame);
  }
  return Continue::NEXT;
}

Continue Interpreter::callInterpreted(Thread* thread, word nargs, Frame* frame,
                                      RawFunction function,
                                      RawObject* post_call_sp) {
  return callInterpretedImpl(thread, nargs, frame, function, post_call_sp,
                             preparePositionalCall);
}

HANDLER_INLINE Continue
Interpreter::handleCall(Thread* thread, word nargs, word callable_idx,
                        word num_extra_pop, PrepareCallFunc prepare_args,
                        Function::Entry (RawFunction::*get_entry)() const) {
  // Warning: This code is using `RawXXX` variables for performance! This is
  // despite the fact that we call functions that do potentially perform memory
  // allocations. This is legal here because we always rely on the functions
  // returning an up-to-date address and we make sure to never access any value
  // produce before a call after that call. Be careful not to break this
  // invariant if you change the code!

  Frame* caller_frame = thread->currentFrame();
  RawObject* post_call_sp =
      caller_frame->valueStackTop() + callable_idx + 1 + num_extra_pop;
  PrepareCallableResult prepare_result =
      prepareCallableCall(thread, caller_frame, nargs, callable_idx);
  if (prepare_result.function.isErrorException()) return Continue::UNWIND;
  RawFunction function = RawFunction::cast(prepare_result.function);
  nargs = prepare_result.nargs;

  SymbolId name = static_cast<SymbolId>(function.intrinsicId());
  if (name != SymbolId::kInvalid && doIntrinsic(thread, caller_frame, name)) {
    return Continue::NEXT;
  }

  if (!function.isInterpreted()) {
    return callTrampoline(thread, (function.*get_entry)(), nargs, post_call_sp);
  }

  return callInterpretedImpl(thread, nargs, caller_frame, function,
                             post_call_sp, prepare_args);
}

ALWAYS_INLINE Continue Interpreter::tailcallPreparedFunction(
    Thread* thread, Frame* frame, RawObject function_obj, word nargs) {
  RawObject* sp = frame->valueStackTop() + nargs + 1;
  DCHECK(function_obj == frame->peek(nargs),
         "frame->peek(nargs) is expected to be the given function");
  RawFunction function = Function::cast(function_obj);
  if (!function.isInterpreted()) {
    return callTrampoline(thread, function.entry(), nargs, sp);
  }
  return callInterpretedImpl(thread, nargs, frame, function, sp,
                             preparePositionalCall);
}

HANDLER_INLINE Continue Interpreter::doCallFunction(Thread* thread, word arg) {
  return handleCall(thread, arg, arg, 0, preparePositionalCall,
                    &Function::entry);
}

HANDLER_INLINE Continue Interpreter::doMakeFunction(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object qualname(&scope, frame->popValue());
  Code code(&scope, frame->popValue());
  Module module(&scope, frame->function().moduleObject());
  Runtime* runtime = thread->runtime();
  Function function(
      &scope, runtime->newFunctionWithCode(thread, qualname, code, module));
  if (arg & MakeFunctionFlag::CLOSURE) {
    function.setClosure(frame->popValue());
    DCHECK(runtime->isInstanceOfTuple(function.closure()), "expected tuple");
  }
  if (arg & MakeFunctionFlag::ANNOTATION_DICT) {
    function.setAnnotations(frame->popValue());
    DCHECK(runtime->isInstanceOfDict(function.annotations()), "expected dict");
  }
  if (arg & MakeFunctionFlag::DEFAULT_KW) {
    function.setKwDefaults(frame->popValue());
    DCHECK(runtime->isInstanceOfDict(function.kwDefaults()), "expected dict");
  }
  if (arg & MakeFunctionFlag::DEFAULT) {
    function.setDefaults(frame->popValue());
    DCHECK(runtime->isInstanceOfTuple(function.defaults()), "expected tuple");
  }
  frame->pushValue(*function);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildSlice(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject step = arg == 3 ? frame->popValue() : NoneType::object();
  RawObject stop = frame->popValue();
  RawObject start = frame->topValue();
  Runtime* runtime = thread->runtime();
  if (start.isNoneType() && stop.isNoneType() && step.isNoneType()) {
    frame->setTopValue(runtime->emptySlice());
  } else {
    HandleScope scope(thread);
    Object start_obj(&scope, start);
    Object stop_obj(&scope, stop);
    Object step_obj(&scope, step);
    frame->setTopValue(runtime->newSlice(start_obj, stop_obj, step_obj));
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadClosure(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawCode code = Code::cast(frame->code());
  frame->pushValue(frame->local(code.nlocals() + arg));
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
  frame->pushValue(*value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreDeref(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawCode code = Code::cast(frame->code());
  Cell::cast(frame->local(code.nlocals() + arg)).setValue(frame->popValue());
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
  return handleCall(thread, arg, arg + 1, 0, prepareKeywordCall,
                    &Function::entryKw);
}

HANDLER_INLINE Continue Interpreter::doCallFunctionEx(Thread* thread,
                                                      word arg) {
  Frame* caller_frame = thread->currentFrame();
  word callable_idx = (arg & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1;
  RawObject* post_call_sp = caller_frame->valueStackTop() + callable_idx + 1;
  HandleScope scope(thread);
  Object callable(&scope,
                  prepareCallableEx(thread, caller_frame, callable_idx));
  if (callable.isErrorException()) return Continue::UNWIND;

  Function function(&scope, *callable);
  if (!function.isInterpreted()) {
    return callTrampoline(thread, function.entryEx(), arg, post_call_sp);
  }

  if (prepareExplodeCall(thread, *function, caller_frame, arg)
          .isErrorException()) {
    return Continue::UNWIND;
  }

  bool has_freevars_or_cellvars = function.hasFreevarsOrCellvars();
  Frame* callee_frame = thread->pushCallFrame(*function);
  if (UNLIKELY(callee_frame == nullptr)) {
    return Continue::UNWIND;
  }
  caller_frame->setValueStackTop(post_call_sp);
  if (has_freevars_or_cellvars) {
    processFreevarsAndCellvars(thread, callee_frame);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doSetupWith(Thread* thread, word arg) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Frame* frame = thread->currentFrame();
  Object mgr(&scope, frame->topValue());
  Object enter(&scope, lookupMethod(thread, frame, mgr, ID(__enter__)));
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
  Object exit(&scope, lookupMethod(thread, frame, mgr, ID(__exit__)));
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
  Object exit_bound(&scope, runtime->newBoundMethod(exit, mgr));
  frame->setTopValue(*exit_bound);
  Object result(&scope, callMethod1(thread, frame, enter, mgr));
  if (result.isErrorException()) return Continue::UNWIND;

  word stack_depth = frame->valueStackBase() - frame->valueStackTop();
  BlockStack* block_stack = frame->blockStack();
  word handler_pc = frame->virtualPC() + arg;
  block_stack->push(TryBlock(TryBlock::kFinally, handler_pc, stack_depth));
  frame->pushValue(*result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doListAppend(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object value(&scope, frame->popValue());
  List list(&scope, frame->peek(arg - 1));
  thread->runtime()->listAdd(thread, list, value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doSetAdd(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object value(&scope, frame->popValue());
  Object hash_obj(&scope, hash(thread, value));
  if (hash_obj.isErrorException()) {
    return Continue::UNWIND;
  }
  word hash = SmallInt::cast(*hash_obj).value();
  Set set(&scope, Set::cast(frame->peek(arg - 1)));
  setAdd(thread, set, value, hash);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doMapAdd(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object key(&scope, frame->popValue());
  Object value(&scope, frame->popValue());
  Dict dict(&scope, Dict::cast(frame->peek(arg - 1)));
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) return Continue::UNWIND;
  word hash = SmallInt::cast(*hash_obj).value();
  dictAtPut(thread, dict, key, hash, value);
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
    result = moduleAt(thread, module, name);
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
    frame->pushValue(cell.value());
  } else {
    frame->pushValue(*result);
  }

  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildListUnpack(Thread* thread,
                                                       word arg) {
  Frame* frame = thread->currentFrame();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  List list(&scope, runtime->newList());
  Object obj(&scope, NoneType::object());
  for (word i = arg - 1; i >= 0; i--) {
    obj = frame->peek(i);
    RawObject result =
        thread->invokeMethodStatic2(LayoutId::kList, ID(extend), list, obj);
    if (result.isErrorException()) return Continue::UNWIND;
  }
  frame->dropValues(arg - 1);
  frame->setTopValue(*list);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildMapUnpack(Thread* thread,
                                                      word arg) {
  Frame* frame = thread->currentFrame();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Dict dict(&scope, runtime->newDict());
  Object obj(&scope, NoneType::object());
  for (word i = arg - 1; i >= 0; i--) {
    obj = frame->peek(i);
    if (dictMergeOverride(thread, dict, obj).isErrorException()) {
      if (thread->pendingExceptionType() ==
          runtime->typeAt(LayoutId::kAttributeError)) {
        // TODO(bsimmers): Include type name once we have a better formatter.
        thread->clearPendingException();
        thread->raiseWithFmt(LayoutId::kTypeError, "object is not a mapping");
      }
      return Continue::UNWIND;
    }
  }
  frame->dropValues(arg - 1);
  frame->setTopValue(*dict);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildMapUnpackWithCall(Thread* thread,
                                                              word arg) {
  Frame* frame = thread->currentFrame();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Dict dict(&scope, runtime->newDict());
  Object obj(&scope, NoneType::object());
  for (word i = arg - 1; i >= 0; i--) {
    obj = frame->peek(i);
    if (dictMergeError(thread, dict, obj).isErrorException()) {
      if (thread->pendingExceptionType() ==
          runtime->typeAt(LayoutId::kAttributeError)) {
        thread->clearPendingException();
        thread->raiseWithFmt(LayoutId::kTypeError, "object is not a mapping");
      } else if (thread->pendingExceptionType() ==
                 runtime->typeAt(LayoutId::kKeyError)) {
        Object value(&scope, thread->pendingExceptionValue());
        thread->clearPendingException();
        // TODO(bsimmers): Make these error messages more informative once
        // we have a better formatter.
        if (runtime->isInstanceOfStr(*value)) {
          thread->raiseWithFmt(LayoutId::kTypeError,
                               "got multiple values for keyword argument");
        } else {
          thread->raiseWithFmt(LayoutId::kTypeError,
                               "keywords must be strings");
        }
      }
      return Continue::UNWIND;
    }
  }
  frame->dropValues(arg - 1);
  frame->setTopValue(*dict);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildTupleUnpack(Thread* thread,
                                                        word arg) {
  Frame* frame = thread->currentFrame();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  List list(&scope, runtime->newList());
  Object obj(&scope, NoneType::object());
  for (word i = arg - 1; i >= 0; i--) {
    obj = frame->peek(i);
    RawObject result =
        thread->invokeMethodStatic2(LayoutId::kList, ID(extend), list, obj);
    if (result.isErrorException()) return Continue::UNWIND;
  }
  Tuple items(&scope, list.items());
  Tuple tuple(&scope, runtime->tupleSubseq(thread, items, 0, list.numItems()));
  frame->dropValues(arg - 1);
  frame->setTopValue(*tuple);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildSetUnpack(Thread* thread,
                                                      word arg) {
  Frame* frame = thread->currentFrame();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Set set(&scope, runtime->newSet());
  Object obj(&scope, NoneType::object());
  for (word i = 0; i < arg; i++) {
    obj = frame->peek(i);
    if (setUpdate(thread, set, obj).isErrorException()) return Continue::UNWIND;
  }
  frame->dropValues(arg - 1);
  frame->setTopValue(*set);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doSetupAsyncWith(Thread* thread,
                                                      word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object result(&scope, frame->popValue());
  word stack_depth = frame->valueStackSize();
  BlockStack* block_stack = frame->blockStack();
  word handler_pc = frame->virtualPC() + arg;
  block_stack->push(TryBlock(TryBlock::kFinally, handler_pc, stack_depth));
  frame->pushValue(*result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doFormatValue(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object fmt_spec(&scope, Str::empty());
  if (arg & kFormatValueHasSpecBit) {
    fmt_spec = frame->popValue();
  }
  Object value(&scope, frame->popValue());
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
  frame->pushValue(*value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildConstKeyMap(Thread* thread,
                                                        word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Tuple keys(&scope, frame->popValue());
  Dict dict(&scope, thread->runtime()->newDictWithSize(keys.length()));
  Object key(&scope, NoneType::object());
  Object hash_obj(&scope, NoneType::object());
  for (word i = arg - 1; i >= 0; i--) {
    key = keys.at(i);
    hash_obj = Interpreter::hash(thread, key);
    if (hash_obj.isErrorException()) return Continue::UNWIND;
    word hash = SmallInt::cast(*hash_obj).value();
    Object value(&scope, frame->popValue());
    dictAtPut(thread, dict, key, hash, value);
  }
  frame->pushValue(*dict);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildString(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  switch (arg) {
    case 0:  // empty
      frame->pushValue(Str::empty());
      break;
    case 1:  // no-op
      break;
    default: {  // concat
      RawObject res = stringJoin(thread, frame->valueStackTop(), arg);
      frame->dropValues(arg - 1);
      frame->setTopValue(res);
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
  Frame* frame = thread->currentFrame();
  frame->insertValueAt(Unbound::object(), 1);
  return doLoadAttr(thread, arg);
}

Continue Interpreter::loadMethodUpdateCache(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  word original_arg = originalArg(frame->function(), arg);
  Object receiver(&scope, frame->topValue());
  Str name(&scope,
           Tuple::cast(Code::cast(frame->code()).names()).at(original_arg));

  Object location(&scope, NoneType::object());
  LoadAttrKind kind;
  Object result(&scope,
                loadAttrSetLocation(thread, receiver, name, &kind, &location));
  if (result.isErrorException()) return Continue::UNWIND;
  if (kind != LoadAttrKind::kInstanceFunction) {
    frame->pushValue(*result);
    frame->setValueAt(Unbound::object(), 1);
    return Continue::NEXT;
  }

  // Cache the attribute load.
  Tuple caches(&scope, frame->caches());
  Function dependent(&scope, frame->function());
  ICState next_ic_state = icUpdateAttr(thread, caches, arg, receiver.layoutId(),
                                       location, name, dependent);

  word pc = frame->currentPC();
  RawMutableBytes bytecode = frame->bytecode();
  switch (next_ic_state) {
    case ICState::kMonomorphic:
      bytecode.byteAtPut(pc, LOAD_METHOD_INSTANCE_FUNCTION);
      break;
    case ICState::kPolymorphic:
      bytecode.byteAtPut(pc, LOAD_METHOD_POLYMORPHIC);
      break;
    case ICState::kAnamorphic:
      UNREACHABLE("next_ic_state cannot be anamorphic");
      break;
  }
  frame->insertValueAt(*location, 1);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadMethodAnamorphic(Thread* thread,
                                                            word arg) {
  return loadMethodUpdateCache(thread, arg);
}

HANDLER_INLINE Continue
Interpreter::doLoadMethodInstanceFunction(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawTuple caches = frame->caches();
  RawObject receiver = frame->topValue();
  bool is_found;
  RawObject cached =
      icLookupMonomorphic(caches, arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    return loadMethodUpdateCache(thread, arg);
  }
  DCHECK(cached.isFunction(), "cached is expected to be a function");
  frame->insertValueAt(cached, 1);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadMethodPolymorphic(Thread* thread,
                                                             word arg) {
  Frame* frame = thread->currentFrame();
  RawObject receiver = frame->topValue();
  bool is_found;
  RawObject cached =
      icLookupPolymorphic(frame->caches(), arg, receiver.layoutId(), &is_found);
  if (!is_found) {
    return loadMethodUpdateCache(thread, arg);
  }
  DCHECK(cached.isFunction(), "cached is expected to be a function");
  frame->insertValueAt(cached, 1);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doCallMethod(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject maybe_method = frame->peek(arg + 1);
  if (maybe_method.isUnbound()) {
    // Need to pop the extra Unbound.
    return handleCall(thread, arg, arg, 1, preparePositionalCall,
                      &Function::entry);
  }
  // Add one to bind receiver to the self argument. See doLoadMethod()
  // for details on the stack's shape.
  return tailcallPreparedFunction(thread, frame, maybe_method, arg + 1);
}

HANDLER_INLINE Continue Interpreter::cachedBinaryOpImpl(
    Thread* thread, word arg, OpcodeHandler update_cache,
    BinaryOpFallbackHandler fallback) {
  Frame* frame = thread->currentFrame();
  RawObject left_raw = frame->peek(1);
  RawObject right_raw = frame->peek(0);
  LayoutId left_layout_id = left_raw.layoutId();
  LayoutId right_layout_id = right_raw.layoutId();
  BinaryOpFlags flags = kBinaryOpNone;
  RawObject method = icLookupBinOpPolymorphic(
      frame->caches(), arg, left_layout_id, right_layout_id, &flags);
  if (method.isErrorNotFound()) {
    return update_cache(thread, arg);
  }

  // Fast-path: Call cached method and return if possible.
  RawObject result = binaryOperationWithMethod(thread, frame, method, flags,
                                               left_raw, right_raw);
  if (result.isErrorException()) return Continue::UNWIND;
  if (!result.isNotImplementedType()) {
    frame->dropValues(1);
    frame->setTopValue(result);
    return Continue::NEXT;
  }

  return fallback(thread, arg, flags);
}

Continue Interpreter::compareOpUpdateCache(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object right(&scope, frame->popValue());
  Object left(&scope, frame->popValue());
  Function function(&scope, frame->function());
  CompareOp op = static_cast<CompareOp>(originalArg(*function, arg));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  RawObject result = compareOperationSetMethod(thread, frame, op, left, right,
                                               &method, &flags);
  if (result.isErrorException()) return Continue::UNWIND;
  if (!method.isNoneType()) {
    Tuple caches(&scope, frame->caches());
    LayoutId left_layout_id = left.layoutId();
    LayoutId right_layout_id = right.layoutId();
    ICState next_cache_state = icUpdateBinOp(
        thread, caches, arg, left_layout_id, right_layout_id, method, flags);
    icInsertCompareOpDependencies(thread, function, left_layout_id,
                                  right_layout_id, op);
    word pc = frame->currentPC();
    RawMutableBytes bytecode = frame->bytecode();
    bytecode.byteAtPut(pc, next_cache_state == ICState::kMonomorphic
                               ? COMPARE_OP_MONOMORPHIC
                               : COMPARE_OP_POLYMORPHIC);
  }
  frame->pushValue(result);
  return Continue::NEXT;
}

Continue Interpreter::compareOpFallback(Thread* thread, word arg,
                                        BinaryOpFlags flags) {
  // Slow-path: We may need to call the reversed op when the first method
  // returned `NotImplemented`.
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  CompareOp op = static_cast<CompareOp>(originalArg(frame->function(), arg));
  Object right(&scope, frame->popValue());
  Object left(&scope, frame->popValue());
  Object result(&scope,
                compareOperationRetry(thread, frame, op, flags, left, right));
  if (result.isErrorException()) return Continue::UNWIND;
  frame->pushValue(*result);
  return Continue::NEXT;
}

HANDLER_INLINE
Continue Interpreter::doCompareEqSmallInt(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left = frame->peek(1);
  RawObject right = frame->peek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    frame->dropValues(1);
    frame->setTopValue(Bool::fromBool(left_value == right_value));
    return Continue::NEXT;
  }
  return compareOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doCompareGtSmallInt(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left = frame->peek(1);
  RawObject right = frame->peek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    frame->dropValues(1);
    frame->setTopValue(Bool::fromBool(left_value > right_value));
    return Continue::NEXT;
  }
  return compareOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doCompareLtSmallInt(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left = frame->peek(1);
  RawObject right = frame->peek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    frame->dropValues(1);
    frame->setTopValue(Bool::fromBool(left_value < right_value));
    return Continue::NEXT;
  }
  return compareOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doCompareGeSmallInt(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left = frame->peek(1);
  RawObject right = frame->peek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    frame->dropValues(1);
    frame->setTopValue(Bool::fromBool(left_value >= right_value));
    return Continue::NEXT;
  }
  return compareOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doCompareNeSmallInt(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left = frame->peek(1);
  RawObject right = frame->peek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    frame->dropValues(1);
    frame->setTopValue(Bool::fromBool(left_value != right_value));
    return Continue::NEXT;
  }
  return compareOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doCompareLeSmallInt(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left = frame->peek(1);
  RawObject right = frame->peek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    frame->dropValues(1);
    frame->setTopValue(Bool::fromBool(left_value <= right_value));
    return Continue::NEXT;
  }
  return compareOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doCompareEqStr(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left = frame->peek(1);
  RawObject right = frame->peek(0);
  if (left.isStr() && right.isStr()) {
    frame->dropValues(1);
    frame->setTopValue(Bool::fromBool(Str::cast(left).equals(right)));
    return Continue::NEXT;
  }
  return compareOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doCompareOpMonomorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left_raw = frame->peek(1);
  RawObject right_raw = frame->peek(0);
  LayoutId left_layout_id = left_raw.layoutId();
  LayoutId right_layout_id = right_raw.layoutId();
  BinaryOpFlags flags = kBinaryOpNone;
  RawObject method = icLookupBinOpMonomorphic(
      frame->caches(), arg, left_layout_id, right_layout_id, &flags);
  if (method.isErrorNotFound()) {
    return compareOpUpdateCache(thread, arg);
  }
  return binaryOp(thread, arg, method, flags, left_raw, right_raw,
                  compareOpFallback);
}

HANDLER_INLINE
Continue Interpreter::doCompareOpPolymorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left_raw = frame->peek(1);
  RawObject right_raw = frame->peek(0);
  LayoutId left_layout_id = left_raw.layoutId();
  LayoutId right_layout_id = right_raw.layoutId();
  BinaryOpFlags flags = kBinaryOpNone;
  RawObject method = icLookupBinOpPolymorphic(
      frame->caches(), arg, left_layout_id, right_layout_id, &flags);
  if (method.isErrorNotFound()) {
    return compareOpUpdateCache(thread, arg);
  }
  return binaryOp(thread, arg, method, flags, left_raw, right_raw,
                  compareOpFallback);
}

HANDLER_INLINE
Continue Interpreter::doCompareOpAnamorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left = frame->peek(1);
  RawObject right = frame->peek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word pc = frame->currentPC();
    RawMutableBytes bytecode = frame->bytecode();
    switch (static_cast<CompareOp>(originalArg(frame->function(), arg))) {
      case CompareOp::EQ:
        bytecode.byteAtPut(pc, COMPARE_EQ_SMALLINT);
        return doCompareEqSmallInt(thread, arg);
      case CompareOp::GT:
        bytecode.byteAtPut(pc, COMPARE_GT_SMALLINT);
        return doCompareGtSmallInt(thread, arg);
      case CompareOp::LT:
        bytecode.byteAtPut(pc, COMPARE_LT_SMALLINT);
        return doCompareLtSmallInt(thread, arg);
      case CompareOp::GE:
        bytecode.byteAtPut(pc, COMPARE_GE_SMALLINT);
        return doCompareGeSmallInt(thread, arg);
      case CompareOp::NE:
        bytecode.byteAtPut(pc, COMPARE_NE_SMALLINT);
        return doCompareNeSmallInt(thread, arg);
      case CompareOp::LE:
        bytecode.byteAtPut(pc, COMPARE_LE_SMALLINT);
        return doCompareLeSmallInt(thread, arg);
      default:
        return compareOpUpdateCache(thread, arg);
    }
  }
  if (left.isStr() && right.isStr() &&
      static_cast<CompareOp>(originalArg(frame->function(), arg)) ==
          CompareOp::EQ) {
    word pc = frame->currentPC();
    RawMutableBytes bytecode = frame->bytecode();
    bytecode.byteAtPut(pc, COMPARE_EQ_STR);
    return doCompareEqStr(thread, arg);
  }
  return compareOpUpdateCache(thread, arg);
}

Continue Interpreter::inplaceOpUpdateCache(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object right(&scope, frame->popValue());
  Object left(&scope, frame->popValue());
  Function function(&scope, frame->function());
  BinaryOp op = static_cast<BinaryOp>(originalArg(*function, arg));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  RawObject result = inplaceOperationSetMethod(thread, frame, op, left, right,
                                               &method, &flags);
  if (!method.isNoneType()) {
    Tuple caches(&scope, frame->caches());
    LayoutId left_layout_id = left.layoutId();
    LayoutId right_layout_id = right.layoutId();
    ICState next_cache_state = icUpdateBinOp(
        thread, caches, arg, left_layout_id, right_layout_id, method, flags);
    icInsertInplaceOpDependencies(thread, function, left_layout_id,
                                  right_layout_id, op);
    word pc = frame->currentPC();
    RawMutableBytes bytecode = frame->bytecode();
    bytecode.byteAtPut(pc, next_cache_state == ICState::kMonomorphic
                               ? INPLACE_OP_MONOMORPHIC
                               : INPLACE_OP_POLYMORPHIC);
  }
  if (result.isErrorException()) return Continue::UNWIND;
  frame->pushValue(result);
  return Continue::NEXT;
}

Continue Interpreter::inplaceOpFallback(Thread* thread, word arg,
                                        BinaryOpFlags flags) {
  // Slow-path: We may need to try other ways to resolve things when the first
  // call returned `NotImplemented`.
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  BinaryOp op = static_cast<BinaryOp>(originalArg(frame->function(), arg));
  Object right(&scope, frame->popValue());
  Object left(&scope, frame->popValue());
  Object result(&scope, NoneType::object());
  if (flags & kInplaceBinaryOpRetry) {
    // The cached operation was an in-place operation we have to try to the
    // usual binary operation mechanics now.
    result = binaryOperation(thread, frame, op, left, right);
  } else {
    // The cached operation was already a binary operation (e.g. __add__ or
    // __radd__) so we have to invoke `binaryOperationRetry`.
    result = binaryOperationRetry(thread, frame, op, flags, left, right);
  }
  if (result.isErrorException()) return Continue::UNWIND;
  frame->pushValue(*result);
  return Continue::NEXT;
}

HANDLER_INLINE
Continue Interpreter::doInplaceOpMonomorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left_raw = frame->peek(1);
  RawObject right_raw = frame->peek(0);
  LayoutId left_layout_id = left_raw.layoutId();
  LayoutId right_layout_id = right_raw.layoutId();
  BinaryOpFlags flags = kBinaryOpNone;
  RawObject method = icLookupBinOpMonomorphic(
      frame->caches(), arg, left_layout_id, right_layout_id, &flags);
  if (method.isErrorNotFound()) {
    return inplaceOpUpdateCache(thread, arg);
  }
  return binaryOp(thread, arg, method, flags, left_raw, right_raw,
                  inplaceOpFallback);
}

HANDLER_INLINE
Continue Interpreter::doInplaceOpPolymorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left_raw = frame->peek(1);
  RawObject right_raw = frame->peek(0);
  LayoutId left_layout_id = left_raw.layoutId();
  LayoutId right_layout_id = right_raw.layoutId();
  BinaryOpFlags flags = kBinaryOpNone;
  RawObject method = icLookupBinOpPolymorphic(
      frame->caches(), arg, left_layout_id, right_layout_id, &flags);
  if (method.isErrorNotFound()) {
    return inplaceOpUpdateCache(thread, arg);
  }
  return binaryOp(thread, arg, method, flags, left_raw, right_raw,
                  inplaceOpFallback);
}

HANDLER_INLINE
Continue Interpreter::doInplaceAddSmallInt(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left = frame->peek(1);
  RawObject right = frame->peek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    word result_value = left_value + right_value;
    if (SmallInt::isValid(result_value)) {
      frame->dropValues(1);
      frame->setTopValue(SmallInt::fromWord(result_value));
      return Continue::NEXT;
    }
  }
  return inplaceOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doInplaceOpAnamorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  if (frame->peek(0).isSmallInt() && frame->peek(1).isSmallInt()) {
    word pc = frame->currentPC();
    RawMutableBytes bytecode = frame->bytecode();
    switch (static_cast<BinaryOp>(originalArg(frame->function(), arg))) {
      case BinaryOp::ADD:
        bytecode.byteAtPut(pc, INPLACE_ADD_SMALLINT);
        return doInplaceAddSmallInt(thread, arg);
      default:
        return inplaceOpUpdateCache(thread, arg);
    }
  }
  return inplaceOpUpdateCache(thread, arg);
}

Continue Interpreter::binaryOpUpdateCache(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object right(&scope, frame->popValue());
  Object left(&scope, frame->popValue());
  Function function(&scope, frame->function());
  BinaryOp op = static_cast<BinaryOp>(originalArg(*function, arg));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  Object result(&scope, binaryOperationSetMethod(thread, frame, op, left, right,
                                                 &method, &flags));
  if (!method.isNoneType()) {
    Tuple caches(&scope, frame->caches());
    LayoutId left_layout_id = left.layoutId();
    LayoutId right_layout_id = right.layoutId();
    ICState next_cache_state = icUpdateBinOp(
        thread, caches, arg, left_layout_id, right_layout_id, method, flags);
    icInsertBinaryOpDependencies(thread, function, left_layout_id,
                                 right_layout_id, op);
    word pc = frame->currentPC();
    RawMutableBytes bytecode = frame->bytecode();
    bytecode.byteAtPut(pc, next_cache_state == ICState::kMonomorphic
                               ? BINARY_OP_MONOMORPHIC
                               : BINARY_OP_POLYMORPHIC);
  }
  if (result.isErrorException()) return Continue::UNWIND;
  frame->pushValue(*result);
  return Continue::NEXT;
}

Continue Interpreter::binaryOpFallback(Thread* thread, word arg,
                                       BinaryOpFlags flags) {
  // Slow-path: We may need to call the reversed op when the first method
  // returned `NotImplemented`.
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  BinaryOp op = static_cast<BinaryOp>(originalArg(frame->function(), arg));
  Object right(&scope, frame->popValue());
  Object left(&scope, frame->popValue());
  Object result(&scope,
                binaryOperationRetry(thread, frame, op, flags, left, right));
  if (result.isErrorException()) return Continue::UNWIND;
  frame->pushValue(*result);
  return Continue::NEXT;
}

ALWAYS_INLINE Continue Interpreter::binaryOp(Thread* thread, word arg,
                                             RawObject method,
                                             BinaryOpFlags flags,
                                             RawObject left, RawObject right,
                                             BinaryOpFallbackHandler fallback) {
  DCHECK(method.isFunction(), "method is expected to be a function");
  Frame* frame = thread->currentFrame();
  RawObject result =
      binaryOperationWithMethod(thread, frame, method, flags, left, right);
  if (result.isErrorException()) return Continue::UNWIND;
  if (!result.isNotImplementedType()) {
    frame->dropValues(1);
    frame->setTopValue(result);
    return Continue::NEXT;
  }
  return fallback(thread, arg, flags);
}

HANDLER_INLINE
Continue Interpreter::doBinaryOpMonomorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left_raw = frame->peek(1);
  RawObject right_raw = frame->peek(0);
  LayoutId left_layout_id = left_raw.layoutId();
  LayoutId right_layout_id = right_raw.layoutId();
  BinaryOpFlags flags = kBinaryOpNone;
  RawObject method = icLookupBinOpMonomorphic(
      frame->caches(), arg, left_layout_id, right_layout_id, &flags);
  if (method.isErrorNotFound()) {
    return binaryOpUpdateCache(thread, arg);
  }
  return binaryOp(thread, arg, method, flags, left_raw, right_raw,
                  binaryOpFallback);
}

HANDLER_INLINE
Continue Interpreter::doBinaryOpPolymorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left_raw = frame->peek(1);
  RawObject right_raw = frame->peek(0);
  LayoutId left_layout_id = left_raw.layoutId();
  LayoutId right_layout_id = right_raw.layoutId();
  BinaryOpFlags flags = kBinaryOpNone;
  RawObject method = icLookupBinOpPolymorphic(
      frame->caches(), arg, left_layout_id, right_layout_id, &flags);
  if (method.isErrorNotFound()) {
    return binaryOpUpdateCache(thread, arg);
  }
  return binaryOp(thread, arg, method, flags, left_raw, right_raw,
                  binaryOpFallback);
}

HANDLER_INLINE
Continue Interpreter::doBinaryAddSmallInt(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left = frame->peek(1);
  RawObject right = frame->peek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    word result_value = left_value + right_value;
    if (SmallInt::isValid(result_value)) {
      frame->dropValues(1);
      frame->setTopValue(SmallInt::fromWord(result_value));
      return Continue::NEXT;
    }
  }
  return binaryOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doBinaryAndSmallInt(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left = frame->peek(1);
  RawObject right = frame->peek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    word result_value = left_value & right_value;
    DCHECK(SmallInt::isValid(result_value), "result should be a SmallInt");
    frame->dropValues(1);
    frame->setTopValue(SmallInt::fromWord(result_value));
    return Continue::NEXT;
  }
  return binaryOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doBinaryFloordivSmallInt(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left = frame->peek(1);
  RawObject right = frame->peek(0);
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
    frame->dropValues(1);
    frame->setTopValue(SmallInt::fromWord(result_value));
    return Continue::NEXT;
  }
  return binaryOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doBinarySubSmallInt(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left = frame->peek(1);
  RawObject right = frame->peek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    word result_value = left_value - right_value;
    if (SmallInt::isValid(result_value)) {
      frame->dropValues(1);
      frame->setTopValue(SmallInt::fromWord(result_value));
      return Continue::NEXT;
    }
  }
  return binaryOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doBinaryOrSmallInt(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject left = frame->peek(1);
  RawObject right = frame->peek(0);
  if (left.isSmallInt() && right.isSmallInt()) {
    word left_value = SmallInt::cast(left).value();
    word right_value = SmallInt::cast(right).value();
    word result_value = left_value | right_value;
    if (SmallInt::isValid(result_value)) {
      frame->dropValues(1);
      frame->setTopValue(SmallInt::fromWord(result_value));
      return Continue::NEXT;
    }
  }
  return binaryOpUpdateCache(thread, arg);
}

HANDLER_INLINE
Continue Interpreter::doBinaryOpAnamorphic(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  if (frame->peek(0).isSmallInt() && frame->peek(1).isSmallInt()) {
    word pc = frame->currentPC();
    RawMutableBytes bytecode = frame->bytecode();
    switch (static_cast<BinaryOp>(originalArg(frame->function(), arg))) {
      case BinaryOp::ADD:
        bytecode.byteAtPut(pc, BINARY_ADD_SMALLINT);
        return doBinaryAddSmallInt(thread, arg);
      case BinaryOp::AND:
        bytecode.byteAtPut(pc, BINARY_AND_SMALLINT);
        return doBinaryAndSmallInt(thread, arg);
      case BinaryOp::FLOORDIV:
        bytecode.byteAtPut(pc, BINARY_FLOORDIV_SMALLINT);
        return doBinaryFloordivSmallInt(thread, arg);
      case BinaryOp::SUB:
        bytecode.byteAtPut(pc, BINARY_SUB_SMALLINT);
        return doBinarySubSmallInt(thread, arg);
      case BinaryOp::OR:
        bytecode.byteAtPut(pc, BINARY_OR_SMALLINT);
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
  // Generator ended with return.
  generator_frame.setVirtualPC(Frame::kFinishedGeneratorPC);
  if (result.isErrorException()) return *result;
  return thread->raise(LayoutId::kStopIteration, *result);
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
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  Frame* frame = thread->pushGeneratorFrame(generator_frame);
  if (frame == nullptr) {
    return Error::exception();
  }
  if (pc != 0) {
    frame->pushValue(*send_value);
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

  // TODO(T38009294): Improve the compiler to avoid this exception state
  // overhead on every generator entry.
  ExceptionState exc_state(&scope, generator.exceptionState());
  exc_state.setPrevious(thread->caughtExceptionState());
  thread->setCaughtExceptionState(*exc_state);
  thread->setPendingExceptionType(*type);
  thread->setPendingExceptionValue(*value);
  thread->setPendingExceptionTraceback(*traceback);
  if (unwind(thread, frame)) {
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

namespace {

class CppInterpreter : public Interpreter {
 public:
  ~CppInterpreter() override;
  void setupThread(Thread* thread) override;

 private:
  static RawObject interpreterLoop(Thread* thread);
};

CppInterpreter::~CppInterpreter() {}

void CppInterpreter::setupThread(Thread* thread) {
  thread->setInterpreterFunc(interpreterLoop);
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

  Frame* entry_frame = thread->currentFrame();
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
    if (unwind(thread, entry_frame)) {
      return Error::exception();
    }
  } else if (cont == Continue::RETURN) {
    RawObject result = handleReturn(thread, entry_frame);
    if (!result.isErrorError()) {
      return result;
    }
  } else {
    DCHECK(cont == Continue::YIELD, "expected RETURN, UNWIND or YIELD");
    return thread->currentFrame()->popValue();
  }
  goto* next_label();
#pragma GCC diagnostic pop
}

}  // namespace

Interpreter* createCppInterpreter() { return new CppInterpreter; }

}  // namespace py
