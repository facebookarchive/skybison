#pragma once

#include "handles.h"
#include "objects.h"
#include "thread.h"

namespace python {

struct FormatSpec {
  char alignment;
  char positive_sign;
  char thousands_separator;
  bool alternate;
  int32_t fill_char;
  int32_t type;
  word width;
  word precision;
};

RawObject formatStr(Thread* thread, const Str& str, FormatSpec* format);

RawObject parseFormatSpec(Thread* thread, const Str& spec, int32_t default_type,
                          char default_align, FormatSpec* result);

}  // namespace python
