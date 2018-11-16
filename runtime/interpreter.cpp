#include "interpreter.h"

#include <cstdio>
#include <cstdlib>

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines.h"

namespace python {

Object* Interpreter::call(Thread* thread, Frame* frame, word nargs) {
  Object* callable = frame->peek(nargs);
  Object** sp = frame->valueStackTop() + nargs + 1;
  Object* result = None::object();
  switch (callable->layoutId()) {
    case LayoutId::kFunction: {
      result = Function::cast(callable)->entry()(thread, frame, nargs);
      break;
    }
    case LayoutId::kBoundMethod: {
      result = callBoundMethod(thread, frame, nargs);
      break;
    }
    default: { result = callCallable(thread, frame, nargs); }
  }
  // Clear the stack of the function object.
  frame->setValueStackTop(sp);
  return result;
}

Object* Interpreter::callCallable(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread->handles());
  Handle<Object> receiver(&scope, frame->peek(nargs));
  Runtime* runtime = thread->runtime();

  Handle<Object> name(&scope, runtime->symbols()->DunderCall());
  Handle<Class> type(&scope, runtime->classOf(*receiver));
  Handle<Object> dunder_call(
      &scope, runtime->lookupNameInMro(thread, type, name));
  CHECK(!dunder_call->isError(), "object has no __call__ attribute");
  CHECK(dunder_call->isFunction(), "__call__ attribute is not a function");

  Handle<Function> function(&scope, *dunder_call);

  // rewrite the stack into the method call form.  the function implementing
  // the method goes at the bottom of the stack, followed by the receiver,
  // followed by zero or more arguments.

  Object** sp = frame->valueStackTop();
  sp -= 1;
  for (word i = 0; i <= nargs; i++) {
    sp[i] = sp[i + 1];
  }
  sp[nargs + 1] = *function;
  frame->setValueStackTop(sp);

  // now call the function
  return function->entry()(thread, frame, nargs + 1);
}

Object* Interpreter::callKw(Thread* thread, Frame* frame, word nargs) {
  // Top of stack is a tuple of keyword argument names in the order they
  // appear on the stack.
  Function* function = Function::cast(frame->peek(nargs + 1));
  return function->entryKw()(thread, frame, nargs);
}

Object* Interpreter::callEx(Thread* thread, Frame* frame, word flags) {
  // Low bit of flags indicates whether var-keyword argument is on TOS.
  // In all cases, var-positional tuple is next, followed by the function
  // pointer.
  word function_position = (flags & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1;
  Function* function = Function::cast(frame->peek(function_position));
  return function->entryEx()(thread, frame, flags);
}

Object* Interpreter::callBoundMethod(Thread* thread, Frame* frame, word nargs) {
  // Shift all arguments on the stack down by 1 and unpack the BoundMethod.
  //
  // We don't need to worry too much about the performance overhead for method
  // calls here.
  //
  // Python 3.7 introduces two new opcodes, LOAD_METHOD and CALL_METHOD, that
  // eliminate the need to create a temporary BoundMethod object when performing
  // a method call.
  //
  // The other pattern of bound method usage occurs when someone passes around a
  // reference to a method e.g.:
  //
  //   m = foo.method
  //   m()
  //
  // Our contention is that uses of this pattern are not performance sensitive.
  Object** sp = frame->valueStackTop();
  sp -= 1;
  for (word i = 0; i < nargs; i++) {
    sp[i] = sp[i + 1];
  }
  sp[nargs] = BoundMethod::cast(sp[nargs + 1])->self();
  sp[nargs + 1] = BoundMethod::cast(sp[nargs + 1])->function();
  frame->setValueStackTop(sp);

  // Call the bound function
  Function* function = Function::cast(frame->peek(nargs + 1));
  return function->entry()(thread, frame, nargs + 1);
}

Object* Interpreter::stringJoin(Thread* thread, Object** sp, word num) {
  word new_len = 0;
  for (word i = num - 1; i >= 0; i--) {
    if (!sp[i]->isString()) {
      UNIMPLEMENTED("Conversion of non-string values not supported.");
    }
    new_len += String::cast(sp[i])->length();
  }

  if (new_len <= SmallString::kMaxLength) {
    byte buffer[SmallString::kMaxLength];
    byte* ptr = buffer;
    for (word i = num - 1; i >= 0; i--) {
      String* str = String::cast(sp[i]);
      word len = str->length();
      str->copyTo(ptr, len);
      ptr += len;
    }
    return SmallString::fromBytes(View<byte>(buffer, new_len));
  }

  HandleScope scope(thread->handles());
  Handle<LargeString> result(
      &scope, thread->runtime()->heap()->createLargeString(new_len));
  word offset = LargeString::kDataOffset;
  for (word i = num - 1; i >= 0; i--) {
    String* str = String::cast(sp[i]);
    word len = str->length();
    str->copyTo(reinterpret_cast<byte*>(result->address() + offset), len);
    offset += len;
  }
  return *result;
}

Object* Interpreter::callDescriptorGet(
    Thread* thread,
    Frame* caller,
    const Handle<Object>& descriptor,
    const Handle<Object>& receiver,
    const Handle<Object>& receiver_type) {
  HandleScope scope(thread->handles());
  Runtime* runtime = thread->runtime();
  Handle<Object> selector(&scope, runtime->symbols()->DunderGet());
  Handle<Class> descriptor_type(&scope, runtime->classOf(*descriptor));
  Handle<Object> method(
      &scope, runtime->lookupNameInMro(thread, descriptor_type, selector));
  return callMethod3(
      thread, caller, method, descriptor, receiver, receiver_type);
}

Object* Interpreter::lookupMethod(
    Thread* thread,
    Frame* caller,
    const Handle<Object>& receiver,
    const Handle<Object>& selector) {
  HandleScope scope(thread->handles());
  Runtime* runtime = thread->runtime();
  Handle<Class> type(&scope, runtime->classOf(*receiver));
  Handle<Object> method(
      &scope, runtime->lookupNameInMro(thread, type, selector));
  if (method->isFunction()) {
    // Do not create a short-lived bound method object.
    return *method;
  }
  if (!method->isError()) {
    if (runtime->isNonDataDescriptor(thread, method)) {
      Handle<Object> owner(&scope, *type);
      return callDescriptorGet(thread, caller, method, receiver, owner);
    }
  }
  return *method;
}

Object* Interpreter::callMethod1(
    Thread* thread,
    Frame* caller,
    const Handle<Object>& method,
    const Handle<Object>& self) {
  Object* result;
  if (method->isFunction()) {
    caller->pushValue(*method);
    caller->pushValue(*self);
    result = call(thread, caller, 1);
  } else {
    caller->pushValue(*method);
    result = call(thread, caller, 0);
  }
  return result;
}

Object* Interpreter::callMethod2(
    Thread* thread,
    Frame* caller,
    const Handle<Object>& method,
    const Handle<Object>& self,
    const Handle<Object>& other) {
  Object* result;
  if (method->isFunction()) {
    caller->pushValue(*method);
    caller->pushValue(*self);
    caller->pushValue(*other);
    result = call(thread, caller, 2);
  } else {
    caller->pushValue(*method);
    caller->pushValue(*other);
    result = call(thread, caller, 1);
  }
  return result;
}

Object* Interpreter::callMethod3(
    Thread* thread,
    Frame* caller,
    const Handle<Object>& method,
    const Handle<Object>& self,
    const Handle<Object>& arg1,
    const Handle<Object>& arg2) {
  Object* result;
  if (method->isFunction()) {
    caller->pushValue(*method);
    caller->pushValue(*self);
    caller->pushValue(*arg1);
    caller->pushValue(*arg2);
    result = call(thread, caller, 3);
  } else {
    caller->pushValue(*method);
    caller->pushValue(*arg1);
    caller->pushValue(*arg2);
    result = call(thread, caller, 2);
  }
  return result;
}

Object* Interpreter::callMethod4(
    Thread* thread,
    Frame* caller,
    const Handle<Object>& method,
    const Handle<Object>& self,
    const Handle<Object>& arg1,
    const Handle<Object>& arg2,
    const Handle<Object>& arg3) {
  Object* result;
  if (method->isFunction()) {
    caller->pushValue(*method);
    caller->pushValue(*self);
    caller->pushValue(*arg1);
    caller->pushValue(*arg2);
    caller->pushValue(*arg3);
    result = call(thread, caller, 4);
  } else {
    caller->pushValue(*method);
    caller->pushValue(*arg1);
    caller->pushValue(*arg2);
    caller->pushValue(*arg3);
    result = call(thread, caller, 3);
  }
  return result;
}

Object* Interpreter::unaryOperation(
    Thread* thread,
    Frame* caller,
    const Handle<Object>& self,
    const Handle<Object>& selector) {
  HandleScope scope(thread->handles());
  Handle<Object> method(&scope, lookupMethod(thread, caller, self, selector));
  CHECK(!method->isError(), "unknown unary operation");
  return callMethod1(thread, caller, method, self);
}

Object* Interpreter::binaryOperation(
    Thread* thread,
    Frame* caller,
    BinaryOp op,
    const Handle<Object>& self,
    const Handle<Object>& other) {
  HandleScope scope(thread->handles());
  Runtime* runtime = thread->runtime();

  Handle<Class> self_type(&scope, runtime->classOf(*self));
  Handle<Class> other_type(&scope, runtime->classOf(*other));
  bool is_derived_type = (*self_type != *other_type) &&
      (runtime->isSubClass(other_type, self_type) == Boolean::trueObj());

  Handle<Object> selector(&scope, runtime->binaryOperationSelector(op));
  Handle<Object> self_method(
      &scope, lookupMethod(thread, caller, self, selector));
  Handle<Object> other_method(
      &scope, lookupMethod(thread, caller, other, selector));

  Handle<Object> swapped_selector(
      &scope, runtime->swappedBinaryOperationSelector(op));
  Handle<Object> self_reflected_method(
      &scope, lookupMethod(thread, caller, self, swapped_selector));
  Handle<Object> other_reflected_method(
      &scope, lookupMethod(thread, caller, other, swapped_selector));

  bool try_other = true;
  if (!self_method->isError()) {
    if (is_derived_type && !other_reflected_method->isError() &&
        *self_reflected_method != *other_reflected_method) {
      Object* result =
          callMethod2(thread, caller, other_reflected_method, other, self);
      if (result != runtime->notImplemented()) {
        return result;
      }
      try_other = false;
    }
    Object* result = callMethod2(thread, caller, self_method, self, other);
    if (result != runtime->notImplemented()) {
      return result;
    }
  }
  if (try_other) {
    if (!other_reflected_method->isError()) {
      Object* result =
          callMethod2(thread, caller, other_reflected_method, other, self);
      if (result != runtime->notImplemented()) {
        return result;
      }
    }
  }
  UNIMPLEMENTED("throw");
}

Object* Interpreter::inplaceOperation(
    Thread* thread,
    Frame* caller,
    BinaryOp op,
    const Handle<Object>& self,
    const Handle<Object>& other) {
  HandleScope scope(thread->handles());
  Runtime* runtime = thread->runtime();
  Handle<Object> selector(&scope, runtime->inplaceOperationSelector(op));
  Handle<Object> method(&scope, lookupMethod(thread, caller, self, selector));
  if (!method->isError()) {
    Object* result = callMethod2(thread, caller, method, self, other);
    if (result != runtime->notImplemented()) {
      return result;
    }
  }
  return binaryOperation(thread, caller, op, self, other);
}

Object* Interpreter::compareOperation(
    Thread* thread,
    Frame* caller,
    CompareOp op,
    const Handle<Object>& left,
    const Handle<Object>& right) {
  HandleScope scope(thread->handles());
  Runtime* runtime = thread->runtime();

  Handle<Class> left_type(&scope, runtime->classOf(*left));
  Handle<Class> right_type(&scope, runtime->classOf(*right));

  bool try_swapped = true;
  bool has_different_type = (*left_type != *right_type);
  if (has_different_type && runtime->isSubClass(right_type, left_type)) {
    try_swapped = false;
    Handle<Object> selector(&scope, runtime->swappedComparisonSelector(op));
    Handle<Object> method(
        &scope, lookupMethod(thread, caller, right, selector));
    if (!method->isError()) {
      Object* result = callMethod2(thread, caller, method, right, left);
      if (result != runtime->notImplemented()) {
        return result;
      }
    }
  } else {
    Handle<Object> selector(&scope, runtime->comparisonSelector(op));
    Handle<Object> method(&scope, lookupMethod(thread, caller, left, selector));
    if (!method->isError()) {
      Object* result = callMethod2(thread, caller, method, left, right);
      if (result != runtime->notImplemented()) {
        return result;
      }
    }
  }
  if (has_different_type && try_swapped) {
    Handle<Object> selector(&scope, runtime->swappedComparisonSelector(op));
    Handle<Object> method(
        &scope, lookupMethod(thread, caller, right, selector));
    if (!method->isError()) {
      Object* result = callMethod2(thread, caller, method, right, left);
      if (result != runtime->notImplemented()) {
        return result;
      }
    }
  }
  if (op == CompareOp::EQ) {
    return Boolean::fromBool(*left == *right);
  } else if (op == CompareOp::NE) {
    return Boolean::fromBool(*left != *right);
  }
  UNIMPLEMENTED("throw");
}

Object* Interpreter::sequenceContains(
    Thread* thread,
    Frame* caller,
    const Handle<Object>& value,
    const Handle<Object>& container) {
  HandleScope scope(thread);
  Handle<Object> selector(
      &scope, thread->runtime()->symbols()->DunderContains());
  Handle<Object> method(
      &scope, lookupMethod(thread, caller, container, selector));
  if (!method->isError()) {
    Handle<Object> result(
        &scope, callMethod2(thread, caller, method, container, value));
    caller->pushValue(*result);
    Object* is_true = isTrue(thread, caller);
    caller->popValue();
    return is_true;
  } else {
    UNIMPLEMENTED("fallback to iter search.");
  }
}

Object* Interpreter::isTrue(Thread* thread, Frame* caller) {
  HandleScope scope(thread->handles());
  Handle<Object> self(&scope, caller->topValue());
  Handle<Object> selector(&scope, thread->runtime()->symbols()->DunderBool());
  Handle<Object> method(&scope, lookupMethod(thread, caller, self, selector));
  if (!method->isError()) {
    Handle<Object> result(&scope, callMethod1(thread, caller, method, self));
    if (result->isBoolean()) {
      return *result;
    }
    if (result->isInteger()) {
      Handle<Integer> integer(&scope, *result);
      return Boolean::fromBool(integer->asWord() > 0);
    }
    UNIMPLEMENTED("throw");
  }
  selector = thread->runtime()->symbols()->DunderLen();
  method = lookupMethod(thread, caller, self, selector);
  if (!method->isError()) {
    Handle<Object> result(&scope, callMethod1(thread, caller, method, self));
    if (result->isInteger()) {
      Handle<Integer> integer(&scope, *result);
      if (integer->isPositive()) {
        return Boolean::trueObj();
      }
      if (integer->isZero()) {
        return Boolean::falseObj();
      }
      UNIMPLEMENTED("throw");
    }
  }
  return Boolean::trueObj();
}

static Bytecode currentBytecode(const Interpreter::Context* ctx) {
  ByteArray* code = ByteArray::cast(Code::cast(ctx->frame->code())->code());
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
void Interpreter::doPopTop(Context* ctx, word) {
  ctx->frame->popValue();
}

// opcode 2
void Interpreter::doRotTwo(Context* ctx, word) {
  Object** sp = ctx->frame->valueStackTop();
  Object* top = *sp;
  *sp = *(sp + 1);
  *(sp + 1) = top;
}

// opcode 3
void Interpreter::doRotThree(Context* ctx, word) {
  Object** sp = ctx->frame->valueStackTop();
  Object* top = *sp;
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
  Object* first = ctx->frame->topValue();
  Object* second = ctx->frame->peek(1);
  ctx->frame->pushValue(second);
  ctx->frame->pushValue(first);
}

// opcode 9
void Interpreter::doNop(Context*, word) {}

// opcode 10
void Interpreter::doUnaryPositive(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread->handles());
  Handle<Object> receiver(&scope, ctx->frame->topValue());
  Handle<Object> selector(&scope, thread->runtime()->symbols()->DunderPos());
  Object* result = unaryOperation(thread, ctx->frame, receiver, selector);
  ctx->frame->setTopValue(result);
}

// opcode 11
void Interpreter::doUnaryNegative(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread->handles());
  Handle<Object> receiver(&scope, ctx->frame->topValue());
  Handle<Object> selector(&scope, thread->runtime()->symbols()->DunderNeg());
  Object* result = unaryOperation(thread, ctx->frame, receiver, selector);
  ctx->frame->setTopValue(result);
}

// opcode 12
void Interpreter::doUnaryNot(Context* ctx, word) {
  if (isTrue(ctx->thread, ctx->frame) == Boolean::trueObj()) {
    ctx->frame->setTopValue(Boolean::falseObj());
  } else {
    ctx->frame->setTopValue(Boolean::trueObj());
  }
}

// opcode 15
void Interpreter::doUnaryInvert(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread->handles());
  Handle<Object> receiver(&scope, ctx->frame->topValue());
  Handle<Object> selector(&scope, thread->runtime()->symbols()->DunderInvert());
  Object* result = unaryOperation(thread, ctx->frame, receiver, selector);
  ctx->frame->setTopValue(result);
}

// opcode 20
void Interpreter::doBinaryMultiply(Context* ctx, word) {
  if (ctx->frame->peek(1)->isSmallInteger()) {
    word right = SmallInteger::cast(ctx->frame->popValue())->value();
    word left = SmallInteger::cast(ctx->frame->topValue())->value();
    word result = left * right;
    if (!(left == 0 || (result / left) == right)) {
      UNIMPLEMENTED("small integer overflow");
    }
    ctx->frame->setTopValue(SmallInteger::fromWord(result));
  } else {
    word ntimes = SmallInteger::cast(ctx->frame->popValue())->value();
    Thread* thread = ctx->thread;
    HandleScope scope(thread);
    Handle<List> list(&scope, ctx->frame->topValue());
    ctx->frame->setTopValue(
        thread->runtime()->listReplicate(thread, list, ntimes));
  }
}

// opcode 22
void Interpreter::doBinaryModulo(Context* ctx, word) {
  HandleScope scope(ctx->thread);
  Handle<Object> divisor(&scope, ctx->frame->popValue());
  Handle<Object> dividend(&scope, ctx->frame->topValue());
  if (divisor->isSmallInteger() && dividend->isSmallInteger()) {
    word smi_divisor = SmallInteger::cast(*divisor)->value();
    word smi_dividend = SmallInteger::cast(*dividend)->value();
    // TODO: Throw:
    //   ZeroDivisionError: integer division or modulo by zero
    if (smi_divisor == 0) {
      UNIMPLEMENTED("ZeroDivisionError");
    }
    ctx->frame->setTopValue(SmallInteger::fromWord(smi_dividend % smi_divisor));
  } else if (dividend->isString()) { // string formatting
    Handle<String> src(&scope, *dividend);
    if (divisor->isObjectArray()) {
      Handle<ObjectArray> args(&scope, *divisor);
      ctx->frame->setTopValue(
          ctx->thread->runtime()->stringFormat(ctx->thread, src, args));
    } else {
      Handle<ObjectArray> args(
          &scope, ctx->thread->runtime()->newObjectArray(1));
      args->atPut(0, *divisor);
      ctx->frame->setTopValue(
          ctx->thread->runtime()->stringFormat(ctx->thread, src, args));
    }
  }
}

// opcode 23
void Interpreter::doBinaryAdd(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread->handles());
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      binaryOperation(thread, ctx->frame, BinaryOp::ADD, self, other);
  ctx->frame->pushValue(result);
}

// opcode 24
void Interpreter::doBinarySubtract(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread->handles());
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      binaryOperation(thread, ctx->frame, BinaryOp::SUB, self, other);
  ctx->frame->pushValue(result);
}

// opcode 25
void Interpreter::doBinarySubscr(Context* ctx, word) {
  // TODO: The implementation of the {BINARY,STORE}_SUBSCR opcodes are
  // enough to get richards working.
  HandleScope scope;
  Handle<Object> key(&scope, ctx->frame->popValue());
  Handle<Object> container(&scope, ctx->frame->popValue());
  if (container->isInstance()) {
    container = ctx->thread->runtime()->instanceDelegate(container);
  }
  if (container->isList()) {
    if (key->isSmallInteger()) {
      word idx = SmallInteger::cast(*key)->value();
      ctx->frame->pushValue(List::cast(*container)->at(idx));
    } else if (key->isSlice()) { // slice as key: custom behavior
      Handle<Slice> slice(&scope, *key);
      Handle<List> list(&scope, *container);
      ctx->frame->pushValue(ctx->thread->runtime()->listSlice(list, slice));
    }
  } else if (container->isDictionary()) {
    Handle<Dictionary> dict(&scope, *container);
    Handle<Object> value(
        &scope, ctx->thread->runtime()->dictionaryAt(dict, key));
    CHECK(!value->isError(), "KeyError");
    ctx->frame->pushValue(*value);
  } else if (container->isObjectArray()) {
    word idx = SmallInteger::cast(*key)->value();
    ctx->frame->pushValue(ObjectArray::cast(*container)->at(idx));
  } else if (container->isString()) {
    // TODO: throw TypeError & IndexError
    if (!key->isInteger()) {
      UNIMPLEMENTED("TypeError: string indices must be integers");
    }
    if (!key->isSmallInteger()) {
      UNIMPLEMENTED("IndexError: cannot fit 'int' into an index-sized integer");
    }
    word idx = SmallInteger::cast(*key)->value();
    byte c = String::cast(*container)->charAt(idx); // TODO: u8charAt?
    ctx->frame->pushValue(
        SmallString::fromBytes(View<byte>(&c, 1))); // safe for SmallString
  } else {
    UNIMPLEMENTED("Custom Subscription");
  }
}

// opcode 26
void Interpreter::doBinaryFloorDivide(Context* ctx, word) {
  word divisor = SmallInteger::cast(ctx->frame->popValue())->value();
  word dividend = SmallInteger::cast(ctx->frame->topValue())->value();
  // TODO: Throw:
  //   ZeroDivisionError: integer division or modulo by zero
  if (divisor == 0) {
    UNIMPLEMENTED("ZeroDivisionError");
  }
  ctx->frame->setTopValue(SmallInteger::fromWord(dividend / divisor));
}

// opcode 27
void Interpreter::doBinaryTrueDivide(Context* ctx, word) {
  HandleScope scope;
  double dividend, divisor;
  Handle<Object> right(&scope, ctx->frame->popValue());
  Handle<Object> left(&scope, ctx->frame->popValue());
  if (right->isSmallInteger()) {
    dividend = SmallInteger::cast(*right)->value();
  } else if (right->isDouble()) {
    dividend = Double::cast(*right)->value();
  } else {
    UNIMPLEMENTED("Arbitrary object binary true divide not supported.");
  }
  if (left->isSmallInteger()) {
    divisor = SmallInteger::cast(*left)->value();
  } else if (left->isDouble()) {
    divisor = Double::cast(*left)->value();
  } else {
    UNIMPLEMENTED("Arbitrary object binary true divide not supported.");
  }
  ctx->frame->pushValue(ctx->thread->runtime()->newDouble(divisor / dividend));
}

// opcode 56
void Interpreter::doInplaceSubtract(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread->handles());
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      inplaceOperation(thread, ctx->frame, BinaryOp::SUB, self, other);
  ctx->frame->pushValue(result);
}

// opcode 60
void Interpreter::doStoreSubscr(Context* ctx, word) {
  // TODO: The implementation of the {BINARY,STORE}_SUBSCR opcodes are
  // enough to get richards working.
  word idx = SmallInteger::cast(ctx->frame->popValue())->value();
  auto list = List::cast(ctx->frame->popValue());
  list->atPut(idx, ctx->frame->popValue());
}

// opcode 64
void Interpreter::doBinaryAnd(Context* ctx, word) {
  word right = SmallInteger::cast(ctx->frame->popValue())->value();
  word left = SmallInteger::cast(ctx->frame->topValue())->value();
  ctx->frame->setTopValue(SmallInteger::fromWord(left & right));
}

// opcode 65
void Interpreter::doBinaryXor(Context* ctx, word) {
  word right = SmallInteger::cast(ctx->frame->popValue())->value();
  word left = SmallInteger::cast(ctx->frame->topValue())->value();
  ctx->frame->setTopValue(SmallInteger::fromWord(left ^ right));
}

// opcode 68
void Interpreter::doGetIter(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread->handles());
  Handle<Object> iterable(&scope, ctx->frame->topValue());
  ctx->frame->setTopValue(thread->runtime()->getIter(iterable));
}

// opcode 71
void Interpreter::doLoadBuildClass(Context* ctx, word) {
  Thread* thread = ctx->thread;
  ValueCell* value_cell = ValueCell::cast(thread->runtime()->buildClass());
  ctx->frame->pushValue(value_cell->value());
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
  Handle<Object> exc(&scope, ctx->frame->popValue());
  if (exc->isNone()) {
    // This is a bound method.
    Handle<Object> exit(&scope, ctx->frame->topValue());
    Handle<Object> none(&scope, None::object());
    ctx->frame->setTopValue(*exc);
    Handle<Object> result(
        &scope,
        callMethod4(ctx->thread, ctx->frame, exit, none, none, none, none));
    ctx->frame->pushValue(*exc);
    ctx->frame->pushValue(*result);
  } else {
    UNIMPLEMENTED("exception handling in context manager");
  }
}

// opcode 82
void Interpreter::doWithCleanupFinish(Context* ctx, word) {
  HandleScope scope(ctx->thread);
  Handle<Object> result(&scope, ctx->frame->popValue());
  Handle<Object> exc(&scope, ctx->frame->popValue());
  if (!exc->isNone()) {
    UNIMPLEMENTED("exception handling in context manager");
  }
}

// opcode 87
void Interpreter::doPopBlock(Context* ctx, word) {
  Frame* frame = ctx->frame;
  TryBlock block = frame->blockStack()->pop();
  frame->setValueStackTop(frame->valueStackBase() - block.level());
}

void Interpreter::doEndFinally(Context* ctx, word) {
  Object* status = ctx->frame->popValue();
  if (!status->isNone()) {
    UNIMPLEMENTED("exception handling in context manager");
  }
}

// opcode 90
void Interpreter::doStoreName(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  DCHECK(frame->implicitGlobals()->isDictionary(), "expected dictionary");
  HandleScope scope;
  Handle<Dictionary> implicit_globals(&scope, frame->implicitGlobals());
  Object* names = Code::cast(frame->code())->names();
  Handle<Object> key(&scope, ObjectArray::cast(names)->at(arg));
  Handle<Object> value(&scope, frame->popValue());
  thread->runtime()->dictionaryAtPutInValueCell(implicit_globals, key, value);
}

// opcode 92
void Interpreter::doUnpackSequence(Context* ctx, word arg) {
  Object* seq = ctx->frame->popValue();
  if (seq->isObjectArray()) {
    DCHECK(
        ObjectArray::cast(seq)->length() == arg,
        "Wrong number of items to unpack");
    while (arg--) {
      ctx->frame->pushValue(ObjectArray::cast(seq)->at(arg));
    }
  } else if (seq->isList()) {
    DCHECK(
        List::cast(seq)->allocated() == arg, "Wrong number of items to unpack");
    while (arg--) {
      ctx->frame->pushValue(List::cast(seq)->at(arg));
    }
  } else if (seq->isRange()) {
    HandleScope scope(ctx->thread);
    Handle<Range> range(&scope, seq);
    word start = range->start();
    word stop = range->stop();
    word step = range->step();
    for (word i = stop; i >= start; i -= step) {
      ctx->frame->pushValue(ctx->thread->runtime()->newInteger(i));
    }
  } else {
    UNIMPLEMENTED("Iterable unpack not supported.");
  }
}

// opcode 93
void Interpreter::doForIter(Context* ctx, word arg) {
  Object* top = ctx->frame->topValue();
  Object* next = Error::object();
  if (top->isRangeIterator()) {
    next = RangeIterator::cast(top)->next();
    // TODO: Support StopIteration exceptions.
  } else if (top->isListIterator()) {
    next = ListIterator::cast(top)->next();
  } else {
    UNIMPLEMENTED("FOR_ITER only support List & Range");
  }

  if (next->isError()) {
    ctx->frame->popValue();
    ctx->pc += arg;
  } else {
    ctx->frame->pushValue(next);
  }
}

// opcode 95
void Interpreter::doStoreAttr(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<Object> receiver(&scope, ctx->frame->popValue());
  auto* names = Code::cast(ctx->frame->code())->names();
  Handle<Object> name(&scope, ObjectArray::cast(names)->at(arg));
  Handle<Object> value(&scope, ctx->frame->popValue());
  thread->runtime()->attributeAtPut(thread, receiver, name, value);
  thread->abortOnPendingException();
}

// opcode 97
void Interpreter::doStoreGlobal(Context* ctx, word arg) {
  ValueCell::cast(ObjectArray::cast(ctx->frame->fastGlobals())->at(arg))
      ->setValue(ctx->frame->popValue());
}

// opcode 98
void Interpreter::doDeleteGlobal(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<ValueCell> value_cell(
      &scope,
      ValueCell::cast(ObjectArray::cast(frame->fastGlobals())->at(arg)));
  CHECK(!value_cell->value()->isValueCell(), "Unbound Globals");
  Handle<Object> key(
      &scope, ObjectArray::cast(Code::cast(frame->code())->names())->at(arg));
  Handle<Dictionary> builtins(&scope, frame->builtins());
  Runtime* runtime = thread->runtime();
  Handle<Object> value_in_builtin(&scope, runtime->dictionaryAt(builtins, key));
  if (value_in_builtin->isError()) {
    value_in_builtin =
        runtime->dictionaryAtPutInValueCell(builtins, key, value_in_builtin);
    ValueCell::cast(*value_in_builtin)->makeUnbound();
  }
  value_cell->setValue(*value_in_builtin);
}

// opcode 100
void Interpreter::doLoadConst(Context* ctx, word arg) {
  Object* consts = Code::cast(ctx->frame->code())->consts();
  ctx->frame->pushValue(ObjectArray::cast(consts)->at(arg));
}

// opcode 101
void Interpreter::doLoadName(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Runtime* runtime = ctx->thread->runtime();
  HandleScope scope(ctx->thread);

  Handle<Object> names(&scope, Code::cast(frame->code())->names());
  Handle<Object> key(&scope, ObjectArray::cast(*names)->at(arg));

  // 1. implicitGlobals
  Handle<Dictionary> implicit_globals(&scope, frame->implicitGlobals());
  Object* value = runtime->dictionaryAt(implicit_globals, key);
  if (value->isValueCell()) {
    // 3a. found in [implicit]/globals but with up to 2-layers of indirection
    DCHECK(!ValueCell::cast(value)->isUnbound(), "unbound globals");
    value = ValueCell::cast(value)->value();
    if (value->isValueCell()) {
      DCHECK(!ValueCell::cast(value)->isUnbound(), "unbound builtins");
      value = ValueCell::cast(value)->value();
    }
    frame->pushValue(value);
    return;
  }

  // In the module body, globals == implicit_globals, so no need to check twice.
  // However in class body, it is a different dictionary.
  if (frame->implicitGlobals() != frame->globals()) {
    // 2. globals
    Handle<Dictionary> globals(&scope, frame->globals());
    value = runtime->dictionaryAt(globals, key);
  }
  if (value->isValueCell()) {
    // 3a. found in [implicit]/globals but with up to 2-layers of indirection
    DCHECK(!ValueCell::cast(value)->isUnbound(), "unbound globals");
    value = ValueCell::cast(value)->value();
    if (value->isValueCell()) {
      DCHECK(!ValueCell::cast(value)->isUnbound(), "unbound builtins");
      value = ValueCell::cast(value)->value();
    }
    frame->pushValue(value);
    return;
  }

  // 3b. not found; check builtins -- one layer of indirection
  Handle<Dictionary> builtins(&scope, frame->builtins());
  value = runtime->dictionaryAt(builtins, key);
  if (value->isValueCell()) {
    DCHECK(!ValueCell::cast(value)->isUnbound(), "unbound builtins");
    value = ValueCell::cast(value)->value();
  }

  if (value->isError()) {
    UNIMPLEMENTED("Unbound variable");
  }
  frame->pushValue(value);
}

// opcode 102
void Interpreter::doBuildTuple(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<ObjectArray> tuple(&scope, thread->runtime()->newObjectArray(arg));
  for (word i = arg - 1; i >= 0; i--) {
    tuple->atPut(i, ctx->frame->popValue());
  }
  ctx->frame->pushValue(*tuple);
}

// opcode 103
void Interpreter::doBuildList(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<ObjectArray> array(&scope, thread->runtime()->newObjectArray(arg));
  for (word i = arg - 1; i >= 0; i--) {
    array->atPut(i, ctx->frame->popValue());
  }
  List* list = List::cast(thread->runtime()->newList());
  list->setItems(*array);
  list->setAllocated(array->length());
  ctx->frame->pushValue(list);
}

// opcode 104
void Interpreter::doBuildSet(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Runtime* runtime = thread->runtime();
  Handle<Set> set(&scope, Set::cast(runtime->newSet()));
  for (word i = arg - 1; i >= 0; i--) {
    runtime->setAdd(set, Handle<Object>(&scope, ctx->frame->popValue()));
  }
  ctx->frame->pushValue(*set);
}

// opcode 105
void Interpreter::doBuildMap(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  Runtime* runtime = thread->runtime();
  HandleScope scope;
  Handle<Dictionary> dict(&scope, runtime->newDictionary(arg));
  for (word i = 0; i < arg; i++) {
    Handle<Object> value(&scope, ctx->frame->popValue());
    Handle<Object> key(&scope, ctx->frame->popValue());
    runtime->dictionaryAtPut(dict, key, value);
  }
  ctx->frame->pushValue(*dict);
}

// opcode 106
void Interpreter::doLoadAttr(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<Object> receiver(&scope, ctx->frame->topValue());
  auto* names = Code::cast(ctx->frame->code())->names();
  Handle<Object> name(&scope, ObjectArray::cast(names)->at(arg));
  ctx->frame->setTopValue(
      thread->runtime()->attributeAt(ctx->thread, receiver, name));
  thread->abortOnPendingException();
}

// opcode 107
void Interpreter::doCompareOp(Context* ctx, word arg) {
  HandleScope scope;
  Handle<Object> right(&scope, ctx->frame->popValue());
  Handle<Object> left(&scope, ctx->frame->popValue());
  CompareOp op = static_cast<CompareOp>(arg);
  Object* result;
  if (op == IS) {
    result = Boolean::fromBool(*left == *right);
  } else if (op == IS_NOT) {
    result = Boolean::fromBool(*left != *right);
  } else if (op == IN) {
    result = sequenceContains(ctx->thread, ctx->frame, left, right);
  } else if (op == NOT_IN) {
    result =
        Boolean::negate(sequenceContains(ctx->thread, ctx->frame, left, right));
  } else {
    result = compareOperation(ctx->thread, ctx->frame, op, left, right);
  }
  ctx->frame->pushValue(result);
}

// opcode 108
void Interpreter::doImportName(Context* ctx, word arg) {
  HandleScope scope;
  Handle<Code> code(&scope, ctx->frame->code());
  Handle<Object> name(&scope, ObjectArray::cast(code->names())->at(arg));
  Handle<Object> fromlist(&scope, ctx->frame->popValue());
  Handle<Object> level(&scope, ctx->frame->topValue());
  Thread* thread = ctx->thread;
  Runtime* runtime = thread->runtime();
  ctx->frame->setTopValue(runtime->importModule(name));
  thread->abortOnPendingException();
}

// opcode 109
void Interpreter::doImportFrom(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Code> code(&scope, ctx->frame->code());
  Handle<Object> name(&scope, ObjectArray::cast(code->names())->at(arg));
  CHECK(name->isString(), "name not found");
  Handle<Module> module(&scope, ctx->frame->topValue());
  Runtime* runtime = thread->runtime();
  CHECK(module->isModule(), "Unexpected type to import from");
  Object* value = runtime->moduleAt(module, name);
  CHECK(!value->isError(), "cannot import name");
  ctx->frame->pushValue(value);
}

// opcode 110
void Interpreter::doJumpForward(Context* ctx, word arg) {
  ctx->pc += arg;
}

// opcode 111
void Interpreter::doJumpIfFalseOrPop(Context* ctx, word arg) {
  Object* result = isTrue(ctx->thread, ctx->frame);
  if (result == Boolean::falseObj()) {
    ctx->pc = arg;
  } else {
    ctx->frame->popValue();
  }
}

// opcode 112
void Interpreter::doJumpIfTrueOrPop(Context* ctx, word arg) {
  Object* result = isTrue(ctx->thread, ctx->frame);
  if (result == Boolean::trueObj()) {
    ctx->pc = arg;
  } else {
    ctx->frame->popValue();
  }
}

// opcode 113
void Interpreter::doJumpAbsolute(Context* ctx, word arg) {
  ctx->pc = arg;
}

// opcode 114
void Interpreter::doPopJumpIfFalse(Context* ctx, word arg) {
  Object* result = isTrue(ctx->thread, ctx->frame);
  ctx->frame->popValue();
  if (result == Boolean::falseObj()) {
    ctx->pc = arg;
  }
}

// opcode 115
void Interpreter::doPopJumpIfTrue(Context* ctx, word arg) {
  Object* result = isTrue(ctx->thread, ctx->frame);
  ctx->frame->popValue();
  if (result == Boolean::trueObj()) {
    ctx->pc = arg;
  }
}

// opcode 116
void Interpreter::doLoadGlobal(Context* ctx, word arg) {
  Object* value =
      ValueCell::cast(ObjectArray::cast(ctx->frame->fastGlobals())->at(arg))
          ->value();
  if (value->isValueCell()) {
    CHECK(!ValueCell::cast(value)->isUnbound(), "Unbound Globals");
    value = ValueCell::cast(value)->value();
  }
  ctx->frame->pushValue(value);
  DCHECK(ctx->frame->topValue() != Error::object(), "unexpected error object");
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

// opcode 124
void Interpreter::doLoadFast(Context* ctx, word arg) {
  // TODO: Need to handle unbound local error
  ctx->frame->pushValue(ctx->frame->getLocal(arg));
}

// opcode 125
void Interpreter::doStoreFast(Context* ctx, word arg) {
  Object* value = ctx->frame->popValue();
  ctx->frame->setLocal(arg, value);
}

// opcode 131
void Interpreter::doCallFunction(Context* ctx, word arg) {
  Object* result = call(ctx->thread, ctx->frame, arg);
  ctx->frame->pushValue(result);
  ctx->thread->abortOnPendingException();
}

// opcode 132
void Interpreter::doMakeFunction(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<Function> function(&scope, thread->runtime()->newFunction());
  function->setName(frame->popValue());
  function->setCode(frame->popValue());
  function->setGlobals(frame->globals());
  Handle<Dictionary> globals(&scope, frame->globals());
  Handle<Dictionary> builtins(&scope, frame->builtins());
  Handle<Code> code(&scope, function->code());
  function->setFastGlobals(
      thread->runtime()->computeFastGlobals(code, globals, builtins));
  function->setEntry(interpreterTrampoline);
  function->setEntryKw(interpreterTrampolineKw);
  function->setEntryEx(interpreterTrampolineEx);
  if (arg & MakeFunctionFlag::CLOSURE) {
    DCHECK((frame->topValue())->isObjectArray(), "Closure expects tuple");
    function->setClosure(frame->popValue());
  }
  if (arg & MakeFunctionFlag::ANNOTATION_DICT) {
    DCHECK(
        (frame->topValue())->isDictionary(),
        "Parameter annotations expect dictionary");
    function->setAnnotations(frame->popValue());
  }
  if (arg & MakeFunctionFlag::DEFAULT_KW) {
    DCHECK(
        (frame->topValue())->isDictionary(),
        "Keyword arguments expect dictionary");
    function->setKwDefaults(frame->popValue());
  }
  if (arg & MakeFunctionFlag::DEFAULT) {
    DCHECK(
        (frame->topValue())->isObjectArray(), "Default arguments expect tuple");
    function->setDefaults(frame->popValue());
  }
  frame->pushValue(*function);
}

// opcode 133
void Interpreter::doBuildSlice(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> step(
      &scope, (arg == 3) ? ctx->frame->popValue() : None::object());
  Handle<Object> stop(&scope, ctx->frame->popValue());
  Handle<Object> start(&scope, ctx->frame->topValue()); // TOP
  Handle<Slice> slice(&scope, thread->runtime()->newSlice(start, stop, step));
  ctx->frame->setTopValue(*slice);
}

// opcode 135
void Interpreter::doLoadClosure(Context* ctx, word arg) {
  Code* code = Code::cast(ctx->frame->code());
  ctx->frame->pushValue(ctx->frame->getLocal(code->nlocals() + arg));
}

// opcode 136
void Interpreter::doLoadDeref(Context* ctx, word arg) {
  Code* code = Code::cast(ctx->frame->code());
  ctx->frame->pushValue(
      ValueCell::cast(ctx->frame->getLocal(code->nlocals() + arg))->value());
}

// opcode 137
void Interpreter::doStoreDeref(Context* ctx, word arg) {
  Code* code = Code::cast(ctx->frame->code());
  ValueCell::cast(ctx->frame->getLocal(code->nlocals() + arg))
      ->setValue(ctx->frame->popValue());
}

// opcode 141
void Interpreter::doCallFunctionKw(Context* ctx, word arg) {
  Object* result = callKw(ctx->thread, ctx->frame, arg);
  ctx->frame->pushValue(result);
  ctx->thread->abortOnPendingException();
}

// opcode 142
void Interpreter::doCallFunctionEx(Context* ctx, word arg) {
  Object* result = callEx(ctx->thread, ctx->frame, arg);
  ctx->frame->pushValue(result);
  ctx->thread->abortOnPendingException();
}

// opcode 143
void Interpreter::doSetupWith(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  Thread* thread = ctx->thread;
  Runtime* runtime = thread->runtime();
  Frame* frame = ctx->frame;
  Handle<Object> mgr(&scope, frame->topValue());
  Handle<Object> enter_selector(&scope, runtime->symbols()->DunderEnter());
  Handle<Object> exit_selector(&scope, runtime->symbols()->DunderExit());
  Handle<Object> enter(
      &scope, lookupMethod(thread, frame, mgr, enter_selector));
  Handle<BoundMethod> exit(
      &scope, runtime->attributeAt(thread, mgr, exit_selector));
  frame->setTopValue(*exit);
  Handle<Object> result(&scope, callMethod1(thread, frame, enter, mgr));

  word stack_depth = frame->valueStackBase() - frame->valueStackTop();
  BlockStack* block_stack = frame->blockStack();
  block_stack->push(
      TryBlock(Bytecode::SETUP_FINALLY, ctx->pc + arg, stack_depth));
  frame->pushValue(*result);
}

// opcode 145
void Interpreter::doListAppend(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  Handle<Object> value(&scope, ctx->frame->popValue());
  Handle<List> list(&scope, ctx->frame->peek(arg - 1));
  ctx->thread->runtime()->listAdd(list, value);
}

// opcode 146
void Interpreter::doSetAdd(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  Handle<Object> value(&scope, ctx->frame->popValue());
  Handle<Set> set(&scope, Set::cast(ctx->frame->peek(arg - 1)));
  ctx->thread->runtime()->setAdd(set, value);
}

// opcode 147
void Interpreter::doMapAdd(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  Handle<Object> key(&scope, ctx->frame->popValue());
  Handle<Object> value(&scope, ctx->frame->popValue());
  Handle<Dictionary> dict(&scope, Dictionary::cast(ctx->frame->peek(arg - 1)));
  ctx->thread->runtime()->dictionaryAtPut(dict, key, value);
}

// opcode 149
void Interpreter::doBuildListUnpack(Context* ctx, word arg) {
  Runtime* runtime = ctx->thread->runtime();
  HandleScope scope(ctx->thread);
  Handle<List> list(&scope, runtime->newList());
  Handle<Object> obj(&scope, None::object());
  for (word i = arg - 1; i >= 0; i--) {
    obj = ctx->frame->peek(i);
    runtime->listExtend(list, obj);
  }
  ctx->frame->dropValues(arg - 1);
  ctx->frame->setTopValue(*list);
}

// opcode 152 & opcode 158
void Interpreter::doBuildTupleUnpack(Context* ctx, word arg) {
  Runtime* runtime = ctx->thread->runtime();
  HandleScope scope(ctx->thread);
  Handle<List> list(&scope, runtime->newList());
  Handle<Object> obj(&scope, None::object());
  for (word i = arg - 1; i >= 0; i--) {
    obj = ctx->frame->peek(i);
    runtime->listExtend(list, obj);
  }
  ObjectArray* tuple =
      ObjectArray::cast(runtime->newObjectArray(list->allocated()));
  for (word i = 0; i < list->allocated(); i++)
    tuple->atPut(i, list->at(i));
  ctx->frame->dropValues(arg - 1);
  ctx->frame->setTopValue(tuple);
}

// opcode 153
void Interpreter::doBuildSetUnpack(
    python::Interpreter::Context* ctx,
    word arg) {
  Runtime* runtime = ctx->thread->runtime();
  HandleScope scope(ctx->thread);
  Handle<Set> set(&scope, runtime->newSet());
  Handle<Object> obj(&scope, None::object());
  for (word i = 0; i < arg; i++) {
    obj = ctx->frame->peek(i);
    runtime->setUpdate(set, obj);
  }
  ctx->frame->dropValues(arg - 1);
  ctx->frame->setTopValue(*set);
}

// opcode 155
// A incomplete impl of FORMAT_VALUE; assumes no conv
void Interpreter::doFormatValue(Context* ctx, word flags) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  int conv = (flags & FVC_MASK);
  int have_fmt_spec = (flags & FVS_MASK) == FVS_HAVE_SPEC;
  switch (conv) {
    case FVC_STR:
    case FVC_REPR:
    case FVC_ASCII:
      UNIMPLEMENTED("Conversion not supported.");
    default: // 0: no conv
      break;
  }

  if (have_fmt_spec) {
    Handle<String> fmt_str(&scope, ctx->frame->popValue());
    Handle<String> value(&scope, ctx->frame->popValue());
    ctx->frame->pushValue(thread->runtime()->stringConcat(fmt_str, value));
  } // else no-op
}

// opcode 156
void Interpreter::doBuildConstKeyMap(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<ObjectArray> keys(&scope, ctx->frame->popValue());
  Handle<Dictionary> dict(
      &scope, thread->runtime()->newDictionary(keys->length()));
  for (word i = arg - 1; i >= 0; i--) {
    Handle<Object> key(&scope, keys->at(i));
    Handle<Object> value(&scope, ctx->frame->popValue());
    thread->runtime()->dictionaryAtPut(dict, key, value);
  }
  ctx->frame->pushValue(*dict);
}

// opcode 157
void Interpreter::doBuildString(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  switch (arg) {
    case 0: // empty
      ctx->frame->pushValue(runtime->newStringWithAll(View<byte>(nullptr, 0)));
      break;
    case 1: // no-op
      break;
    default: { // concat
      Object* res = stringJoin(thread, ctx->frame->valueStackTop(), arg);
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

Object* Interpreter::execute(Thread* thread, Frame* frame) {
  HandleScope scope(thread);
  Code* code = Code::cast(frame->code());
  Handle<ByteArray> byte_array(&scope, code->code());
  Context ctx;
  ctx.pc = 0;
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
        Object* result = ctx.frame->popValue();
        // Clean up after ourselves
        thread->popFrame();
        return result;
      }
      default:
        kOpTable[bc](&ctx, arg);
    }
  }
}

} // namespace python
