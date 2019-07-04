#include "interpreter.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>

#include "builtins-module.h"
#include "dict-builtins.h"
#include "exception-builtins.h"
#include "frame.h"
#include "generator-builtins.h"
#include "ic.h"
#include "int-builtins.h"
#include "list-builtins.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "trampolines.h"
#include "tuple-builtins.h"
#include "type-builtins.h"
#include "utils.h"

namespace python {

using Continue = Interpreter::Continue;

// We want opcode handlers inlined into the interpreter in optimized builds.
// Keep them outlined for nicer debugging in debug builds.
#ifdef NDEBUG
#define HANDLER_INLINE ALWAYS_INLINE
#else
#define HANDLER_INLINE __attribute__((noinline))
#endif

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

    // TODO(T44238481): Look into using lookupMethod() once it's fixed.
    Type type(&scope, runtime->typeOf(**callable));
    Object dunder_call(
        &scope, typeLookupSymbolInMro(thread, type, SymbolId::kDunderCall));
    if (!dunder_call.isError()) {
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
        if (callable->isError()) return **callable;
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

HANDLER_INLINE USED RawObject Interpreter::prepareCallableCall(
    Thread* thread, Frame* frame, word callable_idx, word* nargs) {
  RawObject callable = frame->peek(callable_idx);
  if (callable.isFunction()) {
    return callable;
  }

  if (callable.isBoundMethod()) {
    RawBoundMethod method = BoundMethod::cast(callable);
    RawObject method_function = method.function();
    frame->setValueAt(method_function, callable_idx);
    frame->insertValueAt(method.self(), callable_idx);
    *nargs += 1;
    return method_function;
  }
  return prepareCallableCallDunderCall(thread, frame, callable_idx, nargs);
}

NEVER_INLINE
RawObject Interpreter::prepareCallableCallDunderCall(Thread* thread,
                                                     Frame* frame,
                                                     word callable_idx,
                                                     word* nargs) {
  HandleScope scope(thread);
  Object callable(&scope, frame->peek(callable_idx));
  Object self(&scope, NoneType::object());
  RawObject result = prepareCallable(thread, frame, &callable, &self);
  if (result.isError()) return result;
  frame->setValueAt(*callable, callable_idx);
  if (result == Bool::trueObj()) {
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
    *nargs += 1;
  }
  return *callable;
}

RawObject Interpreter::call(Thread* thread, Frame* frame, word nargs) {
  DCHECK(!thread->hasPendingException(), "unhandled exception lingering");
  RawObject* sp = frame->valueStackTop() + nargs + 1;
  RawObject callable = frame->peek(nargs);
  callable = prepareCallableCall(thread, frame, nargs, &nargs);
  if (callable.isError()) {
    frame->setValueStackTop(sp);
    return callable;
  }
  RawObject result = Function::cast(callable).entry()(thread, frame, nargs);
  // Clear the stack of the function object and return.
  frame->setValueStackTop(sp);
  return result;
}

RawObject Interpreter::callKw(Thread* thread, Frame* frame, word nargs) {
  // Top of stack is a tuple of keyword argument names in the order they
  // appear on the stack.
  RawObject* sp = frame->valueStackTop() + nargs + 2;
  RawObject result;
  RawObject callable = frame->peek(nargs + 1);
  callable = prepareCallableCall(thread, frame, nargs + 1, &nargs);
  if (callable.isError()) {
    frame->setValueStackTop(sp);
    return callable;
  }
  result = Function::cast(callable).entryKw()(thread, frame, nargs);
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
  if (callable.isError()) return *callable;
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
    args_obj = sequenceAsTuple(thread, args_obj);
    if (args_obj.isError()) return *args_obj;
    frame->setValueAt(*args_obj, args_idx);
  }
  if (!callable.isFunction()) {
    Object self(&scope, NoneType::object());
    Object result(&scope, prepareCallable(thread, frame, &callable, &self));
    if (result.isError()) return *result;
    frame->setValueAt(*callable, callable_idx);

    if (result == Bool::trueObj()) {
      // Create a new argument tuple with self as the first argument
      Tuple args(&scope, *args_obj);
      Tuple new_args(&scope, thread->runtime()->newTuple(args.length() + 1));
      new_args.atPut(0, *self);
      new_args.replaceFromWith(1, *args);
      frame->setValueAt(*new_args, args_idx);
    }
  }
  return *callable;
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
  LargeStr result(&scope, thread->runtime()->heap()->createLargeStr(new_len));
  word offset = RawLargeStr::kDataOffset;
  for (word i = num - 1; i >= 0; i--) {
    RawStr str = Str::cast(sp[i]);
    word len = str.length();
    str.copyTo(reinterpret_cast<byte*>(result.address() + offset), len);
    offset += len;
  }
  return *result;
}

RawObject Interpreter::callDescriptorGet(Thread* thread, Frame* caller,
                                         const Object& descriptor,
                                         const Object& receiver,
                                         const Object& receiver_type) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type descriptor_type(&scope, runtime->typeOf(*descriptor));
  Object method(&scope, typeLookupSymbolInMro(thread, descriptor_type,
                                              SymbolId::kDunderGet));
  DCHECK(!method.isError(), "no __get__ method found");
  return callMethod3(thread, caller, method, descriptor, receiver,
                     receiver_type);
}

RawObject Interpreter::callDescriptorSet(Thread* thread, Frame* caller,
                                         const Object& descriptor,
                                         const Object& receiver,
                                         const Object& value) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type descriptor_type(&scope, runtime->typeOf(*descriptor));
  Object method(&scope, typeLookupSymbolInMro(thread, descriptor_type,
                                              SymbolId::kDunderSet));
  DCHECK(!method.isError(), "no __set__ method found");
  return callMethod3(thread, caller, method, descriptor, receiver, value);
}

RawObject Interpreter::callDescriptorDelete(Thread* thread, Frame* caller,
                                            const Object& descriptor,
                                            const Object& receiver) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type descriptor_type(&scope, runtime->typeOf(*descriptor));
  Object method(&scope, typeLookupSymbolInMro(thread, descriptor_type,
                                              SymbolId::kDunderDelete));
  DCHECK(!method.isError(), "no __delete__ method found");
  return callMethod2(thread, caller, method, descriptor, receiver);
}

RawObject Interpreter::lookupMethod(Thread* thread, Frame* /* caller */,
                                    const Object& receiver, SymbolId selector) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(*receiver));
  Object method(&scope, typeLookupSymbolInMro(thread, type, selector));
  if (method.isFunction() || method.isError()) {
    // Do not create a short-lived bound method object, and propagate
    // exceptions.
    return *method;
  }
  return resolveDescriptorGet(thread, method, receiver, type);
}

RawObject Interpreter::callFunction0(Thread* thread, Frame* caller,
                                     const Object& func) {
  caller->pushValue(*func);
  return call(thread, caller, 0);
}

RawObject Interpreter::callFunction1(Thread* thread, Frame* caller,
                                     const Object& func, const Object& arg1) {
  caller->pushValue(*func);
  caller->pushValue(*arg1);
  return call(thread, caller, 1);
}

RawObject Interpreter::callFunction2(Thread* thread, Frame* caller,
                                     const Object& func, const Object& arg1,
                                     const Object& arg2) {
  caller->pushValue(*func);
  caller->pushValue(*arg1);
  caller->pushValue(*arg2);
  return call(thread, caller, 2);
}

RawObject Interpreter::callFunction3(Thread* thread, Frame* caller,
                                     const Object& func, const Object& arg1,
                                     const Object& arg2, const Object& arg3) {
  caller->pushValue(*func);
  caller->pushValue(*arg1);
  caller->pushValue(*arg2);
  caller->pushValue(*arg3);
  return call(thread, caller, 3);
}

RawObject Interpreter::callFunction4(Thread* thread, Frame* caller,
                                     const Object& func, const Object& arg1,
                                     const Object& arg2, const Object& arg3,
                                     const Object& arg4) {
  caller->pushValue(*func);
  caller->pushValue(*arg1);
  caller->pushValue(*arg2);
  caller->pushValue(*arg3);
  caller->pushValue(*arg4);
  return call(thread, caller, 4);
}

RawObject Interpreter::callFunction5(Thread* thread, Frame* caller,
                                     const Object& func, const Object& arg1,
                                     const Object& arg2, const Object& arg3,
                                     const Object& arg4, const Object& arg5) {
  caller->pushValue(*func);
  caller->pushValue(*arg1);
  caller->pushValue(*arg2);
  caller->pushValue(*arg3);
  caller->pushValue(*arg4);
  caller->pushValue(*arg5);
  return call(thread, caller, 5);
}

RawObject Interpreter::callFunction6(Thread* thread, Frame* caller,
                                     const Object& func, const Object& arg1,
                                     const Object& arg2, const Object& arg3,
                                     const Object& arg4, const Object& arg5,
                                     const Object& arg6) {
  caller->pushValue(*func);
  caller->pushValue(*arg1);
  caller->pushValue(*arg2);
  caller->pushValue(*arg3);
  caller->pushValue(*arg4);
  caller->pushValue(*arg5);
  caller->pushValue(*arg6);
  return call(thread, caller, 6);
}

RawObject Interpreter::callFunction(Thread* thread, Frame* caller,
                                    const Object& func, const Tuple& args) {
  caller->pushValue(*func);
  word length = args.length();
  for (word i = 0; i < length; i++) {
    caller->pushValue(args.at(i));
  }
  return call(thread, caller, length);
}

RawObject Interpreter::callMethod1(Thread* thread, Frame* caller,
                                   const Object& method, const Object& self) {
  word nargs = 0;
  caller->pushValue(*method);
  if (method.isFunction()) {
    caller->pushValue(*self);
    nargs += 1;
  }
  return call(thread, caller, nargs);
}

RawObject Interpreter::callMethod2(Thread* thread, Frame* caller,
                                   const Object& method, const Object& self,
                                   const Object& other) {
  word nargs = 1;
  caller->pushValue(*method);
  if (method.isFunction()) {
    caller->pushValue(*self);
    nargs += 1;
  }
  caller->pushValue(*other);
  return call(thread, caller, nargs);
}

RawObject Interpreter::callMethod3(Thread* thread, Frame* caller,
                                   const Object& method, const Object& self,
                                   const Object& arg1, const Object& arg2) {
  word nargs = 2;
  caller->pushValue(*method);
  if (method.isFunction()) {
    caller->pushValue(*self);
    nargs += 1;
  }
  caller->pushValue(*arg1);
  caller->pushValue(*arg2);
  return call(thread, caller, nargs);
}

RawObject Interpreter::callMethod4(Thread* thread, Frame* caller,
                                   const Object& method, const Object& self,
                                   const Object& arg1, const Object& arg2,
                                   const Object& arg3) {
  word nargs = 3;
  caller->pushValue(*method);
  if (method.isFunction()) {
    caller->pushValue(*self);
    nargs += 1;
  }
  caller->pushValue(*arg1);
  caller->pushValue(*arg2);
  caller->pushValue(*arg3);
  return call(thread, caller, nargs);
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
  }
  return doCallFunction(thread, nargs);
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
  }
  frame->pushValue(arg1);
  return doCallFunction(thread, nargs);
}

static RawObject raiseUnaryOpTypeError(Thread* thread, const Object& object,
                                       SymbolId selector) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Str type_name(&scope, Type::cast(runtime->typeOf(*object)).name());
  Str op_name(&scope, runtime->symbols()->at(selector));
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
  if (result.isError()) return Continue::UNWIND;
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
      &scope, typeLookupSymbolInMro(thread, right_type, swapped_selector));
  if (right_reversed_method.isErrorNotFound()) return *right_reversed_method;

  // Python doesn't bother calling the reverse method when the slot on left and
  // right points to the same method. We compare the reverse methods to get
  // close to this behavior.
  Type left_type(&scope, runtime->typeOf(*left));
  Object left_reversed_method(
      &scope, typeLookupSymbolInMro(thread, left_type, swapped_selector));
  if (left_reversed_method == right_reversed_method) {
    return Error::notFound();
  }

  return *right_reversed_method;
}

static RawObject executeAndCacheBinop(Thread* thread, Frame* caller,
                                      const Object& method, IcBinopFlags flags,
                                      const Object& left, const Object& right,
                                      Object* method_out,
                                      IcBinopFlags* flags_out) {
  if (method.isErrorNotFound()) {
    return NotImplementedType::object();
  }

  if (method_out != nullptr) {
    DCHECK(method.isFunction(), "must be a plain function");
    *method_out = *method;
    *flags_out = flags;
    return Interpreter::binaryOperationWithMethod(thread, caller, *method,
                                                  flags, *left, *right);
  }
  if (flags & IC_BINOP_REFLECTED) {
    return Interpreter::callMethod2(thread, caller, method, right, left);
  }
  return Interpreter::callMethod2(thread, caller, method, left, right);
}

RawObject Interpreter::binaryOperationSetMethod(Thread* thread, Frame* caller,
                                                BinaryOp op, const Object& left,
                                                const Object& right,
                                                Object* method_out,
                                                IcBinopFlags* flags_out) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  SymbolId selector = runtime->binaryOperationSelector(op);
  Type left_type(&scope, runtime->typeOf(*left));
  Type right_type(&scope, runtime->typeOf(*right));
  Object left_method(&scope,
                     typeLookupSymbolInMro(thread, left_type, selector));

  // Figure out whether we want to run the normal or the reverse operation
  // first and set `flags` accordingly.
  Object method(&scope, NoneType::object());
  IcBinopFlags flags = IC_BINOP_NONE;
  if (left_type != right_type && (left_method.isErrorNotFound() ||
                                  runtime->isSubclass(right_type, left_type))) {
    method = binaryOperationLookupReflected(thread, op, left, right);
    if (!method.isError()) {
      flags = IC_BINOP_REFLECTED;
      if (!left_method.isErrorNotFound()) {
        flags =
            static_cast<IcBinopFlags>(flags | IC_BINOP_NOTIMPLEMENTED_RETRY);
      }
      if (!method.isFunction()) {
        method_out = nullptr;
        method = resolveDescriptorGet(thread, method, right, right_type);
        if (method.isError()) return *method;
      }
    }
  }
  if (flags == IC_BINOP_NONE) {
    flags = IC_BINOP_NOTIMPLEMENTED_RETRY;
    method = *left_method;
    if (!method.isFunction() && !method.isError()) {
      method_out = nullptr;
      method = resolveDescriptorGet(thread, method, left, left_type);
      if (method.isError()) return *method;
    }
  }

  Object result(&scope,
                executeAndCacheBinop(thread, caller, method, flags, left, right,
                                     method_out, flags_out));
  if (!result.isNotImplementedType()) return *result;

  // Invoke a 2nd method (normal or reverse depends on what we did the first
  // time) or report an error.
  return binaryOperationRetry(thread, caller, op, flags, left, right);
}

RawObject Interpreter::binaryOperation(Thread* thread, Frame* caller,
                                       BinaryOp op, const Object& left,
                                       const Object& right) {
  return binaryOperationSetMethod(thread, caller, op, left, right, nullptr,
                                  nullptr);
}

HANDLER_INLINE Continue Interpreter::doBinaryOperation(BinaryOp op,
                                                       Thread* thread) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object other(&scope, frame->popValue());
  Object self(&scope, frame->popValue());
  RawObject result = binaryOperation(thread, frame, op, self, other);
  if (result.isError()) return Continue::UNWIND;
  frame->pushValue(result);
  return Continue::NEXT;
}

RawObject Interpreter::inplaceOperationSetMethod(
    Thread* thread, Frame* caller, BinaryOp op, const Object& left,
    const Object& right, Object* method_out, IcBinopFlags* flags_out) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  SymbolId selector = runtime->inplaceOperationSelector(op);
  Type left_type(&scope, runtime->typeOf(*left));
  Object method(&scope, typeLookupSymbolInMro(thread, left_type, selector));
  if (!method.isError()) {
    if (method.isFunction()) {
      if (method_out != nullptr) {
        *method_out = *method;
        *flags_out = IC_INPLACE_BINOP_RETRY;
      }
    } else {
      method = resolveDescriptorGet(thread, method, left, left_type);
      if (method.isError()) return *method;
    }

    // Make sure we do not put a possible 2nd method call (from
    // binaryOperationSetMethod() down below) into the cache.
    method_out = nullptr;
    Object result(&scope, callMethod2(thread, caller, method, left, right));
    if (result != NotImplementedType::object()) {
      return *result;
    }
  }
  return binaryOperationSetMethod(thread, caller, op, left, right, method_out,
                                  flags_out);
}

RawObject Interpreter::inplaceOperation(Thread* thread, Frame* caller,
                                        BinaryOp op, const Object& left,
                                        const Object& right) {
  return inplaceOperationSetMethod(thread, caller, op, left, right, nullptr,
                                   nullptr);
}

HANDLER_INLINE Continue Interpreter::doInplaceOperation(BinaryOp op,
                                                        Thread* thread) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object right(&scope, frame->popValue());
  Object left(&scope, frame->popValue());
  RawObject result = inplaceOperation(thread, frame, op, left, right);
  if (result.isError()) return Continue::UNWIND;
  frame->pushValue(result);
  return Continue::NEXT;
}

RawObject Interpreter::compareOperationSetMethod(
    Thread* thread, Frame* caller, CompareOp op, const Object& left,
    const Object& right, Object* method_out, IcBinopFlags* flags_out) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  SymbolId selector = runtime->comparisonSelector(op);
  Type left_type(&scope, runtime->typeOf(*left));
  Type right_type(&scope, runtime->typeOf(*right));
  Object left_method(&scope,
                     typeLookupSymbolInMro(thread, left_type, selector));

  // Figure out whether we want to run the normal or the reverse operation
  // first and set `flags` accordingly.
  Object method(&scope, *left_method);
  IcBinopFlags flags = IC_BINOP_NONE;
  if (left_type != right_type && (left_method.isErrorNotFound() ||
                                  runtime->isSubclass(right_type, left_type))) {
    SymbolId reverse_selector = runtime->swappedComparisonSelector(op);
    method = typeLookupSymbolInMro(thread, right_type, reverse_selector);
    if (!method.isError()) {
      flags = IC_BINOP_REFLECTED;
      if (!left_method.isErrorNotFound()) {
        flags =
            static_cast<IcBinopFlags>(flags | IC_BINOP_NOTIMPLEMENTED_RETRY);
      }
      if (!method.isFunction()) {
        method_out = nullptr;
        method = resolveDescriptorGet(thread, method, right, right_type);
        if (method.isError()) return *method;
      }
    }
  }
  if (flags == IC_BINOP_NONE) {
    flags = IC_BINOP_NOTIMPLEMENTED_RETRY;
    method = *left_method;
    if (!method.isFunction() && !method.isError()) {
      method_out = nullptr;
      method = resolveDescriptorGet(thread, method, left, left_type);
      if (method.isError()) return *method;
    }
  }

  Object result(&scope,
                executeAndCacheBinop(thread, caller, method, flags, left, right,
                                     method_out, flags_out));
  if (!result.isNotImplementedType()) return *result;

  return compareOperationRetry(thread, caller, op, flags, left, right);
}

RawObject Interpreter::compareOperationRetry(Thread* thread, Frame* caller,
                                             CompareOp op, IcBinopFlags flags,
                                             const Object& left,
                                             const Object& right) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  if (flags & IC_BINOP_NOTIMPLEMENTED_RETRY) {
    // If we tried reverse first, try normal now and vice versa.
    SymbolId selector = flags & IC_BINOP_REFLECTED
                            ? runtime->comparisonSelector(op)
                            : runtime->swappedComparisonSelector(op);
    Object method(&scope, lookupMethod(thread, caller, right, selector));
    if (method.isError()) {
      if (method.isErrorException()) return *method;
      DCHECK(method.isErrorNotFound(), "expected not found");
    } else {
      Object result(&scope, NoneType::object());
      if (flags & IC_BINOP_REFLECTED) {
        result = callMethod2(thread, caller, method, left, right);
      } else {
        result = callMethod2(thread, caller, method, right, left);
      }
      if (!result.isNotImplementedType()) return *result;
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
    Thread* thread, Frame* caller, RawObject method, IcBinopFlags flags,
    RawObject left, RawObject right) {
  caller->pushValue(method);
  if (flags & IC_BINOP_REFLECTED) {
    caller->pushValue(right);
    caller->pushValue(left);
  } else {
    caller->pushValue(left);
    caller->pushValue(right);
  }
  return call(thread, caller, 2);
}

RawObject Interpreter::binaryOperationRetry(Thread* thread, Frame* caller,
                                            BinaryOp op, IcBinopFlags flags,
                                            const Object& left,
                                            const Object& right) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  if (flags & IC_BINOP_NOTIMPLEMENTED_RETRY) {
    // If we tried reflected first, try normal now.
    if (flags & IC_BINOP_REFLECTED) {
      SymbolId selector = runtime->binaryOperationSelector(op);
      Object method(&scope, lookupMethod(thread, caller, right, selector));
      if (method.isError()) {
        if (method.isErrorException()) return *method;
        DCHECK(method.isErrorNotFound(), "expected not found");
      } else {
        Object result(&scope, callMethod2(thread, caller, method, left, right));
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
          if (method.isError()) return *method;
        }
        Object result(&scope, callMethod2(thread, caller, method, right, left));
        if (!result.isNotImplementedType()) return *result;
      }
    }
  }

  SymbolId op_symbol = runtime->binaryOperationSelector(op);
  return thread->raiseUnsupportedBinaryOperation(left, right, op_symbol);
}

RawObject Interpreter::compareOperation(Thread* thread, Frame* caller,
                                        CompareOp op, const Object& left,
                                        const Object& right) {
  return compareOperationSetMethod(thread, caller, op, left, right, nullptr,
                                   nullptr);
}

RawObject Interpreter::sequenceIterSearch(Thread* thread, Frame* caller,
                                          const Object& value,
                                          const Object& container) {
  HandleScope scope(thread);
  Object dunder_iter(
      &scope, lookupMethod(thread, caller, container, SymbolId::kDunderIter));
  if (dunder_iter.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__iter__ not defined on object");
  }
  Object iter(&scope, callMethod1(thread, caller, dunder_iter, container));
  if (iter.isError()) {
    return *iter;
  }
  Object dunder_next(&scope,
                     lookupMethod(thread, caller, iter, SymbolId::kDunderNext));
  if (dunder_next.isError()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__next__ not defined on iterator");
  }
  Object current(&scope, NoneType::object());
  Object compare_result(&scope, NoneType::object());
  Object result(&scope, NoneType::object());
  for (;;) {
    current = callMethod1(thread, caller, dunder_next, iter);
    if (current.isError()) {
      if (thread->hasPendingStopIteration()) {
        thread->clearPendingStopIteration();
        break;
      }
      return *current;
    }
    compare_result = compareOperation(thread, caller, EQ, value, current);
    if (compare_result.isError()) {
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

RawObject Interpreter::sequenceContains(Thread* thread, Frame* caller,
                                        const Object& value,
                                        const Object& container) {
  HandleScope scope(thread);
  Object method(&scope, lookupMethod(thread, caller, container,
                                     SymbolId::kDunderContains));
  if (!method.isError()) {
    Object result(&scope,
                  callMethod2(thread, caller, method, container, value));
    if (result.isError()) {
      return *result;
    }
    return isTrue(thread, *result);
  }
  return sequenceIterSearch(thread, caller, value, container);
}

HANDLER_INLINE USED RawObject Interpreter::isTrue(Thread* thread,
                                                  RawObject value_obj) {
  if (value_obj == Bool::trueObj()) return Bool::trueObj();
  if (value_obj == Bool::falseObj()) return Bool::falseObj();
  if (value_obj.isNoneType()) return Bool::falseObj();
  return isTrueSlowPath(thread, value_obj);
}

RawObject Interpreter::isTrueSlowPath(Thread* thread, RawObject value_obj) {
  HandleScope scope(thread);
  Object value(&scope, value_obj);
  Object result(&scope, thread->invokeMethod1(value, SymbolId::kDunderBool));
  if (!result.isError()) {
    if (result.isBool()) return *result;
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__bool__ should return bool");
  }
  if (result.isErrorException()) {
    return *result;
  }
  DCHECK(result.isErrorNotFound(), "expected error not found");

  result = thread->invokeMethod1(value, SymbolId::kDunderLen);
  if (!result.isError()) {
    if (thread->runtime()->isInstanceOfInt(*result)) {
      Int integer(&scope, intUnderlying(thread, result));
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

RawObject Interpreter::makeFunction(Thread* thread, const Object& qualname_str,
                                    const Code& code,
                                    const Object& closure_tuple,
                                    const Object& annotations_dict,
                                    const Object& kw_defaults_dict,
                                    const Object& defaults_tuple,
                                    const Dict& globals) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Function::Entry entry;
  Function::Entry entry_kw;
  Function::Entry entry_ex;
  word flags = code.flags();
  if (!code.hasOptimizedAndNewLocals()) {
    // We do not support calling non-optimized functions directly. We only allow
    // them in Thread::exec() and Thread::runClassFunction().
    entry = unimplementedTrampoline;
    entry_kw = unimplementedTrampoline;
    entry_ex = unimplementedTrampoline;
  } else if (code.isCoroutineOrGenerator()) {
    if (code.hasFreevarsOrCellvars()) {
      entry = generatorClosureTrampoline;
      entry_kw = generatorClosureTrampolineKw;
      entry_ex = generatorClosureTrampolineEx;
    } else {
      entry = generatorTrampoline;
      entry_kw = generatorTrampolineKw;
      entry_ex = generatorTrampolineEx;
    }
  } else {
    if (code.hasFreevarsOrCellvars()) {
      entry = interpreterClosureTrampoline;
      entry_kw = interpreterClosureTrampolineKw;
      entry_ex = interpreterClosureTrampolineEx;
    } else {
      entry = interpreterTrampoline;
      entry_kw = interpreterTrampolineKw;
      entry_ex = interpreterTrampolineEx;
    }
    flags |= Function::Flags::kInterpreted;
  }
  Object name(&scope, code.name());
  word total_args = code.totalArgs();
  word total_vars =
      code.nlocals() - total_args + code.numCellvars() + code.numFreevars();
  // HACK: Reserve one extra stack slot for the case where we need to unwrap a
  // bound method.
  word stacksize = code.stacksize() + 1;

  Function function(
      &scope, runtime->newInterpreterFunction(
                  thread, name, qualname_str, code, flags, code.argcount(),
                  total_args, total_vars, stacksize, closure_tuple,
                  annotations_dict, kw_defaults_dict, defaults_tuple, globals,
                  entry, entry_kw, entry_ex));

  Object dunder_name(&scope, runtime->symbols()->at(SymbolId::kDunderName));
  Object value_cell(&scope, runtime->dictAt(thread, globals, dunder_name));
  if (value_cell.isValueCell()) {
    DCHECK(!ValueCell::cast(*value_cell).isUnbound(), "unbound globals");
    function.setModule(ValueCell::cast(*value_cell).value());
  }
  Object consts_obj(&scope, code.consts());
  if (consts_obj.isTuple() && Tuple::cast(*consts_obj).length() >= 1) {
    Tuple consts(&scope, *consts_obj);
    if (consts.at(0).isStr()) {
      function.setDoc(consts.at(0));
    }
  }
  Bytes bytecode(&scope, code.code());
  function.setRewrittenBytecode(
      runtime->mutableBytesFromBytes(thread, bytecode));
  function.setCaches(runtime->emptyTuple());
  function.setOriginalArguments(runtime->emptyTuple());
  if (runtime->isCacheEnabled()) {
    // TODO(T45382423): Move this into a separate function to be called by a
    // relevant opcode during opcode execution.
    icRewriteBytecode(thread, function);
  }
  return *function;
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
    if (value.isError()) return;
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
  if (!cause.isError()) {  // TODO(T25860930) use Unbound rather than Error.
    if (runtime->isInstanceOfType(*cause) &&
        Type(&scope, *cause).isBaseExceptionSubclass()) {
      cause = Interpreter::callFunction0(thread, frame, cause);
      if (cause.isError()) return;
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

static void markGeneratorFinished(Frame* frame) {
  // Write to the Generator's HeapFrame directly so we don't have to save the
  // live frame to it one last time.
  RawGeneratorBase gen = generatorFromStackFrame(frame);
  RawHeapFrame heap_frame = HeapFrame::cast(gen.heapFrame());
  heap_frame.setVirtualPC(Frame::kFinishedGeneratorPC);
}

// If the current frame is executing a Generator, mark it as finished.
HANDLER_INLINE
static void finishCurrentGenerator(Frame* frame) {
  if (frame->function().isGenerator()) {
    markGeneratorFinished(frame);
  }
}

bool Interpreter::handleReturn(Thread* thread, Frame* entry_frame) {
  Frame* frame = thread->currentFrame();
  RawObject retval = frame->popValue();
  while (frame->blockStack()->depth() > 0) {
    if (popBlock(thread, TryBlock::Why::kReturn, retval)) {
      return false;
    }
  }
  finishCurrentGenerator(frame);
  if (frame == entry_frame) {
    frame->pushValue(retval);
    return true;
  }

  frame = thread->popFrame();
  frame->pushValue(retval);
  return false;
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
      return false;
    }

    if (frame == entry_frame) break;
    finishCurrentGenerator(frame);
    frame = thread->popFrame();
  }

  finishCurrentGenerator(frame);
  frame->pushValue(Error::exception());
  return true;
}

static Bytecode currentBytecode(Thread* thread) {
  Frame* frame = thread->currentFrame();
  word pc = frame->virtualPC() - Frame::kCodeUnitSize;
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
  return doUnaryOperation(SymbolId::kDunderPos, thread);
}

HANDLER_INLINE Continue Interpreter::doUnaryNegative(Thread* thread, word) {
  return doUnaryOperation(SymbolId::kDunderNeg, thread);
}

HANDLER_INLINE Continue Interpreter::doUnaryNot(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  RawObject value = frame->topValue();
  if (!value.isBool()) {
    value = isTrue(thread, value);
    if (value.isError()) return Continue::UNWIND;
  }
  frame->setTopValue(RawBool::negate(value));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doUnaryInvert(Thread* thread, word) {
  return doUnaryOperation(SymbolId::kDunderInvert, thread);
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
  Type type(&scope, thread->runtime()->typeOf(*container));
  Object getitem(&scope,
                 typeLookupSymbolInMro(thread, type, SymbolId::kDunderGetitem));
  if (getitem.isErrorNotFound()) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "object does not support indexing");
    return Continue::UNWIND;
  }
  if (index >= 0 && getitem.isFunction()) {
    icUpdate(frame->caches(), index, container.layoutId(), *getitem);
  }

  getitem = resolveDescriptorGet(thread, getitem, container, type);
  // Tail-call getitem(key)
  frame->setValueAt(*getitem, 1);
  return doCallFunction(thread, 1);
}

HANDLER_INLINE Continue Interpreter::doBinarySubscr(Thread* thread, word) {
  return binarySubscrUpdateCache(thread, -1);
}

HANDLER_INLINE Continue Interpreter::doBinarySubscrCached(Thread* thread,
                                                          word arg) {
  Frame* frame = thread->currentFrame();
  LayoutId container_layout_id = frame->peek(1).layoutId();
  RawObject cached = icLookup(frame->caches(), arg, container_layout_id);
  if (cached.isErrorNotFound()) {
    return binarySubscrUpdateCache(thread, arg);
  }

  DCHECK(cached.isFunction(), "Unexpected cached value");
  // Tail-call cached(container, key)
  frame->insertValueAt(cached, 2);
  return doCallFunction(thread, 2);
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
  Object method(&scope,
                lookupMethod(thread, frame, obj, SymbolId::kDunderAiter));
  if (method.isError()) {
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
  Object anext(&scope,
               lookupMethod(thread, frame, obj, SymbolId::kDunderAnext));
  if (anext.isError()) {
    thread->raiseWithFmt(
        LayoutId::kTypeError,
        "'async for' requires an iterator with __anext__ method");
    return Continue::UNWIND;
  }
  Object awaitable(&scope, callMethod1(thread, frame, anext, obj));
  if (awaitable.isError()) return Continue::UNWIND;

  // TODO(T33628943): Check if `awaitable` is a native or generator-based
  // coroutine and if it is, no need to call __await__
  Object await(&scope,
               lookupMethod(thread, frame, obj, SymbolId::kDunderAwait));
  if (await.isError()) {
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
  Object exit_selector(&scope, runtime->symbols()->DunderAexit());
  Object exit(&scope, runtime->attributeAt(thread, manager, exit_selector));
  if (exit.isError()) {
    UNIMPLEMENTED("throw TypeError");
  }
  frame->pushValue(*exit);

  // resolve __aenter__ call it and push the return value
  Object enter(&scope,
               lookupMethod(thread, frame, manager, SymbolId::kDunderAenter));
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
  Object setitem(
      &scope, lookupMethod(thread, frame, container, SymbolId::kDunderSetitem));
  if (setitem.isError()) {
    if (setitem.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "'%T' object does not support item assignment",
                           &container);
    }
    return Continue::UNWIND;
  }
  Object value(&scope, frame->popValue());
  if (callMethod3(thread, frame, setitem, container, key, value).isError()) {
    return Continue::UNWIND;
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doDeleteSubscr(Thread* thread, word) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object key(&scope, frame->popValue());
  Object container(&scope, frame->popValue());
  Object delitem(
      &scope, lookupMethod(thread, frame, container, SymbolId::kDunderDelitem));
  if (delitem.isError()) {
    if (delitem.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "'%T' object does not support item deletion",
                           &container);
    }
    return Continue::UNWIND;
  }
  if (callMethod2(thread, frame, delitem, container, key).isError()) {
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
  Frame* frame = thread->currentFrame();
  Object iterable(&scope, frame->popValue());
  Object method(&scope,
                lookupMethod(thread, frame, iterable, SymbolId::kDunderIter));
  if (method.isError()) {
    thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
    return Continue::UNWIND;
  }
  return tailcallMethod1(thread, *method, *iterable);
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
  Object method(&scope,
                lookupMethod(thread, frame, iterable, SymbolId::kDunderIter));
  if (method.isError()) {
    thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
    return Continue::UNWIND;
  }
  return tailcallMethod1(thread, *method, *iterable);
}

HANDLER_INLINE Continue Interpreter::doPrintExpr(Thread* thread, word) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object value(&scope, frame->popValue());
  ValueCell value_cell(&scope, thread->runtime()->displayHook());
  if (value_cell.isUnbound()) {
    UNIMPLEMENTED("RuntimeError: lost sys.displayhook");
  }
  frame->pushValue(value_cell.value());
  frame->pushValue(*value);
  return doCallFunction(thread, 1);
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
    Object next_method(
        &scope, lookupMethod(thread, frame, iterator, SymbolId::kDunderNext));
    if (next_method.isError()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "iter() returned non-iterator");
      return Continue::UNWIND;
    }
    result = callMethod1(thread, frame, next_method, iterator);
  } else {
    Object send_method(&scope,
                       lookupMethod(thread, frame, iterator, SymbolId::kSend));
    if (send_method.isError()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "iter() returned non-iterator");
      return Continue::UNWIND;
    }
    result = callMethod2(thread, frame, send_method, iterator, value);
  }
  if (result.isError()) {
    if (!thread->hasPendingStopIteration()) return Continue::UNWIND;

    frame->setTopValue(thread->pendingStopIterationValue());
    thread->clearPendingException();
    return Continue::NEXT;
  }

  // Unlike YIELD_VALUE, don't update PC in the frame: we want this
  // instruction to re-execute until the subiterator is exhausted.
  GeneratorBase gen(&scope, generatorFromStackFrame(frame));
  thread->runtime()->genSave(thread, gen);
  HeapFrame heap_frame(&scope, gen.heapFrame());
  heap_frame.setVirtualPC(heap_frame.virtualPC() - Frame::kCodeUnitSize);
  frame->pushValue(*result);
  return Continue::YIELD;
}

HANDLER_INLINE Continue Interpreter::doGetAwaitable(Thread* thread, word) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object obj(&scope, frame->popValue());

  // TODO(T33628943): Check if `obj` is a native or generator-based
  // coroutine and if it is, no need to call __await__
  Object await(&scope,
               lookupMethod(thread, frame, obj, SymbolId::kDunderAwait));
  if (await.isError()) {
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
  } else {
    // The stack contains the caught exception, the previous exception state,
    // then __exit__. Grab __exit__ then shift everything else down.
    exit = frame->peek(5);
    for (word i = 5; i > 0; i--) {
      frame->setValueAt(frame->peek(i - 1), i);
    }
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

  // Regardless of what happened above, exc should be put back at the new top of
  // the stack.
  frame->setTopValue(*exc);

  Object result(&scope,
                callFunction3(thread, frame, exit, exc, value, traceback));
  if (result.isError()) return Continue::UNWIND;

  // Push exc and result to be consumed by WITH_CLEANUP_FINISH.
  frame->pushValue(*exc);
  frame->pushValue(*result);

  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doWithCleanupFinish(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object result(&scope, frame->popValue());
  Object exc(&scope, frame->popValue());
  if (!exc.isNoneType()) {
    Object is_true(&scope, isTrue(thread, *result));
    if (is_true.isError()) return Continue::UNWIND;
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
  Dict implicit_globals(&scope, frame->implicitGlobals());
  Object annotations(&scope,
                     runtime->symbols()->at(SymbolId::kDunderAnnotations));
  Object anno_dict(&scope,
                   runtime->dictAt(thread, implicit_globals, annotations));
  if (anno_dict.isError()) {
    Object new_dict(&scope, runtime->newDict());
    runtime->dictAtPutInValueCell(thread, implicit_globals, annotations,
                                  new_dict);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doYieldValue(Thread* thread, word) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);

  Object result(&scope, frame->popValue());
  GeneratorBase gen(&scope, generatorFromStackFrame(frame));
  thread->runtime()->genSave(thread, gen);
  frame->pushValue(*result);
  return Continue::YIELD;
}

HANDLER_INLINE Continue Interpreter::doImportStar(Thread* thread, word) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();

  // Pre-python3 this used to merge the locals with the locals dict. However,
  // that's not necessary anymore. You can't import * inside a function
  // body anymore.

  Module module(&scope, frame->popValue());
  CHECK(module.isModule(), "Unexpected type to import from");

  Dict implicit_globals(&scope, frame->implicitGlobals());
  thread->runtime()->moduleImportAllFrom(implicit_globals, module);
  return Continue::NEXT;
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
  // TypeDict results are not cached since LOAD_NAME doesn't use caching.
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Dict implicit_globals(&scope, frame->implicitGlobals());
  RawObject names = Code::cast(frame->code()).names();
  Object key(&scope, Tuple::cast(names).at(arg));
  Object value(&scope, frame->popValue());
  Runtime* runtime = thread->runtime();
  runtime->dictAtPutInValueCell(thread, implicit_globals, key, value);
  if (isCacheEnabledForCurrentFunction(frame) &&
      frame->implicitGlobals() == frame->function().globals()) {
    Dict globals(&scope, frame->function().globals());
    Dict builtins(&scope, runtime->moduleDictBuiltins(thread, globals));
    // TODO(T45203542): Use a placeholder in globals to avoid builtin lookup.
    Object builtin_result(&scope, runtime->dictAt(thread, builtins, key));
    DCHECK(builtin_result.isErrorNotFound() || !builtin_result.isError(),
           "dictAt should not raise an exception other than ErrorNotFound");
    if (!builtin_result.isErrorNotFound()) {
      // The newly created global variable shadows the previously cached value
      // from _builtins__, so invalidate the cached builtin value.
      DCHECK(builtin_result.isValueCell(), "result must be a ValueCell");
      ValueCell value_cell(&scope, *builtin_result);
      icInvalidateGlobalVar(thread, value_cell);
    }
  }
  return Continue::NEXT;
}

static Continue raiseUndefinedName(Thread* thread, const Str& name) {
  thread->raiseWithFmt(LayoutId::kNameError, "name '%S' is not defined", &name);
  return Continue::UNWIND;
}

HANDLER_INLINE Continue Interpreter::doDeleteName(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Dict implicit_globals(&scope, frame->implicitGlobals());
  RawObject names = Code::cast(frame->code()).names();
  Str key(&scope, Tuple::cast(names).at(arg));
  Object result(&scope,
                thread->runtime()->dictRemove(thread, implicit_globals, key));
  DCHECK(result.isErrorNotFound() || result.isValueCell(),
         "dictRemove must return either ErrorNotFound or ValueCell");
  if (result.isErrorNotFound()) {
    return raiseUndefinedName(thread, key);
  }
  // Only a module dict needs cache invalidation since a type dict entry is
  // only read via LOAD_NAME which never involves caching.
  Dict globals(&scope, frame->function().globals());
  if (isCacheEnabledForCurrentFunction(frame) &&
      frame->implicitGlobals() == *globals) {
    DCHECK(result.isValueCell(), "result must be a ValueCell");
    ValueCell value_cell(&scope, *result);
    icInvalidateGlobalVar(thread, value_cell);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doUnpackSequence(Thread* thread,
                                                      word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object iterable(&scope, frame->popValue());
  Object iter_method(
      &scope, lookupMethod(thread, frame, iterable, SymbolId::kDunderIter));
  if (iter_method.isError()) {
    thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
    return Continue::UNWIND;
  }
  Object iterator(&scope, callMethod1(thread, frame, iter_method, iterable));
  if (iterator.isError()) return Continue::UNWIND;

  Object next_method(
      &scope, lookupMethod(thread, frame, iterator, SymbolId::kDunderNext));
  if (next_method.isError()) {
    thread->raiseWithFmt(LayoutId::kTypeError, "iter() returned non-iterator");
    return Continue::UNWIND;
  }
  word num_pushed = 0;
  Object value(&scope, RawNoneType::object());
  for (;;) {
    value = callMethod1(thread, frame, next_method, iterator);
    if (value.isError()) {
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
  return forIterUpdateCache(thread, arg, -1) ? Continue::UNWIND
                                             : Continue::NEXT;
}

bool Interpreter::forIterUpdateCache(Thread* thread, word arg, word index) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object iter(&scope, frame->topValue());
  Type type(&scope, thread->runtime()->typeOf(*iter));
  Object next(&scope,
              typeLookupSymbolInMro(thread, type, SymbolId::kDunderNext));
  if (next.isErrorNotFound()) {
    thread->raiseWithFmt(LayoutId::kTypeError, "iter() returned non-iterator");
    return true;
  }

  if (index >= 0 && next.isFunction()) {
    icUpdate(frame->caches(), index, iter.layoutId(), *next);
  }

  next = resolveDescriptorGet(thread, next, iter, type);
  Object result(&scope, callFunction0(thread, frame, next));
  if (result.isErrorException()) {
    if (thread->clearPendingStopIteration()) {
      frame->popValue();
      frame->setVirtualPC(frame->virtualPC() + arg);
      return false;
    }
    return true;
  }
  frame->pushValue(*result);
  return false;
}

HANDLER_INLINE Continue Interpreter::doForIterCached(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawObject iter = frame->topValue();
  LayoutId iter_layout_id = iter.layoutId();
  RawObject cached = icLookup(frame->caches(), arg, iter_layout_id);
  if (cached.isErrorNotFound()) {
    return forIterUpdateCache(thread, icOriginalArg(frame->function(), arg),
                              arg)
               ? Continue::UNWIND
               : Continue::NEXT;
  }

  DCHECK(cached.isFunction(), "Unexpected cached value");
  frame->pushValue(cached);
  frame->pushValue(iter);
  RawObject result = call(thread, frame, 1);
  if (result.isErrorException()) {
    if (thread->clearPendingStopIteration()) {
      frame->popValue();
      // TODO(bsimmers): icOriginalArg() is only meant for slow paths, but we
      // currently have no other way of getting this information.
      frame->setVirtualPC(frame->virtualPC() +
                          icOriginalArg(frame->function(), arg));
      return Continue::NEXT;
    }
    return Continue::UNWIND;
  }
  frame->pushValue(result);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doUnpackEx(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object iterable(&scope, frame->popValue());
  Object iter_method(
      &scope, lookupMethod(thread, frame, iterable, SymbolId::kDunderIter));
  if (iter_method.isError()) {
    thread->raiseWithFmt(LayoutId::kTypeError, "object is not iterable");
    return Continue::UNWIND;
  }
  Object iterator(&scope, callMethod1(thread, frame, iter_method, iterable));
  if (iterator.isError()) return Continue::UNWIND;

  Object next_method(
      &scope, lookupMethod(thread, frame, iterator, SymbolId::kDunderNext));
  if (next_method.isError()) {
    thread->raiseWithFmt(LayoutId::kTypeError, "iter() returned non-iterator");
    return Continue::UNWIND;
  }

  word before = arg & kMaxByte;
  word after = (arg >> kBitsPerByte) & kMaxByte;
  word num_pushed = 0;
  Object value(&scope, RawNoneType::object());
  for (; num_pushed < before; ++num_pushed) {
    value = callMethod1(thread, frame, next_method, iterator);
    if (value.isError()) {
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
    if (value.isError()) {
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
  RawHeapObject heap_object = HeapObject::cast(receiver);
  if (offset >= 0) {
    heap_object.instanceVariableAtPut(offset, value);
    return;
  }

  RawLayout layout =
      Layout::cast(thread->runtime()->layoutAt(receiver.layoutId()));
  RawTuple overflow =
      Tuple::cast(heap_object.instanceVariableAt(layout.overflowOffset()));
  overflow.atPut(-offset - 1, value);
}

RawObject Interpreter::storeAttrSetLocation(Thread* thread,
                                            const Object& object,
                                            const Object& name_str,
                                            const Object& value,
                                            Object* location_out) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Type type(&scope, runtime->typeOf(*object));
  Object dunder_setattr(
      &scope, typeLookupSymbolInMro(thread, type, SymbolId::kDunderSetattr));
  if (dunder_setattr == runtime->objectDunderSetattr()) {
    return objectSetAttrSetLocation(thread, object, name_str, value,
                                    location_out);
  }
  Object result(&scope, thread->invokeMethod3(object, SymbolId::kDunderSetattr,
                                              name_str, value));
  // Invalidate attribute caches for type attribute changes.
  if (!result.isError() && object.isType()) {
    // TODO(T46243890): Disable attribute caching when fundamental objects are
    // modified (e.g., object.__getattribute__).
    // TODO(T46362789): Move cache invalidation logic into dict API.
    Type type_object(&scope, *object);
    DCHECK(name_str.isStr(), "attribute name must be Str");
    Str attr_name(&scope, *name_str);
    Type value_type(&scope, thread->runtime()->typeOf(*value));
    icInvalidateCachesForTypeAttr(thread, type_object, attr_name,
                                  typeIsDataDescriptor(thread, value_type));
  }
  return *result;
}

Continue Interpreter::storeAttrUpdateCache(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  word original_arg = icOriginalArg(frame->function(), arg);
  HandleScope scope(thread);
  Object receiver(&scope, frame->popValue());
  Object name(&scope,
              Tuple::cast(Code::cast(frame->code()).names()).at(original_arg));
  Object value(&scope, frame->popValue());

  Object location(&scope, NoneType::object());
  Object result(&scope,
                storeAttrSetLocation(thread, receiver, name, value, &location));
  if (result.isError()) return Continue::UNWIND;
  if (!location.isNoneType()) {
    LayoutId layout_id = receiver.layoutId();
    icUpdate(frame->caches(), arg, layout_id, *location);
    Type receiver_type(&scope, thread->runtime()->typeOf(*receiver));
    Object dependent(&scope, frame->function());
    icInsertDependencyForTypeLookupInMro(thread, receiver_type, name,
                                         dependent);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreAttrCached(Thread* thread,
                                                       word arg) {
  Frame* frame = thread->currentFrame();
  RawObject receiver_raw = frame->topValue();
  LayoutId layout_id = receiver_raw.layoutId();
  RawObject cached = icLookup(frame->caches(), arg, layout_id);
  if (cached.isError()) {
    return storeAttrUpdateCache(thread, arg);
  }
  DCHECK(!receiver_raw.isType(), "type attributes must not be cached");
  RawObject value_raw = frame->peek(1);
  frame->dropValues(2);
  storeAttrWithLocation(thread, receiver_raw, cached, value_raw);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreAttr(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object receiver(&scope, frame->popValue());
  auto names = Code::cast(frame->code()).names();
  Object name(&scope, Tuple::cast(names).at(arg));
  Object value(&scope, frame->popValue());
  if (thread->invokeMethod3(receiver, SymbolId::kDunderSetattr, name, value)
          .isError()) {
    return Continue::UNWIND;
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doDeleteAttr(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object receiver(&scope, frame->popValue());
  auto names = Code::cast(frame->code()).names();
  Object name(&scope, Tuple::cast(names).at(arg));
  if (thread->runtime()->attributeDel(thread, receiver, name).isError()) {
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
  Dict globals(&scope, frame->function().globals());
  Runtime* runtime = thread->runtime();
  if (!isCacheEnabledForCurrentFunction(frame)) {
    runtime->moduleDictAtPut(thread, globals, key, value);
    return Continue::NEXT;
  }
  Object global_result(&scope, runtime->dictAt(thread, globals, key));
  if (global_result.isValueCell() &&
      ValueCell::cast(*global_result).isPlaceholder()) {
    // A builtin entry is cached under the same key, so invalidate its caches.
    Dict builtins(&scope, runtime->moduleDictBuiltins(thread, globals));
    Object builtin_result(
        &scope, runtime->moduleDictValueCellAt(thread, builtins, key));
    DCHECK(builtin_result.isValueCell(), "a builtin entry must exist");
    ValueCell builtin_value_cell(&scope, *builtin_result);
    DCHECK(!builtin_value_cell.dependencyLink().isNoneType(),
           "the builtin valuecell must have a dependent");
    icInvalidateGlobalVar(thread, builtin_value_cell);
  }
  runtime->moduleDictAtPut(thread, globals, key, value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doStoreGlobalCached(Thread* thread,
                                                         word arg) {
  Frame* frame = thread->currentFrame();
  RawObject cached = icLookupGlobalVar(frame->caches(), arg);
  DCHECK(cached.isValueCell(), "the cached value must not be a ValueCell");
  ValueCell::cast(cached).setValue(frame->popValue());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doDeleteGlobal(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Tuple names(&scope, Code::cast(frame->code()).names());
  Str key(&scope, names.at(arg));
  Dict globals(&scope, frame->function().globals());
  Object result(&scope, thread->runtime()->dictRemove(thread, globals, key));
  DCHECK(result.isErrorNotFound() || result.isValueCell(),
         "dictRemove must return either ErrorNotFound or ValueCell");
  if (result.isErrorNotFound()) {
    return raiseUndefinedName(thread, key);
  }
  if (isCacheEnabledForCurrentFunction(frame)) {
    ValueCell value_cell(&scope, *result);
    // TODO(T45091174): Move this into a module instance.
    icInvalidateGlobalVar(thread, value_cell);
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
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object names(&scope, Code::cast(frame->code()).names());
  Str key(&scope, Tuple::cast(*names).at(arg));

  // 1. implicitGlobals
  Dict implicit_globals(&scope, frame->implicitGlobals());
  Object result(&scope, runtime->moduleDictAt(thread, implicit_globals, key));
  if (!result.isErrorNotFound()) {
    // 3a. found in [implicit]/globals
    frame->pushValue(*result);
    return Continue::NEXT;
  }

  Dict globals(&scope, frame->function().globals());
  // In the module body, globals == implicit_globals, so no need to check
  // twice. However in class body, it is a different dict.
  if (frame->implicitGlobals() != *globals) {
    // 2. globals
    result = runtime->moduleDictAt(thread, globals, key);
    if (!result.isErrorNotFound()) {
      // 3a. found in [implicit]/globals
      frame->pushValue(*result);
      return Continue::NEXT;
    }
  }

  // 3b. not found; check builtins.
  Dict builtins(&scope, runtime->moduleDictBuiltins(thread, globals));
  result = runtime->moduleDictAt(thread, builtins, key);
  if (!result.isErrorNotFound()) {
    frame->pushValue(*result);
    return Continue::NEXT;
  }

  return raiseUndefinedName(thread, key);
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
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Tuple array(&scope, thread->runtime()->newTuple(arg));
  for (word i = arg - 1; i >= 0; i--) {
    array.atPut(i, frame->popValue());
  }
  RawList list = List::cast(thread->runtime()->newList());
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
  for (word i = arg - 1; i >= 0; i--) {
    Object key(&scope, frame->popValue());
    Object key_hash(&scope, thread->invokeMethod1(key, SymbolId::kDunderHash));
    if (key_hash.isError()) return Continue::UNWIND;
    runtime->setAddWithHash(thread, set, key, key_hash);
  }
  frame->pushValue(*set);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildMap(Thread* thread, word arg) {
  Runtime* runtime = thread->runtime();
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Dict dict(&scope, runtime->newDictWithSize(arg));
  for (word i = 0; i < arg; i++) {
    Object value(&scope, frame->popValue());
    Object key(&scope, frame->popValue());
    Object key_hash(&scope, thread->invokeMethod1(key, SymbolId::kDunderHash));
    if (key_hash.isError()) return Continue::UNWIND;
    runtime->dictAtPutWithHash(thread, dict, key, value, key_hash);
  }
  frame->pushValue(*dict);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadAttr(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object receiver(&scope, frame->topValue());
  auto names = Code::cast(frame->code()).names();
  Object name(&scope, Tuple::cast(names).at(arg));
  RawObject result = thread->runtime()->attributeAt(thread, receiver, name);
  if (result.isError()) return Continue::UNWIND;
  frame->setTopValue(result);
  return Continue::NEXT;
}

RawObject Interpreter::loadAttrSetLocation(Thread* thread, const Object& object,
                                           const Object& name_str,
                                           Object* location_out) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(*object));
  Object dunder_getattribute(
      &scope,
      typeLookupSymbolInMro(thread, type, SymbolId::kDunderGetattribute));
  if (dunder_getattribute == runtime->objectDunderGetattribute()) {
    Object result(&scope, objectGetAttributeSetLocation(
                              thread, object, name_str, location_out));
    if (result.isErrorNotFound()) {
      result =
          thread->invokeMethod2(object, SymbolId::kDunderGetattr, name_str);
      if (result.isErrorNotFound()) {
        return objectRaiseAttributeError(thread, object, name_str);
      }
    }
    return *result;
  }

  return thread->runtime()->attributeAt(thread, object, name_str);
}

Continue Interpreter::loadAttrUpdateCache(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  word original_arg = icOriginalArg(frame->function(), arg);
  Object receiver(&scope, frame->topValue());
  Object name(&scope,
              Tuple::cast(Code::cast(frame->code()).names()).at(original_arg));

  Object location(&scope, NoneType::object());
  Object result(&scope, loadAttrSetLocation(thread, receiver, name, &location));
  if (result.isError()) return Continue::UNWIND;
  if (!location.isNoneType()) {
    LayoutId layout_id = receiver.layoutId();
    icUpdate(frame->caches(), arg, layout_id, *location);
    Type receiver_type(&scope, thread->runtime()->typeOf(*receiver));
    Object dependent(&scope, frame->function());
    icInsertDependencyForTypeLookupInMro(thread, receiver_type, name,
                                         dependent);
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
  RawHeapObject heap_object = HeapObject::cast(receiver);
  if (offset >= 0) {
    return heap_object.instanceVariableAt(offset);
  }

  RawLayout layout =
      Layout::cast(thread->runtime()->layoutAt(receiver.layoutId()));
  RawTuple overflow =
      Tuple::cast(heap_object.instanceVariableAt(layout.overflowOffset()));
  return overflow.at(-offset - 1);
}

HANDLER_INLINE Continue Interpreter::doLoadAttrCached(Thread* thread,
                                                      word arg) {
  Frame* frame = thread->currentFrame();
  RawObject receiver_raw = frame->topValue();
  LayoutId layout_id = receiver_raw.layoutId();
  RawObject cached = icLookup(frame->caches(), arg, layout_id);
  if (cached.isErrorNotFound()) {
    return loadAttrUpdateCache(thread, arg);
  }

  RawObject result = loadAttrWithLocation(thread, receiver_raw, cached);
  frame->setTopValue(result);
  return Continue::NEXT;
}

static RawObject excMatch(Thread* thread, const Object& left,
                          const Object& right) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  static const char* cannot_catch_msg =
      "catching classes that do not inherit from BaseException is not allowed";
  if (runtime->isInstanceOfTuple(*right)) {
    Tuple tuple(&scope, tupleUnderlying(thread, right));
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
  RawObject result;
  if (op == IS) {
    result = Bool::fromBool(*left == *right);
  } else if (op == IS_NOT) {
    result = Bool::fromBool(*left != *right);
  } else if (op == IN) {
    result = sequenceContains(thread, frame, left, right);
  } else if (op == NOT_IN) {
    result = Bool::negate(sequenceContains(thread, frame, left, right));
  } else if (op == EXC_MATCH) {
    result = excMatch(thread, left, right);
  } else {
    result = compareOperation(thread, frame, op, left, right);
  }

  if (result.isError()) return Continue::UNWIND;
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
  Dict dict_no_value_cells(&scope, runtime->newDict());
  // TODO(T41326706) Pass in a real globals dict here. For now create a small
  // dict with just the things needed by importlib.
  Dict globals(&scope, frame->function().globals());
  Object key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  for (SymbolId id : {SymbolId::kDunderPackage, SymbolId::kDunderSpec,
                      SymbolId::kDunderName}) {
    key = runtime->symbols()->at(id);
    value = runtime->moduleDictAt(thread, globals, key);
    runtime->dictAtPut(thread, dict_no_value_cells, key, value);
  }
  // TODO(T41634372) Pass in a dict that is similar to what `builtins.locals`
  // returns. Use `None` for now since the default importlib behavior is to
  // ignore the value and this only matters if `__import__` is replaced.
  Object locals(&scope, NoneType::object());

  // Call builtins.__import__(name, dict_no_value_cells, locals, fromlist,
  //                          level).
  ValueCell dunder_import_cell(&scope, runtime->dunderImport());
  DCHECK(!dunder_import_cell.isUnbound(), "builtins module not initialized");
  Object dunder_import(&scope, dunder_import_cell.value());

  frame->pushValue(*dunder_import);
  frame->pushValue(*name);
  frame->pushValue(*dict_no_value_cells);
  frame->pushValue(*locals);
  frame->pushValue(*fromlist);
  frame->pushValue(*level);
  return doCallFunction(thread, 5);
}

HANDLER_INLINE Continue Interpreter::doImportFrom(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Code code(&scope, frame->code());
  Object name(&scope, Tuple::cast(code.names()).at(arg));
  CHECK(name.isStr(), "name not found");
  Module module(&scope, frame->topValue());
  Runtime* runtime = thread->runtime();
  CHECK(module.isModule(), "Unexpected type to import from");
  RawObject value = runtime->moduleAt(module, name);
  if (value.isError()) {
    Str module_name(&scope, module.name());
    thread->raiseWithFmt(LayoutId::kImportError,
                         "cannot import name '%S' from '%S'", &name,
                         &module_name);
    return Continue::UNWIND;
  }
  frame->pushValue(value);
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
  DCHECK(value.isError(), "value must be error");
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
  DCHECK(value.isError(), "value must be error");
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
  DCHECK(value.isError(), "value must be error");
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
  DCHECK(value.isError(), "value must be error");
  return Continue::UNWIND;
}

HANDLER_INLINE Continue Interpreter::doLoadGlobal(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Tuple names(&scope, Code::cast(frame->code()).names());
  Str key(&scope, names.at(arg));
  Dict globals(&scope, frame->function().globals());
  Runtime* runtime = thread->runtime();
  Object result(&scope, runtime->moduleDictValueCellAt(thread, globals, key));
  if (result.isValueCell()) {
    ValueCell value_cell(&scope, *result);
    if (isCacheEnabledForCurrentFunction(frame)) {
      Function function(&scope, frame->function());
      icUpdateGlobalVar(thread, function, arg, value_cell);
    }
    frame->pushValue(value_cell.value());
    return Continue::NEXT;
  }
  Dict builtins(&scope, thread->runtime()->moduleDictBuiltins(thread, globals));
  Object builtin_result(&scope,
                        runtime->moduleDictValueCellAt(thread, builtins, key));
  if (builtin_result.isValueCell()) {
    ValueCell value_cell(&scope, *builtin_result);
    if (isCacheEnabledForCurrentFunction(frame)) {
      Function function(&scope, frame->function());
      icUpdateGlobalVar(thread, function, arg, value_cell);
      // Insert a placeholder to the module dict to show that a builtins entry
      // got cached under the same key.
      NoneType none(&scope, NoneType::object());
      ValueCell global_value_cell(
          &scope, runtime->dictAtPutInValueCell(thread, globals, key, none));
      global_value_cell.makePlaceholder();
    }
    frame->pushValue(value_cell.value());
    return Continue::NEXT;
  }

  return raiseUndefinedName(thread, key);
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
  Object key(&scope, Tuple::cast(*names).at(arg));

  Dict implicit_globals(&scope, frame->implicitGlobals());
  Object annotations(&scope,
                     runtime->symbols()->at(SymbolId::kDunderAnnotations));
  Object value_cell(&scope,
                    runtime->dictAt(thread, implicit_globals, annotations));
  Dict anno_dict(&scope, ValueCell::cast(*value_cell).value());
  runtime->dictAtPut(thread, anno_dict, key, value);
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

HANDLER_INLINE Frame* Interpreter::pushFrame(Thread* thread,
                                             RawFunction function,
                                             RawObject* post_call_sp) {
  Frame* caller_frame = thread->currentFrame();
  Frame* callee_frame = thread->pushCallFrame(function);
  // Pop the arguments off of the caller's stack now that the callee "owns"
  // them.
  caller_frame->setValueStackTop(post_call_sp);
  return callee_frame;
}

HANDLER_INLINE
Continue Interpreter::callTrampoline(Thread* thread, Function::Entry entry,
                                     word argc, RawObject* post_call_sp) {
  Frame* frame = thread->currentFrame();
  RawObject result = entry(thread, frame, argc);
  if (result.isError()) return Continue::UNWIND;
  frame->setValueStackTop(post_call_sp);
  frame->pushValue(result);
  return Continue::NEXT;
}

// Performs the function without pushing a new frame. Pops the arguments off of
// the caller's frame and sets the top value to the result. Returns true
// if the call succeeds.
static bool doIntrinsic(Thread* thread, Frame* frame, SymbolId name) {
  Runtime* runtime = thread->runtime();
  switch (name) {
    case SymbolId::kUnderByteArrayCheck:
      frame->setTopValue(
          Bool::fromBool(runtime->isInstanceOfByteArray(frame->popValue())));
      return true;
    case SymbolId::kUnderBytesCheck:
      frame->setTopValue(
          Bool::fromBool(runtime->isInstanceOfBytes(frame->popValue())));
      return true;
    case SymbolId::kUnderDictCheck:
      frame->setTopValue(
          Bool::fromBool(runtime->isInstanceOfDict(frame->popValue())));
      return true;
    case SymbolId::kUnderFloatCheck:
      frame->setTopValue(
          Bool::fromBool(runtime->isInstanceOfFloat(frame->popValue())));
      return true;
    case SymbolId::kUnderFrozenSetCheck:
      frame->setTopValue(
          Bool::fromBool(runtime->isInstanceOfFrozenSet(frame->popValue())));
      return true;
    case SymbolId::kUnderIntCheck:
      frame->setTopValue(
          Bool::fromBool(runtime->isInstanceOfInt(frame->popValue())));
      return true;
    case SymbolId::kUnderListCheck:
      frame->setTopValue(
          Bool::fromBool(runtime->isInstanceOfList(frame->popValue())));
      return true;
    case SymbolId::kUnderSetCheck:
      frame->setTopValue(
          Bool::fromBool(runtime->isInstanceOfSet(frame->popValue())));
      return true;
    case SymbolId::kUnderSliceCheck:
      frame->setTopValue(Bool::fromBool(frame->popValue().isSlice()));
      return true;
    case SymbolId::kUnderStrCheck:
      frame->setTopValue(
          Bool::fromBool(runtime->isInstanceOfStr(frame->popValue())));
      return true;
    case SymbolId::kUnderTupleCheck:
      frame->setTopValue(
          Bool::fromBool(runtime->isInstanceOfTuple(frame->popValue())));
      return true;
    case SymbolId::kUnderType:
      frame->setTopValue(runtime->typeOf(frame->popValue()));
      return true;
    case SymbolId::kUnderTypeCheck:
      frame->setTopValue(
          Bool::fromBool(runtime->isInstanceOfType(frame->popValue())));
      return true;
    case SymbolId::kUnderTypeCheckExact:
      frame->setTopValue(Bool::fromBool(frame->popValue().isType()));
      return true;
    case SymbolId::kIsInstance:
      if (runtime->typeOf(frame->peek(1)) == frame->peek(0)) {
        frame->dropValues(2);
        frame->setTopValue(Bool::trueObj());
        return true;
      }
      return false;
    default:
      UNREACHABLE("function %s does not have an intrinsic implementation",
                  Symbols::predefinedSymbolAt(name));
  }
}

HANDLER_INLINE Continue
Interpreter::handleCall(Thread* thread, word argc, word callable_idx,
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
  RawObject callable = caller_frame->peek(callable_idx);
  callable = prepareCallableCall(thread, caller_frame, callable_idx, &argc);
  if (callable.isError()) return Continue::UNWIND;
  RawFunction function = RawFunction::cast(callable);

  SymbolId name = static_cast<SymbolId>(function.intrinsicId());
  if (name != SymbolId::kInvalid && doIntrinsic(thread, caller_frame, name)) {
    return Continue::NEXT;
  }

  if (!function.isInterpreted()) {
    return callTrampoline(thread, (function.*get_entry)(), argc, post_call_sp);
  }

  RawObject result = prepare_args(thread, function, caller_frame, argc);
  if (result.isError()) return Continue::UNWIND;
  function = RawFunction::cast(result);

  Frame* callee_frame = pushFrame(thread, function, post_call_sp);
  if (function.hasFreevarsOrCellvars()) {
    HandleScope scope(thread);
    Function function_handle(&scope, function);
    processFreevarsAndCellvars(thread, function_handle, callee_frame);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doCallFunction(Thread* thread, word arg) {
  return handleCall(thread, arg, arg, 0, preparePositionalCall,
                    &Function::entry);
}

HANDLER_INLINE Continue Interpreter::doMakeFunction(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object qualname(&scope, frame->popValue());
  Code code(&scope, frame->popValue());
  Object closure(&scope, NoneType::object());
  Object annotations(&scope, NoneType::object());
  Object kw_defaults(&scope, NoneType::object());
  Object defaults(&scope, NoneType::object());
  if (arg & MakeFunctionFlag::CLOSURE) closure = frame->popValue();
  if (arg & MakeFunctionFlag::ANNOTATION_DICT) annotations = frame->popValue();
  if (arg & MakeFunctionFlag::DEFAULT_KW) kw_defaults = frame->popValue();
  if (arg & MakeFunctionFlag::DEFAULT) defaults = frame->popValue();
  Dict globals(&scope, frame->function().globals());
  frame->pushValue(makeFunction(thread, qualname, code, closure, annotations,
                                kw_defaults, defaults, globals));
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildSlice(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Slice slice(&scope, thread->runtime()->newSlice());
  if (arg == 3) slice.setStep(frame->popValue());
  slice.setStop(frame->popValue());
  slice.setStart(frame->topValue());  // TOP
  frame->setTopValue(*slice);
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
  ValueCell value_cell(&scope, frame->local(code.nlocals() + arg));
  Object value(&scope, value_cell.value());
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
  ValueCell::cast(frame->local(code.nlocals() + arg))
      .setValue(frame->popValue());
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doDeleteDeref(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  RawCode code = Code::cast(frame->code());
  ValueCell::cast(frame->local(code.nlocals() + arg))
      .setValue(Unbound::object());
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
  if (callable.isError()) return Continue::UNWIND;

  Function function(&scope, *callable);
  if (!function.isInterpreted()) {
    return callTrampoline(thread, function.entryEx(), arg, post_call_sp);
  }

  if (prepareExplodeCall(thread, *function, caller_frame, arg).isError()) {
    return Continue::UNWIND;
  }

  Frame* callee_frame = pushFrame(thread, *function, post_call_sp);
  if (function.hasFreevarsOrCellvars()) {
    processFreevarsAndCellvars(thread, function, callee_frame);
  }
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doSetupWith(Thread* thread, word arg) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Frame* frame = thread->currentFrame();
  Object mgr(&scope, frame->topValue());
  Object enter(&scope,
               lookupMethod(thread, frame, mgr, SymbolId::kDunderEnter));
  if (enter.isError()) {
    if (enter.isErrorNotFound()) {
      thread->raise(LayoutId::kAttributeError,
                    runtime->symbols()->at(SymbolId::kDunderEnter));
    }
    return Continue::UNWIND;
  }
  Object exit(&scope, lookupMethod(thread, frame, mgr, SymbolId::kDunderExit));
  if (exit.isError()) {
    if (exit.isErrorNotFound()) {
      thread->raise(LayoutId::kAttributeError,
                    runtime->symbols()->at(SymbolId::kDunderExit));
    }
    return Continue::UNWIND;
  }
  Object exit_bound(&scope, runtime->newBoundMethod(exit, mgr));
  frame->setTopValue(*exit_bound);
  Object result(&scope, callMethod1(thread, frame, enter, mgr));
  if (result.isError()) return Continue::UNWIND;

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
  Set set(&scope, Set::cast(frame->peek(arg - 1)));
  thread->runtime()->setAdd(thread, set, value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doMapAdd(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Object key(&scope, frame->popValue());
  Object value(&scope, frame->popValue());
  Dict dict(&scope, Dict::cast(frame->peek(arg - 1)));
  thread->runtime()->dictAtPut(thread, dict, key, value);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doLoadClassDeref(Thread* thread,
                                                      word arg) {
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  Code code(&scope, frame->code());
  word idx = arg - code.numCellvars();
  Object name(&scope, Tuple::cast(code.freevars()).at(idx));
  Dict implicit_global(&scope, frame->implicitGlobals());
  Object result(&scope,
                thread->runtime()->dictAt(thread, implicit_global, name));
  if (result.isError()) {
    ValueCell value_cell(&scope, frame->local(code.nlocals() + arg));
    if (value_cell.isUnbound()) {
      UNIMPLEMENTED("unbound free var %s", Str::cast(*name).toCStr());
    }
    frame->pushValue(value_cell.value());
  } else {
    frame->pushValue(ValueCell::cast(*result).value());
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
    if (listExtend(thread, list, obj).isError()) return Continue::UNWIND;
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
    if (dictMergeOverride(thread, dict, obj).isError()) {
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
    if (dictMergeError(thread, dict, obj).isError()) {
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
    if (listExtend(thread, list, obj).isError()) return Continue::UNWIND;
  }
  Tuple tuple(&scope, runtime->newTuple(list.numItems()));
  for (word i = 0; i < list.numItems(); i++) {
    tuple.atPut(i, list.at(i));
  }
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
    if (runtime->setUpdate(thread, set, obj).isError()) return Continue::UNWIND;
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
  int conv = (arg & FVC_MASK_FLAG);
  int have_fmt_spec = (arg & FVS_MASK_FLAG) == FVS_HAVE_SPEC_FLAG;
  Runtime* runtime = thread->runtime();
  Object fmt_spec(&scope, Str::empty());
  if (have_fmt_spec) {
    fmt_spec = frame->popValue();
  }
  Object value(&scope, frame->popValue());
  Object method(&scope, NoneType::object());
  switch (conv) {
    case FVC_STR_FLAG: {
      method = lookupMethod(thread, frame, value, SymbolId::kDunderStr);
      CHECK(!method.isError(),
            "__str__ doesn't exist for this object, which is impossible since "
            "object has a __str__, and everything descends from object");
      value = callMethod1(thread, frame, method, value);
      if (value.isError()) {
        return Continue::UNWIND;
      }
      if (!runtime->isInstanceOfStr(*value)) {
        thread->raiseWithFmt(LayoutId::kTypeError,
                             "__str__ returned non-string");
        return Continue::UNWIND;
      }
      break;
    }
    case FVC_REPR_FLAG: {
      method = lookupMethod(thread, frame, value, SymbolId::kDunderRepr);
      CHECK(!method.isError(),
            "__repr__ doesn't exist for this object, which is impossible since "
            "object has a __repr__, and everything descends from object");
      value = callMethod1(thread, frame, method, value);
      if (value.isError()) {
        return Continue::UNWIND;
      }
      if (!runtime->isInstanceOfStr(*value)) {
        thread->raiseWithFmt(LayoutId::kTypeError,
                             "__repr__ returned non-string");
        return Continue::UNWIND;
      }
      break;
    }
    case FVC_ASCII_FLAG: {
      method = lookupMethod(thread, frame, value, SymbolId::kDunderRepr);
      CHECK(!method.isError(),
            "__repr__ doesn't exist for this object, which is impossible since "
            "object has a __repr__, and everything descends from object");
      value = callMethod1(thread, frame, method, value);
      if (value.isError()) {
        return Continue::UNWIND;
      }
      if (!runtime->isInstanceOfStr(*value)) {
        thread->raiseWithFmt(LayoutId::kTypeError,
                             "__repr__ returned non-string");
        return Continue::UNWIND;
      }
      value = strEscapeNonASCII(thread, value);
      break;
    }
    default:  // 0: no conv
      break;
  }
  method = lookupMethod(thread, frame, value, SymbolId::kDunderFormat);
  if (method.isError()) {
    return Continue::UNWIND;
  }
  value = callMethod2(thread, frame, method, value, fmt_spec);
  if (value.isError()) {
    return Continue::UNWIND;
  }
  if (!runtime->isInstanceOfStr(*value)) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "__format__ returned non-string");
    return Continue::UNWIND;
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
  for (word i = arg - 1; i >= 0; i--) {
    Object key(&scope, keys.at(i));
    Object value(&scope, frame->popValue());
    thread->runtime()->dictAtPut(thread, dict, key, value);
  }
  frame->pushValue(*dict);
  return Continue::NEXT;
}

HANDLER_INLINE Continue Interpreter::doBuildString(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  Runtime* runtime = thread->runtime();
  switch (arg) {
    case 0:  // empty
      frame->pushValue(runtime->newStrWithAll(View<byte>(nullptr, 0)));
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
//     Unbound
//     callable <- Top of stack / lower memory addresses
//
// LOAD_METHOD is paired with a CALL_METHOD, and the matching CALL_METHOD
// falls back to the behavior of CALL_FUNCTION in this shape of the stack.
HANDLER_INLINE Continue Interpreter::doLoadMethod(Thread* thread, word arg) {
  Frame* frame = thread->currentFrame();
  frame->insertValueAt(Unbound::object(), 1);
  return doLoadAttr(thread, arg);
}

// LOAD_METHOD_CACHED shapes the stack in case of cache hit as follows:
//
//     Function
//     Receiver <- Top of stack / lower memory addresses
//
// LOAD_METHOD_CACHED is paired with a CALL_METHOD, and the matching CALL_METHOD
// bind Receiver to the self parameter to call Function to avoid creating
// a BoundMethod object.
//
// In case of cache miss, LOAD_METHOD_CACHED shapes the stack in the same way as
// LOAD_METHOD.
HANDLER_INLINE Continue Interpreter::doLoadMethodCached(Thread* thread,
                                                        word arg) {
  Frame* frame = thread->currentFrame();
  RawObject receiver = frame->topValue();
  LayoutId layout_id = receiver.layoutId();
  RawObject cached = icLookup(frame->caches(), arg, layout_id);
  // A function object is cached only when LOAD_ATTR_CACHED is guaranteed to
  // push a BoundMethod with the function via objectGetAttributeSetLocation().
  // Otherwise, LOAD_ATTR_CACHED caches only attribute's offsets.
  // Therefore, it's safe to push function/receiver pair to avoid BoundMethod
  // creation.
  if (cached.isFunction()) {
    frame->insertValueAt(cached, 1);
    return Continue::NEXT;
  }

  frame->insertValueAt(Unbound::object(), 1);
  if (cached.isErrorNotFound()) {
    return loadAttrUpdateCache(thread, arg);
  }
  RawObject result = loadAttrWithLocation(thread, receiver, cached);
  frame->setTopValue(result);
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
  DCHECK(maybe_method.isFunction(),
         "The pushed method should be either a function or Unbound");
  // Add one to bind receiver to the self argument. See doLoadMethod()
  // for details on the stack's shape.
  return handleCall(thread, arg + 1, arg + 1, 0, preparePositionalCall,
                    &Function::entry);
}

HANDLER_INLINE Continue Interpreter::cachedBinaryOpImpl(
    Thread* thread, word arg, OpcodeHandler update_cache,
    BinopFallbackHandler fallback) {
  Frame* frame = thread->currentFrame();
  RawObject left_raw = frame->peek(1);
  RawObject right_raw = frame->peek(0);
  LayoutId left_layout_id = left_raw.layoutId();
  LayoutId right_layout_id = right_raw.layoutId();
  IcBinopFlags flags;
  RawObject method = icLookupBinop(frame->caches(), arg, left_layout_id,
                                   right_layout_id, &flags);
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
  CompareOp op = static_cast<CompareOp>(icOriginalArg(frame->function(), arg));
  Object method(&scope, NoneType::object());
  IcBinopFlags flags;
  RawObject result = compareOperationSetMethod(thread, frame, op, left, right,
                                               &method, &flags);
  if (result.isError()) return Continue::UNWIND;
  if (!method.isNoneType()) {
    LayoutId left_layout_id = left.layoutId();
    LayoutId right_layout_id = right.layoutId();
    icUpdateBinop(frame->caches(), arg, left_layout_id, right_layout_id,
                  *method, flags);
  }
  frame->pushValue(result);
  return Continue::NEXT;
}

Continue Interpreter::compareOpFallback(Thread* thread, word arg,
                                        IcBinopFlags flags) {
  // Slow-path: We may need to call the reversed op when the first method
  // returned `NotImplemented`.
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  CompareOp op = static_cast<CompareOp>(icOriginalArg(frame->function(), arg));
  Object right(&scope, frame->popValue());
  Object left(&scope, frame->popValue());
  Object result(&scope,
                compareOperationRetry(thread, frame, op, flags, left, right));
  if (result.isError()) return Continue::UNWIND;
  frame->pushValue(*result);
  return Continue::NEXT;
}

HANDLER_INLINE
Continue Interpreter::doCompareOpCached(Thread* thread, word arg) {
  return cachedBinaryOpImpl(thread, arg, compareOpUpdateCache,
                            compareOpFallback);
}

Continue Interpreter::inplaceOpUpdateCache(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object right(&scope, frame->popValue());
  Object left(&scope, frame->popValue());
  BinaryOp op = static_cast<BinaryOp>(icOriginalArg(frame->function(), arg));
  Object method(&scope, NoneType::object());
  IcBinopFlags flags;
  RawObject result = inplaceOperationSetMethod(thread, frame, op, left, right,
                                               &method, &flags);
  if (!method.isNoneType()) {
    LayoutId left_layout_id = left.layoutId();
    LayoutId right_layout_id = right.layoutId();
    icUpdateBinop(frame->caches(), arg, left_layout_id, right_layout_id,
                  *method, flags);
  }
  if (result.isError()) return Continue::UNWIND;
  frame->pushValue(result);
  return Continue::NEXT;
}

Continue Interpreter::inplaceOpFallback(Thread* thread, word arg,
                                        IcBinopFlags flags) {
  // Slow-path: We may need to try other ways to resolve things when the first
  // call returned `NotImplemented`.
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  BinaryOp op = static_cast<BinaryOp>(icOriginalArg(frame->function(), arg));
  Object right(&scope, frame->popValue());
  Object left(&scope, frame->popValue());
  Object result(&scope, NoneType::object());
  if (flags & IC_INPLACE_BINOP_RETRY) {
    // The cached operation was an in-place operation we have to try to the
    // usual binary operation mechanics now.
    result = binaryOperation(thread, frame, op, left, right);
  } else {
    // The cached operation was already a binary operation (e.g. __add__ or
    // __radd__) so we have to invoke `binaryOperationRetry`.
    result = binaryOperationRetry(thread, frame, op, flags, left, right);
  }
  if (result.isError()) return Continue::UNWIND;
  frame->pushValue(*result);
  return Continue::NEXT;
}

HANDLER_INLINE
Continue Interpreter::doInplaceOpCached(Thread* thread, word arg) {
  return cachedBinaryOpImpl(thread, arg, inplaceOpUpdateCache,
                            inplaceOpFallback);
}

Continue Interpreter::binaryOpUpdateCache(Thread* thread, word arg) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object right(&scope, frame->popValue());
  Object left(&scope, frame->popValue());
  BinaryOp op = static_cast<BinaryOp>(icOriginalArg(frame->function(), arg));
  Object method(&scope, NoneType::object());
  IcBinopFlags flags;
  Object result(&scope, binaryOperationSetMethod(thread, frame, op, left, right,
                                                 &method, &flags));
  if (!method.isNoneType()) {
    icUpdateBinop(frame->caches(), arg, left.layoutId(), right.layoutId(),
                  *method, flags);
  }
  if (result.isErrorException()) return Continue::UNWIND;
  frame->pushValue(*result);
  return Continue::NEXT;
}

Continue Interpreter::binaryOpFallback(Thread* thread, word arg,
                                       IcBinopFlags flags) {
  // Slow-path: We may need to call the reversed op when the first method
  // returned `NotImplemented`.
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  BinaryOp op = static_cast<BinaryOp>(icOriginalArg(frame->function(), arg));
  Object right(&scope, frame->popValue());
  Object left(&scope, frame->popValue());
  Object result(&scope,
                binaryOperationRetry(thread, frame, op, flags, left, right));
  if (result.isErrorException()) return Continue::UNWIND;
  frame->pushValue(*result);
  return Continue::NEXT;
}

HANDLER_INLINE
Continue Interpreter::doBinaryOpCached(Thread* thread, word arg) {
  return cachedBinaryOpImpl(thread, arg, binaryOpUpdateCache, binaryOpFallback);
}

RawObject Interpreter::execute(Thread* thread) {
  auto do_return = [thread] {
    Frame* frame = thread->currentFrame();
    RawObject return_val = frame->popValue();
    thread->popFrame();
    return return_val;
  };

  Frame* entry_frame = thread->currentFrame();

  // TODO(bsimmers): This check is only relevant for generators, and each
  // callsite of Interpreter::execute() can know statically whether or not an
  // exception is ready for throwing. Once the shape of the interpreter settles
  // down, we should restructure it to take advantage of this fact, likely by
  // adding an alternate entry point that always throws (and asserts that an
  // exception is pending).
  if (thread->hasPendingException()) {
    DCHECK(entry_frame->function().isCoroutineOrGenerator(),
           "Entered dispatch loop with a pending exception outside of "
           "generator/coroutine");
    if (unwind(thread, entry_frame)) return do_return();
  }

  executeImpl(thread, entry_frame);
  return do_return();
}

void Interpreter::executeImpl(Thread* thread, Frame* entry_frame) {
  // Silence warnings about computed goto
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

  static const void* const dispatch_table[] = {
#define OP(name, id, handler)                                                  \
  name == EXTENDED_ARG ? &&extendedArg : &&handle##name,
      FOREACH_BYTECODE(OP)
#undef OP
  };

  Bytecode bc;
  int32_t arg;
  Continue cont;
  auto next_label = [&]() __attribute__((always_inline)) {
    Frame* current_frame = thread->currentFrame();
    word pc = current_frame->virtualPC();
    static_assert(endian::native == endian::little, "big endian unsupported");
    static_assert(Frame::kCodeUnitSize == sizeof(uint16_t), "matching type");
    arg = current_frame->bytecode().uint16At(pc);
    current_frame->setVirtualPC(pc + Frame::kCodeUnitSize);
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
    static_assert(Frame::kCodeUnitSize == sizeof(uint16_t), "matching type");
    uint16_t bytes_at = current_frame->bytecode().uint16At(pc);
    current_frame->setVirtualPC(pc + Frame::kCodeUnitSize);
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
    if (unwind(thread, entry_frame)) return;
  } else if (cont == Continue::RETURN) {
    if (handleReturn(thread, entry_frame)) return;
  } else {
    DCHECK(cont == Continue::YIELD, "expected RETURN, UNWIND or YIELD");
    return;
  }
  goto* next_label();
#pragma GCC diagnostic pop
}

}  // namespace python
