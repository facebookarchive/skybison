#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

class UnderCodecsModule {
 public:
  static void initialize(Thread* thread, const Module& module);

  static RawObject underAsciiDecode(Thread* thread, Frame* frame, word nargs);
  static RawObject underAsciiEncode(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytearrayStringAppend(Thread* thread, Frame* frame,
                                              word nargs);
  static RawObject underEscapeDecode(Thread* thread, Frame* frame, word nargs);
  static RawObject underLatin1Decode(Thread* thread, Frame* frame, word nargs);
  static RawObject underLatin1Encode(Thread* thread, Frame* frame, word nargs);
  static RawObject underUnicodeEscapeDecode(Thread* thread, Frame* frame,
                                            word nargs);
  static RawObject underUtf16Encode(Thread* thread, Frame* frame, word nargs);
  static RawObject underUtf32Encode(Thread* thread, Frame* frame, word nargs);
  static RawObject underUtf8Decode(Thread* thread, Frame* frame, word nargs);
  static RawObject underUtf8Encode(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
};

}  // namespace py
