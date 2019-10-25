#pragma once

namespace py {

double parseInfOrNan(const char* p, char** endptr);

enum class ConversionResult {
  kSuccess,
  kOutOfMemory,
  kInvalid,
  kOverflow,
};

double parseFloat(const char* s, char** endptr, ConversionResult* result);

enum class FormatResultKind {
  kFinite,
  kInfinite,
  kNan,
};

char* formatFloat(double value, char format_code, int precision, bool skip_sign,
                  bool add_dot_0, bool use_alt_formatting,
                  FormatResultKind* type);

// Round double value to `ndigits` decimal digits.
double doubleRoundDecimals(double value, int ndigits);

}  // namespace py
