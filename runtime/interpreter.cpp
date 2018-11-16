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
  Object** sp = frame->top();
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
      case Bytecode::LOAD_NAME: {
        break;
      }
      case Bytecode::POP_TOP: {
        sp++;
        break;
      }
      case Bytecode::CALL_FUNCTION: {
        frame->setTop(sp);
        auto function = Function::cast(*(sp + arg));
        Object* result = function->entry()(thread, frame, arg);
        // Pop arguments + called function and push return value
        sp += arg;
        *sp = result;
        break;
      }

      default:
        abort();
    }
  }
}

} // namespace python
