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
    case IntrinsicLayoutId::kType: {
      return callType(thread, frame, sp, nargs);
    }
    default: {
      // TODO(T25382628): Handle __call__
      UNIMPLEMENTED("Callable type");
    }
  }
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

Object*
Interpreter::callType(Thread* thread, Frame* frame, Object** sp, word nargs) {
  HandleScope scope;
  Runtime* runtime = thread->runtime();
  Handle<Class> klass(&scope, sp[nargs]);

  Handle<Object> name(&scope, runtime->symbols()->DunderNew());
  Handle<Function> dunder_new(
      &scope, runtime->lookupNameInMro(thread, klass, name));

  sp -= 1;
  for (word i = 0; i < nargs; i++) {
    sp[i] = sp[i + 1];
  }
  sp[nargs] = *klass;
  sp[nargs + 1] = *dunder_new;

  // call the dunder new
  frame->setValueStackTop(sp);
  Handle<Object> result(&scope, dunder_new->entry()(thread, frame, nargs + 1));

  // Check for an __init__ method.
  //
  // If no __init__ method exists for this class we can just return the newly
  // allocated instance.
  Handle<Object> dunder_init(&scope, runtime->symbols()->DunderInit());
  Handle<Object> init(
      &scope, runtime->lookupNameInMro(thread, klass, dunder_init));
  CHECK(init->isFunction(), "instance is missing an init method");

  // Move the method and receiver into the expected location.
  sp[nargs] = *result;
  sp[nargs + 1] = *init;

  // Call the initializer method.
  frame->setValueStackTop(sp);
  Function::cast(*init)->entry()(thread, frame, nargs + 1);

  // Return the initialized instance.
  return *result;
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

namespace interpreter {
struct Context {
  Thread* thread;
  Frame* frame;
  Object** sp;
  word pc;
};

Bytecode currentBytecode(const Context* ctx) {
  ByteArray* code = ByteArray::cast(Code::cast(ctx->frame->code())->code());
  word pc = ctx->pc;
  word result;
  do {
    pc -= 2;
    result = code->byteAt(pc);
  } while (result == Bytecode::EXTENDED_ARG);
  return static_cast<Bytecode>(result);
}

void INVALID_BYTECODE(Context* ctx, word) {
  Bytecode bc = currentBytecode(ctx);
  UNREACHABLE("bytecode '%s'", kBytecodeNames[bc]);
}

void NOT_IMPLEMENTED(Context* ctx, word) {
  Bytecode bc = currentBytecode(ctx);
  UNIMPLEMENTED("bytecode '%s'", kBytecodeNames[bc]);
}

void LOAD_CONST(Context* ctx, word arg) {
  Object* consts = Code::cast(ctx->frame->code())->consts();
  *--ctx->sp = ObjectArray::cast(consts)->at(arg);
}

void LOAD_FAST(Context* ctx, word arg) {
  // TODO: Need to handle unbound local error
  *--ctx->sp = ctx->frame->getLocal(arg);
}

void STORE_FAST(Context* ctx, word arg) {
  Object* value = *ctx->sp++;
  ctx->frame->setLocal(arg, value);
}

void LOAD_NAME(Context* ctx, word arg) {
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

void STORE_NAME(Context* ctx, word arg) {
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

void POP_TOP(Context* ctx, word) {
  ctx->sp++;
}

void DUP_TOP(Context* ctx, word) {
  Object* top = *ctx->sp;
  *--ctx->sp = top;
}

void DUP_TOP_TWO(Context* ctx, word) {
  Object* first = *ctx->sp;
  Object* second = *(ctx->sp + 1);
  *--ctx->sp = second;
  *--ctx->sp = first;
}

void ROT_TWO(Context* ctx, word) {
  Object* top = *ctx->sp;
  *ctx->sp = *(ctx->sp + 1);
  *(ctx->sp + 1) = top;
}

void ROT_THREE(Context* ctx, word) {
  Object* top = *ctx->sp;
  *ctx->sp = *(ctx->sp + 1);
  *(ctx->sp + 1) = *(ctx->sp + 2);
  *(ctx->sp + 2) = top;
}

void CALL_FUNCTION(Context* ctx, word arg) {
  Object* result = Interpreter::call(ctx->thread, ctx->frame, ctx->sp, arg);
  ctx->sp += arg;
  *ctx->sp = result;
}

void CALL_FUNCTION_KW(Context* ctx, word arg) {
  Object* result = Interpreter::callKw(ctx->thread, ctx->frame, ctx->sp, arg);
  ctx->sp += arg + 1;
  *ctx->sp = result;
}

void LOAD_GLOBAL(Context* ctx, word arg) {
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

void STORE_GLOBAL(Context* ctx, word arg) {
  ValueCell::cast(ObjectArray::cast(ctx->frame->fastGlobals())->at(arg))
      ->setValue(*ctx->sp++);
}

void DELETE_GLOBAL(Context* ctx, word arg) {
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

void MAKE_FUNCTION(Context* ctx, word arg) {
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

void LIST_APPEND(Context* ctx, word arg) {
  HandleScope scope(ctx->thread);
  Handle<Object> value(&scope, *ctx->sp++);
  Handle<List> list(&scope, List::cast(*(ctx->sp + arg - 1)));
  ctx->thread->runtime()->listAdd(list, value);
}

void BUILD_LIST(Context* ctx, word arg) {
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

void BUILD_SET(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Runtime* runtime = thread->runtime();
  Handle<Set> set(&scope, Set::cast(runtime->newSet()));
  for (word i = arg - 1; i >= 0; i--) {
    runtime->setAdd(set, Handle<Object>(&scope, *ctx->sp++));
  }
  *--ctx->sp = *set;
}

void BUILD_SLICE(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread);
  Handle<Object> step(&scope, (arg == 3) ? *ctx->sp++ : None::object());
  Handle<Object> stop(&scope, *ctx->sp++);
  Handle<Object> start(&scope, *ctx->sp); // TOP
  Handle<Slice> slice(&scope, thread->runtime()->newSlice(start, stop, step));
  *ctx->sp = *slice;
}

void BUILD_TUPLE(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<ObjectArray> tuple(&scope, thread->runtime()->newObjectArray(arg));
  Object**& sp = ctx->sp;
  for (word i = arg - 1; i >= 0; i--) {
    tuple->atPut(i, *sp++);
  }
  *--sp = *tuple;
}

void BUILD_MAP(Context* ctx, word arg) {
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

void POP_JUMP_IF_FALSE(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  Object* cond = *ctx->sp++;
  if (!thread->runtime()->isTruthy(cond)) {
    ctx->pc = arg;
  }
}

void POP_JUMP_IF_TRUE(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  Object* cond = *ctx->sp++;
  if (thread->runtime()->isTruthy(cond)) {
    ctx->pc = arg;
  }
}

void JUMP_IF_FALSE_OR_POP(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  Object* cond = *ctx->sp;
  if (!thread->runtime()->isTruthy(cond)) {
    ctx->pc = arg;
  } else {
    ctx->sp++;
  }
}

void JUMP_IF_TRUE_OR_POP(Context* ctx, word arg) {
  Object* cond = *ctx->sp;
  Thread* thread = ctx->thread;
  if (thread->runtime()->isTruthy(cond)) {
    ctx->pc = arg;
  } else {
    ctx->sp++;
  }
}

void UNARY_NOT(Context* ctx, word) {
  Thread* thread = ctx->thread;
  bool negated = !thread->runtime()->isTruthy(*ctx->sp);
  *ctx->sp = Boolean::fromBool(negated);
}

void LOAD_BUILD_CLASS(Context* ctx, word) {
  Thread* thread = ctx->thread;
  ValueCell* value_cell = ValueCell::cast(thread->runtime()->buildClass());
  *--ctx->sp = value_cell->value();
}

void JUMP_ABSOLUTE(Context* ctx, word arg) {
  ctx->pc = arg;
}

void JUMP_FORWARD(Context* ctx, word arg) {
  ctx->pc += arg;
}

void SETUP_EXCEPT(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  word stackDepth = frame->valueStackBase() - ctx->sp;
  BlockStack* blockStack = frame->blockStack();
  blockStack->push(TryBlock(Bytecode::SETUP_EXCEPT, ctx->pc + arg, stackDepth));
}

void SETUP_LOOP(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  word stackDepth = frame->valueStackBase() - ctx->sp;
  BlockStack* blockStack = frame->blockStack();
  blockStack->push(TryBlock(Bytecode::SETUP_LOOP, ctx->pc + arg, stackDepth));
}

void POP_BLOCK(Context* ctx, word) {
  Frame* frame = ctx->frame;
  TryBlock block = frame->blockStack()->pop();
  ctx->sp = frame->valueStackBase() - block.level();
}

void BREAK_LOOP(Context* ctx, word) {
  Frame* frame = ctx->frame;
  TryBlock block = frame->blockStack()->pop();
  ctx->pc = block.handler();
}

void GET_ITER(Context* ctx, word) {
  Thread* thread = ctx->thread;
  HandleScope scope(thread->handles());
  Handle<Object> iterable(&scope, *ctx->sp);
  *ctx->sp = thread->runtime()->getIter(iterable);
}

void FOR_ITER(Context* ctx, word arg) {
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

void BINARY_AND(Context* ctx, word) {
  word right = SmallInteger::cast(*ctx->sp++)->value();
  word left = SmallInteger::cast(*ctx->sp)->value();
  *ctx->sp = SmallInteger::fromWord(left & right);
}

void BINARY_ADD(Context* ctx, word) {
  word right = SmallInteger::cast(*ctx->sp++)->value();
  word left = SmallInteger::cast(*ctx->sp)->value();
  *ctx->sp = SmallInteger::fromWord(left + right);
}

void BINARY_FLOOR_DIVIDE(Context* ctx, word) {
  word divisor = SmallInteger::cast(*ctx->sp++)->value();
  word dividend = SmallInteger::cast(*ctx->sp)->value();
  // TODO: Throw:
  //   ZeroDivisionError: integer division or modulo by zero
  if (divisor == 0) {
    UNIMPLEMENTED("ZeroDivisionError");
  }
  *ctx->sp = SmallInteger::fromWord(dividend / divisor);
}

void BINARY_MODULO(Context* ctx, word) {
  word divisor = SmallInteger::cast(*ctx->sp++)->value();
  word dividend = SmallInteger::cast(*ctx->sp)->value();
  // TODO: Throw:
  //   ZeroDivisionError: integer division or modulo by zero
  if (divisor == 0) {
    UNIMPLEMENTED("ZeroDivisionError");
  }
  *ctx->sp = SmallInteger::fromWord(dividend % divisor);
}

void BINARY_SUBTRACT(Context* ctx, word) {
  word right = SmallInteger::cast(*ctx->sp++)->value();
  word left = SmallInteger::cast(*ctx->sp)->value();
  word result = left - right;
  *ctx->sp = SmallInteger::fromWord(result);
}

void BINARY_MULTIPLY(Context* ctx, word) {
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

void BINARY_XOR(Context* ctx, word) {
  word right = SmallInteger::cast(*ctx->sp++)->value();
  word left = SmallInteger::cast(*ctx->sp)->value();
  *ctx->sp = SmallInteger::fromWord(left ^ right);
}

void BINARY_SUBSCR(Context* ctx, word) {
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

void STORE_SUBSCR(Context* ctx, word) {
  // TODO: The implementation of the {BINARY,STORE}_SUBSCR opcodes are
  // enough to get richards working.
  word idx = SmallInteger::cast(*ctx->sp++)->value();
  auto list = List::cast(*ctx->sp++);
  list->atPut(idx, *ctx->sp++);
}

void LOAD_ATTR(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<Object> receiver(&scope, *ctx->sp);
  auto* names = Code::cast(ctx->frame->code())->names();
  Handle<Object> name(&scope, ObjectArray::cast(names)->at(arg));
  *ctx->sp = thread->runtime()->attributeAt(ctx->thread, receiver, name);
  thread->abortOnPendingException();
}

void STORE_ATTR(Context* ctx, word arg) {
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

void COMPARE_OP(Context* ctx, word arg) {
  Object**& sp = ctx->sp;
  HandleScope scope;
  Handle<Object> right(&scope, *sp++);
  Handle<Object> left(&scope, *sp++);
  Object* res = Interpreter::compare(static_cast<CompareOp>(arg), left, right);
  DCHECK(res->isBoolean(), "unexpected comparison result");
  *--sp = res;
}

void IMPORT_NAME(Context* ctx, word arg) {
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

void BUILD_CONST_KEY_MAP(Context* ctx, word arg) {
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

void STORE_DEREF(Context* ctx, word arg) {
  Code* code = Code::cast(ctx->frame->code());
  ValueCell::cast(ctx->frame->getLocal(code->nlocals() + arg))
      ->setValue(*ctx->sp++);
}

void LOAD_DEREF(Context* ctx, word arg) {
  Code* code = Code::cast(ctx->frame->code());
  *--ctx->sp =
      ValueCell::cast(ctx->frame->getLocal(code->nlocals() + arg))->value();
}

void LOAD_CLOSURE(Context* ctx, word arg) {
  Code* code = Code::cast(ctx->frame->code());
  *--ctx->sp = ctx->frame->getLocal(code->nlocals() + arg);
}

void UNPACK_SEQUENCE(Context* ctx, word arg) {
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

void BINARY_TRUE_DIVIDE(Context* ctx, word) {
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

void BUILD_STRING(Context* ctx, word arg) {
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

// A incomplete impl of FORMAT_VALUE; assumes no conv
void FORMAT_VALUE(Context* ctx, word flags) {
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

using Op = void (*)(Context*, word);
const Op opTable[] = {
#define HANDLER(name, value, handler) handler,
    FOREACH_BYTECODE(HANDLER)
#undef HANDLER
};
} // namespace interpreter

using namespace interpreter;
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

Object* Interpreter::compare(
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
      return richCompare(op, left, right);
  }
  return Boolean::fromBool(res);
}

Object* Interpreter::richCompare(
    CompareOp op,
    const Handle<Object>& left,
    const Handle<Object>& right) {
  bool res = false;
  // TODO: call rich comparison method
  if (left->isSmallInteger() && right->isSmallInteger()) {
    word sign = SmallInteger::cast(*left)->value() -
        SmallInteger::cast(*right)->value();
    switch (op) {
      case LT: {
        res = (sign < 0);
        break;
      }
      case LE: {
        res = (sign <= 0);
        break;
      }
      case EQ: {
        res = (sign == 0);
        break;
      }
      case NE: {
        res = (sign != 0);
        break;
      }
      case GT: {
        res = (sign > 0);
        break;
      }
      case GE: {
        res = (sign >= 0);
        break;
      }
      default:
        UNREACHABLE("rich comparison with op %x", op);
    }
  } else if (left->isString() && right->isString()) {
    word cmp = String::cast(*left)->compare(*right);
    switch (op) {
      case EQ:
        res = (cmp == 0);
        break;
      case NE:
        res = (cmp != 0);
        break;
      case LE:
        res = (cmp <= 0);
        break;
      case LT:
        res = (cmp < 0);
        break;
      case GE:
        res = (cmp >= 0);
        break;
      case GT:
        res = (cmp > 0);
        break;
      default:
        UNIMPLEMENTED("string comparison with op %x", op);
        break;
    }
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
        Object* cmp = richCompare(op, next_left, next_right);
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

} // namespace python
