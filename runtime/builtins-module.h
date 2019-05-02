#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace python {

RawObject getAttribute(Thread* thread, const Object& self, const Object& name);
RawObject hasAttribute(Thread* thread, const Object& self, const Object& name);
RawObject setAttribute(Thread* thread, const Object& self, const Object& name,
                       const Object& value);
void copyFunctionEntries(Thread* thread, const Function& base,
                         const Function& patch);

class BuiltinsModule {
 public:
  static RawObject callable(Thread* thread, Frame* frame, word nargs);
  static RawObject chr(Thread* thread, Frame* frame, word nargs);
  static RawObject compile(Thread* thread, Frame* frame, word nargs);
  static RawObject divmod(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderBuildClass(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderImport(Thread* thread, Frame* frame, word nargs);
  static RawObject exec(Thread* thread, Frame* frame, word nargs);
  static RawObject getattr(Thread* thread, Frame* frame, word nargs);
  static RawObject hasattr(Thread* thread, Frame* frame, word nargs);
  static RawObject isinstance(Thread* thread, Frame* frame, word nargs);
  static RawObject issubclass(Thread* thread, Frame* frame, word nargs);
  static RawObject ord(Thread* thread, Frame* frame, word nargs);
  static RawObject print(Thread* thread, Frame* frame, word nargs);
  static RawObject printKw(Thread* thread, Frame* frame, word nargs);
  static RawObject setattr(Thread* thread, Frame* frame, word nargs);
  static RawObject underAddress(Thread* thread, Frame* frame, word nargs);
  static RawObject underByteArrayCheck(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underBytesCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesFromInts(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesGetItem(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesGetItemSlice(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underBytesMaketrans(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underBytesRepeat(Thread* thread, Frame* frame, word nargs);
  static RawObject underIntCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underIntFromByteArray(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underIntFromBytes(Thread* thread, Frame* frame, word nargs);
  static RawObject underIntFromInt(Thread* thread, Frame* frame, word nargs);
  static RawObject underIntFromStr(Thread* thread, Frame* frame, word nargs);
  static RawObject underListSort(Thread* thread, Frame* frame, word nargs);
  static RawObject underPatch(Thread* thread, Frame* frame, word nargs);
  static RawObject underReprEnter(Thread* thread, Frame* frame, word nargs);
  static RawObject underReprLeave(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrEscapeNonAscii(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underStrFind(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrFromStr(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrReplace(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrRFind(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrSplitlines(Thread* thread, Frame* frame, word nargs);
  static RawObject underUnimplemented(Thread* thread, Frame* frame, word nargs);
  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinType kBuiltinTypes[];
  static const char* const kFrozenData;
};

}  // namespace python
