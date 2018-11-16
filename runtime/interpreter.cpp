#include <cstdlib>

#include "bytecode.h"
#include "frame.h"
#include "interpreter.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* Interpreter::execute(Thread* thread, Frame* frame) {
  Code* code = Code::cast(frame->f_code);
  ByteArray* byteArray = ByteArray::cast(code->code());
  int pc = 0;
  for (;;) {
    Bytecode bc = static_cast<Bytecode>(byteArray->byteAt(pc++));
    byte arg = byteArray->byteAt(pc++);
    switch (bc) {
      case Bytecode::RETURN_VALUE: {
        Object* result = *--frame->f_stacktop;
        return result;
      }
      case Bytecode::LOAD_CONST: {
        Object* consts = Code::cast(frame->f_code)->consts();
        *frame->f_stacktop++ = ObjectArray::cast(consts)->at(arg);
        break;
      }
      case Bytecode::LOAD_NAME: {
        break;
      }
      case Bytecode::POP_TOP: {
        --frame->f_stacktop;
        break;
      }
      case Bytecode::CALL_FUNCTION: {
        break;
      }

      default:
        abort();
    }
  }
}

} // namespace python
