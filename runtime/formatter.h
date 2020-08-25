#pragma once

#include "handles.h"
#include "objects.h"
#include "thread.h"

namespace py {

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

// Format float as a decimal number with `format` spec. Returns a `str`.
RawObject formatFloat(Thread* thread, double value, FormatSpec* format);

// Format int as decimal number with `format` spec. Returns a `str`.
RawObject formatIntDecimal(Thread* thread, const Int& value,
                           FormatSpec* format);

// Format int as decimal number. Returns a `str`.
RawObject formatIntDecimalSimple(Thread* thread, const Int& value);

// Format int as binary number with `format` spec. Returns a `str`.
RawObject formatIntBinary(Thread* thread, const Int& value, FormatSpec* format);

// Format int as binary number with `0b` prefix. Returns a `str`.
RawObject formatIntBinarySimple(Thread* thread, const Int& value);

// Format int as lower cased hexadecimal number with `format` spec.
// Returns a `str`.
RawObject formatIntHexadecimalLowerCase(Thread* thread, const Int& value,
                                        FormatSpec* format);

// Format int as upper cased hexadecimal number with `format` spec.
// Returns a `str`.
RawObject formatIntHexadecimalUpperCase(Thread* thread, const Int& value,
                                        FormatSpec* format);

// Format int as lower cased hexadecimal number with `0x` prefix.
// Returns a `str`.
RawObject formatIntHexadecimalSimple(Thread* thread, const Int& value);

// Format int as octal number with `format` spec. Returns a `str`.
RawObject formatIntOctal(Thread* thread, const Int& value, FormatSpec* format);

// Format int as octal number with `0o` prefix. Returns a `str`.
RawObject formatIntOctalSimple(Thread* thread, const Int& value);

// Format double as hexadecimal number with `0x` prefix. Returns a `str`.
RawObject formatDoubleHexadecimalSimple(Runtime* runtime, double value);

RawObject formatStr(Thread* thread, const Str& str, FormatSpec* format);

RawObject parseFormatSpec(Thread* thread, const Str& spec, int32_t default_type,
                          char default_align, FormatSpec* result);

RawObject raiseUnknownFormatError(Thread* thread, int32_t format_code,
                                  const Object& object);

}  // namespace py
