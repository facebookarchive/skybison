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

// Format int as decimal number. Returns a `str`.
RawObject formatIntDecimalSimple(Thread* thread, const Int& value);

// Format int as binary number with `0b` prefix. Returns a `str`.
RawObject formatIntBinarySimple(Thread* thread, const Int& value);

// Format int as hexadecimal number with `0x` prefix. Returns a `str`.
RawObject formatIntHexadecimalSimple(Thread* thread, const Int& value);

// Format int as octal number with `0o` prefix. Returns a `str`.
RawObject formatIntOctalSimple(Thread* thread, const Int& value);

RawObject formatStr(Thread* thread, const Str& str, FormatSpec* format);

RawObject parseFormatSpec(Thread* thread, const Str& spec, int32_t default_type,
                          char default_align, FormatSpec* result);

RawObject raiseUnknownFormatError(Thread* thread, int32_t format_code,
                                  const Object& object);

}  // namespace python
