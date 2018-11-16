#include "builtins.h"

#include <iostream>

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

std::ostream* builtinPrintStream = &std::cout;

static void printString(String* str) {
  for (int i = 0; i < str->length(); i++) {
    *builtinPrintStream << str->charAt(i);
  }
}

Object* builtinPrint(Thread*, Frame* callerFrame, word argc) {
  const char* separator = " ";
  const char* terminator = "\n";
  Object** argv = callerFrame->valueStackTop();

  // Args come in in reverse order
  for (word i = argc - 1; i >= 0; i--) {
    Object* arg = argv[i];
    if (arg->isString()) {
      printString(String::cast(arg));
    } else if (arg->isSmallInteger()) {
      *builtinPrintStream << SmallInteger::cast(arg)->value();
    } else if (arg->isBoolean()) {
      *builtinPrintStream << (Boolean::cast(arg)->value() ? "True" : "False");
    } else {
      std::abort();
    }
    if (i > 0) {
      *builtinPrintStream << separator;
    }
  }
  *builtinPrintStream << terminator;

  return None::object();
}

} // namespace python
