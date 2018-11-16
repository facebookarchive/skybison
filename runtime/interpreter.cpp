#include <cstdlib>

#include "bytecode.h"
#include "frame.h"
#include "interpreter.h"
#include "objects.h"
#include "thread.h"
#include "trampolines.h"

namespace python {

Object* Interpreter::execute(Thread* thread, Frame* frame) {
  Code* code = Code::cast(frame->code());
  ByteArray* byteArray = ByteArray::cast(code->code());
  int pc = 0;
  for (;;) {
    Bytecode bc = static_cast<Bytecode>(byteArray->byteAt(pc++));
    byte arg = byteArray->byteAt(pc++);
    switch (bc) {
      case Bytecode::RETURN_VALUE: {
        auto result = thread->popObject();
        // Clean up after ourselves
        thread->popFrame(frame);
        return result;
      }
      case Bytecode::LOAD_CONST: {
        Object* consts = Code::cast(frame->code())->consts();
        thread->pushObject(ObjectArray::cast(consts)->at(arg));
        break;
      }
      case Bytecode::LOAD_NAME: {
        break;
      }
      case Bytecode::POP_TOP: {
        thread->popObject();
        break;
      }
      case Bytecode::CALL_FUNCTION: {
        auto function = Function::cast(thread->peekObject(arg));
        auto trampoline = trampolineFromObject(function->entry());
        Object* result = trampoline(thread, frame, arg);
        // Pop arguments + called function
        thread->popObjects(arg + 1);
        thread->pushObject(result);
        break;
      }

      default:
        abort();
    }
  }
}

} // namespace python
