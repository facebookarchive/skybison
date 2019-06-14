#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace python {

class UnderCodecsModule
    : public ModuleBase<UnderCodecsModule, SymbolId::kUnderCodecs> {
 public:
  static void postInitialize(Thread* thread, Runtime* runtime,
                             const Module& module);
  static RawObject underAsciiDecode(Thread* thread, Frame* frame, word nargs);
  static RawObject underAsciiEncode(Thread* thread, Frame* frame, word nargs);
  static RawObject underEscapeDecode(Thread* thread, Frame* frame, word nargs);
  static RawObject underLatin1Encode(Thread* thread, Frame* frame, word nargs);
  static RawObject underUnicodeEscapeDecode(Thread* thread, Frame* frame,
                                            word nargs);
  static RawObject underUtf16Encode(Thread* thread, Frame* frame, word nargs);
  static RawObject underUtf32Encode(Thread* thread, Frame* frame, word nargs);
  static RawObject underUtf8Encode(Thread* thread, Frame* frame, word nargs);
  static RawObject underByteArrayAsBytes(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underByteArrayStringAppend(Thread* thread, Frame* frame,
                                              word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const char* const kFrozenData;
};

}  // namespace python
