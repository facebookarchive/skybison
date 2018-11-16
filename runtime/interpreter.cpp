#include "interpreter.h"

#include <cassert>
#include <cstdlib>

#include "bytecode.h"
#include "frame.h"
#include "objects.h"
#include "thread.h"
#include "trampolines.h"

namespace python {

Object* Interpreter::execute(Thread* thread, Frame* frame) {
  Code* code = Code::cast(frame->code());
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

      default:
        abort();
    }
  }
}

} // namespace python
