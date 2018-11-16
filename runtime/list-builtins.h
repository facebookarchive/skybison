#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class ListBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* append(Thread* thread, Frame* frame, word nargs);
  static Object* extend(Thread* thread, Frame* frame, word nargs);
  static Object* dunderAdd(Thread* thread, Frame* frame, word nargs);
  static Object* dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLen(Thread* thread, Frame* frame, word nargs);
  static Object* dunderMul(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNew(Thread* thread, Frame* frame, word nargs);
  static Object* dunderSetItem(Thread* thread, Frame* frame, word nargs);
  static Object* insert(Thread* thread, Frame* frame, word nargs);
  static Object* pop(Thread* thread, Frame* frame, word nargs);
  static Object* remove(Thread* thread, Frame* frame, word nargs);

 private:
  static Object* slice(Thread* thread, List* list, Slice* slice);
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kMethods[];

  // TODO(jeethu): use FRIEND_TEST
  friend class ListBuiltinsTest_SlicePositiveStartIndex_Test;
  friend class ListBuiltinsTest_SliceNegativeStartIndexIsRelativeToEnd_Test;
  friend class ListBuiltinsTest_SlicePositiveStopIndex_Test;
  friend class ListBuiltinsTest_SliceNegativeStopIndexIsRelativeToEnd_Test;
  friend class ListBuiltinsTest_SlicePositiveStep_Test;
  friend class ListBuiltinsTest_SliceNegativeStepReversesOrder_Test;
  friend class ListBuiltinsTest_SliceStartOutOfBounds_Test;
  friend class ListBuiltinsTest_SliceStopOutOfBounds_Test;
  friend class ListBuiltinsTest_SliceStepOutOfBounds_Test;
  friend class ListBuiltinsTest_IdenticalSliceIsCopy_Test;

  DISALLOW_IMPLICIT_CONSTRUCTORS(ListBuiltins);
};

}  // namespace python
