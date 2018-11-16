#pragma once

#include "frame.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class StrBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderAdd(Thread* thread, Frame* frame, word nargs);
  static Object* dunderEq(Thread* thread, Frame* frame, word nargs);
  static Object* dunderGe(Thread* thread, Frame* frame, word nargs);
  static Object* dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static Object* dunderGt(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLe(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLen(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLt(Thread* thread, Frame* frame, word nargs);
  static Object* dunderMod(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNe(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNew(Thread* thread, Frame* frame, word nargs);
  static Object* dunderRepr(Thread* thread, Frame* frame, word nargs);
  static Object* lower(Thread* thread, Frame* frame, word nargs);
  static Object* join(Thread* thread, Frame* frame, word nargs);

 private:
  static word strFormatBufferLength(const Handle<Str>& fmt,
                                    const Handle<ObjectArray>& args);
  static Object* strFormat(Thread* thread, const Handle<Str>& fmt,
                           const Handle<ObjectArray>& args);
  static void byteToHex(byte** buf, byte convert);
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(StrBuiltins);
};

}  // namespace python
