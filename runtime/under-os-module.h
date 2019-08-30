#pragma once

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class UnderOsModule : public ModuleBase<UnderOsModule, SymbolId::kUnderOs> {
 public:
  static RawObject close(Thread* thread, Frame* frame, word nargs);
  static RawObject fstatSize(Thread* thread, Frame* frame, word nargs);
  static RawObject ftruncate(Thread* thread, Frame* frame, word nargs);
  static RawObject isatty(Thread* thread, Frame* frame, word nargs);
  static RawObject isdir(Thread* thread, Frame* frame, word nargs);
  static RawObject lseek(Thread* thread, Frame* frame, word nargs);
  static RawObject open(Thread* thread, Frame* frame, word nargs);
  static RawObject parseMode(Thread* thread, Frame* frame, word nargs);
  static RawObject read(Thread* thread, Frame* frame, word nargs);
  static RawObject setNoinheritable(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const char* const kFrozenData;
};

}  // namespace python
