#include "interpreter.h"

#include <cassert>
#include <cstdlib>

#include "bytecode.h"
#include "frame.h"
#include "handles.h"
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
      // TODO(T25382534): Handle classes
      std::abort();
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

Object* Interpreter::execute(Thread* thread, Frame* frame) {
  Code* code = Code::cast(frame->code());
  BlockStack* blockStack = frame->blockStack();
  ByteArray* byteArray = ByteArray::cast(code->code());
  Object** sp = frame->valueStackTop();
  word pc = 0;
  for (;;) {
    Bytecode bc = static_cast<Bytecode>(byteArray->byteAt(pc++));
    int32 arg = byteArray->byteAt(pc++);
  dispatch:
    switch (bc) {
      case Bytecode::EXTENDED_ARG: {
        bc = static_cast<Bytecode>(byteArray->byteAt(pc++));
        arg = (arg << 8) | byteArray->byteAt(pc++);
        goto dispatch;
      }
      case Bytecode::RETURN_VALUE: {
        Object* result = *sp++;
        // Clean up after ourselves
        thread->popFrame(frame);
        return result;
      }
      case Bytecode::LOAD_CONST: {
        Object* consts = Code::cast(frame->code())->consts();
        *--sp = ObjectArray::cast(consts)->at(arg);
        break;
      }
      case Bytecode::LOAD_FAST: {
        // TODO: Need to handle unbound local error
        *--sp = frame->getLocal(arg);
        break;
      }
      case Bytecode::STORE_FAST: {
        Object* value = *sp--;
        frame->setLocal(arg, value);
        break;
      }
      case Bytecode::LOAD_NAME: {
        HandleScope scope;
        Handle<Dictionary> implicit_globals(&scope, frame->implicitGlobals());
        Handle<Object> key(
            &scope,
            ObjectArray::cast(Code::cast(frame->code())->names())->at(arg));
        Object* value = None::object();
        thread->runtime()->dictionaryAt(implicit_globals, key, &value);
        // TODO(cshapiro): check for unbound implicit global variables.
        assert(value != None::object());
        *--sp = ValueCell::cast(value)->value();
        break;
      }
      case Bytecode::STORE_NAME: {
        assert(frame->implicitGlobals()->isDictionary());
        HandleScope scope;
        Handle<Dictionary> implicit_globals(&scope, frame->implicitGlobals());
        Object* names = Code::cast(frame->code())->names();
        Handle<Object> key(&scope, ObjectArray::cast(names)->at(arg));
        Handle<Object> value(&scope, *sp++);
        thread->runtime()->dictionaryAtPutInValueCell(
            implicit_globals, key, value);
        break;
      }
      case Bytecode::POP_TOP: {
        sp++;
        break;
      }
      case Bytecode::DUP_TOP: {
        Object* top = *sp;
        *--sp = top;
        break;
      }
      case Bytecode::ROT_TWO: {
        Object* top = *sp;
        *sp = *(sp + 1);
        *(sp + 1) = top;
        break;
      }
      case Bytecode::CALL_FUNCTION: {
        Object* result = call(thread, frame, sp, arg);
        // Pop arguments + called function and push return value
        sp += arg;
        *sp = result;
        break;
      }
      case Bytecode::CALL_FUNCTION_KW: {
        Object* result = callKw(thread, frame, sp, arg);
        // Pop kw-arg tuple + arguments + called function and push return value
        sp += arg + 1;
        *sp = result;
        break;
      }
      case Bytecode::JUMP_ABSOLUTE: {
        assert(arg < byteArray->length());
        pc = arg;
        break;
      }
      case Bytecode::JUMP_FORWARD: {
        assert(pc + arg < byteArray->length());
        pc += arg;
        break;
      }
      case Bytecode::LOAD_GLOBAL: {
        HandleScope scope;
        Handle<Dictionary> globals(&scope, frame->globals());
        Handle<Object> key(
            &scope,
            ObjectArray::cast(Code::cast(frame->code())->names())->at(arg));
        Object* value = None::object();
        thread->runtime()->dictionaryAt(globals, key, &value);
        // TODO(cshapiro): check for unbound global variables.
        assert(value != None::object());
        *--sp = ValueCell::cast(value)->value();
        break;
      }
      case Bytecode::STORE_GLOBAL: {
        HandleScope scope;
        Handle<Dictionary> globals(&scope, frame->globals());
        Object* names = Code::cast(frame->code())->names();
        Handle<Object> key(&scope, ObjectArray::cast(names)->at(arg));
        Handle<Object> value(&scope, *sp++);
        Runtime* runtime = thread->runtime();
        runtime->dictionaryAtPutInValueCell(globals, key, value);
        break;
      }
      case Bytecode::MAKE_FUNCTION: {
        assert(arg == 0);
        Function* function = Function::cast(thread->runtime()->newFunction());
        function->setName(*sp++);
        function->setCode(*sp++);
        function->setGlobals(frame->globals());
        function->setEntry(interpreterTrampoline);
        *--sp = function;
        break;
      }
      case Bytecode::BUILD_LIST: {
        HandleScope scope;
        Handle<ObjectArray> array(
            &scope, thread->runtime()->newObjectArray(arg));
        for (int i = arg - 1; i >= 0; i--) {
          array->atPut(i, *sp++);
        }
        List* list = List::cast(thread->runtime()->newList());
        list->setItems(*array);
        list->setAllocated(array->length());
        *--sp = list;
        break;
      }
      case Bytecode::POP_JUMP_IF_FALSE: {
        assert(arg < byteArray->length());
        Object* cond = *sp++;
        if (!thread->runtime()->isTruthy(cond)) {
          pc = arg;
        }
        break;
      }
      case Bytecode::POP_JUMP_IF_TRUE: {
        assert(arg < byteArray->length());
        Object* cond = *sp++;
        if (thread->runtime()->isTruthy(cond)) {
          pc = arg;
        }
        break;
      }
      case Bytecode::JUMP_IF_FALSE_OR_POP: {
        assert(arg < byteArray->length());
        if (!thread->runtime()->isTruthy(*sp)) {
          pc = arg;
        } else {
          sp++;
        }
        break;
      }
      case Bytecode::JUMP_IF_TRUE_OR_POP: {
        assert(arg < byteArray->length());
        if (thread->runtime()->isTruthy(*sp)) {
          pc = arg;
        } else {
          sp++;
        }
        break;
      }
      case Bytecode::UNARY_NOT: {
        bool negated = !thread->runtime()->isTruthy(*sp);
        *sp = Boolean::fromBool(negated);
        break;
      }
      case Bytecode::LOAD_BUILD_CLASS: {
        assert(arg == 0);
        ValueCell* value_cell =
            ValueCell::cast(thread->runtime()->buildClass());
        *--sp = value_cell->value();
        break;
      }
      case Bytecode::SETUP_LOOP: {
        word stackDepth = frame->valueStackBase() - sp;
        blockStack->push(TryBlock(bc, pc + arg, stackDepth));
        break;
      }
      case Bytecode::POP_BLOCK: {
        TryBlock block = frame->blockStack()->pop();
        sp = frame->valueStackBase() - block.level();
        break;
      }

      case Bytecode::GET_ITER: {
        *sp = thread->runtime()->getIter(*sp);
        // This is currently the only implemented iterator type.
        assert((*sp)->isRangeIterator());
        break;
      }
      case Bytecode::FOR_ITER: {
        auto iter = RangeIterator::cast(*sp);
        Object* next = iter->next();
        // TODO: Support StopIteration exceptions.
        if (next->isError()) {
          sp++;
          pc += arg;
        } else {
          *--sp = next;
        }
        break;
      }

      // TODO: arithmetic operations are currently implemented only for
      // SmallIntegers, for the purposes of running Richards.
      //
      // Note that the INPLACE opcodes are only relevant for method dispatch.
      // Python compiles `a *= b` to `a = a * b`, but it uses different opcodes
      // so that the user-defined __iadd__, __imul__, etc can be called instead.
      case Bytecode::INPLACE_AND:
      case Bytecode::BINARY_AND: {
        word right = SmallInteger::cast(*sp++)->value();
        word left = SmallInteger::cast(*sp)->value();
        *sp = SmallInteger::fromWord(left & right);
        break;
      }

      case Bytecode::INPLACE_ADD:
      case Bytecode::BINARY_ADD: {
        word right = SmallInteger::cast(*sp++)->value();
        word left = SmallInteger::cast(*sp)->value();
        *sp = SmallInteger::fromWord(left + right);
        break;
      }

      case Bytecode::INPLACE_FLOOR_DIVIDE:
      case Bytecode::BINARY_FLOOR_DIVIDE: {
        word divisor = SmallInteger::cast(*sp++)->value();
        word dividend = SmallInteger::cast(*sp)->value();
        // TODO: Throw:
        //   ZeroDivisionError: integer division or modulo by zero
        assert(divisor != 0);
        *sp = SmallInteger::fromWord(dividend / divisor);
        break;
      }

      case Bytecode::INPLACE_MODULO:
      case Bytecode::BINARY_MODULO: {
        word divisor = SmallInteger::cast(*sp++)->value();
        word dividend = SmallInteger::cast(*sp)->value();
        // TODO: Throw:
        //   ZeroDivisionError: integer division or modulo by zero
        assert(divisor != 0);
        *sp = SmallInteger::fromWord(dividend % divisor);
        break;
      }

      case Bytecode::INPLACE_MULTIPLY:
      case Bytecode::BINARY_MULTIPLY: {
        word right = SmallInteger::cast(*sp++)->value();
        word left = SmallInteger::cast(*sp)->value();
        word result = left * right;
        assert(left == 0 || (result / left) == right);
        *sp = SmallInteger::fromWord(result);
        break;
      }

      case Bytecode::INPLACE_SUBTRACT:
      case Bytecode::BINARY_SUBTRACT: {
        word right = SmallInteger::cast(*sp++)->value();
        word left = SmallInteger::cast(*sp)->value();
        *sp = SmallInteger::fromWord(left - right);
        break;
      }

      case Bytecode::INPLACE_XOR:
      case Bytecode::BINARY_XOR: {
        word right = SmallInteger::cast(*sp++)->value();
        word left = SmallInteger::cast(*sp)->value();
        *sp = SmallInteger::fromWord(left ^ right);
        break;
      }

      // TODO: The implementation of the {BINARY,STORE}_SUBSCR opcodes are
      // enough to get richards working.
      case Bytecode::BINARY_SUBSCR: {
        word idx = SmallInteger::cast(*sp++)->value();
        auto list = List::cast(*sp);
        *sp = list->at(idx);
        break;
      }

      case Bytecode::STORE_SUBSCR: {
        word idx = SmallInteger::cast(*sp++)->value();
        auto list = List::cast(*sp++);
        list->atPut(idx, *sp++);
        break;
      }

      default:
        abort();
    }
  }
}

} // namespace python
