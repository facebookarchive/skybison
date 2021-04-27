#pragma once

#include "handles-decl.h"
#include "thread.h"

namespace py {

class Interpreter;

Interpreter* createAsmInterpreter();
void compileFunction(Thread* thread, const Function& function);

}  // namespace py
