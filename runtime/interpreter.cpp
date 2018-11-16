#include "interpreter.h"

#include <cstdio>
#include <cstdlib>

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines.h"

namespace python {

Object* Interpreter::prepareCallableCall(Thread* thread, Frame* frame,
                                         word callable_idx, word* nargs) {
  HandleScope scope(thread);
  Handle<Object> callable(&scope, frame->peek(callable_idx));
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
    Handle<BoundMethod> method(&scope, *callable);
    frame->insertValueAt(method->self(), callable_idx);
    is_bound = false;
    callable = method->function();
  } else {
    Runtime* runtime = thread->runtime();
    for (;;) {
      Handle<Type> type(&scope, runtime->typeOf(*callable));
      Handle<Object> attr(&scope, runtime->lookupSymbolInMro(
                                      thread, type, SymbolId::kDunderCall));
      if (!attr->isError()) {
        if (attr->isFunction()) {
          // Do not create a short-lived bound method object.
          is_bound = false;
          callable = *attr;
          break;
        }
        if (runtime->isNonDataDescriptor(thread, attr)) {
          Handle<Object> owner(&scope, *type);
          callable = callDescriptorGet(thread, frame, attr, callable, owner);
          if (callable->isFunction()) {
            // Descriptors do not return unbound methods.
            is_bound = true;
            break;
          } else {
            // Retry the lookup using the object returned by the descriptor.
            continue;
          }
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

Object* Interpreter::call(Thread* thread, Frame* frame, word nargs) {
  Object** sp = frame->valueStackTop() + nargs + 1;
  Object* callable = frame->peek(nargs);
  if (!callable->isFunction()) {
    callable = prepareCallableCall(thread, frame, nargs, &nargs);
  }
  Object* result = Function::cast(callable)->entry()(thread, frame, nargs);
  // Clear the stack of the function object.
  frame->setValueStackTop(sp);
  return result;
}

Object* Interpreter::callKw(Thread* thread, Frame* frame, word nargs) {
  // Top of stack is a tuple of keyword argument names in the order they
  // appear on the stack.
  Object** sp = frame->valueStackTop() + nargs + 2;
  Object* result;
  Object* callable = frame->peek(nargs + 1);
  if (callable->isFunction()) {
    result = Function::cast(callable)->entryKw()(thread, frame, nargs);
  } else {
    callable = prepareCallableCall(thread, frame, nargs + 1, &nargs);
    result = Function::cast(callable)->entryKw()(thread, frame, nargs);
  }
  frame->setValueStackTop(sp);
  return result;
}

Object* Interpreter::callEx(Thread* thread, Frame* frame, word flags) {
  // Low bit of flags indicates whether var-keyword argument is on TOS.
  // In all cases, var-positional tuple is next, followed by the function
  // pointer.
  word function_position = (flags & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1;
  Object** sp = frame->valueStackTop() + function_position + 1;
  Object* function = frame->peek(function_position);
  Object* result;
  if (!function->isFunction()) {
    HandleScope scope(thread);
    Handle<Object> callable(&scope, function);
    // Create a new argument tuple with self as the first argument
    word args_position = function_position - 1;
    Handle<ObjectArray> args(&scope, frame->peek(args_position));
    Handle<ObjectArray> new_args(
        &scope, thread->runtime()->newObjectArray(args->length() + 1));
    Handle<Object> target(&scope, None::object());
    if (callable->isBoundMethod()) {
      Handle<BoundMethod> method(&scope, *callable);
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
  result = Function::cast(function)->entryEx()(thread, frame, flags);
  frame->setValueStackTop(sp);
  return result;
}

Object* Interpreter::stringJoin(Thread* thread, Object** sp, word num) {
  word new_len = 0;
  for (word i = num - 1; i >= 0; i--) {
    if (!sp[i]->isString()) {
      UNIMPLEMENTED("Conversion of non-string values not supported.");
    }
    new_len += String::cast(sp[i])->length();
  }

  if (new_len <= SmallStr::kMaxLength) {
    byte buffer[SmallStr::kMaxLength];
    byte* ptr = buffer;
    for (word i = num - 1; i >= 0; i--) {
      String* str = String::cast(sp[i]);
      word len = str->length();
      str->copyTo(ptr, len);
      ptr += len;
    }
    return SmallStr::fromBytes(View<byte>(buffer, new_len));
  }

  HandleScope scope(thread);
  Handle<LargeStr> result(&scope,
                          thread->runtime()->heap()->createLargeStr(new_len));
  word offset = LargeStr::kDataOffset;
  for (word i = num - 1; i >= 0; i--) {
    String* str = String::cast(sp[i]);
    word len = str->length();
    str->copyTo(reinterpret_cast<byte*>(result->address() + offset), len);
    offset += len;
  }
  return *result;
}

Object* Interpreter::callDescriptorGet(Thread* thread, Frame* caller,
                                       const Handle<Object>& descriptor,
                                       const Handle<Object>& receiver,
                                       const Handle<Object>& receiver_type) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Handle<Type> descriptor_type(&scope, runtime->typeOf(*descriptor));
  Handle<Object> method(&scope,
                        runtime->lookupSymbolInMro(thread, descriptor_type,
                                                   SymbolId::kDunderGet));
  DCHECK(!method->isError(), "no __get__ method found");
  return callMethod3(thread, caller, method, descriptor, receiver,
                     receiver_type);
}

Object* Interpreter::callDescriptorSet(Thread* thread, Frame* caller,
                                       const Handle<Object>& descriptor,
                                       const Handle<Object>& receiver,
                                       const Handle<Object>& value) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Handle<Type> descriptor_type(&scope, runtime->typeOf(*descriptor));
  Handle<Object> method(&scope,
                        runtime->lookupSymbolInMro(thread, descriptor_type,
                                                   SymbolId::kDunderSet));
  DCHECK(!method->isError(), "no __set__ method found");
  return callMethod3(thread, caller, method, descriptor, receiver, value);
}

Object* Interpreter::callDescriptorDelete(Thread* thread, Frame* caller,
                                          const Handle<Object>& descriptor,
                                          const Handle<Object>& receiver) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Handle<Type> descriptor_type(&scope, runtime->typeOf(*descriptor));
  Handle<Object> method(&scope,
                        runtime->lookupSymbolInMro(thread, descriptor_type,
                                                   SymbolId::kDunderDelete));
  DCHECK(!method->isError(), "no __delete__ method found");
  return callMethod2(thread, caller, method, descriptor, receiver);
}

Object* Interpreter::lookupMethod(Thread* thread, Frame* caller,
                                  const Handle<Object>& receiver,
                                  SymbolId selector) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Handle<Type> type(&scope, runtime->typeOf(*receiver));
  Handle<Object> method(&scope,
                        runtime->lookupSymbolInMro(thread, type, selector));
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

Object* Interpreter::callMethod1(Thread* thread, Frame* caller,
                                 const Handle<Object>& method,
                                 const Handle<Object>& self) {
  word nargs = 0;
  caller->pushValue(*method);
  if (method->isFunction()) {
    caller->pushValue(*self);
    nargs += 1;
  }
  return call(thread, caller, nargs);
}

Object* Interpreter::callMethod2(Thread* thread, Frame* caller,
                                 const Handle<Object>& method,
                                 const Handle<Object>& self,
                                 const Handle<Object>& other) {
  word nargs = 1;
  caller->pushValue(*method);
  if (method->isFunction()) {
    caller->pushValue(*self);
    nargs += 1;
  }
  caller->pushValue(*other);
  return call(thread, caller, nargs);
}

Object* Interpreter::callMethod3(Thread* thread, Frame* caller,
                                 const Handle<Object>& method,
                                 const Handle<Object>& self,
                                 const Handle<Object>& arg1,
                                 const Handle<Object>& arg2) {
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

Object* Interpreter::callMethod4(Thread* thread, Frame* caller,
                                 const Handle<Object>& method,
                                 const Handle<Object>& self,
                                 const Handle<Object>& arg1,
                                 const Handle<Object>& arg2,
                                 const Handle<Object>& arg3) {
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

Object* Interpreter::unaryOperation(Thread* thread, Frame* caller,
                                    const Handle<Object>& self,
                                    SymbolId selector) {
  HandleScope scope(thread);
  Handle<Object> method(&scope, lookupMethod(thread, caller, self, selector));
  CHECK(!method->isError(), "unknown unary operation");
  return callMethod1(thread, caller, method, self);
}

Object* Interpreter::binaryOperation(Thread* thread, Frame* caller, BinaryOp op,
                                     const Handle<Object>& self,
                                     const Handle<Object>& other) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Handle<Type> self_type(&scope, runtime->typeOf(*self));
  Handle<Type> other_type(&scope, runtime->typeOf(*other));
  bool is_derived_type =
      (*self_type != *other_type) &&
      (runtime->isSubClass(other_type, self_type) == Bool::trueObj());

  SymbolId selector = runtime->binaryOperationSelector(op);
  Handle<Object> self_method(&scope,
                             lookupMethod(thread, caller, self, selector));
  Handle<Object> other_method(&scope,
                              lookupMethod(thread, caller, other, selector));

  SymbolId swapped_selector = runtime->swappedBinaryOperationSelector(op);
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
  UNIMPLEMENTED("Cannot do binary op %ld for types '%s' and '%s'",
                static_cast<word>(op),
                String::cast(self_type->name())->toCString(),
                String::cast(other_type->name())->toCString());
}

Object* Interpreter::inplaceOperation(Thread* thread, Frame* caller,
                                      BinaryOp op, const Handle<Object>& self,
                                      const Handle<Object>& other) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  SymbolId selector = runtime->inplaceOperationSelector(op);
  Handle<Object> method(&scope, lookupMethod(thread, caller, self, selector));
  if (!method->isError()) {
    Object* result = callMethod2(thread, caller, method, self, other);
    if (result != runtime->notImplemented()) {
      return result;
    }
  }
  return binaryOperation(thread, caller, op, self, other);
}

Object* Interpreter::compareOperation(Thread* thread, Frame* caller,
                                      CompareOp op, const Handle<Object>& left,
                                      const Handle<Object>& right) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Handle<Type> left_type(&scope, runtime->typeOf(*left));
  Handle<Type> right_type(&scope, runtime->typeOf(*right));

  bool try_swapped = true;
  bool has_different_type = (*left_type != *right_type);
  if (has_different_type && runtime->isSubClass(right_type, left_type)) {
    try_swapped = false;
    SymbolId selector = runtime->swappedComparisonSelector(op);
    Handle<Object> method(&scope,
                          lookupMethod(thread, caller, right, selector));
    if (!method->isError()) {
      Object* result = callMethod2(thread, caller, method, right, left);
      if (result != runtime->notImplemented()) {
        return result;
      }
    }
  } else {
    SymbolId selector = runtime->comparisonSelector(op);
    Handle<Object> method(&scope, lookupMethod(thread, caller, left, selector));
    if (!method->isError()) {
      Object* result = callMethod2(thread, caller, method, left, right);
      if (result != runtime->notImplemented()) {
        return result;
      }
    }
  }
  if (has_different_type && try_swapped) {
    SymbolId selector = runtime->swappedComparisonSelector(op);
    Handle<Object> method(&scope,
                          lookupMethod(thread, caller, right, selector));
    if (!method->isError()) {
      Object* result = callMethod2(thread, caller, method, right, left);
      if (result != runtime->notImplemented()) {
        return result;
      }
    }
  }
  if (op == CompareOp::EQ) {
    return Bool::fromBool(*left == *right);
  } else if (op == CompareOp::NE) {
    return Bool::fromBool(*left != *right);
  }
  UNIMPLEMENTED("throw");
}

Object* Interpreter::sequenceContains(Thread* thread, Frame* caller,
                                      const Handle<Object>& value,
                                      const Handle<Object>& container) {
  HandleScope scope(thread);
  Handle<Object> method(&scope, lookupMethod(thread, caller, container,
                                             SymbolId::kDunderContains));
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
  HandleScope scope(thread);
  Handle<Object> self(&scope, caller->topValue());
  if (self->isBool()) {
    return *self;
  }
  if (self->isNone()) {
    return Bool::falseObj();
  }
  Handle<Object> method(
      &scope, lookupMethod(thread, caller, self, SymbolId::kDunderBool));
  if (!method->isError()) {
    Handle<Object> result(&scope, callMethod1(thread, caller, method, self));
    if (result->isBool()) {
      return *result;
    }
    if (result->isInt()) {
      Handle<Int> integer(&scope, *result);
      return Bool::fromBool(integer->asWord() > 0);
    }
    UNIMPLEMENTED("throw");
  }
  method = lookupMethod(thread, caller, self, SymbolId::kDunderLen);
  if (!method->isError()) {
    Handle<Object> result(&scope, callMethod1(thread, caller, method, self));
    if (result->isInt()) {
      Handle<Int> integer(&scope, *result);
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
void Interpreter::doPopTop(Context* ctx, word) { ctx->frame->popValue(); }

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
  HandleScope scope(thread);
  Handle<Object> receiver(&scope, ctx->frame->topValue());
  Object* result =
      unaryOperation(thread, ctx->frame, receiver, SymbolId::kDunderPos);
  ctx->frame->setTopValue(result);
}

// opcode 11
void Interpreter::doUnaryNegative(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> receiver(&scope, ctx->frame->topValue());
  Object* result =
      unaryOperation(thread, ctx->frame, receiver, SymbolId::kDunderNeg);
  ctx->frame->setTopValue(result);
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
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> receiver(&scope, ctx->frame->topValue());
  Object* result =
      unaryOperation(thread, ctx->frame, receiver, SymbolId::kDunderInvert);
  ctx->frame->setTopValue(result);
}

// opcode 16
void Interpreter::doBinaryMatrixMultiply(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      binaryOperation(thread, ctx->frame, BinaryOp::MATMUL, self, other);
  ctx->frame->pushValue(result);
}

// opcode 17
void Interpreter::doInplaceMatrixMultiply(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      inplaceOperation(thread, ctx->frame, BinaryOp::MATMUL, self, other);
  ctx->frame->pushValue(result);
}

// opcode 19
void Interpreter::doBinaryPower(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      binaryOperation(thread, ctx->frame, BinaryOp::POW, self, other);
  ctx->frame->pushValue(result);
}

// opcode 20
void Interpreter::doBinaryMultiply(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      binaryOperation(thread, ctx->frame, BinaryOp::MUL, self, other);
  ctx->frame->pushValue(result);
}

// opcode 22
void Interpreter::doBinaryModulo(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      binaryOperation(thread, ctx->frame, BinaryOp::MOD, self, other);
  ctx->frame->pushValue(result);
}

// opcode 23
void Interpreter::doBinaryAdd(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      binaryOperation(thread, ctx->frame, BinaryOp::ADD, self, other);
  ctx->frame->pushValue(result);
}

// opcode 24
void Interpreter::doBinarySubtract(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
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
  HandleScope scope(ctx->thread);
  Runtime* runtime = ctx->thread->runtime();
  Handle<Object> key(&scope, ctx->frame->popValue());
  Handle<Object> container(&scope, ctx->frame->popValue());
  Handle<Type> type(&scope, runtime->typeOf(*container));
  Handle<Object> getitem(
      &scope,
      runtime->lookupSymbolInMro(ctx->thread, type, SymbolId::kDunderGetItem));
  if (getitem->isError()) {
    ctx->frame->pushValue(ctx->thread->throwTypeErrorFromCString(
        "object does not support indexing"));
  } else {
    ctx->frame->pushValue(
        callMethod2(ctx->thread, ctx->frame, getitem, container, key));
  }
}

// opcode 26
void Interpreter::doBinaryFloorDivide(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      binaryOperation(thread, ctx->frame, BinaryOp::FLOORDIV, self, other);
  ctx->frame->pushValue(result);
}

// opcode 27
void Interpreter::doBinaryTrueDivide(Context* ctx, word) {
  HandleScope scope;
  double dividend, divisor;
  Handle<Object> right(&scope, ctx->frame->popValue());
  Handle<Object> left(&scope, ctx->frame->popValue());
  if (right->isSmallInt()) {
    dividend = SmallInt::cast(*right)->value();
  } else if (right->isFloat()) {
    dividend = Float::cast(*right)->value();
  } else {
    UNIMPLEMENTED("Arbitrary object binary true divide not supported.");
  }
  if (left->isSmallInt()) {
    divisor = SmallInt::cast(*left)->value();
  } else if (left->isFloat()) {
    divisor = Float::cast(*left)->value();
  } else {
    UNIMPLEMENTED("Arbitrary object binary true divide not supported.");
  }
  ctx->frame->pushValue(ctx->thread->runtime()->newFloat(divisor / dividend));
}

// opcode 28
void Interpreter::doInplaceFloorDivide(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      inplaceOperation(thread, ctx->frame, BinaryOp::FLOORDIV, self, other);
  ctx->frame->pushValue(result);
}

// opcode 29
void Interpreter::doInplaceTrueDivide(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      inplaceOperation(thread, ctx->frame, BinaryOp::TRUEDIV, self, other);
  ctx->frame->pushValue(result);
}

// opcode 55
void Interpreter::doInplaceAdd(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      inplaceOperation(thread, ctx->frame, BinaryOp::ADD, self, other);
  ctx->frame->pushValue(result);
}

// opcode 56
void Interpreter::doInplaceSubtract(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      inplaceOperation(thread, ctx->frame, BinaryOp::SUB, self, other);
  ctx->frame->pushValue(result);
}

// opcode 57
void Interpreter::doInplaceMultiply(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      inplaceOperation(thread, ctx->frame, BinaryOp::MUL, self, other);
  ctx->frame->pushValue(result);
}

// opcode 59
void Interpreter::doInplaceModulo(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      inplaceOperation(thread, ctx->frame, BinaryOp::MOD, self, other);
  ctx->frame->pushValue(result);
}

// opcode 60
void Interpreter::doStoreSubscr(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> key(&scope, ctx->frame->popValue());
  Handle<Object> container(&scope, ctx->frame->popValue());
  Handle<Object> setitem(&scope, lookupMethod(thread, ctx->frame, container,
                                              SymbolId::kDunderSetItem));
  if (setitem->isError()) {
    UNIMPLEMENTED("throw TypeError");
  }
  Handle<Object> value(&scope, ctx->frame->popValue());
  Object* result =
      callMethod3(thread, ctx->frame, setitem, container, key, value);
  // TODO(T31788973): propagate an exception
  thread->abortOnPendingException();
  ctx->frame->pushValue(result);
}

// opcode 61
void Interpreter::doDeleteSubscr(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> key(&scope, ctx->frame->popValue());
  Handle<Object> container(&scope, ctx->frame->popValue());
  Handle<Object> delitem(&scope, lookupMethod(thread, ctx->frame, container,
                                              SymbolId::kDunderDelItem));
  if (delitem->isError()) {
    UNIMPLEMENTED("throw TypeError");
  }
  Object* result = callMethod2(thread, ctx->frame, delitem, container, key);
  // TODO(T31788973): propagate an exception
  thread->abortOnPendingException();
  ctx->frame->pushValue(result);
}

// opcode 62
void Interpreter::doBinaryLshift(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      binaryOperation(thread, ctx->frame, BinaryOp::LSHIFT, self, other);
  ctx->frame->pushValue(result);
}

// opcode 63
void Interpreter::doBinaryRshift(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      binaryOperation(thread, ctx->frame, BinaryOp::RSHIFT, self, other);
  ctx->frame->pushValue(result);
}

// opcode 64
void Interpreter::doBinaryAnd(Context* ctx, word) {
  word right = SmallInt::cast(ctx->frame->popValue())->value();
  word left = SmallInt::cast(ctx->frame->topValue())->value();
  ctx->frame->setTopValue(SmallInt::fromWord(left & right));
}

// opcode 65
void Interpreter::doBinaryXor(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      binaryOperation(thread, ctx->frame, BinaryOp::XOR, self, other);
  ctx->frame->pushValue(result);
}

// opcode 66
void Interpreter::doBinaryOr(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      binaryOperation(thread, ctx->frame, BinaryOp::OR, self, other);
  ctx->frame->pushValue(result);
}

// opcode 67
void Interpreter::doInplacePower(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      inplaceOperation(thread, ctx->frame, BinaryOp::POW, self, other);
  ctx->frame->pushValue(result);
}

// opcode 68
void Interpreter::doGetIter(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> iterable(&scope, ctx->frame->topValue());
  Handle<Object> method(&scope, lookupMethod(thread, thread->currentFrame(),
                                             iterable, SymbolId::kDunderIter));
  if (method->isError()) {
    thread->throwTypeErrorFromCString("object is not iterable");
    thread->abortOnPendingException();
  }
  Handle<Object> iterator(
      &scope, callMethod1(thread, thread->currentFrame(), method, iterable));
  thread->abortOnPendingException();
  ctx->frame->setTopValue(*iterator);
}

// opcode 70
void Interpreter::doPrintExpr(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Frame* frame = ctx->frame;
  Handle<Object> value(&scope, frame->popValue());
  Handle<ValueCell> value_cell(&scope, thread->runtime()->displayHook());
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
  ValueCell* value_cell = ValueCell::cast(thread->runtime()->buildClass());
  ctx->frame->pushValue(value_cell->value());
}

// opcode 75
void Interpreter::doInplaceLshift(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      inplaceOperation(thread, ctx->frame, BinaryOp::LSHIFT, self, other);
  ctx->frame->pushValue(result);
}

// opcode 76
void Interpreter::doInplaceRshift(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      inplaceOperation(thread, ctx->frame, BinaryOp::RSHIFT, self, other);
  ctx->frame->pushValue(result);
}

// opcode 77
void Interpreter::doInplaceAnd(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      inplaceOperation(thread, ctx->frame, BinaryOp::AND, self, other);
  ctx->frame->pushValue(result);
}

// opcode 78
void Interpreter::doInplaceXor(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      inplaceOperation(thread, ctx->frame, BinaryOp::XOR, self, other);
  ctx->frame->pushValue(result);
}

// opcode 79
void Interpreter::doInplaceOr(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> other(&scope, ctx->frame->popValue());
  Handle<Object> self(&scope, ctx->frame->popValue());
  Object* result =
      inplaceOperation(thread, ctx->frame, BinaryOp::OR, self, other);
  ctx->frame->pushValue(result);
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
    Handle<Object> result(&scope, callMethod4(ctx->thread, ctx->frame, exit,
                                              none, none, none, none));
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

// opcode 85
void Interpreter::doSetupAnnotations(Context* ctx, word) {
  HandleScope scope(ctx->thread);
  Runtime* runtime = ctx->thread->runtime();
  Handle<Dict> implicit_globals(&scope, ctx->frame->implicitGlobals());
  Handle<Object> annotations(
      &scope, runtime->symbols()->at(SymbolId::kDunderAnnotations));
  Handle<Object> anno_dict(&scope,
                           runtime->dictAt(implicit_globals, annotations));
  if (anno_dict->isError()) {
    Handle<Object> new_dict(&scope, runtime->newDict());
    runtime->dictAtPutInValueCell(implicit_globals, annotations, new_dict);
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
  DCHECK(frame->implicitGlobals()->isDict(), "expected dict");
  HandleScope scope;
  Handle<Dict> implicit_globals(&scope, frame->implicitGlobals());
  Object* names = Code::cast(frame->code())->names();
  Handle<Object> key(&scope, ObjectArray::cast(names)->at(arg));
  Handle<Object> value(&scope, frame->popValue());
  thread->runtime()->dictAtPutInValueCell(implicit_globals, key, value);
}

// opcode 91
void Interpreter::doDeleteName(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  DCHECK(frame->implicitGlobals()->isDict(), "expected dict");
  HandleScope scope;
  Handle<Dict> implicit_globals(&scope, frame->implicitGlobals());
  Object* names = Code::cast(frame->code())->names();
  Handle<Object> key(&scope, ObjectArray::cast(names)->at(arg));
  Object* value;
  if (!thread->runtime()->dictRemove(implicit_globals, key, &value)) {
    UNIMPLEMENTED("item not found in delete name");
  }
}

// opcode 92
void Interpreter::doUnpackSequence(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> iterable(&scope, ctx->frame->popValue());
  Handle<Object> iter_method(
      &scope, lookupMethod(thread, thread->currentFrame(), iterable,
                           SymbolId::kDunderIter));
  if (iter_method->isError()) {
    thread->throwTypeErrorFromCString("object is not iterable");
    thread->abortOnPendingException();
  }
  Handle<Object> iterator(&scope, callMethod1(thread, thread->currentFrame(),
                                              iter_method, iterable));
  thread->abortOnPendingException();
  Handle<Object> next_method(&scope, lookupMethod(thread, ctx->frame, iterator,
                                                  SymbolId::kDunderNext));
  if (next_method->isError()) {
    thread->throwTypeErrorFromCString("iter() returned non-iterator");
    thread->abortOnPendingException();
  }
  for (word i = 0; i < arg; i++) {
    ctx->frame->pushValue(None::object());
  }
  word num_pushed;
  for (num_pushed = 0; num_pushed < arg; num_pushed++) {
    if (thread->runtime()->isIteratorExhausted(thread, iterator)) {
      break;
    }
    Handle<Object> value(
        &scope, callMethod1(thread, ctx->frame, next_method, iterator));
    if (value->isError()) {
      thread->abortOnPendingException();
      break;
    }
    ctx->frame->setValueAt(*value, num_pushed);
  }
  if (num_pushed < arg) {
    ctx->frame->dropValues(arg);
    thread->throwValueErrorFromCString("not enough values to unpack");
    thread->abortOnPendingException();
  }
  if (!thread->runtime()->isIteratorExhausted(thread, iterator)) {
    ctx->frame->dropValues(arg);
    thread->throwValueErrorFromCString("too many values to unpack");
    thread->abortOnPendingException();
  }
}

// opcode 93
void Interpreter::doForIter(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> iterator(&scope, ctx->frame->topValue());
  Handle<Object> next_method(&scope, lookupMethod(thread, ctx->frame, iterator,
                                                  SymbolId::kDunderNext));
  if (next_method->isError()) {
    thread->throwValueErrorFromCString("iter() returned non-iterator");
    thread->abortOnPendingException();
  }
  if (thread->runtime()->isIteratorExhausted(thread, iterator)) {
    // Break out of the loop
    ctx->frame->popValue();
    ctx->pc += arg;
    return;
  }
  Handle<Object> value(&scope,
                       callMethod1(thread, ctx->frame, next_method, iterator));
  if (!value->isError()) {
    // Common case of the iterator returning a value
    ctx->frame->pushValue(*value);
    return;
  }
  // Slow-path for all other exception types
  ctx->thread->abortOnPendingException();
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
  // TODO(T31788973): propagate an exception
  thread->abortOnPendingException();
}

// opcode 96
void Interpreter::doDeleteAttr(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<Object> receiver(&scope, ctx->frame->popValue());
  auto* names = Code::cast(ctx->frame->code())->names();
  Handle<Object> name(&scope, ObjectArray::cast(names)->at(arg));
  thread->runtime()->attributeDel(ctx->thread, receiver, name);
  // TODO(T31788973): propagate an exception
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
  Handle<Dict> builtins(&scope, frame->builtins());
  Runtime* runtime = thread->runtime();
  Handle<Object> value_in_builtin(&scope, runtime->dictAt(builtins, key));
  if (value_in_builtin->isError()) {
    value_in_builtin =
        runtime->dictAtPutInValueCell(builtins, key, value_in_builtin);
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
  Handle<Dict> implicit_globals(&scope, frame->implicitGlobals());
  Object* value = runtime->dictAt(implicit_globals, key);
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
  // However in class body, it is a different dict.
  if (frame->implicitGlobals() != frame->globals()) {
    // 2. globals
    Handle<Dict> globals(&scope, frame->globals());
    value = runtime->dictAt(globals, key);
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
  Handle<Dict> builtins(&scope, frame->builtins());
  value = runtime->dictAt(builtins, key);
  if (value->isValueCell()) {
    DCHECK(!ValueCell::cast(value)->isUnbound(), "unbound builtins");
    value = ValueCell::cast(value)->value();
  }

  if (value->isError()) {
    UNIMPLEMENTED("Unbound variable '%s'", String::cast(*key)->toCString());
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
  Handle<Dict> dict(&scope, runtime->newDict(arg));
  for (word i = 0; i < arg; i++) {
    Handle<Object> value(&scope, ctx->frame->popValue());
    Handle<Object> key(&scope, ctx->frame->popValue());
    runtime->dictAtPut(dict, key, value);
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
  Object* result = thread->runtime()->attributeAt(thread, receiver, name);
  // TODO(T31788973): propagate an exception
  thread->abortOnPendingException();
  ctx->frame->setTopValue(result);
}

// opcode 107
void Interpreter::doCompareOp(Context* ctx, word arg) {
  HandleScope scope;
  Handle<Object> right(&scope, ctx->frame->popValue());
  Handle<Object> left(&scope, ctx->frame->popValue());
  CompareOp op = static_cast<CompareOp>(arg);
  Object* result;
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
  Handle<Code> code(&scope, ctx->frame->code());
  Handle<Object> name(&scope, ObjectArray::cast(code->names())->at(arg));
  Handle<Object> fromlist(&scope, ctx->frame->popValue());
  Handle<Object> level(&scope, ctx->frame->topValue());
  Thread* thread = ctx->thread;
  Runtime* runtime = thread->runtime();
  Object* result = runtime->importModule(name);
  // TODO(T31788973): propagate an exception
  thread->abortOnPendingException();
  ctx->frame->setTopValue(result);
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
void Interpreter::doJumpForward(Context* ctx, word arg) { ctx->pc += arg; }

// opcode 111
void Interpreter::doJumpIfFalseOrPop(Context* ctx, word arg) {
  Object* result = isTrue(ctx->thread, ctx->frame);
  if (result == Bool::falseObj()) {
    ctx->pc = arg;
  } else {
    ctx->frame->popValue();
  }
}

// opcode 112
void Interpreter::doJumpIfTrueOrPop(Context* ctx, word arg) {
  Object* result = isTrue(ctx->thread, ctx->frame);
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
  Object* result = isTrue(ctx->thread, ctx->frame);
  ctx->frame->popValue();
  if (result == Bool::falseObj()) {
    ctx->pc = arg;
  }
}

// opcode 115
void Interpreter::doPopJumpIfTrue(Context* ctx, word arg) {
  Object* result = isTrue(ctx->thread, ctx->frame);
  ctx->frame->popValue();
  if (result == Bool::trueObj()) {
    ctx->pc = arg;
  }
}

// opcode 116
void Interpreter::doLoadGlobal(Context* ctx, word arg) {
  Object* value =
      ValueCell::cast(ObjectArray::cast(ctx->frame->fastGlobals())->at(arg))
          ->value();
  if (value->isValueCell()) {
    CHECK(
        !ValueCell::cast(value)->isUnbound(), "Unbound global '%s'",
        String::cast(
            ObjectArray::cast(Code::cast(ctx->frame->code())->names())->at(arg))
            ->toCString());
    value = ValueCell::cast(value)->value();
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
  // TODO: Need to handle unbound local error
  Object* value = ctx->frame->getLocal(arg);
  if (value->isError()) {
    Object* name =
        ObjectArray::cast(Code::cast(ctx->frame->code())->varnames())->at(arg);
    UNIMPLEMENTED("unbound local %s", String::cast(name)->toCString());
  }
  ctx->frame->pushValue(ctx->frame->getLocal(arg));
}

// opcode 125
void Interpreter::doStoreFast(Context* ctx, word arg) {
  Object* value = ctx->frame->popValue();
  ctx->frame->setLocal(arg, value);
}

// opcode 126
void Interpreter::doDeleteFast(Context* ctx, word arg) {
  // TODO(T32821785): use another immediate value than Error to signal unbound
  // local
  if (ctx->frame->getLocal(arg) == Error::object()) {
    Object* name =
        ObjectArray::cast(Code::cast(ctx->frame->code())->varnames())->at(arg);
    UNIMPLEMENTED("unbound local %s", String::cast(name)->toCString());
  }
  ctx->frame->setLocal(arg, Error::object());
}

// opcode 127
void Interpreter::doStoreAnnotation(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  Runtime* runtime = ctx->thread->runtime();
  Handle<Object> names(&scope, Code::cast(ctx->frame->code())->names());
  Handle<Object> value(&scope, ctx->frame->popValue());
  Handle<Object> key(&scope, ObjectArray::cast(*names)->at(arg));

  Handle<Dict> implicit_globals(&scope, ctx->frame->implicitGlobals());
  Handle<Object> annotations(
      &scope, runtime->symbols()->at(SymbolId::kDunderAnnotations));
  Handle<Object> value_cell(&scope,
                            runtime->dictAt(implicit_globals, annotations));
  Handle<Dict> anno_dict(&scope, ValueCell::cast(*value_cell)->value());
  runtime->dictAtPut(anno_dict, key, value);
}

// opcode 131
void Interpreter::doCallFunction(Context* ctx, word arg) {
  Object* result = call(ctx->thread, ctx->frame, arg);
  // TODO(T31788973): propagate an exception
  ctx->thread->abortOnPendingException();
  ctx->frame->pushValue(result);
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
  Handle<Dict> globals(&scope, frame->globals());
  Handle<Dict> builtins(&scope, frame->builtins());
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
    DCHECK((frame->topValue())->isDict(), "Parameter annotations expect dict");
    function->setAnnotations(frame->popValue());
  }
  if (arg & MakeFunctionFlag::DEFAULT_KW) {
    DCHECK((frame->topValue())->isDict(), "Keyword arguments expect dict");
    function->setKwDefaults(frame->popValue());
  }
  if (arg & MakeFunctionFlag::DEFAULT) {
    DCHECK((frame->topValue())->isObjectArray(),
           "Default arguments expect tuple");
    function->setDefaults(frame->popValue());
  }
  frame->pushValue(*function);
}

// opcode 133
void Interpreter::doBuildSlice(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> step(&scope,
                      (arg == 3) ? ctx->frame->popValue() : None::object());
  Handle<Object> stop(&scope, ctx->frame->popValue());
  Handle<Object> start(&scope, ctx->frame->topValue());  // TOP
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
  // TODO(T31788973): propagate an exception
  ctx->thread->abortOnPendingException();
  ctx->frame->pushValue(result);
}

// opcode 142
void Interpreter::doCallFunctionEx(Context* ctx, word arg) {
  Object* result = callEx(ctx->thread, ctx->frame, arg);
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
  Handle<Object> mgr(&scope, frame->topValue());
  Handle<Object> exit_selector(&scope, runtime->symbols()->DunderExit());
  Handle<Object> enter(
      &scope, lookupMethod(thread, frame, mgr, SymbolId::kDunderEnter));
  Handle<BoundMethod> exit(&scope,
                           runtime->attributeAt(thread, mgr, exit_selector));
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
  Handle<Dict> dict(&scope, Dict::cast(ctx->frame->peek(arg - 1)));
  ctx->thread->runtime()->dictAtPut(dict, key, value);
}

// opcode 149
void Interpreter::doBuildListUnpack(Context* ctx, word arg) {
  Runtime* runtime = ctx->thread->runtime();
  HandleScope scope(ctx->thread);
  Handle<List> list(&scope, runtime->newList());
  Handle<Object> obj(&scope, None::object());
  for (word i = arg - 1; i >= 0; i--) {
    obj = ctx->frame->peek(i);
    runtime->listExtend(ctx->thread, list, obj);
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
    runtime->listExtend(ctx->thread, list, obj);
  }
  ObjectArray* tuple =
      ObjectArray::cast(runtime->newObjectArray(list->allocated()));
  for (word i = 0; i < list->allocated(); i++) {
    tuple->atPut(i, list->at(i));
  }
  ctx->frame->dropValues(arg - 1);
  ctx->frame->setTopValue(tuple);
}

// opcode 153
void Interpreter::doBuildSetUnpack(Context* ctx, word arg) {
  Runtime* runtime = ctx->thread->runtime();
  HandleScope scope(ctx->thread);
  Handle<Set> set(&scope, runtime->newSet());
  Handle<Object> obj(&scope, None::object());
  for (word i = 0; i < arg; i++) {
    obj = ctx->frame->peek(i);
    runtime->setUpdate(ctx->thread, set, obj);
  }
  ctx->frame->dropValues(arg - 1);
  ctx->frame->setTopValue(*set);
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
    Handle<String> fmt_str(&scope, ctx->frame->popValue());
    Handle<String> value(&scope, ctx->frame->popValue());
    ctx->frame->pushValue(thread->runtime()->stringConcat(fmt_str, value));
  }  // else no-op
}

// opcode 156
void Interpreter::doBuildConstKeyMap(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<ObjectArray> keys(&scope, ctx->frame->popValue());
  Handle<Dict> dict(&scope, thread->runtime()->newDict(keys->length()));
  for (word i = arg - 1; i >= 0; i--) {
    Handle<Object> key(&scope, keys->at(i));
    Handle<Object> value(&scope, ctx->frame->popValue());
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
      ctx->frame->pushValue(runtime->newStringWithAll(View<byte>(nullptr, 0)));
      break;
    case 1:  // no-op
      break;
    default: {  // concat
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

}  // namespace python
