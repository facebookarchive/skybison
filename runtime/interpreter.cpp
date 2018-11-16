#include "interpreter.h"

#include <cassert>
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
  switch (callable->classId()) {
    case ClassId::kFunction: {
      frame->setValueStackTop(sp);
      return Function::cast(callable)->entry()(thread, frame, nargs);
    }
    case ClassId::kBoundMethod: {
      return callBoundMethod(thread, frame, sp, nargs);
    }
    case ClassId::kType: {
      return callType(thread, frame, sp, nargs);
    }
    default: {
      // TODO(T25382628): Handle __call__
      std::abort();
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
  Handle<Object> result(&scope, runtime->newInstance(klass->id()));

  // Check for an __init__ method.
  //
  // If no __init__ method exists for this class we can just return the newly
  // allocated instance.
  Handle<Object> dunder_init(&scope, runtime->symbols()->DunderInit());
  Handle<Object> init(
      &scope, runtime->lookupNameInMro(thread, klass, dunder_init));
  if (init->isError()) {
    return *result;
  }

  // An __init__ method exists and now we will call it.
  //
  // Rewrite the type object call to an __init__ method call as follows
  //   From: type, arg0, arg1, ... argN
  //   To: function, instance, arg0, arg1, .. argN
  // Increase the stack by one, copy all of the arguments up, and replace the
  // type object with the function and the instance above it.
  sp -= 1;
  for (word i = 0; i < nargs; i++) {
    sp[i] = sp[i + 1];
  }

  // Move the method and receiver into the expected location.
  sp[nargs] = *result;
  sp[nargs + 1] = *init;

  // Call the initializer method.
  frame->setValueStackTop(sp);
  Function::cast(*init)->entry()(thread, frame, nargs + 1);

  // Return the initialized instance.
  return *result;
}

namespace interpreter {
enum class Result {
  CONTINUE,
  RETURN,
  NOT_IMPLEMENTED,
};
struct Context {
  Thread* thread;
  Frame* frame;
  Object** sp;
  word pc;
};
Result NOT_IMPLEMENTED(Context*, word) {
  return Result::NOT_IMPLEMENTED;
}
Result LOAD_CONST(Context* ctx, word arg) {
  Object* consts = Code::cast(ctx->frame->code())->consts();
  *--ctx->sp = ObjectArray::cast(consts)->at(arg);
  return Result::CONTINUE;
}
Result LOAD_FAST(Context* ctx, word arg) {
  // TODO: Need to handle unbound local error
  *--ctx->sp = ctx->frame->getLocal(arg);
  return Result::CONTINUE;
}
Result STORE_FAST(Context* ctx, word arg) {
  Object* value = *ctx->sp++;
  ctx->frame->setLocal(arg, value);
  return Result::CONTINUE;
}
Result LOAD_NAME(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  // This is for module level lookup, behaves the same as LOAD_GLOBAL
  Object* value =
      ValueCell::cast(ObjectArray::cast(frame->fastGlobals())->at(arg))
          ->value();

  if (value->isValueCell()) {
    CHECK(!ValueCell::cast(value)->isUnbound(), "unbound implicit globals\n");

    value = ValueCell::cast(value)->value();
  }

  *--ctx->sp = value;
  return Result::CONTINUE;
}
Result STORE_NAME(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  assert(frame->implicitGlobals()->isDictionary());
  HandleScope scope;
  Handle<Dictionary> implicit_globals(&scope, frame->implicitGlobals());
  Object* names = Code::cast(frame->code())->names();
  Handle<Object> key(&scope, ObjectArray::cast(names)->at(arg));
  Handle<Object> value(&scope, *ctx->sp++);
  thread->runtime()->dictionaryAtPutInValueCell(implicit_globals, key, value);
  return Result::CONTINUE;
}
Result POP_TOP(Context* ctx, word) {
  ctx->sp++;
  return Result::CONTINUE;
}
Result DUP_TOP(Context* ctx, word) {
  Object* top = *ctx->sp;
  *--ctx->sp = top;
  return Result::CONTINUE;
}
Result ROT_TWO(Context* ctx, word) {
  Object* top = *ctx->sp;
  *ctx->sp = *(ctx->sp + 1);
  *(ctx->sp + 1) = top;
  return Result::CONTINUE;
}
Result CALL_FUNCTION(Context* ctx, word arg) {
  Object* result = Interpreter::call(ctx->thread, ctx->frame, ctx->sp, arg);
  ctx->sp += arg;
  *ctx->sp = result;
  return Result::CONTINUE;
}
Result CALL_FUNCTION_KW(Context* ctx, word arg) {
  Object* result = Interpreter::callKw(ctx->thread, ctx->frame, ctx->sp, arg);
  ctx->sp += arg + 1;
  *ctx->sp = result;
  return Result::CONTINUE;
}
Result LOAD_GLOBAL(Context* ctx, word arg) {
  Object* value =
      ValueCell::cast(ObjectArray::cast(ctx->frame->fastGlobals())->at(arg))
          ->value();
  if (value->isValueCell()) {
    CHECK(!ValueCell::cast(value)->isUnbound(), "Unbound Globals");
    value = ValueCell::cast(value)->value();
  }
  *--ctx->sp = value;
  assert(*ctx->sp != Error::object());
  return Result::CONTINUE;
}
Result STORE_GLOBAL(Context* ctx, word arg) {
  ValueCell::cast(ObjectArray::cast(ctx->frame->fastGlobals())->at(arg))
      ->setValue(*ctx->sp++);
  return Result::CONTINUE;
}
Result DELETE_GLOBAL(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<ValueCell> value_cell(
      &scope,
      ValueCell::cast(ObjectArray::cast(frame->fastGlobals())->at(arg)));
  CHECK(!value_cell->value()->isValueCell(), "Unbound Globals");
  Object* value_in_builtin = Error::object();
  Handle<Object> key(
      &scope, ObjectArray::cast(Code::cast(frame->code())->names())->at(arg));
  Handle<Dictionary> builtins(&scope, frame->builtins());
  if (!thread->runtime()->dictionaryAt(builtins, key, &value_in_builtin)) {
    Handle<Object> handle(&scope, value_in_builtin);
    value_in_builtin =
        thread->runtime()->dictionaryAtPutInValueCell(builtins, key, handle);
    ValueCell::cast(value_in_builtin)->makeUnbound();
  }
  value_cell->setValue(value_in_builtin);
  return Result::CONTINUE;
}
Result MAKE_FUNCTION(Context* ctx, word) {
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
  *--ctx->sp = *function;
  return Result::CONTINUE;
}
Result BUILD_LIST(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<ObjectArray> array(&scope, thread->runtime()->newObjectArray(arg));
  for (int i = arg - 1; i >= 0; i--) {
    array->atPut(i, *ctx->sp++);
  }
  List* list = List::cast(thread->runtime()->newList());
  list->setItems(*array);
  list->setAllocated(array->length());
  *--ctx->sp = list;
  return Result::CONTINUE;
}
Result POP_JUMP_IF_FALSE(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  Object* cond = *ctx->sp++;
  if (!thread->runtime()->isTruthy(cond)) {
    ctx->pc = arg;
  }
  return Result::CONTINUE;
}
Result POP_JUMP_IF_TRUE(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  Object* cond = *ctx->sp++;
  if (thread->runtime()->isTruthy(cond)) {
    ctx->pc = arg;
  }
  return Result::CONTINUE;
}
Result JUMP_IF_FALSE_OR_POP(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  Object* cond = *ctx->sp;
  if (!thread->runtime()->isTruthy(cond)) {
    ctx->pc = arg;
  } else {
    ctx->sp++;
  }
  return Result::CONTINUE;
}
Result JUMP_IF_TRUE_OR_POP(Context* ctx, word arg) {
  Object* cond = *ctx->sp;
  Thread* thread = ctx->thread;
  if (thread->runtime()->isTruthy(cond)) {
    ctx->pc = arg;
  } else {
    ctx->sp++;
  }
  return Result::CONTINUE;
}
Result UNARY_NOT(Context* ctx, word) {
  Thread* thread = ctx->thread;
  bool negated = !thread->runtime()->isTruthy(*ctx->sp);
  *ctx->sp = Boolean::fromBool(negated);
  return Result::CONTINUE;
}
Result LOAD_BUILD_CLASS(Context* ctx, word) {
  Thread* thread = ctx->thread;
  ValueCell* value_cell = ValueCell::cast(thread->runtime()->buildClass());
  *--ctx->sp = value_cell->value();
  return Result::CONTINUE;
}
Result JUMP_ABSOLUTE(Context* ctx, word arg) {
  ctx->pc = arg;
  return Result::CONTINUE;
}
Result JUMP_FORWARD(Context* ctx, word arg) {
  ctx->pc += arg;
  return Result::CONTINUE;
}
Result SETUP_LOOP(Context* ctx, word arg) {
  Frame* frame = ctx->frame;
  word stackDepth = frame->valueStackBase() - ctx->sp;
  BlockStack* blockStack = frame->blockStack();
  blockStack->push(TryBlock(Bytecode::SETUP_LOOP, ctx->pc + arg, stackDepth));
  return Result::CONTINUE;
}
Result POP_BLOCK(Context* ctx, word) {
  Frame* frame = ctx->frame;
  TryBlock block = frame->blockStack()->pop();
  ctx->sp = frame->valueStackBase() - block.level();
  return Result::CONTINUE;
}
Result GET_ITER(Context* ctx, word) {
  Thread* thread = ctx->thread;
  *ctx->sp = thread->runtime()->getIter(*ctx->sp);
  // This is currently the only implemented iterator type.
  assert((*ctx->sp)->isRangeIterator());
  return Result::CONTINUE;
}
Result FOR_ITER(Context* ctx, word arg) {
  auto iter = RangeIterator::cast(*ctx->sp);
  Object* next = iter->next();
  // TODO: Support StopIteration exceptions.
  if (next->isError()) {
    ctx->sp++;
    ctx->pc += arg;
  } else {
    *--ctx->sp = next;
  }
  return Result::CONTINUE;
}

Result BINARY_AND(Context* ctx, word) {
  word right = SmallInteger::cast(*ctx->sp++)->value();
  word left = SmallInteger::cast(*ctx->sp)->value();
  *ctx->sp = SmallInteger::fromWord(left & right);
  return Result::CONTINUE;
}
Result BINARY_ADD(Context* ctx, word) {
  word right = SmallInteger::cast(*ctx->sp++)->value();
  word left = SmallInteger::cast(*ctx->sp)->value();
  *ctx->sp = SmallInteger::fromWord(left + right);
  return Result::CONTINUE;
}
Result BINARY_FLOOR_DIVIDE(Context* ctx, word) {
  word divisor = SmallInteger::cast(*ctx->sp++)->value();
  word dividend = SmallInteger::cast(*ctx->sp)->value();
  // TODO: Throw:
  //   ZeroDivisionError: integer division or modulo by zero
  assert(divisor != 0);
  *ctx->sp = SmallInteger::fromWord(dividend / divisor);
  return Result::CONTINUE;
}
Result BINARY_MODULO(Context* ctx, word) {
  word divisor = SmallInteger::cast(*ctx->sp++)->value();
  word dividend = SmallInteger::cast(*ctx->sp)->value();
  // TODO: Throw:
  //   ZeroDivisionError: integer division or modulo by zero
  assert(divisor != 0);
  *ctx->sp = SmallInteger::fromWord(dividend % divisor);
  return Result::CONTINUE;
}
Result BINARY_SUBTRACT(Context* ctx, word) {
  word right = SmallInteger::cast(*ctx->sp++)->value();
  word left = SmallInteger::cast(*ctx->sp)->value();
  word result = left - right;
  *ctx->sp = SmallInteger::fromWord(result);
  return Result::CONTINUE;
}
Result INPLACE_MULTIPLY(Context* ctx, word) {
  word right = SmallInteger::cast(*ctx->sp++)->value();
  word left = SmallInteger::cast(*ctx->sp)->value();
  word result = left * right;
  assert(left == 0 || (result / left) == right);
  *ctx->sp = SmallInteger::fromWord(result);
  return Result::CONTINUE;
}
Result BINARY_MULTIPLY(Context* ctx, word) {
  Object**& sp = ctx->sp;
  if (sp[1]->isSmallInteger()) {
    word right = SmallInteger::cast(*sp++)->value();
    word left = SmallInteger::cast(*sp)->value();
    word result = left * right;
    assert(left == 0 || (result / left) == right);
    *sp = SmallInteger::fromWord(result);
  } else {
    word ntimes = SmallInteger::cast(*sp++)->value();
    Thread* thread = ctx->thread;
    HandleScope scope(thread->handles());
    Handle<List> list(&scope, *sp);
    *sp = thread->runtime()->listReplicate(thread, list, ntimes);
  }
  return Result::CONTINUE;
}
Result BINARY_XOR(Context* ctx, word) {
  word right = SmallInteger::cast(*ctx->sp++)->value();
  word left = SmallInteger::cast(*ctx->sp)->value();
  *ctx->sp = SmallInteger::fromWord(left ^ right);
  return Result::CONTINUE;
}
Result BINARY_SUBSCR(Context* ctx, word) {
  // TODO: The implementation of the {BINARY,STORE}_SUBSCR opcodes are
  // enough to get richards working.
  word idx = SmallInteger::cast(*ctx->sp++)->value();
  auto list = List::cast(*ctx->sp);
  *ctx->sp = list->at(idx);
  return Result::CONTINUE;
}
Result STORE_SUBSCR(Context* ctx, word) {
  // TODO: The implementation of the {BINARY,STORE}_SUBSCR opcodes are
  // enough to get richards working.
  word idx = SmallInteger::cast(*ctx->sp++)->value();
  auto list = List::cast(*ctx->sp++);
  list->atPut(idx, *ctx->sp++);
  return Result::CONTINUE;
}
Result LOAD_ATTR(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<Object> receiver(&scope, *ctx->sp);
  auto* names = Code::cast(ctx->frame->code())->names();
  Handle<Object> name(&scope, ObjectArray::cast(names)->at(arg));
  *ctx->sp = thread->runtime()->attributeAt(ctx->thread, receiver, name);
  thread->abortOnPendingException();
  return Result::CONTINUE;
}
Result STORE_ATTR(Context* ctx, word arg) {
  Thread* thread = ctx->thread;
  HandleScope scope;
  Handle<Object> receiver(&scope, *ctx->sp);
  auto* names = Code::cast(ctx->frame->code())->names();
  Handle<Object> name(&scope, ObjectArray::cast(names)->at(arg));
  Handle<Object> value(&scope, *(ctx->sp + 1));
  ctx->sp += 2;
  thread->runtime()->attributeAtPut(thread, receiver, name, value);
  thread->abortOnPendingException();
  return Result::CONTINUE;
}
Result COMPARE_OP(Context* ctx, word arg) {
  Object**& sp = ctx->sp;
  HandleScope scope;
  Handle<Object> right(&scope, *sp++);
  Handle<Object> left(&scope, *sp++);
  Object* res = Interpreter::compare(static_cast<CompareOp>(arg), left, right);
  assert(res->isBoolean());
  *--sp = res;
  return Result::CONTINUE;
}
Result IMPORT_NAME(Context* ctx, word arg) {
  HandleScope scope;
  Handle<Code> code(&scope, ctx->frame->code());
  Handle<Object> name(&scope, ObjectArray::cast(code->names())->at(arg));
  Handle<Object> fromlist(&scope, *ctx->sp++);
  Handle<Object> level(&scope, *ctx->sp);
  Thread* thread = ctx->thread;
  Runtime* runtime = thread->runtime();
  *ctx->sp = runtime->importModule(name);
  thread->abortOnPendingException();
  return Result::CONTINUE;
}

using Op = Result (*)(Context*, word);
Op opTable[256];
} // namespace interpreter
using namespace interpreter;
void Interpreter::initOpTable() {
  for (word i = 0; i < 256; i++) {
    opTable[i] = interpreter::NOT_IMPLEMENTED;
  }
  opTable[Bytecode::LOAD_CONST] = interpreter::LOAD_CONST;
  opTable[Bytecode::LOAD_FAST] = interpreter::LOAD_FAST;
  opTable[Bytecode::STORE_FAST] = interpreter::STORE_FAST;
  opTable[Bytecode::LOAD_NAME] = interpreter::LOAD_NAME;
  opTable[Bytecode::STORE_NAME] = interpreter::STORE_NAME;
  opTable[Bytecode::POP_TOP] = interpreter::POP_TOP;
  opTable[Bytecode::DUP_TOP] = interpreter::DUP_TOP;
  opTable[Bytecode::ROT_TWO] = interpreter::ROT_TWO;
  opTable[Bytecode::CALL_FUNCTION] = interpreter::CALL_FUNCTION;
  opTable[Bytecode::CALL_FUNCTION_KW] = interpreter::CALL_FUNCTION_KW;
  opTable[Bytecode::LOAD_GLOBAL] = interpreter::LOAD_GLOBAL;
  opTable[Bytecode::STORE_GLOBAL] = interpreter::STORE_GLOBAL;
  opTable[Bytecode::MAKE_FUNCTION] = interpreter::MAKE_FUNCTION;
  opTable[Bytecode::BUILD_LIST] = interpreter::BUILD_LIST;
  opTable[Bytecode::POP_JUMP_IF_FALSE] = interpreter::POP_JUMP_IF_FALSE;
  opTable[Bytecode::POP_JUMP_IF_TRUE] = interpreter::POP_JUMP_IF_TRUE;
  opTable[Bytecode::JUMP_IF_TRUE_OR_POP] = interpreter::JUMP_IF_TRUE_OR_POP;
  opTable[Bytecode::JUMP_IF_FALSE_OR_POP] = interpreter::JUMP_IF_FALSE_OR_POP;
  opTable[Bytecode::LOAD_BUILD_CLASS] = interpreter::LOAD_BUILD_CLASS;
  opTable[Bytecode::UNARY_NOT] = interpreter::UNARY_NOT;
  opTable[Bytecode::POP_JUMP_IF_TRUE] = interpreter::POP_JUMP_IF_TRUE;
  opTable[Bytecode::JUMP_ABSOLUTE] = interpreter::JUMP_ABSOLUTE;
  opTable[Bytecode::JUMP_FORWARD] = interpreter::JUMP_FORWARD;
  opTable[Bytecode::SETUP_LOOP] = interpreter::SETUP_LOOP;
  opTable[Bytecode::POP_BLOCK] = interpreter::POP_BLOCK;
  opTable[Bytecode::FOR_ITER] = interpreter::FOR_ITER;
  opTable[Bytecode::GET_ITER] = interpreter::GET_ITER;
  opTable[Bytecode::BINARY_AND] = interpreter::BINARY_AND;
  opTable[Bytecode::INPLACE_AND] = interpreter::BINARY_AND;
  opTable[Bytecode::BINARY_ADD] = interpreter::BINARY_ADD;
  opTable[Bytecode::INPLACE_ADD] = interpreter::BINARY_ADD;
  opTable[Bytecode::BINARY_MODULO] = interpreter::BINARY_MODULO;
  opTable[Bytecode::INPLACE_MODULO] = interpreter::BINARY_MODULO;
  opTable[Bytecode::BINARY_SUBTRACT] = interpreter::BINARY_SUBTRACT;
  opTable[Bytecode::INPLACE_SUBTRACT] = interpreter::BINARY_SUBTRACT;
  opTable[Bytecode::BINARY_MULTIPLY] = interpreter::BINARY_MULTIPLY;
  opTable[Bytecode::INPLACE_MULTIPLY] = interpreter::BINARY_MULTIPLY;
  opTable[Bytecode::BINARY_XOR] = interpreter::BINARY_XOR;
  opTable[Bytecode::INPLACE_XOR] = interpreter::BINARY_XOR;
  opTable[Bytecode::BINARY_FLOOR_DIVIDE] = interpreter::BINARY_FLOOR_DIVIDE;
  opTable[Bytecode::INPLACE_FLOOR_DIVIDE] = interpreter::BINARY_FLOOR_DIVIDE;
  opTable[Bytecode::BINARY_SUBSCR] = interpreter::BINARY_SUBSCR;
  opTable[Bytecode::STORE_SUBSCR] = interpreter::STORE_SUBSCR;
  opTable[Bytecode::LOAD_ATTR] = interpreter::LOAD_ATTR;
  opTable[Bytecode::STORE_ATTR] = interpreter::STORE_ATTR;
  opTable[Bytecode::COMPARE_OP] = interpreter::COMPARE_OP;
  opTable[Bytecode::IMPORT_NAME] = interpreter::IMPORT_NAME;
  opTable[Bytecode::DELETE_GLOBAL] = interpreter::DELETE_GLOBAL;
}

Object* Interpreter::execute(Thread* thread, Frame* frame) {
  HandleScope scope(thread->handles());
  Code* code = Code::cast(frame->code());
  Handle<ByteArray> byteArray(&scope, code->code());
  Context ctx;
  ctx.pc = 0;
  ctx.sp = frame->valueStackTop();
  ctx.thread = thread;
  ctx.frame = frame;
  for (;;) {
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
        thread->popFrame(frame);
        return result;
      }
      default: {
        Result r = opTable[bc](&ctx, arg);
        if (r != Result::CONTINUE) {
          assert(r == Result::NOT_IMPLEMENTED);
          // TODO: Distinguish between intentionally unimplemented bytecode
          // and unreachable code.
          fprintf(stderr, "aborting due to unimplemented bytecode: %d\n", bc);
          abort();
        }
      }
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
    case IN: {
      // TODO: fill abstract sequence contains
      abort();
      break;
    }
    case NOT_IN: {
      // TODO: fill abstract sequence contains
      abort();
      break;
    }
    case EXC_MATCH: {
      // TODO: fill execption compare
      abort();
      break;
    }
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
        // PyErr_BadArgument - other cases should not go in here
        abort();
    }
  }
  return Boolean::fromBool(res);
}

} // namespace python
