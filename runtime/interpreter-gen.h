#pragma once

#include "handles-decl.h"
#include "thread.h"

namespace py {

class Interpreter;

Interpreter* createAsmInterpreter();
bool canCompileFunction(Thread* thread, const Function& function);
void compileFunction(Thread* thread, const Function& function);

}  // namespace py
