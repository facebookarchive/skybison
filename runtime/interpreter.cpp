#include "interpreter.h"

#include <cstdio>
#include <cstdlib>

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines.h"

namespace python {

RawObject Interpreter::prepareCallableCall(Thread* thread, Frame* frame,
                                           word callable_idx, word* nargs) {
  HandleScope scope(thread);
  Object callable(&scope, frame->peek(callable_idx));
  DCHECK(!callable->isFunction(),
         "prepareCallableCall should only be called on non-function types");
  bool is_bound;
  if (callable->isBoundMethod()) {
    // Shift all arguments on the stack down by 1 and unpack the BoundMethod.
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
    BoundMethod method(&scope, *callable);
    frame->insertValueAt(method->self(), callable_idx);
    is_bound = false;
    callable = method->function();
  } else {
    Runtime* runtime = thread->runtime();
    for (;;) {
      Type type(&scope, runtime->typeOf(*callable));
      Object attr(&scope, runtime->lookupSymbolInMro(thread, type,
                                                     SymbolId::kDunderCall));
      if (!attr->isError()) {
        if (attr->isFunction()) {
          // Do not create a short-lived bound method object.
          is_bound = false;
          callable = *attr;
          break;
        }
        if (runtime->isNonDataDescriptor(thread, attr)) {
          Object owner(&scope, *type);
          callable = callDescriptorGet(thread, frame, attr, callable, owner);
          if (callable->isFunction()) {
            // Descriptors do not return unbound methods.
            is_bound = true;
            break;
          }
          // Retry the lookup using the object returned by the descriptor.
          continue;
        }
      }
      UNIMPLEMENTED("throw TypeError: object is not callable");
    }
  }
  if (is_bound) {
    frame->setValueAt(*callable, callable_idx);
  } else {
    frame->insertValueAt(*callable, callable_idx + 1);
    *nargs += 1;
  }
  return *callable;
}

RawObject Interpreter::call(Thread* thread, Frame* frame, word nargs) {
  RawObject* sp = frame->valueStackTop() + nargs + 1;
  RawObject callable = frame->peek(nargs);
  if (!callable->isFunction()) {
    callable = prepareCallableCall(thread, frame, nargs, &nargs);
  }
  RawObject result = RawFunction::cast(callable)->entry()(thread, frame, nargs);
  // Clear the stack of the function object.
  frame->setValueStackTop(sp);
  return result;
}

RawObject Interpreter::callKw(Thread* thread, Frame* frame, word nargs) {
  // Top of stack is a tuple of keyword argument names in the order they
  // appear on the stack.
  RawObject* sp = frame->valueStackTop() + nargs + 2;
  RawObject result;
  RawObject callable = frame->peek(nargs + 1);
  if (callable->isFunction()) {
    result = RawFunction::cast(callable)->entryKw()(thread, frame, nargs);
  } else {
    callable = prepareCallableCall(thread, frame, nargs + 1, &nargs);
    result = RawFunction::cast(callable)->entryKw()(thread, frame, nargs);
  }
  frame->setValueStackTop(sp);
  return result;
}

RawObject Interpreter::callEx(Thread* thread, Frame* frame, word flags) {
  // Low bit of flags indicates whether var-keyword argument is on TOS.
  // In all cases, var-positional tuple is next, followed by the function
  // pointer.
  word function_position = (flags & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1;
  RawObject* sp = frame->valueStackTop() + function_position + 1;
  RawObject function = frame->peek(function_position);
  if (!function->isFunction()) {
    HandleScope scope(thread);
    Object callable(&scope, function);
    // Create a new argument tuple with self as the first argument
    word args_position = function_position - 1;
    Tuple args(&scope, frame->peek(args_position));
    Tuple new_args(&scope, thread->runtime()->newTuple(args->length() + 1));
    Object target(&scope, NoneType::object());
    if (callable->isBoundMethod()) {
      BoundMethod method(&scope, *callable);
      new_args->atPut(0, method->self());
      target = method->function();
    } else {
      new_args->atPut(0, *callable);
      target = lookupMethod(thread, frame, callable, SymbolId::kDunderCall);
    }
    new_args->replaceFromWith(1, *args);
    frame->setValueAt(*target, function_position);
    frame->setValueAt(*new_args, args_position);
    function = *target;
  }
  RawObject result =
      RawFunction::cast(function)->entryEx()(thread, frame, flags);
  frame->setValueStackTop(sp);
  return result;
}

RawObject Interpreter::stringJoin(Thread* thread, RawObject* sp, word num) {
  word new_len = 0;
  for (word i = num - 1; i >= 0; i--) {
    if (!sp[i]->isStr()) {
      UNIMPLEMENTED("Conversion of non-string values not supported.");
    }
    new_len += RawStr::cast(sp[i])->length();
  }

  if (new_len <= RawSmallStr::kMaxLength) {
    byte buffer[RawSmallStr::kMaxLength];
    byte* ptr = buffer;
    for (word i = num - 1; i >= 0; i--) {
      RawStr str = RawStr::cast(sp[i]);
      word len = str->length();
      str->copyTo(ptr, len);
      ptr += len;
    }
    return SmallStr::fromBytes(View<byte>(buffer, new_len));
  }

  HandleScope scope(thread);
  LargeStr result(&scope, thread->runtime()->heap()->createLargeStr(new_len));
  word offset = RawLargeStr::kDataOffset;
  for (word i = num - 1; i >= 0; i--) {
    RawStr str = RawStr::cast(sp[i]);
    word len = str->length();
    str->copyTo(reinterpret_cast<byte*>(result->address() + offset), len);
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
  Object method(&scope, runtime->lookupSymbolInMro(thread, descriptor_type,
                                                   SymbolId::kDunderGet));
  DCHECK(!method->isError(), "no __get__ method found");
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
  Object method(&scope, runtime->lookupSymbolInMro(thread, descriptor_type,
                                                   SymbolId::kDunderSet));
  DCHECK(!method->isError(), "no __set__ method found");
  return callMethod3(thread, caller, method, descriptor, receiver, value);
}

RawObject Interpreter::callDescriptorDelete(Thread* thread, Frame* caller,
                                            const Object& descriptor,
                                            const Object& receiver) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type descriptor_type(&scope, runtime->typeOf(*descriptor));
  Object method(&scope, runtime->lookupSymbolInMro(thread, descriptor_type,
                                                   SymbolId::kDunderDelete));
  DCHECK(!method->isError(), "no __delete__ method found");
  return callMethod2(thread, caller, method, descriptor, receiver);
}

RawObject Interpreter::lookupMethod(Thread* thread, Frame* caller,
                                    const Object& receiver, SymbolId selector) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(*receiver));
  Object method(&scope, runtime->lookupSymbolInMro(thread, type, selector));
  if (method->isFunction()) {
    // Do not create a short-lived bound method object.
    return *method;
  }
  if (!method->isError()) {
    if (runtime->isNonDataDescriptor(thread, method)) {
      Object owner(&scope, *type);
      return callDescriptorGet(thread, caller, method, receiver, owner);
    }
  }
  return *method;
}

RawObject Interpreter::callMethod1(Thread* thread, Frame* caller,
                                   const Object& method, const Object& self) {
  word nargs = 0;
  caller->pushValue(*method);
  if (method->isFunction()) {
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
  if (method->isFunction()) {
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
  if (method->isFunction()) {
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
  if (method->isFunction()) {
    caller->pushValue(*self);
    nargs += 1;
  }
  caller->pushValue(*arg1);
  caller->pushValue(*arg2);
  caller->pushValue(*arg3);
  return call(thread, caller, nargs);
}

RawObject Interpreter::unaryOperation(Thread* thread, Frame* caller,
                                      const Object& self, SymbolId selector) {
  HandleScope scope(thread);
  Object method(&scope, lookupMethod(thread, caller, self, selector));
  CHECK(!method->isError(), "unknown unary operation");
  return callMethod1(thread, caller, method, self);
}

void Interpreter::doUnaryOperation(SymbolId selector, Context* ctx) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Object receiver(&scope, ctx->frame->topValue());
  RawObject result = unaryOperation(thread, ctx->frame, receiver, selector);
  ctx->frame->setTopValue(result);
}

RawObject Interpreter::binaryOperation(Thread* thread, Frame* caller,
                                       BinaryOp op, const Object& self,
                                       const Object& other) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  SymbolId selector = runtime->binaryOperationSelector(op);
  Object self_method(&scope, lookupMethod(thread, caller, self, selector));

  SymbolId swapped_selector = runtime->swappedBinaryOperationSelector(op);
  Object self_reversed_method(
      &scope, lookupMethod(thread, caller, self, swapped_selector));
  Object other_reversed_method(
      &scope, lookupMethod(thread, caller, other, swapped_selector));

  bool try_other = true;
  if (!self_method->isError()) {
    if (runtime->shouldReverseBinaryOperation(
            thread, self, self_reversed_method, other, other_reversed_method)) {
      Object result(&scope, callMethod2(thread, caller, other_reversed_method,
                                        other, self));
      if (!result->isNotImplemented()) return *result;
      try_other = false;
    }
    Object result(&scope,
                  callMethod2(thread, caller, self_method, self, other));
    if (!result->isNotImplemented()) return *result;
  }
  if (try_other && !other_reversed_method->isError()) {
    Object result(&scope, callMethod2(thread, caller, other_reversed_method,
                                      other, self));
    if (!result->isNotImplemented()) return *result;
  }

  Type self_type(&scope, runtime->typeOf(*self));
  Type other_type(&scope, runtime->typeOf(*other));
  UNIMPLEMENTED("Cannot do binary op %ld for types '%s' and '%s'",
                static_cast<word>(op),
                RawStr::cast(self_type->name())->toCStr(),
                RawStr::cast(other_type->name())->toCStr());
}

void Interpreter::doBinaryOperation(BinaryOp op, Context* ctx) {
  Thread* thread = ctx->thread;
  Frame* frame = ctx->frame;
  HandleScope scope(thread);
  Object other(&scope, frame->popValue());
  Object self(&scope, frame->popValue());
  RawObject result = binaryOperation(thread, frame, op, self, other);
  ctx->frame->pushValue(result);
}

RawObject Interpreter::inplaceOperation(Thread* thread, Frame* caller,
                                        BinaryOp op, const Object& self,
                                        const Object& other) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  SymbolId selector = runtime->inplaceOperationSelector(op);
  Object method(&scope, lookupMethod(thread, caller, self, selector));
  if (!method->isError()) {
    RawObject result = callMethod2(thread, caller, method, self, other);
    if (result != runtime->notImplemented()) {
      return result;
    }
  }
  return binaryOperation(thread, caller, op, self, other);
}

void Interpreter::doInplaceOperation(BinaryOp op, Context* ctx) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Object other(&scope, ctx->frame->popValue());
  Object self(&scope, ctx->frame->popValue());
  RawObject result = inplaceOperation(thread, ctx->frame, op, self, other);
  ctx->frame->pushValue(result);
}

RawObject Interpreter::compareOperation(Thread* thread, Frame* caller,
                                        CompareOp op, const Object& left,
                                        const Object& right) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Type left_type(&scope, runtime->typeOf(*left));
  Type right_type(&scope, runtime->typeOf(*right));

  bool try_swapped = true;
  bool has_different_type = (*left_type != *right_type);
  if (has_different_type &&
      runtime->isSubclass(right_type, left_type) == Bool::trueObj()) {
    try_swapped = false;
    SymbolId selector = runtime->swappedComparisonSelector(op);
    Object method(&scope, lookupMethod(thread, caller, right, selector));
    if (!method->isError()) {
      RawObject result = callMethod2(thread, caller, method, right, left);
      if (result != runtime->notImplemented()) {
        return result;
      }
    }
  } else {
    SymbolId selector = runtime->comparisonSelector(op);
    Object method(&scope, lookupMethod(thread, caller, left, selector));
    if (!method->isError()) {
      RawObject result = callMethod2(thread, caller, method, left, right);
      if (result != runtime->notImplemented()) {
        return result;
      }
    }
  }
  if (has_different_type && try_swapped) {
    SymbolId selector = runtime->swappedComparisonSelector(op);
    Object method(&scope, lookupMethod(thread, caller, right, selector));
    if (!method->isError()) {
      RawObject result = callMethod2(thread, caller, method, right, left);
      if (result != runtime->notImplemented()) {
        return result;
      }
    }
  }
  if (op == CompareOp::EQ) {
    return Bool::fromBool(*left == *right);
  }
  if (op == CompareOp::NE) {
    return Bool::fromBool(*left != *right);
  }
  UNIMPLEMENTED("throw");
}

RawObject Interpreter::sequenceContains(Thread* thread, Frame* caller,
                                        const Object& value,
                                        const Object& container) {
  HandleScope scope(thread);
  Object method(&scope, lookupMethod(thread, caller, container,
                                     SymbolId::kDunderContains));
  if (!method->isError()) {
    Object result(&scope,
                  callMethod2(thread, caller, method, container, value));
    caller->pushValue(*result);
    RawObject is_true = isTrue(thread, caller);
    caller->popValue();
    return is_true;
  }
  UNIMPLEMENTED("fallback to iter search.");
}

RawObject Interpreter::isTrue(Thread* thread, Frame* caller) {
  HandleScope scope(thread);
  Object self(&scope, caller->topValue());
  if (self->isBool()) {
    return *self;
  }
  if (self->isNoneType()) {
    return Bool::falseObj();
  }
  Object method(&scope,
                lookupMethod(thread, caller, self, SymbolId::kDunderBool));
  if (!method->isError()) {
    Object result(&scope, callMethod1(thread, caller, method, self));
    if (result->isBool()) {
      return *result;
    }
    if (result->isInt()) {
      Int integer(&scope, *result);
      return Bool::fromBool(integer->asWord() > 0);
    }
    UNIMPLEMENTED("throw");
  }
  method = lookupMethod(thread, caller, self, SymbolId::kDunderLen);
  if (!method->isError()) {
    Object result(&scope, callMethod1(thread, caller, method, self));
    if (result->isInt()) {
      Int integer(&scope, *result);
      if (integer->isPositive()) {
        return Bool::trueObj();
      }
      if (integer->isZero()) {
        return Bool::falseObj();
      }
      UNIMPLEMENTED("throw");
    }
  }
  return Bool::trueObj();
}

RawObject Interpreter::yieldFrom(Thread* thread, Frame* frame) {
  HandleScope scope(thread);
  Object value(&scope, frame->popValue());
  Object iterator(&scope, frame->topValue());
  Object result(&scope, NoneType::object());
  if (value->isNoneType()) {
    Object next_method(
        &scope, lookupMethod(thread, frame, iterator, SymbolId::kDunderNext));
    if (next_method->isError()) {
      thread->raiseTypeErrorWithCStr("iter() returned non-iterator");
      thread->abortOnPendingException();
    }
    result = callMethod1(thread, frame, next_method, iterator);
    if (result->isError() && !thread->hasPendingStopIteration()) {
      thread->abortOnPendingException();
    }
  } else {
    Object send_method(&scope,
                       lookupMethod(thread, frame, iterator, SymbolId::kSend));
    if (send_method->isError()) {
      thread->raiseTypeErrorWithCStr("iter() returned non-iterator");
      thread->abortOnPendingException();
    }
    result = callMethod2(thread, frame, send_method, iterator, value);
    if (result->isError() && !thread->hasPendingStopIteration()) {
      thread->abortOnPendingException();
    }
  }
  if (result->isError()) {
    if (thread->hasPendingStopIteration()) {
      frame->setTopValue(thread->exceptionValue());
      thread->clearPendingException();
      return Error::object();
    }
    thread->abortOnPendingException();
  }
  // Unlike YIELD_VALUE, don't update PC in the frame: we want this
  // instruction to re-execute until the subiterator is exhausted.
  GeneratorBase gen(&scope, thread->runtime()->genFromStackFrame(frame));
  thread->runtime()->genSave(thread, gen);
  return *result;
}

static Bytecode currentBytecode(const Interpreter::Context* ctx) {
  RawBytes code = RawBytes::cast(RawCode::cast(ctx->frame->code())->code());
  word pc = ctx->pc;
  word result;
  do {
    pc -= 2;
    result = code->byteAt(pc);
  } while (result == Bytecode::EXTENDED_ARG);
  return static_cast<Bytecode>(result);
}

void Interpreter::doInvalidBytecode(Context* ctx, word) {
  Bytecode bc = currentBytecode(ctx);
  UNREACHABLE("bytecode '%s'", kBytecodeNames[bc]);
}

void Interpreter::doNotImplemented(Context* ctx, word) {
  Bytecode bc = currentBytecode(ctx);
  UNIMPLEMENTED("bytecode '%s'", kBytecodeNames[bc]);
}

// opcode 1
void Interpreter::doPopTop(Context* ctx, word) { ctx->frame->popValue(); }

// opcode 2
void Interpreter::doRotTwo(Context* ctx, word) {
  RawObject* sp = ctx->frame->valueStackTop();
  RawObject top = *sp;
  *sp = *(sp + 1);
  *(sp + 1) = top;
}

// opcode 3
void Interpreter::doRotThree(Context* ctx, word) {
  RawObject* sp = ctx->frame->valueStackTop();
  RawObject top = *sp;
  *sp = *(sp + 1);
  *(sp + 1) = *(sp + 2);
  *(sp + 2) = top;
}

// opcode 4
void Interpreter::doDupTop(Context* ctx, word) {
  ctx->frame->pushValue(ctx->frame->topValue());
}

// opcode 5
void Interpreter::doDupTopTwo(Context* ctx, word) {
  RawObject first = ctx->frame->topValue();
  RawObject second = ctx->frame->peek(1);
  ctx->frame->pushValue(second);
  ctx->frame->pushValue(first);
}

// opcode 9
void Interpreter::doNop(Context*, word) {}

// opcode 10
void Interpreter::doUnaryPositive(Context* ctx, word) {
  doUnaryOperation(SymbolId::kDunderPos, ctx);
}

// opcode 11
void Interpreter::doUnaryNegative(Context* ctx, word) {
  doUnaryOperation(SymbolId::kDunderNeg, ctx);
}

// opcode 12
void Interpreter::doUnaryNot(Context* ctx, word) {
  if (isTrue(ctx->thread, ctx->frame) == Bool::trueObj()) {
    ctx->frame->setTopValue(Bool::falseObj());
  } else {
    ctx->frame->setTopValue(Bool::trueObj());
  }
}

// opcode 15
void Interpreter::doUnaryInvert(Context* ctx, word) {
  doUnaryOperation(SymbolId::kDunderInvert, ctx);
}

// opcode 16
void Interpreter::doBinaryMatrixMultiply(Context* ctx, word) {
  doBinaryOperation(BinaryOp::MATMUL, ctx);
}

// opcode 17
void Interpreter::doInplaceMatrixMultiply(Context* ctx, word) {
  doInplaceOperation(BinaryOp::MATMUL, ctx);
}

// opcode 19
void Interpreter::doBinaryPower(Context* ctx, word) {
  doBinaryOperation(BinaryOp::POW, ctx);
}

// opcode 20
void Interpreter::doBinaryMultiply(Context* ctx, word) {
  doBinaryOperation(BinaryOp::MUL, ctx);
}

// opcode 22
void Interpreter::doBinaryModulo(Context* ctx, word) {
  doBinaryOperation(BinaryOp::MOD, ctx);
}

// opcode 23
void Interpreter::doBinaryAdd(Context* ctx, word) {
  doBinaryOperation(BinaryOp::ADD, ctx);
}

// opcode 24
void Interpreter::doBinarySubtract(Context* ctx, word) {
  doBinaryOperation(BinaryOp::SUB, ctx);
}

// opcode 25
void Interpreter::doBinarySubscr(Context* ctx, word) {
  HandleScope scope(ctx->thread);
  Runtime* runtime = ctx->thread->runtime();
  Object key(&scope, ctx->frame->popValue());
  Object container(&scope, ctx->frame->popValue());
  Type type(&scope, runtime->typeOf(*container));
  Object getitem(&scope, runtime->lookupSymbolInMro(ctx->thread, type,
                                                    SymbolId::kDunderGetItem));
  if (getitem->isError()) {
    ctx->frame->pushValue(ctx->thread->raiseTypeErrorWithCStr(
        "object does not support indexing"));
  } else {
    ctx->frame->pushValue(
        callMethod2(ctx->thread, ctx->frame, getitem, container, key));
  }
}

// opcode 26
void Interpreter::doBinaryFloorDivide(Context* ctx, word) {
  doBinaryOperation(BinaryOp::FLOORDIV, ctx);
}

// opcode 27
void Interpreter::doBinaryTrueDivide(Context* ctx, word) {
  doBinaryOperation(BinaryOp::TRUEDIV, ctx);
}

// opcode 28
void Interpreter::doInplaceFloorDivide(Context* ctx, word) {
  doInplaceOperation(BinaryOp::FLOORDIV, ctx);
}

// opcode 29
void Interpreter::doInplaceTrueDivide(Context* ctx, word) {
  doInplaceOperation(BinaryOp::TRUEDIV, ctx);
}

// opcode 50
void Interpreter::doGetAiter(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Object obj(&scope, ctx->frame->topValue());
  Object method(&scope,
                lookupMethod(thread, ctx->frame, obj, SymbolId::kDunderAiter));
  if (method->isError()) {
    thread->raiseTypeErrorWithCStr(
        "'async for' requires an object with __aiter__ method");
    thread->abortOnPendingException();
  }
  Object aiter(&scope, callMethod1(thread, ctx->frame, method, obj));
  thread->abortOnPendingException();
  ctx->frame->setTopValue(*aiter);
}

// opcode 51
void Interpreter::doGetAnext(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Object obj(&scope, ctx->frame->topValue());
  Object anext(&scope,
               lookupMethod(thread, ctx->frame, obj, SymbolId::kDunderAnext));
  if (anext->isError()) {
    thread->raiseTypeErrorWithCStr(
        "'async for' requires an iterator with __anext__ method");
    thread->abortOnPendingException();
  }
  Object awaitable(&scope, callMethod1(thread, ctx->frame, anext, obj));
  thread->abortOnPendingException();

  // TODO(T33628943): Check if `awaitable` is a native or generator-based
  // coroutine and if it is, no need to call __await__
  Object await(&scope,
               lookupMethod(thread, ctx->frame, obj, SymbolId::kDunderAwait));
  if (await->isError()) {
    thread->raiseTypeErrorWithCStr(
        "'async for' received an invalid object from __anext__");
    thread->abortOnPendingException();
  }
  Object aiter(&scope, callMethod1(thread, ctx->frame, await, awaitable));
  thread->abortOnPendingException();

  ctx->frame->setTopValue(*aiter);
}

// opcode 52
void Interpreter::doBeforeAsyncWith(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Frame* frame = ctx->frame;
  Object manager(&scope, frame->popValue());

  // resolve __aexit__ an push it
  Runtime* runtime = thread->runtime();
  Object exit_selector(&scope, runtime->symbols()->DunderAexit());
  Object exit(&scope, runtime->attributeAt(thread, manager, exit_selector));
  if (exit->isError()) {
    UNIMPLEMENTED("throw TypeError");
  }
  frame->pushValue(*exit);

  // resolve __aenter__ call it and push the return value
  Object enter(&scope,
               lookupMethod(thread, frame, manager, SymbolId::kDunderAenter));
  if (enter->isError()) {
    UNIMPLEMENTED("throw TypeError");
  }
  RawObject result = callMethod1(thread, frame, enter, manager);
  thread->abortOnPendingException();
  frame->pushValue(result);
}

// opcode 55
void Interpreter::doInplaceAdd(Context* ctx, word) {
  doInplaceOperation(BinaryOp::ADD, ctx);
}

// opcode 56
void Interpreter::doInplaceSubtract(Context* ctx, word) {
  doInplaceOperation(BinaryOp::SUB, ctx);
}

// opcode 57
void Interpreter::doInplaceMultiply(Context* ctx, word) {
  doInplaceOperation(BinaryOp::MUL, ctx);
}

// opcode 59
void Interpreter::doInplaceModulo(Context* ctx, word) {
  doInplaceOperation(BinaryOp::MOD, ctx);
}

// opcode 60
void Interpreter::doStoreSubscr(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Object key(&scope, ctx->frame->popValue());
  Object container(&scope, ctx->frame->popValue());
  Object setitem(&scope, lookupMethod(thread, ctx->frame, container,
                                      SymbolId::kDunderSetItem));
  if (setitem->isError()) {
    UNIMPLEMENTED("throw TypeError");
  }
  Object value(&scope, ctx->frame->popValue());
  RawObject result =
      callMethod3(thread, ctx->frame, setitem, container, key, value);
  // TODO(T31788973): propagate an exception
  thread->abortOnPendingException();
  ctx->frame->pushValue(result);
}

// opcode 61
void Interpreter::doDeleteSubscr(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Object key(&scope, ctx->frame->popValue());
  Object container(&scope, ctx->frame->popValue());
  Object delitem(&scope, lookupMethod(thread, ctx->frame, container,
                                      SymbolId::kDunderDelItem));
  if (delitem->isError()) {
    UNIMPLEMENTED("throw TypeError");
  }
  RawObject result = callMethod2(thread, ctx->frame, delitem, container, key);
  // TODO(T31788973): propagate an exception
  thread->abortOnPendingException();
  ctx->frame->pushValue(result);
}

// opcode 62
void Interpreter::doBinaryLshift(Context* ctx, word) {
  doBinaryOperation(BinaryOp::LSHIFT, ctx);
}

// opcode 63
void Interpreter::doBinaryRshift(Context* ctx, word) {
  doBinaryOperation(BinaryOp::RSHIFT, ctx);
}

// opcode 64
void Interpreter::doBinaryAnd(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Object other(&scope, ctx->frame->popValue());
  Object self(&scope, ctx->frame->popValue());
  RawObject result =
      binaryOperation(thread, ctx->frame, BinaryOp::AND, self, other);
  ctx->frame->pushValue(result);
}

// opcode 65
void Interpreter::doBinaryXor(Context* ctx, word) {
  doBinaryOperation(BinaryOp::XOR, ctx);
}

// opcode 66
void Interpreter::doBinaryOr(Context* ctx, word) {
  doBinaryOperation(BinaryOp::OR, ctx);
}

// opcode 67
void Interpreter::doInplacePower(Context* ctx, word) {
  doInplaceOperation(BinaryOp::POW, ctx);
}

// opcode 68
void Interpreter::doGetIter(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Object iterable(&scope, ctx->frame->topValue());
  Object method(&scope, lookupMethod(thread, ctx->frame, iterable,
                                     SymbolId::kDunderIter));
  if (method->isError()) {
    thread->raiseTypeErrorWithCStr("object is not iterable");
    thread->abortOnPendingException();
  }
  Object iterator(&scope, callMethod1(thread, ctx->frame, method, iterable));
  thread->abortOnPendingException();
  ctx->frame->setTopValue(*iterator);
}

// opcode 69
void Interpreter::doGetYieldFromIter(Context* ctx, word) {
  Thread* thread = ctx->thread;
  Frame* frame = ctx->frame;
  HandleScope scope(thread);
  Object iterable(&scope, frame->topValue());

  if (iterable->isGenerator()) {
    return;
  }

  if (iterable->isCoroutine()) {
    Code code(&scope, frame->code());
    if (code->hasCoroutine() || code->hasIterableCoroutine()) {
      thread->raiseTypeErrorWithCStr(
          "cannot 'yield from' a coroutine object in a non-coroutine "
          "generator");
      thread->abortOnPendingException();
    }
    return;
  }

  Object method(&scope,
                lookupMethod(thread, frame, iterable, SymbolId::kDunderIter));
  if (method->isError()) {
    thread->raiseTypeErrorWithCStr("object is not iterable");
    thread->abortOnPendingException();
  }
  Object iterator(&scope, callMethod1(thread, frame, method, iterable));
  thread->abortOnPendingException();
  frame->setTopValue(*iterator);
}

// opcode 70
void Interpreter::doPrintExpr(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Frame* frame = ctx->frame;
  Object value(&scope, frame->popValue());
  ValueCell value_cell(&scope, thread->runtime()->displayHook());
  if (value_cell->isUnbound()) {
    UNIMPLEMENTED("RuntimeError: lost sys.displayhook");
  }
  frame->pushValue(value_cell->value());
  frame->pushValue(*value);
  call(thread, frame, 1);
}

// opcode 71
void Interpreter::doLoadBuildClass(Context* ctx, word) {
  Thread* thread = ctx->thread;
  RawValueCell value_cell = RawValueCell::cast(thread->runtime()->buildClass());
  ctx->frame->pushValue(value_cell->value());
}

// opcode 73
void Interpreter::doGetAwaitable(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Object obj(&scope, ctx->frame->topValue());

  // TODO(T33628943): Check if `obj` is a native or generator-based
  // coroutine and if it is, no need to call __await__
  Object await(&scope,
               lookupMethod(thread, ctx->frame, obj, SymbolId::kDunderAwait));
  if (await->isError()) {
    thread->raiseTypeErrorWithCStr(
        "object can't be used in 'await' expression");
    thread->abortOnPendingException();
  }
  Object iter(&scope, callMethod1(thread, ctx->frame, await, obj));
  thread->abortOnPendingException();

  ctx->frame->setTopValue(*iter);
}

// opcode 75
void Interpreter::doInplaceLshift(Context* ctx, word) {
  doInplaceOperation(BinaryOp::LSHIFT, ctx);
}

// opcode 76
void Interpreter::doInplaceRshift(Context* ctx, word) {
  doInplaceOperation(BinaryOp::RSHIFT, ctx);
}

// opcode 77
void Interpreter::doInplaceAnd(Context* ctx, word) {
  doInplaceOperation(BinaryOp::AND, ctx);
}

// opcode 78
void Interpreter::doInplaceXor(Context* ctx, word) {
  doInplaceOperation(BinaryOp::XOR, ctx);
}

// opcode 79
void Interpreter::doInplaceOr(Context* ctx, word) {
  doInplaceOperation(BinaryOp::OR, ctx);
}

// opcode 80
void Interpreter::doBreakLoop(Context* ctx, word) {
  Frame* frame = ctx->frame;
  TryBlock block = frame->blockStack()->pop();
  ctx->pc = block.handler();
}

// opcode 81
void Interpreter::doWithCleanupStart(Context* ctx, word) {
  HandleScope scope(ctx->thread);
  Object exc(&scope, ctx->frame->popValue());
  if (exc->isNoneType()) {
    // This is a bound method.
    Object exit(&scope, ctx->frame->topValue());
    Object none(&scope, NoneType::object());
    ctx->frame->setTopValue(*exc);
    Object result(&scope, callMethod4(ctx->thread, ctx->frame, exit, none, none,
                                      none, none));
    ctx->frame->pushValue(*exc);
    ctx->frame->pushValue(*result);
  } else {
    UNIMPLEMENTED("exception handling in context manager");
  }
}

// opcode 82
void Interpreter::doWithCleanupFinish(Context* ctx, word) {
  HandleScope scope(ctx->thread);
  Object result(&scope, ctx->frame->popValue());
  Object exc(&scope, ctx->frame->popValue());
  if (!exc->isNoneType()) {
    UNIMPLEMENTED("exception handling in context manager");
  }
}

// opcode 85
void Interpreter::doSetupAnnotations(Context* ctx, word) {
  HandleScope scope(ctx->thread);
  Runtime* runtime = ctx->thread->runtime();
  Dict implicit_globals(&scope, ctx->frame->implicitGlobals());
  Object annotations(&scope,
                     runtime->symbols()->at(SymbolId::kDunderAnnotations));
  Object anno_dict(&scope, runtime->dictAt(implicit_globals, annotations));
  if (anno_dict->isError()) {
    Object new_dict(&scope, runtime->newDict());
    runtime->dictAtPutInValueCell(implicit_globals, annotations, new_dict);
  }
}

// opcode 84
void Interpreter::doImportStar(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);

  // Pre-python3 this used to merge the locals with the locals dict. However,
  // that's not necessary anymore. You can't import * inside a function
  // body anymore.

  Code code(&scope, ctx->frame->code());
  Module module(&scope, ctx->frame->popValue());
  CHECK(module->isModule(), "Unexpected type to import from");

  Frame* frame = ctx->frame;
  Dict implicit_globals(&scope, frame->implicitGlobals());
  thread->runtime()->moduleImportAllFrom(implicit_globals, module);
}

// opcode 87
void Interpreter::doPopBlock(Context* ctx, word) {
  Frame* frame = ctx->frame;
  TryBlock block = frame->blockStack()->pop();
  frame->setValueStackTop(frame->valueStackBase() - block.level());
}

void Interpreter::doEndFinally(Context* ctx, word) {
  RawObject status = ctx->frame->popValue();
  if (!status->isNoneType()) {
    UNIMPLEMENTED("exception handling in context manager");
  }
}

// opcode 90
void Interpreter::doStoreName(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  DCHECK(frame->implicitGlobals()->isDict(), "expected dict");
  HandleScope scope;
  Dict implicit_globals(&scope, frame->implicitGlobals());
  RawObject names = RawCode::cast(frame->code())->names();
  Object key(&scope, RawTuple::cast(names)->at(arg));
  Object value(&scope, frame->popValue());
  thread->runtime()->dictAtPutInValueCell(implicit_globals, key, value);
}

// opcode 91
void Interpreter::doDeleteName(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  DCHECK(frame->implicitGlobals()->isDict(), "expected dict");
  HandleScope scope;
  Dict implicit_globals(&scope, frame->implicitGlobals());
  RawObject names = RawCode::cast(frame->code())->names();
  Object key(&scope, RawTuple::cast(names)->at(arg));
  if (thread->runtime()->dictRemove(implicit_globals, key)->isError()) {
    UNIMPLEMENTED("item not found in delete name");
  }
}

// opcode 92
void Interpreter::doUnpackSequence(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  Frame* frame = ctx->frame;
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object iterable(&scope, frame->popValue());
  Object iter_method(
      &scope, lookupMethod(thread, frame, iterable, SymbolId::kDunderIter));
  if (iter_method->isError()) {
    thread->raiseTypeErrorWithCStr("object is not iterable");
    thread->abortOnPendingException();
  }
  Object iterator(&scope, callMethod1(thread, frame, iter_method, iterable));
  thread->abortOnPendingException();
  Object next_method(
      &scope, lookupMethod(thread, frame, iterator, SymbolId::kDunderNext));
  if (next_method->isError()) {
    thread->raiseTypeErrorWithCStr("iter() returned non-iterator");
    thread->abortOnPendingException();
  }
  word num_pushed = 0;
  for (; num_pushed < arg && !runtime->isIteratorExhausted(thread, iterator);
       ++num_pushed) {
    Object value(&scope, callMethod1(thread, frame, next_method, iterator));
    if (value->isError()) {
      thread->abortOnPendingException();
      return;
    }
    frame->pushValue(*value);
  }

  if (num_pushed < arg) {
    frame->dropValues(num_pushed);
    thread->raiseValueErrorWithCStr("not enough values to unpack");
    thread->abortOnPendingException();
    return;
  }
  if (!runtime->isIteratorExhausted(thread, iterator)) {
    frame->dropValues(num_pushed);
    thread->raiseValueErrorWithCStr("too many values to unpack");
    thread->abortOnPendingException();
    return;
  }

  // swap values on the stack
  Object tmp(&scope, NoneType::object());
  for (word i = 0, j = num_pushed - 1, half = num_pushed / 2; i < half;
       ++i, --j) {
    tmp = frame->peek(i);
    frame->setValueAt(frame->peek(j), i);
    frame->setValueAt(*tmp, j);
  }
}

// opcode 93
void Interpreter::doForIter(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Object iterator(&scope, ctx->frame->topValue());
  Object next_method(&scope, lookupMethod(thread, ctx->frame, iterator,
                                          SymbolId::kDunderNext));
  if (next_method->isError()) {
    thread->raiseTypeErrorWithCStr("iter() returned non-iterator");
    thread->abortOnPendingException();
  }
  Object value(&scope, callMethod1(thread, ctx->frame, next_method, iterator));
  if (value->isError() && !thread->hasPendingStopIteration()) {
    thread->abortOnPendingException();
  }
  if (thread->hasPendingStopIteration()) {
    thread->clearPendingException();
    ctx->frame->popValue();
    ctx->pc += arg;
    return;
  }
  ctx->frame->pushValue(*value);
}

// opcode 94
void Interpreter::doUnpackEx(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  Frame* frame = ctx->frame;
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object iterable(&scope, frame->popValue());
  Object iter_method(
      &scope, lookupMethod(thread, frame, iterable, SymbolId::kDunderIter));
  if (iter_method->isError()) {
    thread->raiseTypeErrorWithCStr("object is not iterable");
    thread->abortOnPendingException();
  }
  Object iterator(&scope, callMethod1(thread, frame, iter_method, iterable));
  thread->abortOnPendingException();
  Object next_method(
      &scope, lookupMethod(thread, frame, iterator, SymbolId::kDunderNext));
  if (next_method->isError()) {
    thread->raiseTypeErrorWithCStr("iter() returned non-iterator");
    thread->abortOnPendingException();
  }

  word before = arg & kMaxByte;
  word after = (arg >> kBitsPerByte) & kMaxByte;
  word num_pushed = 0;

  for (; num_pushed < before &&
         !thread->runtime()->isIteratorExhausted(thread, iterator);
       ++num_pushed) {
    Object value(&scope, callMethod1(thread, frame, next_method, iterator));
    if (value->isError()) {
      frame->dropValues(num_pushed);
      thread->abortOnPendingException();
      return;
    }
    frame->pushValue(*value);
  }

  if (num_pushed < before) {
    frame->dropValues(num_pushed);
    thread->raiseValueErrorWithCStr("not enough values to unpack");
    thread->abortOnPendingException();
  }

  List list(&scope, runtime->newList());
  Object value(&scope, NoneType::object());
  while (!runtime->isIteratorExhausted(thread, iterator)) {
    value = Interpreter::callMethod1(thread, frame, next_method, iterator);
    if (value->isError()) {
      frame->dropValues(num_pushed);
      thread->abortOnPendingException();
      return;
    }
    runtime->listAdd(list, value);
  }

  frame->pushValue(*list);
  num_pushed++;

  if (list->numItems() < after) {
    frame->dropValues(num_pushed);
    thread->raiseValueErrorWithCStr("not enough values to unpack");
    thread->abortOnPendingException();
  }

  if (after > 0) {
    // Pop elements off the list and set them on the stack
    for (word i = list->numItems() - after, j = list->numItems(); i < j;
         ++i, ++num_pushed) {
      frame->pushValue(list->at(i));
      list->atPut(i, NoneType::object());
    }
    list->setNumItems(list->numItems() - after);
  }

  // swap values on the stack
  Object tmp(&scope, NoneType::object());
  for (word i = 0, j = num_pushed - 1, half = num_pushed / 2; i < half;
       ++i, --j) {
    tmp = frame->peek(i);
    frame->setValueAt(frame->peek(j), i);
    frame->setValueAt(*tmp, j);
  }
}

// opcode 95
void Interpreter::doStoreAttr(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Object receiver(&scope, ctx->frame->popValue());
  auto names = RawCode::cast(ctx->frame->code())->names();
  Object name(&scope, RawTuple::cast(names)->at(arg));
  Object value(&scope, ctx->frame->popValue());
  thread->runtime()->attributeAtPut(thread, receiver, name, value);
  // TODO(T31788973): propagate an exception
  thread->abortOnPendingException();
}

// opcode 96
void Interpreter::doDeleteAttr(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Object receiver(&scope, ctx->frame->popValue());
  auto names = RawCode::cast(ctx->frame->code())->names();
  Object name(&scope, RawTuple::cast(names)->at(arg));
  thread->runtime()->attributeDel(ctx->thread, receiver, name);
  // TODO(T31788973): propagate an exception
  thread->abortOnPendingException();
}

// opcode 97
void Interpreter::doStoreGlobal(Context* ctx, word arg) {
  RawValueCell::cast(RawTuple::cast(ctx->frame->fastGlobals())->at(arg))
      ->setValue(ctx->frame->popValue());
}

// opcode 98
void Interpreter::doDeleteGlobal(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  HandleScope scope;
  ValueCell value_cell(
      &scope,
      RawValueCell::cast(RawTuple::cast(frame->fastGlobals())->at(arg)));
  CHECK(!value_cell->value()->isValueCell(), "Unbound Globals");
  Object key(&scope,
             RawTuple::cast(RawCode::cast(frame->code())->names())->at(arg));
  Dict builtins(&scope, frame->builtins());
  Runtime* runtime = thread->runtime();
  Object value_in_builtin(&scope, runtime->dictAt(builtins, key));
  if (value_in_builtin->isError()) {
    value_in_builtin =
        runtime->dictAtPutInValueCell(builtins, key, value_in_builtin);
    RawValueCell::cast(*value_in_builtin)->makeUnbound();
  }
  value_cell->setValue(*value_in_builtin);
}

// opcode 100
void Interpreter::doLoadConst(Context* ctx, word arg) {
  RawObject consts = RawCode::cast(ctx->frame->code())->consts();
  ctx->frame->pushValue(RawTuple::cast(consts)->at(arg));
}

// opcode 101
void Interpreter::doLoadName(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Runtime* runtime = ctx->thread->runtime();
  HandleScope scope(ctx->thread);

  Object names(&scope, RawCode::cast(frame->code())->names());
  Object key(&scope, RawTuple::cast(*names)->at(arg));

  // 1. implicitGlobals
  Dict implicit_globals(&scope, frame->implicitGlobals());
  RawObject value = runtime->dictAt(implicit_globals, key);
  if (value->isValueCell()) {
    // 3a. found in [implicit]/globals but with up to 2-layers of indirection
    DCHECK(!RawValueCell::cast(value)->isUnbound(), "unbound globals");
    value = RawValueCell::cast(value)->value();
    if (value->isValueCell()) {
      DCHECK(!RawValueCell::cast(value)->isUnbound(), "unbound builtins");
      value = RawValueCell::cast(value)->value();
    }
    frame->pushValue(value);
    return;
  }

  // In the module body, globals == implicit_globals, so no need to check twice.
  // However in class body, it is a different dict.
  if (frame->implicitGlobals() != frame->globals()) {
    // 2. globals
    Dict globals(&scope, frame->globals());
    value = runtime->dictAt(globals, key);
  }
  if (value->isValueCell()) {
    // 3a. found in [implicit]/globals but with up to 2-layers of indirection
    DCHECK(!RawValueCell::cast(value)->isUnbound(), "unbound globals");
    value = RawValueCell::cast(value)->value();
    if (value->isValueCell()) {
      DCHECK(!RawValueCell::cast(value)->isUnbound(), "unbound builtins");
      value = RawValueCell::cast(value)->value();
    }
    frame->pushValue(value);
    return;
  }

  // 3b. not found; check builtins -- one layer of indirection
  Dict builtins(&scope, frame->builtins());
  value = runtime->dictAt(builtins, key);
  if (value->isValueCell()) {
    DCHECK(!RawValueCell::cast(value)->isUnbound(), "unbound builtins");
    value = RawValueCell::cast(value)->value();
  }

  if (value->isError()) {
    UNIMPLEMENTED("Unbound variable '%s'", RawStr::cast(*key)->toCStr());
  }
  frame->pushValue(value);
}

// opcode 102
void Interpreter::doBuildTuple(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Tuple tuple(&scope, thread->runtime()->newTuple(arg));
  for (word i = arg - 1; i >= 0; i--) {
    tuple->atPut(i, ctx->frame->popValue());
  }
  ctx->frame->pushValue(*tuple);
}

// opcode 103
void Interpreter::doBuildList(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Tuple array(&scope, thread->runtime()->newTuple(arg));
  for (word i = arg - 1; i >= 0; i--) {
    array->atPut(i, ctx->frame->popValue());
  }
  RawList list = RawList::cast(thread->runtime()->newList());
  list->setItems(*array);
  list->setNumItems(array->length());
  ctx->frame->pushValue(list);
}

// opcode 104
void Interpreter::doBuildSet(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Runtime* runtime = thread->runtime();
  Set set(&scope, RawSet::cast(runtime->newSet()));
  for (word i = arg - 1; i >= 0; i--) {
    runtime->setAdd(set, Object(&scope, ctx->frame->popValue()));
  }
  ctx->frame->pushValue(*set);
}

// opcode 105
void Interpreter::doBuildMap(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  Runtime* runtime = thread->runtime();
  HandleScope scope;
  Dict dict(&scope, runtime->newDictWithSize(arg));
  for (word i = 0; i < arg; i++) {
    Object value(&scope, ctx->frame->popValue());
    Object key(&scope, ctx->frame->popValue());
    runtime->dictAtPut(dict, key, value);
  }
  ctx->frame->pushValue(*dict);
}

// opcode 106
void Interpreter::doLoadAttr(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Object receiver(&scope, ctx->frame->topValue());
  auto names = RawCode::cast(ctx->frame->code())->names();
  Object name(&scope, RawTuple::cast(names)->at(arg));
  RawObject result = thread->runtime()->attributeAt(thread, receiver, name);
  // TODO(T31788973): propagate an exception
  thread->abortOnPendingException();
  ctx->frame->setTopValue(result);
}

// opcode 107
void Interpreter::doCompareOp(Context* ctx, word arg) {
  HandleScope scope;
  Object right(&scope, ctx->frame->popValue());
  Object left(&scope, ctx->frame->popValue());
  CompareOp op = static_cast<CompareOp>(arg);
  RawObject result;
  if (op == IS) {
    result = Bool::fromBool(*left == *right);
  } else if (op == IS_NOT) {
    result = Bool::fromBool(*left != *right);
  } else if (op == IN) {
    result = sequenceContains(ctx->thread, ctx->frame, left, right);
  } else if (op == NOT_IN) {
    result =
        Bool::negate(sequenceContains(ctx->thread, ctx->frame, left, right));
  } else {
    result = compareOperation(ctx->thread, ctx->frame, op, left, right);
  }
  ctx->frame->pushValue(result);
}

// opcode 108
void Interpreter::doImportName(Context* ctx, word arg) {
  HandleScope scope;
  Code code(&scope, ctx->frame->code());
  Object name(&scope, RawTuple::cast(code->names())->at(arg));
  Object fromlist(&scope, ctx->frame->popValue());
  Object level(&scope, ctx->frame->topValue());
  Thread* thread = ctx->thread;
  Runtime* runtime = thread->runtime();
  RawObject result = runtime->importModule(name);
  // TODO(T31788973): propagate an exception
  thread->abortOnPendingException();
  ctx->frame->setTopValue(result);
}

// opcode 109
void Interpreter::doImportFrom(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Code code(&scope, ctx->frame->code());
  Object name(&scope, RawTuple::cast(code->names())->at(arg));
  CHECK(name->isStr(), "name not found");
  Module module(&scope, ctx->frame->topValue());
  Runtime* runtime = thread->runtime();
  CHECK(module->isModule(), "Unexpected type to import from");
  RawObject value = runtime->moduleAt(module, name);
  CHECK(!value->isError(), "cannot import name");
  ctx->frame->pushValue(value);
}

// opcode 110
void Interpreter::doJumpForward(Context* ctx, word arg) { ctx->pc += arg; }

// opcode 111
void Interpreter::doJumpIfFalseOrPop(Context* ctx, word arg) {
  RawObject result = isTrue(ctx->thread, ctx->frame);
  if (result == Bool::falseObj()) {
    ctx->pc = arg;
  } else {
    ctx->frame->popValue();
  }
}

// opcode 112
void Interpreter::doJumpIfTrueOrPop(Context* ctx, word arg) {
  RawObject result = isTrue(ctx->thread, ctx->frame);
  if (result == Bool::trueObj()) {
    ctx->pc = arg;
  } else {
    ctx->frame->popValue();
  }
}

// opcode 113
void Interpreter::doJumpAbsolute(Context* ctx, word arg) { ctx->pc = arg; }

// opcode 114
void Interpreter::doPopJumpIfFalse(Context* ctx, word arg) {
  RawObject result = isTrue(ctx->thread, ctx->frame);
  ctx->frame->popValue();
  if (result == Bool::falseObj()) {
    ctx->pc = arg;
  }
}

// opcode 115
void Interpreter::doPopJumpIfTrue(Context* ctx, word arg) {
  RawObject result = isTrue(ctx->thread, ctx->frame);
  ctx->frame->popValue();
  if (result == Bool::trueObj()) {
    ctx->pc = arg;
  }
}

// opcode 116
void Interpreter::doLoadGlobal(Context* ctx, word arg) {
  RawObject value =
      RawValueCell::cast(RawTuple::cast(ctx->frame->fastGlobals())->at(arg))
          ->value();
  if (value->isValueCell()) {
    CHECK(
        !RawValueCell::cast(value)->isUnbound(), "Unbound global '%s'",
        RawStr::cast(
            RawTuple::cast(RawCode::cast(ctx->frame->code())->names())->at(arg))
            ->toCStr());
    value = RawValueCell::cast(value)->value();
  }
  ctx->frame->pushValue(value);
  DCHECK(ctx->frame->topValue() != Error::object(), "unexpected error object");
}

// opcode 119
void Interpreter::doContinueLoop(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  TryBlock block = frame->blockStack()->peek();
  word kind = block.kind();
  while (kind != Bytecode::SETUP_LOOP) {
    frame->blockStack()->pop();
    kind = frame->blockStack()->peek().kind();
    if (kind != Bytecode::SETUP_LOOP && kind != Bytecode::SETUP_EXCEPT) {
      UNIMPLEMENTED("Can only unwind loop and exception blocks");
    }
  }
  ctx->pc = arg;
}

// opcode 120
void Interpreter::doSetupLoop(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  word stack_depth = frame->valueStackBase() - frame->valueStackTop();
  BlockStack* block_stack = frame->blockStack();
  block_stack->push(TryBlock(Bytecode::SETUP_LOOP, ctx->pc + arg, stack_depth));
}

// opcode 121
void Interpreter::doSetupExcept(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  word stack_depth = frame->valueStackBase() - frame->valueStackTop();
  BlockStack* block_stack = frame->blockStack();
  block_stack->push(
      TryBlock(Bytecode::SETUP_EXCEPT, ctx->pc + arg, stack_depth));
}

// opcode 122
void Interpreter::doSetupFinally(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  word stack_depth = frame->valueStackBase() - frame->valueStackTop();
  BlockStack* block_stack = frame->blockStack();
  block_stack->push(
      TryBlock(Bytecode::SETUP_FINALLY, ctx->pc + arg, stack_depth));
}

// opcode 124
void Interpreter::doLoadFast(Context* ctx, word arg) {
  // TODO(cshapiro): Need to handle unbound local error
  RawObject value = ctx->frame->getLocal(arg);
  if (value->isError()) {
    RawObject name =
        RawTuple::cast(RawCode::cast(ctx->frame->code())->varnames())->at(arg);
    UNIMPLEMENTED("unbound local %s", RawStr::cast(name)->toCStr());
  }
  ctx->frame->pushValue(ctx->frame->getLocal(arg));
}

// opcode 125
void Interpreter::doStoreFast(Context* ctx, word arg) {
  RawObject value = ctx->frame->popValue();
  ctx->frame->setLocal(arg, value);
}

// opcode 126
void Interpreter::doDeleteFast(Context* ctx, word arg) {
  // TODO(T32821785): use another immediate value than Error to signal unbound
  // local
  if (ctx->frame->getLocal(arg) == Error::object()) {
    RawObject name =
        RawTuple::cast(RawCode::cast(ctx->frame->code())->varnames())->at(arg);
    UNIMPLEMENTED("unbound local %s", RawStr::cast(name)->toCStr());
  }
  ctx->frame->setLocal(arg, Error::object());
}

// opcode 127
void Interpreter::doStoreAnnotation(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  Runtime* runtime = ctx->thread->runtime();
  Object names(&scope, RawCode::cast(ctx->frame->code())->names());
  Object value(&scope, ctx->frame->popValue());
  Object key(&scope, RawTuple::cast(*names)->at(arg));

  Dict implicit_globals(&scope, ctx->frame->implicitGlobals());
  Object annotations(&scope,
                     runtime->symbols()->at(SymbolId::kDunderAnnotations));
  Object value_cell(&scope, runtime->dictAt(implicit_globals, annotations));
  Dict anno_dict(&scope, RawValueCell::cast(*value_cell)->value());
  runtime->dictAtPut(anno_dict, key, value);
}

// opcode 131
void Interpreter::doCallFunction(Context* ctx, word arg) {
  RawObject result = call(ctx->thread, ctx->frame, arg);
  // TODO(T31788973): propagate an exception
  ctx->thread->abortOnPendingException();
  ctx->frame->pushValue(result);
}

// opcode 132
void Interpreter::doMakeFunction(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  HandleScope scope;
  Runtime* runtime = thread->runtime();
  Function function(&scope, runtime->newFunction());
  function->setQualname(frame->popValue());
  function->setCode(frame->popValue());
  function->setName(RawCode::cast(function->code())->name());
  Dict globals(&scope, frame->globals());
  function->setGlobals(globals);
  Object name_key(&scope, runtime->symbols()->at(SymbolId::kDunderName));
  Object value_cell(&scope, runtime->dictAt(globals, name_key));
  if (value_cell->isValueCell()) {
    DCHECK(!RawValueCell::cast(value_cell)->isUnbound(), "unbound globals");
    function->setModule(RawValueCell::cast(value_cell)->value());
  }
  Dict builtins(&scope, frame->builtins());
  Code code(&scope, function->code());
  Object consts_obj(&scope, code->consts());
  if (consts_obj->isTuple() && RawTuple::cast(consts_obj)->length() >= 1) {
    Tuple consts(&scope, *consts_obj);
    if (consts->at(0)->isStr()) {
      function->setDoc(consts->at(0));
    }
  }
  function->setFastGlobals(
      runtime->computeFastGlobals(code, globals, builtins));
  if (code->hasCoroutineOrGenerator()) {
    if (code->hasFreevarsOrCellvars()) {
      function->setEntry(generatorClosureTrampoline);
      function->setEntryKw(generatorClosureTrampolineKw);
      function->setEntryEx(generatorClosureTrampolineEx);
    } else {
      function->setEntry(generatorTrampoline);
      function->setEntryKw(generatorTrampolineKw);
      function->setEntryEx(generatorTrampolineEx);
    }
  } else {
    if (code->hasFreevarsOrCellvars()) {
      function->setEntry(interpreterClosureTrampoline);
      function->setEntryKw(interpreterClosureTrampolineKw);
      function->setEntryEx(interpreterClosureTrampolineEx);
    } else {
      function->setEntry(interpreterTrampoline);
      function->setEntryKw(interpreterTrampolineKw);
      function->setEntryEx(interpreterTrampolineEx);
    }
  }
  if (arg & MakeFunctionFlag::CLOSURE) {
    DCHECK((frame->topValue())->isTuple(), "Closure expects tuple");
    function->setClosure(frame->popValue());
  }
  if (arg & MakeFunctionFlag::ANNOTATION_DICT) {
    DCHECK((frame->topValue())->isDict(), "Parameter annotations expect dict");
    function->setAnnotations(frame->popValue());
  }
  if (arg & MakeFunctionFlag::DEFAULT_KW) {
    DCHECK((frame->topValue())->isDict(), "Keyword arguments expect dict");
    function->setKwDefaults(frame->popValue());
  }
  if (arg & MakeFunctionFlag::DEFAULT) {
    DCHECK((frame->topValue())->isTuple(), "Default arguments expect tuple");
    function->setDefaults(frame->popValue());
  }
  frame->pushValue(*function);
}

// opcode 133
void Interpreter::doBuildSlice(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Object step(&scope, (arg == 3) ? ctx->frame->popValue() : NoneType::object());
  Object stop(&scope, ctx->frame->popValue());
  Object start(&scope, ctx->frame->topValue());  // TOP
  Slice slice(&scope, thread->runtime()->newSlice(start, stop, step));
  ctx->frame->setTopValue(*slice);
}

// opcode 135
void Interpreter::doLoadClosure(Context* ctx, word arg) {
  RawCode code = RawCode::cast(ctx->frame->code());
  ctx->frame->pushValue(ctx->frame->getLocal(code->nlocals() + arg));
}

// opcode 136
void Interpreter::doLoadDeref(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  RawCode code = RawCode::cast(ctx->frame->code());
  ValueCell value(&scope, ctx->frame->getLocal(code->nlocals() + arg));
  if (value->isUnbound()) {
    UNIMPLEMENTED(
        "UnboundLocalError: local variable referenced before assignment");
  }
  ctx->frame->pushValue(value->value());
}

// opcode 137
void Interpreter::doStoreDeref(Context* ctx, word arg) {
  RawCode code = RawCode::cast(ctx->frame->code());
  RawValueCell::cast(ctx->frame->getLocal(code->nlocals() + arg))
      ->setValue(ctx->frame->popValue());
}

// opcode 138
void Interpreter::doDeleteDeref(Context* ctx, word arg) {
  RawCode code = RawCode::cast(ctx->frame->code());
  RawValueCell::cast(ctx->frame->getLocal(code->nlocals() + arg))
      ->makeUnbound();
}

// opcode 141
void Interpreter::doCallFunctionKw(Context* ctx, word arg) {
  RawObject result = callKw(ctx->thread, ctx->frame, arg);
  // TODO(T31788973): propagate an exception
  ctx->thread->abortOnPendingException();
  ctx->frame->pushValue(result);
}

// opcode 142
void Interpreter::doCallFunctionEx(Context* ctx, word arg) {
  RawObject result = callEx(ctx->thread, ctx->frame, arg);
  // TODO(T31788973): propagate an exception
  ctx->thread->abortOnPendingException();
  ctx->frame->pushValue(result);
}

// opcode 143
void Interpreter::doSetupWith(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  Thread* thread = ctx->thread;
  Runtime* runtime = thread->runtime();
  Frame* frame = ctx->frame;
  Object mgr(&scope, frame->topValue());
  Object exit_selector(&scope, runtime->symbols()->DunderExit());
  Object enter(&scope,
               lookupMethod(thread, frame, mgr, SymbolId::kDunderEnter));
  BoundMethod exit(&scope, runtime->attributeAt(thread, mgr, exit_selector));
  frame->setTopValue(*exit);
  Object result(&scope, callMethod1(thread, frame, enter, mgr));

  word stack_depth = frame->valueStackBase() - frame->valueStackTop();
  BlockStack* block_stack = frame->blockStack();
  block_stack->push(
      TryBlock(Bytecode::SETUP_FINALLY, ctx->pc + arg, stack_depth));
  frame->pushValue(*result);
}

// opcode 145
void Interpreter::doListAppend(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  Object value(&scope, ctx->frame->popValue());
  List list(&scope, ctx->frame->peek(arg - 1));
  ctx->thread->runtime()->listAdd(list, value);
}

// opcode 146
void Interpreter::doSetAdd(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  Object value(&scope, ctx->frame->popValue());
  Set set(&scope, RawSet::cast(ctx->frame->peek(arg - 1)));
  ctx->thread->runtime()->setAdd(set, value);
}

// opcode 147
void Interpreter::doMapAdd(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  Object key(&scope, ctx->frame->popValue());
  Object value(&scope, ctx->frame->popValue());
  Dict dict(&scope, RawDict::cast(ctx->frame->peek(arg - 1)));
  ctx->thread->runtime()->dictAtPut(dict, key, value);
}

// opcode 148
void Interpreter::doLoadClassDeref(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  Code code(&scope, ctx->frame->code());
  word idx = arg - code->numCellvars();
  Object name(&scope, RawTuple::cast(code->freevars())->at(idx));
  Dict implicit_global(&scope, ctx->frame->implicitGlobals());
  Object result(&scope, ctx->thread->runtime()->dictAt(implicit_global, name));
  if (result->isError()) {
    ValueCell value_cell(&scope, ctx->frame->getLocal(code->nlocals() + arg));
    if (value_cell->isUnbound()) {
      UNIMPLEMENTED("unbound free var %s", RawStr::cast(*name)->toCStr());
    }
    ctx->frame->pushValue(value_cell->value());
  } else {
    ctx->frame->pushValue(RawValueCell::cast(*result)->value());
  }
}

// opcode 149
void Interpreter::doBuildListUnpack(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  List list(&scope, runtime->newList());
  Object obj(&scope, NoneType::object());
  for (word i = arg - 1; i >= 0; i--) {
    obj = frame->peek(i);
    if (runtime->listExtend(thread, list, obj)->isError()) {
      frame->dropValues(arg);
      thread->abortOnPendingException();
    }
  }
  frame->dropValues(arg - 1);
  frame->setTopValue(*list);
}

// opcode 150
void Interpreter::doBuildMapUnpack(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Dict dict(&scope, runtime->newDict());
  Object obj(&scope, NoneType::object());
  for (word i = arg - 1; i >= 0; i--) {
    obj = frame->peek(i);
    if (runtime->dictUpdate(thread, dict, obj)->isError()) {
      frame->dropValues(arg);
      thread->abortOnPendingException();
    }
  }
  frame->dropValues(arg - 1);
  frame->setTopValue(*dict);
}

// opcode 151
void Interpreter::doBuildMapUnpackWithCall(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Dict dict(&scope, runtime->newDict());
  Object obj(&scope, NoneType::object());
  for (word i = arg - 1; i >= 0; i--) {
    obj = frame->peek(i);
    if (runtime->dictMerge(thread, dict, obj)->isError()) {
      frame->dropValues(arg);
      thread->abortOnPendingException();
    }
  }
  frame->dropValues(arg - 1);
  frame->setTopValue(*dict);
}

// opcode 152 & opcode 158
void Interpreter::doBuildTupleUnpack(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  List list(&scope, runtime->newList());
  Object obj(&scope, NoneType::object());
  for (word i = arg - 1; i >= 0; i--) {
    obj = frame->peek(i);
    if (runtime->listExtend(thread, list, obj)->isError()) {
      frame->dropValues(arg);
      thread->abortOnPendingException();
    }
  }
  Tuple tuple(&scope, runtime->newTuple(list->numItems()));
  for (word i = 0; i < list->numItems(); i++) {
    tuple->atPut(i, list->at(i));
  }
  frame->dropValues(arg - 1);
  frame->setTopValue(*tuple);
}

// opcode 153
void Interpreter::doBuildSetUnpack(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Set set(&scope, runtime->newSet());
  Object obj(&scope, NoneType::object());
  for (word i = 0; i < arg; i++) {
    obj = frame->peek(i);
    if (runtime->setUpdate(thread, set, obj)->isError()) {
      frame->dropValues(arg);
      thread->abortOnPendingException();
    }
  }
  frame->dropValues(arg - 1);
  frame->setTopValue(*set);
}

// opcode 154
void Interpreter::doSetupAsyncWith(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  HandleScope scope(ctx->thread);
  Object result(&scope, frame->popValue());
  word stack_depth = frame->valueStackSize();
  BlockStack* block_stack = frame->blockStack();
  block_stack->push(
      TryBlock(Bytecode::SETUP_FINALLY, ctx->pc + arg, stack_depth));
  frame->pushValue(*result);
}

// opcode 155
// A incomplete impl of FORMAT_VALUE; assumes no conv
void Interpreter::doFormatValue(Context* ctx, word flags) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  int conv = (flags & FVC_MASK_FLAG);
  int have_fmt_spec = (flags & FVS_MASK_FLAG) == FVS_HAVE_SPEC_FLAG;
  switch (conv) {
    case FVC_STR_FLAG:
    case FVC_REPR_FLAG:
    case FVC_ASCII_FLAG:
      UNIMPLEMENTED("Conversion not supported.");
    default:  // 0: no conv
      break;
  }

  if (have_fmt_spec) {
    Str fmt_str(&scope, ctx->frame->popValue());
    Str value(&scope, ctx->frame->popValue());
    ctx->frame->pushValue(thread->runtime()->strConcat(fmt_str, value));
  }  // else no-op
}

// opcode 156
void Interpreter::doBuildConstKeyMap(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Tuple keys(&scope, ctx->frame->popValue());
  Dict dict(&scope, thread->runtime()->newDictWithSize(keys->length()));
  for (word i = arg - 1; i >= 0; i--) {
    Object key(&scope, keys->at(i));
    Object value(&scope, ctx->frame->popValue());
    thread->runtime()->dictAtPut(dict, key, value);
  }
  ctx->frame->pushValue(*dict);
}

// opcode 157
void Interpreter::doBuildString(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  switch (arg) {
    case 0:  // empty
      ctx->frame->pushValue(runtime->newStrWithAll(View<byte>(nullptr, 0)));
      break;
    case 1:  // no-op
      break;
    default: {  // concat
      RawObject res = stringJoin(thread, ctx->frame->valueStackTop(), arg);
      ctx->frame->dropValues(arg - 1);
      ctx->frame->setTopValue(res);
      break;
    }
  }
}

using Op = void (*)(Interpreter::Context*, word);
const Op kOpTable[] = {
#define HANDLER(name, value, handler) Interpreter::handler,
    FOREACH_BYTECODE(HANDLER)
#undef HANDLER
};

RawObject Interpreter::execute(Thread* thread, Frame* frame) {
  HandleScope scope(thread);
  Code code(&scope, RawCode::cast(frame->code()));
  Bytes byte_array(&scope, code->code());
  Context ctx;
  ctx.pc = frame->virtualPC();
  ctx.thread = thread;
  ctx.frame = frame;
  for (;;) {
    frame->setVirtualPC(ctx.pc);
    Bytecode bc = static_cast<Bytecode>(byte_array->byteAt(ctx.pc++));
    int32 arg = byte_array->byteAt(ctx.pc++);
  dispatch:
    switch (bc) {
      case Bytecode::EXTENDED_ARG: {
        bc = static_cast<Bytecode>(byte_array->byteAt(ctx.pc++));
        arg = (arg << 8) | byte_array->byteAt(ctx.pc++);
        goto dispatch;
      }
      case Bytecode::RETURN_VALUE: {
        RawObject result = ctx.frame->popValue();
        // Clean up after ourselves
        thread->popFrame();
        if (code->hasGenerator()) {
          return thread->raiseStopIteration(result);
        }
        return result;
      }
      case Bytecode::YIELD_FROM: {
        RawObject result = yieldFrom(thread, frame);
        if (result->isError()) {
          continue;
        }
        return result;
      }
      case Bytecode::YIELD_VALUE: {
        Object result(&scope, frame->popValue());
        frame->setVirtualPC(ctx.pc);
        GeneratorBase gen(&scope, thread->runtime()->genFromStackFrame(frame));
        thread->runtime()->genSave(thread, gen);
        return *result;
      }
      default:
        kOpTable[bc](&ctx, arg);
    }
  }
}

}  // namespace python
