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

Object* Interpreter::execute(Thread* thread, Frame* frame) {
  Code* code = Code::cast(frame->code());
  BlockStack* blockStack = frame->blockStack();
  ByteArray* byteArray = ByteArray::cast(code->code());
  Object** sp = frame->valueStackTop();
  Object** locals = frame->locals() + code->nlocals();
  word pc = 0;
  for (;;) {
    Bytecode bc = static_cast<Bytecode>(byteArray->byteAt(pc++));
    byte arg = byteArray->byteAt(pc++);
    switch (bc) {
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
        *--sp = *(locals - arg);
        break;
      }
      case Bytecode::STORE_FAST: {
        Object* value = *sp--;
        *(locals - arg) = value;
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
        Runtime* runtime = thread->runtime();
        Handle<ValueCell> value_cell(
            &scope,
            runtime->dictionaryAtIfAbsentPut(
                implicit_globals, key, runtime->newValueCellCallback()));
        value_cell->setValue(*sp++);
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
        frame->setValueStackTop(sp);
        auto function = Function::cast(*(sp + arg));
        Object* result = function->entry()(thread, frame, arg);
        // Pop arguments + called function and push return value
        sp += arg;
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
        Runtime* runtime = thread->runtime();
        Handle<ValueCell> value_cell(
            &scope,
            runtime->dictionaryAtIfAbsentPut(
                globals, key, runtime->newValueCellCallback()));
        value_cell->setValue(*sp++);
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
      default:
        abort();
    }
  }
}

} // namespace python
