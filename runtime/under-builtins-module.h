#pragma once

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

// Copy the code, entry, and interpreter info from base to patch.
void copyFunctionEntries(Thread* thread, const Function& base,
                         const Function& patch);

class UnderBuiltinsModule {
 public:
  static RawObject underBytesCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underIntCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underPatch(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrArrayIadd(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underTupleCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underType(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underUnimplemented(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinType kBuiltinTypes[];
  static const char* const kFrozenData;
  static const SymbolId kIntrinsicIds[];
};

}  // namespace python
