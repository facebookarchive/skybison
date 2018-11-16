#include "interpreter.h"

#include <cstdio>
#include <cstdlib>

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines.h"

namespace python {

Object*
Interpreter::call(Thread* thread, Frame* frame, Object** sp, word nargs) {
  Object* callable = sp[nargs];
  switch (callable->layoutId()) {
    case IntrinsicLayoutId::kFunction: {
      frame->setValueStackTop(sp);
      return Function::cast(callable)->entry()(thread, frame, nargs);
    }
    case IntrinsicLayoutId::kBoundMethod: {
      return callBoundMethod(thread, frame, sp, nargs);
    }
    default: { return callCallable(thread, frame, sp, nargs); }
  }
}

Object* Interpreter::callCallable(
    Thread* thread,
    Frame* frame,
    Object** sp,
    word nargs) {
  HandleScope scope(thread->handles());
  Handle<Object> receiver(&scope, sp[nargs]);
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

  sp -= 1;
  for (word i = 0; i <= nargs; i++) {
    sp[i] = sp[i + 1];
  }
  sp[nargs + 1] = *function;
  frame->setValueStackTop(sp);

  // now call the function
  return function->entry()(thread, frame, nargs + 1);
}

Object*
Interpreter::callKw(Thread* thread, Frame* frame, Object** sp, word nargs) {
  // Top of stack is a tuple of keyword argument names in the order they
  // appear on the stack.
  frame->setValueStackTop(sp);
  auto function = Function::cast(*(sp + nargs + 1));
  return function->entryKw()(thread, frame, nargs);
}

Object* Interpreter::callBoundMethod(
    Thread* thread,
    Frame* frame,
    Object** sp,
    word nargs) {
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
    const Handle<Class>& receiver_type) {
  HandleScope scope(thread->handles());
  Runtime* runtime = thread->runtime();
  Handle<Object> selector(&scope, runtime->symbols()->DunderGet());
  Handle<Class> descriptor_type(&scope, runtime->classOf(*descriptor));
  Handle<Function> method(
      &scope, runtime->lookupNameInMro(thread, descriptor_type, selector));
  Object** sp = caller->valueStackTop();
  *--sp = *receiver;
  *--sp = *receiver_type;
  caller->setValueStackTop(sp);
  Object* result = method->entry()(thread, caller, 2);
  sp += 2;
  caller->setValueStackTop(sp);
  return result;
}

Object* Interpreter::lookupMethod(
    Thread* thread,
    Frame* caller,
    const Handle<Object>& receiver,
    const Handle<Object>& selector,
    bool* is_unbound) {
  HandleScope scope(thread->handles());
  Runtime* runtime = thread->runtime();
  Handle<Class> type(&scope, runtime->classOf(*receiver));
  Handle<Object> method(
      &scope, runtime->lookupNameInMro(thread, type, selector));
  if (method->isFunction()) {
    *is_unbound = true;
    return *method;
  }
  *is_unbound = false;
  if (runtime->isNonDataDescriptor(thread, method)) {
    return callDescriptorGet(thread, caller, method, receiver, type);
  }
  return *method;
}

Object* Interpreter::unaryOperation(
    Thread* thread,
    Frame* caller,
    Object** sp,
    const Handle<Object>& receiver,
    const Handle<Object>& selector) {
  HandleScope scope(thread->handles());
  bool is_unbound;
  Handle<Object> method(
      &scope, lookupMethod(thread, caller, receiver, selector, &is_unbound));
  CHECK(!method->isError(), "unknown unary operation");
  if (is_unbound) {
    sp -= 2;
    sp[0] = sp[2];
    sp[1] = *receiver;
    sp[2] = *method;
  } else {
    sp--;
    sp[0] = sp[1];
    sp[1] = *method;
  }
  return Interpreter::call(thread, caller, sp, is_unbound ? 2 : 1);
}

Object* Interpreter::isTrue(Thread* thread, Frame* caller, Object** sp) {
  HandleScope scope(thread->handles());
  Handle<Object> receiver(&scope, *sp);
  Handle<Object> selector(&scope, thread->runtime()->symbols()->DunderBool());
  bool is_unbound;
  Handle<Object> method(
      &scope, lookupMethod(thread, caller, receiver, selector, &is_unbound));
  if (!method->isError()) {
    if (is_unbound) {
      sp -= 2;
      sp[0] = *receiver;
      sp[1] = *method;
    } else {
      sp--;
      sp[0] = *method;
    }
    Handle<Object> result(&scope, Interpreter::call(thread, caller, sp, 1));
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
  method = lookupMethod(thread, caller, receiver, selector, &is_unbound);
  if (!method->isError()) {
    if (is_unbound) {
      sp -= 2;
      sp[0] = *receiver;
      sp[1] = *method;
    } else {
      sp--;
      sp[0] = *method;
    }
    Handle<Object> result(&scope, Interpreter::call(thread, caller, sp, 1));

    if (result->isInteger()) {
      Handle<Integer> integer(&scope, *result);
      if (integer->isPositive()) {
        return Boolean::fromBool(true);
      }
      if (integer->isZero()) {
        return Boolean::fromBool(false);
      }
      UNIMPLEMENTED("throw");
    }
  }
  return Boolean::fromBool(true);
}

Object* Interpreter::compare(
    Thread* thread,
    CompareOp op,
    const Handle<Object>& left,
    const Handle<Object>& right) {
  bool res = false;
  switch (op) {
    case IS: {
      res = (*left == *right);
      break;
    }
    case IS_NOT: {
      res = (*left != *right);
      break;
    }
    case IN:
      UNIMPLEMENTED("IN comparison op");
    case NOT_IN:
      UNIMPLEMENTED("NOT_IN comparison op");
    case EXC_MATCH:
      UNIMPLEMENTED("EXC_MATCH comparison op");
    default:
      return richCompare(thread, op, left, right);
  }
  return Boolean::fromBool(res);
}

template <typename T>
static bool compareUsingDifference(CompareOp op, T cmp) {
  switch (op) {
    case EQ:
      return (cmp == 0);
    case NE:
      return (cmp != 0);
    case LT:
      return (cmp < 0);
    case LE:
      return (cmp <= 0);
    case GT:
      return (cmp > 0);
    case GE:
      return (cmp >= 0);
    default:
      UNREACHABLE("rich comparison with op %x", op);
  }
}

Object* Interpreter::richCompare(
    Thread* thread,
    CompareOp op,
    const Handle<Object>& left,
    const Handle<Object>& right) {
  bool res = false;
  // TODO: call rich comparison method
  if (left->isSmallInteger() && right->isSmallInteger()) {
    word cmp = SmallInteger::cast(*left)->value() -
        SmallInteger::cast(*right)->value();
    res = compareUsingDifference(op, cmp);
  } else if (left->isDouble() && right->isDouble()) {
    double cmp = Double::cast(*left)->value() - Double::cast(*right)->value();
    res = compareUsingDifference(op, cmp);
  } else if (left->isString() && right->isString()) {
    res = compareUsingDifference(op, String::cast(*left)->compare(*right));
  } else if (op == EQ && left->isClass() && right->isClass()) {
    res = (*left == *right);
  } else if (op == EQ && left->isInstance() && right->isInstance()) {
    res = (*left == *right);
  } else if (op == EQ && left->isObjectArray() && right->isObjectArray()) {
    HandleScope scope;
    Handle<ObjectArray> l(&scope, *left);
    Handle<ObjectArray> r(&scope, *right);
    if (l->length() == r->length()) {
      res = true;
      for (word i = 0; i < l->length(); i++) {
        Handle<Object> next_left(&scope, l->at(i));
        Handle<Object> next_right(&scope, r->at(i));
        Object* cmp = richCompare(thread, op, next_left, next_right);
        if (!Boolean::cast(cmp)->value()) {
          res = false;
          break;
        }
      }
    }
  } else if (op == EQ && left->isDictionary() && right->isDictionary()) {
    HandleScope scope;
    Runtime* runtime = thread->runtime();
    Handle<Dictionary> l(&scope, *left);
    Handle<Dictionary> r(&scope, *right);
    if (l->numItems() == r->numItems()) {
      Handle<ObjectArray> l_keys(&scope, runtime->dictionaryKeys(l));
      res = true;
      for (word i = 0; i < l_keys->length(); i++) {
        Handle<Object> l_key(&scope, l_keys->at(i));
        if (!runtime->dictionaryIncludes(r, l_key)) {
          res = false;
          break;
        }
        Handle<Object> next_left(&scope, runtime->dictionaryAt(l, l_key));
        Handle<Object> next_right(&scope, runtime->dictionaryAt(r, l_key));
        Object* cmp = richCompare(thread, op, next_left, next_right);
        if (!Boolean::cast(cmp)->value()) {
          res = false;
          break;
        }
      }
    }
  } else {
    UNIMPLEMENTED("Custom compare");
  }
  return Boolean::fromBool(res);
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
  ctx->sp++;
}

// opcode 2
void Interpreter::doRotTwo(Context* ctx, word) {
  Object* top = *ctx->sp;
  *ctx->sp = *(ctx->sp + 1);
  *(ctx->sp + 1) = top;
}

// opcode 3
void Interpreter::doRotThree(Context* ctx, word) {
  Object* top = *ctx->sp;
  *ctx->sp = *(ctx->sp + 1);
  *(ctx->sp + 1) = *(ctx->sp + 2);
  *(ctx->sp + 2) = top;
}

// opcode 4
void Interpreter::doDupTop(Context* ctx, word) {
  Object* top = *ctx->sp;
  *--ctx->sp = top;
}

// opcode 5
void Interpreter::doDupTopTwo(Context* ctx, word) {
  Object* first = *ctx->sp;
  Object* second = *(ctx->sp + 1);
  *--ctx->sp = second;
  *--ctx->sp = first;
}

// opcode 9
void Interpreter::doNop(Context*, word) {}

// opcode 10
void Interpreter::doUnaryPositive(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread->handles());
  Handle<Object> receiver(&scope, *ctx->sp);
  Handle<Object> selector(&scope, thread->runtime()->symbols()->DunderPos());
  Object* result = Interpreter::unaryOperation(
      thread, ctx->frame, ctx->sp, receiver, selector);
  ctx->sp--;
  *ctx->sp = result;
}

// opcode 11
void Interpreter::doUnaryNegative(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread->handles());
  Handle<Object> receiver(&scope, *ctx->sp);
  Handle<Object> selector(&scope, thread->runtime()->symbols()->DunderNeg());
  Object* result = Interpreter::unaryOperation(
      thread, ctx->frame, ctx->sp, receiver, selector);
  ctx->sp--;
  *ctx->sp = result;
}

// opcode 12
void Interpreter::doUnaryNot(Context* ctx, word) {
  if (Interpreter::isTrue(ctx->thread, ctx->frame, ctx->sp) ==
      Boolean::trueObj()) {
    *ctx->sp = Boolean::falseObj();
  } else {
    *ctx->sp = Boolean::trueObj();
  }
}

// opcode 15
void Interpreter::doUnaryInvert(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread->handles());
  Handle<Object> receiver(&scope, *ctx->sp);
  Handle<Object> selector(&scope, thread->runtime()->symbols()->DunderInvert());
  Object* result = Interpreter::unaryOperation(
      thread, ctx->frame, ctx->sp, receiver, selector);
  ctx->sp--;
  *ctx->sp = result;
}

// opcode 20
void Interpreter::doBinaryMultiply(Context* ctx, word) {
  Object**& sp = ctx->sp;
  if (sp[1]->isSmallInteger()) {
    word right = SmallInteger::cast(*sp++)->value();
    word left = SmallInteger::cast(*sp)->value();
    word result = left * right;
    if (!(left == 0 || (result / left) == right)) {
      UNIMPLEMENTED("small integer overflow");
    }
    *sp = SmallInteger::fromWord(result);
  } else {
    word ntimes = SmallInteger::cast(*sp++)->value();
    Thread* thread = ctx->thread;
    HandleScope scope(thread);
    Handle<List> list(&scope, *sp);
    *sp = thread->runtime()->listReplicate(thread, list, ntimes);
  }
}

// opcode 22
void Interpreter::doBinaryModulo(Context* ctx, word) {
  HandleScope scope(ctx->thread);
  Handle<Object> divisor(&scope, *ctx->sp++);
  Handle<Object> dividend(&scope, *ctx->sp);
  if (divisor->isSmallInteger() && dividend->isSmallInteger()) {
    word smi_divisor = SmallInteger::cast(*divisor)->value();
    word smi_dividend = SmallInteger::cast(*dividend)->value();
    // TODO: Throw:
    //   ZeroDivisionError: integer division or modulo by zero
    if (smi_divisor == 0) {
      UNIMPLEMENTED("ZeroDivisionError");
    }
    *ctx->sp = SmallInteger::fromWord(smi_dividend % smi_divisor);
  } else if (dividend->isString()) { // string formatting
    Handle<String> src(&scope, *dividend);
    if (divisor->isObjectArray()) {
      Handle<ObjectArray> args(&scope, *divisor);
      *ctx->sp = ctx->thread->runtime()->stringFormat(ctx->thread, src, args);
    } else {
      Handle<ObjectArray> args(
          &scope, ctx->thread->runtime()->newObjectArray(1));
      args->atPut(0, *divisor);
      *ctx->sp = ctx->thread->runtime()->stringFormat(ctx->thread, src, args);
    }
  }
}

// opcode 23
void Interpreter::doBinaryAdd(Context* ctx, word) {
  word right = SmallInteger::cast(*ctx->sp++)->value();
  word left = SmallInteger::cast(*ctx->sp)->value();
  *ctx->sp = SmallInteger::fromWord(left + right);
}

// opcode 24
void Interpreter::doBinarySubtract(Context* ctx, word) {
  HandleScope scope(ctx->thread);
  Handle<Object> right(&scope, *ctx->sp++);
  Handle<Object> left(&scope, *ctx->sp);
  if (left->isSmallInteger() && right->isSmallInteger()) {
    *ctx->sp = SmallInteger::fromWord(
        SmallInteger::cast(*left)->value() -
        SmallInteger::cast(*right)->value());
  } else if (left->isDouble() && right->isDouble()) {
    *ctx->sp = ctx->thread->runtime()->newDouble(
        Double::cast(*left)->value() - Double::cast(*right)->value());
  } else {
    UNIMPLEMENTED("Unsupported types for binary ops");
  }
}

// opcode 25
void Interpreter::doBinarySubscr(Context* ctx, word) {
  // TODO: The implementation of the {BINARY,STORE}_SUBSCR opcodes are
  // enough to get richards working.
  Object**& sp = ctx->sp;
  HandleScope scope;
  Handle<Object> key(&scope, *sp++);
  Handle<Object> container(&scope, *sp++);
  if (container->isInstance()) {
    container = ctx->thread->runtime()->instanceDelegate(container);
  }
  if (container->isList()) {
    if (key->isSmallInteger()) {
      word idx = SmallInteger::cast(*key)->value();
      *--sp = List::cast(*container)->at(idx);
    } else if (key->isSlice()) { // slice as key: custom behavior
      Handle<Slice> slice(&scope, *key);
      Handle<List> list(&scope, *container);
      *--sp = ctx->thread->runtime()->listSlice(list, slice);
    }
  } else if (container->isDictionary()) {
    Handle<Dictionary> dict(&scope, *container);
    Handle<Object> value(
        &scope, ctx->thread->runtime()->dictionaryAt(dict, key));
    CHECK(!value->isError(), "KeyError");
    *--sp = *value;
  } else if (container->isObjectArray()) {
    word idx = SmallInteger::cast(*key)->value();
    *--sp = ObjectArray::cast(*container)->at(idx);
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
    *--sp = SmallString::fromBytes(View<byte>(&c, 1)); // safe for SmallString
  } else {
    UNIMPLEMENTED("Custom Subscription");
  }
}

// opcode 26
void Interpreter::doBinaryFloorDivide(Context* ctx, word) {
  word divisor = SmallInteger::cast(*ctx->sp++)->value();
  word dividend = SmallInteger::cast(*ctx->sp)->value();
  // TODO: Throw:
  //   ZeroDivisionError: integer division or modulo by zero
  if (divisor == 0) {
    UNIMPLEMENTED("ZeroDivisionError");
  }
  *ctx->sp = SmallInteger::fromWord(dividend / divisor);
}

// opcode 27
void Interpreter::doBinaryTrueDivide(Context* ctx, word) {
  HandleScope scope;
  double dividend, divisor;
  Handle<Object> right(&scope, *ctx->sp++);
  Handle<Object> left(&scope, *ctx->sp++);
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
  *--ctx->sp = ctx->thread->runtime()->newDouble(divisor / dividend);
}

// opcode 60
void Interpreter::doStoreSubscr(Context* ctx, word) {
  // TODO: The implementation of the {BINARY,STORE}_SUBSCR opcodes are
  // enough to get richards working.
  word idx = SmallInteger::cast(*ctx->sp++)->value();
  auto list = List::cast(*ctx->sp++);
  list->atPut(idx, *ctx->sp++);
}

// opcode 64
void Interpreter::doBinaryAnd(Context* ctx, word) {
  word right = SmallInteger::cast(*ctx->sp++)->value();
  word left = SmallInteger::cast(*ctx->sp)->value();
  *ctx->sp = SmallInteger::fromWord(left & right);
}

// opcode 65
void Interpreter::doBinaryXor(Context* ctx, word) {
  word right = SmallInteger::cast(*ctx->sp++)->value();
  word left = SmallInteger::cast(*ctx->sp)->value();
  *ctx->sp = SmallInteger::fromWord(left ^ right);
}

// opcode 68
void Interpreter::doGetIter(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread->handles());
  Handle<Object> iterable(&scope, *ctx->sp);
  *ctx->sp = thread->runtime()->getIter(iterable);
}

// opcode 71
void Interpreter::doLoadBuildClass(Context* ctx, word) {
  Thread* thread = ctx->thread;
  ValueCell* value_cell = ValueCell::cast(thread->runtime()->buildClass());
  *--ctx->sp = value_cell->value();
}

// opcode 80
void Interpreter::doBreakLoop(Context* ctx, word) {
  Frame* frame = ctx->frame;
  TryBlock block = frame->blockStack()->pop();
  ctx->pc = block.handler();
}

// opcode 87
void Interpreter::doPopBlock(Context* ctx, word) {
  Frame* frame = ctx->frame;
  TryBlock block = frame->blockStack()->pop();
  ctx->sp = frame->valueStackBase() - block.level();
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
  Handle<Object> value(&scope, *ctx->sp++);
  thread->runtime()->dictionaryAtPutInValueCell(implicit_globals, key, value);
}

// opcode 92
void Interpreter::doUnpackSequence(Context* ctx, word arg) {
  Object* seq = *ctx->sp++;
  if (seq->isObjectArray()) {
    DCHECK(
        ObjectArray::cast(seq)->length() == arg,
        "Wrong number of items to unpack");
    while (arg--) {
      *--ctx->sp = ObjectArray::cast(seq)->at(arg);
    }
  } else if (seq->isList()) {
    DCHECK(
        List::cast(seq)->allocated() == arg, "Wrong number of items to unpack");
    while (arg--) {
      *--ctx->sp = List::cast(seq)->at(arg);
    }
  } else if (seq->isRange()) {
    HandleScope scope(ctx->thread);
    Handle<Range> range(&scope, seq);
    word start = range->start();
    word stop = range->stop();
    word step = range->step();
    for (word i = stop; i >= start; i -= step) {
      *--ctx->sp = ctx->thread->runtime()->newInteger(i);
    }
  } else {
    UNIMPLEMENTED("Iterable unpack not supported.");
  }
}

// opcode 93
void Interpreter::doForIter(Context* ctx, word arg) {
  Object* top = *ctx->sp;
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
    ctx->sp++;
    ctx->pc += arg;
  } else {
    *--ctx->sp = next;
  }
}

// opcode 95
void Interpreter::doStoreAttr(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<Object> receiver(&scope, *ctx->sp);
  auto* names = Code::cast(ctx->frame->code())->names();
  Handle<Object> name(&scope, ObjectArray::cast(names)->at(arg));
  Handle<Object> value(&scope, *(ctx->sp + 1));
  ctx->sp += 2;
  thread->runtime()->attributeAtPut(thread, receiver, name, value);
  thread->abortOnPendingException();
}

// opcode 97
void Interpreter::doStoreGlobal(Context* ctx, word arg) {
  ValueCell::cast(ObjectArray::cast(ctx->frame->fastGlobals())->at(arg))
      ->setValue(*ctx->sp++);
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
  *--ctx->sp = ObjectArray::cast(consts)->at(arg);
}

// opcode 101
void Interpreter::doLoadName(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  // This is for module level lookup, behaves the same as LOAD_GLOBAL
  Object* value =
      ValueCell::cast(ObjectArray::cast(frame->fastGlobals())->at(arg))
          ->value();

  if (value->isValueCell()) {
    DCHECK(!ValueCell::cast(value)->isUnbound(), "unbound implicit globals");

    value = ValueCell::cast(value)->value();
  }

  *--ctx->sp = value;
}

// opcode 102
void Interpreter::doBuildTuple(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<ObjectArray> tuple(&scope, thread->runtime()->newObjectArray(arg));
  Object**& sp = ctx->sp;
  for (word i = arg - 1; i >= 0; i--) {
    tuple->atPut(i, *sp++);
  }
  *--sp = *tuple;
}

// opcode 103
void Interpreter::doBuildList(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<ObjectArray> array(&scope, thread->runtime()->newObjectArray(arg));
  for (word i = arg - 1; i >= 0; i--) {
    array->atPut(i, *ctx->sp++);
  }
  List* list = List::cast(thread->runtime()->newList());
  list->setItems(*array);
  list->setAllocated(array->length());
  *--ctx->sp = list;
}

// opcode 104
void Interpreter::doBuildSet(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Runtime* runtime = thread->runtime();
  Handle<Set> set(&scope, Set::cast(runtime->newSet()));
  for (word i = arg - 1; i >= 0; i--) {
    runtime->setAdd(set, Handle<Object>(&scope, *ctx->sp++));
  }
  *--ctx->sp = *set;
}

// opcode 105
void Interpreter::doBuildMap(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  Runtime* runtime = thread->runtime();
  HandleScope scope;
  Handle<Dictionary> dict(&scope, runtime->newDictionary(arg));
  Object**& sp = ctx->sp;
  for (word i = 0; i < arg; i++) {
    Handle<Object> value(&scope, *sp++);
    Handle<Object> key(&scope, *sp++);
    runtime->dictionaryAtPut(dict, key, value);
  }
  *--sp = *dict;
}

// opcode 106
void Interpreter::doLoadAttr(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<Object> receiver(&scope, *ctx->sp);
  auto* names = Code::cast(ctx->frame->code())->names();
  Handle<Object> name(&scope, ObjectArray::cast(names)->at(arg));
  *ctx->sp = thread->runtime()->attributeAt(ctx->thread, receiver, name);
  thread->abortOnPendingException();
}

// opcode 107
void Interpreter::doCompareOp(Context* ctx, word arg) {
  Object**& sp = ctx->sp;
  HandleScope scope;
  Handle<Object> right(&scope, *sp++);
  Handle<Object> left(&scope, *sp++);
  Object* res = Interpreter::compare(
      ctx->thread, static_cast<CompareOp>(arg), left, right);
  DCHECK(res->isBoolean(), "unexpected comparison result");
  *--sp = res;
}

// opcode 108
void Interpreter::doImportName(Context* ctx, word arg) {
  HandleScope scope;
  Handle<Code> code(&scope, ctx->frame->code());
  Handle<Object> name(&scope, ObjectArray::cast(code->names())->at(arg));
  Handle<Object> fromlist(&scope, *ctx->sp++);
  Handle<Object> level(&scope, *ctx->sp);
  Thread* thread = ctx->thread;
  Runtime* runtime = thread->runtime();
  *ctx->sp = runtime->importModule(name);
  thread->abortOnPendingException();
}

// opcode 109
void Interpreter::doImportFrom(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Code> code(&scope, ctx->frame->code());
  Handle<Object> name(&scope, ObjectArray::cast(code->names())->at(arg));
  CHECK(name->isString(), "name not found");
  Handle<Module> module(&scope, *ctx->sp);
  Runtime* runtime = thread->runtime();
  CHECK(module->isModule(), "Unexpected type to import from");
  Object* value = runtime->moduleAt(module, name);
  CHECK(!value->isError(), "cannot import name");
  *--ctx->sp = value;
}

// opcode 110
void Interpreter::doJumpForward(Context* ctx, word arg) {
  ctx->pc += arg;
}

// opcode 111
void Interpreter::doJumpIfFalseOrPop(Context* ctx, word arg) {
  Object* result = Interpreter::isTrue(ctx->thread, ctx->frame, ctx->sp);
  if (result == Boolean::falseObj()) {
    ctx->pc = arg;
  } else {
    ctx->sp++;
  }
}

// opcode 112
void Interpreter::doJumpIfTrueOrPop(Context* ctx, word arg) {
  Object* result = Interpreter::isTrue(ctx->thread, ctx->frame, ctx->sp);
  if (result == Boolean::trueObj()) {
    ctx->pc = arg;
  } else {
    ctx->sp++;
  }
}

// opcode 113
void Interpreter::doJumpAbsolute(Context* ctx, word arg) {
  ctx->pc = arg;
}

// opcode 114
void Interpreter::doPopJumpIfFalse(Context* ctx, word arg) {
  Object* result = Interpreter::isTrue(ctx->thread, ctx->frame, ctx->sp);
  ctx->sp++;
  if (result == Boolean::falseObj()) {
    ctx->pc = arg;
  }
}

// opcode 115
void Interpreter::doPopJumpIfTrue(Context* ctx, word arg) {
  Object* result = Interpreter::isTrue(ctx->thread, ctx->frame, ctx->sp);
  ctx->sp++;
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
  *--ctx->sp = value;
  DCHECK(*ctx->sp != Error::object(), "unexpected error object");
}

// opcode 120
void Interpreter::doSetupLoop(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  word stackDepth = frame->valueStackBase() - ctx->sp;
  BlockStack* blockStack = frame->blockStack();
  blockStack->push(TryBlock(Bytecode::SETUP_LOOP, ctx->pc + arg, stackDepth));
}

// opcode 121
void Interpreter::doSetupExcept(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  word stackDepth = frame->valueStackBase() - ctx->sp;
  BlockStack* blockStack = frame->blockStack();
  blockStack->push(TryBlock(Bytecode::SETUP_EXCEPT, ctx->pc + arg, stackDepth));
}

// opcode 124
void Interpreter::doLoadFast(Context* ctx, word arg) {
  // TODO: Need to handle unbound local error
  *--ctx->sp = ctx->frame->getLocal(arg);
}

// opcode 125
void Interpreter::doStoreFast(Context* ctx, word arg) {
  Object* value = *ctx->sp++;
  ctx->frame->setLocal(arg, value);
}

// opcode 131
void Interpreter::doCallFunction(Context* ctx, word arg) {
  Object* result = Interpreter::call(ctx->thread, ctx->frame, ctx->sp, arg);
  ctx->sp += arg;
  *ctx->sp = result;
}

// opcode 132
void Interpreter::doMakeFunction(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<Function> function(&scope, thread->runtime()->newFunction());
  function->setName(*ctx->sp++);
  function->setCode(*ctx->sp++);
  function->setGlobals(frame->globals());
  Handle<Dictionary> globals(&scope, frame->globals());
  Handle<Dictionary> builtins(&scope, frame->builtins());
  Handle<Code> code(&scope, function->code());
  function->setFastGlobals(
      thread->runtime()->computeFastGlobals(code, globals, builtins));
  function->setEntry(interpreterTrampoline);
  if (arg & MakeFunctionFlag::ANNOTATION_DICT) {
    UNIMPLEMENTED("func annotations");
  }
  if (arg & MakeFunctionFlag::DEFAULT_KW) {
    function->setKwDefaults(*ctx->sp++);
    UNIMPLEMENTED("func keyword defaults");
  }
  if (arg & MakeFunctionFlag::DEFAULT) {
    function->setDefaults(*ctx->sp++);
  }
  if (arg & MakeFunctionFlag::CLOSURE) {
    DCHECK((*ctx->sp)->isObjectArray(), "Closure is not tuple.");
    function->setClosure(*ctx->sp++);
  }
  *--ctx->sp = *function;
}

// opcode 133
void Interpreter::doBuildSlice(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> step(&scope, (arg == 3) ? *ctx->sp++ : None::object());
  Handle<Object> stop(&scope, *ctx->sp++);
  Handle<Object> start(&scope, *ctx->sp); // TOP
  Handle<Slice> slice(&scope, thread->runtime()->newSlice(start, stop, step));
  *ctx->sp = *slice;
}

// opcode 135
void Interpreter::doLoadClosure(Context* ctx, word arg) {
  Code* code = Code::cast(ctx->frame->code());
  *--ctx->sp = ctx->frame->getLocal(code->nlocals() + arg);
}

// opcode 136
void Interpreter::doLoadDeref(Context* ctx, word arg) {
  Code* code = Code::cast(ctx->frame->code());
  *--ctx->sp =
      ValueCell::cast(ctx->frame->getLocal(code->nlocals() + arg))->value();
}

// opcode 137
void Interpreter::doStoreDeref(Context* ctx, word arg) {
  Code* code = Code::cast(ctx->frame->code());
  ValueCell::cast(ctx->frame->getLocal(code->nlocals() + arg))
      ->setValue(*ctx->sp++);
}

// opcode 141
void Interpreter::doCallFunctionKw(Context* ctx, word arg) {
  Object* result = Interpreter::callKw(ctx->thread, ctx->frame, ctx->sp, arg);
  ctx->sp += arg + 1;
  *ctx->sp = result;
}

// opcode 145
void Interpreter::doListAppend(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  Handle<Object> value(&scope, *ctx->sp++);
  Handle<List> list(&scope, *(ctx->sp + arg - 1));
  ctx->thread->runtime()->listAdd(list, value);
}

// opcode 146
void Interpreter::doSetAdd(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  Handle<Object> value(&scope, *ctx->sp++);
  Handle<Set> set(&scope, Set::cast(*(ctx->sp + arg - 1)));
  ctx->thread->runtime()->setAdd(set, value);
}

// opcode 147
void Interpreter::doMapAdd(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  Handle<Object> key(&scope, *ctx->sp++);
  Handle<Object> value(&scope, *ctx->sp++);
  Handle<Dictionary> dict(&scope, Dictionary::cast(*(ctx->sp + arg - 1)));
  ctx->thread->runtime()->dictionaryAtPut(dict, key, value);
}

// opcode 149
void Interpreter::doBuildListUnpack(Context* ctx, word arg) {
  HandleScope scope;
  Runtime* runtime = ctx->thread->runtime();
  Handle<List> list(&scope, runtime->newList());
  Object**& sp = ctx->sp;
  for (word i = arg - 1; i >= 0; i--) {
    HandleScope scope1;
    Handle<Object> obj(&scope1, *(sp + i));
    runtime->listExtend(list, obj);
  }
  sp += arg - 1;
  *sp = *list;
}

// opcode 152
void Interpreter::doBuildTupleUnpack(Context* ctx, word arg) {
  Runtime* runtime = ctx->thread->runtime();
  HandleScope scope;
  Handle<List> list(&scope, runtime->newList());
  Object**& sp = ctx->sp;
  for (word i = arg - 1; i >= 0; i--) {
    HandleScope scope1;
    Handle<Object> obj(&scope1, *(sp + i));
    runtime->listExtend(list, obj);
  }
  ObjectArray* tuple =
      ObjectArray::cast(runtime->newObjectArray(list->allocated()));
  for (word i = 0; i < list->allocated(); i++)
    tuple->atPut(i, list->at(i));
  sp += arg - 1;
  *sp = tuple;
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
    Handle<String> fmt_str(&scope, *ctx->sp++);
    Handle<String> value(&scope, *ctx->sp++);
    *--ctx->sp = thread->runtime()->stringConcat(fmt_str, value);
  } // else no-op
}

// opcode 156
void Interpreter::doBuildConstKeyMap(Context* ctx, word arg) {
  Object**& sp = ctx->sp;
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<ObjectArray> keys(&scope, *sp++);
  Handle<Dictionary> dict(
      &scope, thread->runtime()->newDictionary(keys->length()));
  for (word i = arg - 1; i >= 0; i--) {
    Handle<Object> key(&scope, keys->at(i));
    Handle<Object> value(&scope, *sp++);
    thread->runtime()->dictionaryAtPut(dict, key, value);
  }
  *--sp = *dict;
}

// opcode 157
void Interpreter::doBuildString(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  switch (arg) {
    case 0: // empty
      *--ctx->sp = runtime->newStringWithAll(View<byte>(nullptr, 0));
      break;
    case 1: // no-op
      break;
    default: { // concat
      Object* res = Interpreter::stringJoin(thread, ctx->sp, arg);
      ctx->sp += (arg - 1);
      *ctx->sp = res;
      break;
    }
  }
}

using Op = void (*)(Interpreter::Context*, word);
const Op opTable[] = {
#define HANDLER(name, value, handler) Interpreter::handler,
    FOREACH_BYTECODE(HANDLER)
#undef HANDLER
};

Object* Interpreter::execute(Thread* thread, Frame* frame) {
  HandleScope scope(thread);
  Code* code = Code::cast(frame->code());
  Handle<ByteArray> byteArray(&scope, code->code());
  Context ctx;
  ctx.pc = 0;
  ctx.sp = frame->valueStackTop();
  ctx.thread = thread;
  ctx.frame = frame;
  for (;;) {
    frame->setVirtualPC(ctx.pc);
    Bytecode bc = static_cast<Bytecode>(byteArray->byteAt(ctx.pc++));
    int32 arg = byteArray->byteAt(ctx.pc++);
  dispatch:
    switch (bc) {
      case Bytecode::EXTENDED_ARG: {
        bc = static_cast<Bytecode>(byteArray->byteAt(ctx.pc++));
        arg = (arg << 8) | byteArray->byteAt(ctx.pc++);
        goto dispatch;
      }
      case Bytecode::RETURN_VALUE: {
        Object* result = *ctx.sp++;
        // Clean up after ourselves
        thread->popFrame();
        return result;
      }
      default:
        opTable[bc](&ctx, arg);
    }
  }
}

} // namespace python
