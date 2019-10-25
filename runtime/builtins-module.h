#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace py {

RawObject getAttribute(Thread* thread, const Object& self, const Object& name);
RawObject hasAttribute(Thread* thread, const Object& self, const Object& name);
RawObject setAttribute(Thread* thread, const Object& self, const Object& name,
                       const Object& value);

class BuiltinsModule {
 public:
  static RawObject bin(Thread* thread, Frame* frame, word nargs);
  static RawObject callable(Thread* thread, Frame* frame, word nargs);
  static RawObject chr(Thread* thread, Frame* frame, word nargs);
  static RawObject compile(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderBuildClass(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderImport(Thread* thread, Frame* frame, word nargs);
  static RawObject exec(Thread* thread, Frame* frame, word nargs);
  static RawObject getattr(Thread* thread, Frame* frame, word nargs);
  static RawObject globals(Thread* thread, Frame* frame, word nargs);
  static RawObject hasattr(Thread* thread, Frame* frame, word nargs);
  static RawObject hash(Thread* thread, Frame* frame, word nargs);
  static RawObject hex(Thread* thread, Frame* frame, word nargs);
  static RawObject id(Thread* thread, Frame* frame, word nargs);
  static RawObject oct(Thread* thread, Frame* frame, word nargs);
  static RawObject ord(Thread* thread, Frame* frame, word nargs);
  static RawObject print(Thread* thread, Frame* frame, word nargs);
  static RawObject printKw(Thread* thread, Frame* frame, word nargs);
  static RawObject setattr(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinType kBuiltinTypes[];
  static const char* const kFrozenData;
  static const SymbolId kIntrinsicIds[];
};

}  // namespace py
