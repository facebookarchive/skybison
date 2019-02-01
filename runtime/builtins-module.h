#pragma once

#include <iostream>

#include "frame.h"
#include "globals.h"
#include "objects.h"

namespace python {

extern std::ostream* builtInStdout;
extern std::ostream* builtinStderr;

RawObject getAttribute(Thread* thread, const Object& self, const Object& name);
RawObject hasAttribute(Thread* thread, const Object& self, const Object& name);
void patchFunctionAttrsInTypeDict(Thread* thread, const Dict& type_dict,
                                  const Function& patch);
void printStr(RawStr str, std::ostream* ostream);
RawObject setAttribute(Thread* thread, const Object& self, const Object& name,
                       const Object& value);

class Builtins {
 public:
  static RawObject buildClass(Thread* thread, Frame* frame, word nargs);
  static RawObject buildClassKw(Thread* thread, Frame* frame, word nargs);
  static RawObject callable(Thread* thread, Frame* frame, word nargs);
  static RawObject chr(Thread* thread, Frame* frame, word nargs);
  static RawObject compile(Thread* thread, Frame* frame, word nargs);
  static RawObject exec(Thread* thread, Frame* frame, word nargs);
  static RawObject getattr(Thread* thread, Frame* frame, word nargs);
  static RawObject hasattr(Thread* thread, Frame* frame, word nargs);
  static RawObject isinstance(Thread* thread, Frame* frame, word nargs);
  static RawObject issubclass(Thread* thread, Frame* frame, word nargs);
  static RawObject ord(Thread* thread, Frame* frame, word nargs);
  static RawObject print(Thread* thread, Frame* frame, word nargs);
  static RawObject printKw(Thread* thread, Frame* frame, word nargs);
  static RawObject range(Thread* thread, Frame* frame, word nargs);
  static RawObject setattr(Thread* thread, Frame* frame, word nargs);
  static RawObject underAddress(Thread* thread, Frame* frame, word nargs);
  static RawObject underPatch(Thread* thread, Frame* frame, word nargs);
  static RawObject underPrintStr(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrEscapeNonAscii(Thread* thread, Frame* frame,
                                          word nargs);
};

}  // namespace python
