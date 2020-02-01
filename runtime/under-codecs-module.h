#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

RawObject FUNC(_codecs, _ascii_decode)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_codecs, _ascii_encode)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_codecs, _bytearray_string_append)(Thread* thread, Frame* frame,
                                                  word nargs);
RawObject FUNC(_codecs, _escape_decode)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_codecs, _latin_1_decode)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_codecs, _latin_1_encode)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject FUNC(_codecs, _unicode_escape_decode)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject FUNC(_codecs, _utf_16_encode)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_codecs, _utf_32_encode)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_codecs, _utf_8_decode)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject FUNC(_codecs, _utf_8_encode)(Thread* thread, Frame* frame,
                                       word nargs);

class UnderCodecsModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
};

}  // namespace py
