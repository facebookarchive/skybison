#pragma once

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

word doubleHash(double value);

word floatHash(RawObject value);

RawObject METH(float, __abs__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __add__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __bool__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __eq__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __float__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __ge__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __gt__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __hash__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __int__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __le__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __lt__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __mul__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __neg__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __pow__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __round__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __rtruediv__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __sub__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __truediv__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(float, __trunc__)(Thread* thread, Frame* frame, word nargs);

class FloatBuiltins
    : public Builtins<FloatBuiltins, ID(float), LayoutId::kFloat> {
 public:
  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FloatBuiltins);
};

void decodeDouble(double value, bool* is_neg, int* exp, uint64_t* mantissa);

inline word floatHash(RawObject value) {
  return doubleHash(Float::cast(value).value());
}

}  // namespace py
