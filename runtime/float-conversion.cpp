#include "float-conversion.h"

#include <cerrno>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "globals.h"
#include "utils.h"

// The following code is a copy of `pystrtod.c` and `dtoa.c` from cpython.
// The public interface has been slightly changes and the code was heavily
// modified to conform to our style guides. It also doesn't contain code for
// non-ieee754 architectures any longer.
// However there were no changes to the algorithm and there are no plans to
// do so at the moment!

namespace python {

static char* dtoa(double dd, int mode, int ndigits, int* decpt, int* sign,
                  char** rve);

static double strtod(const char* s00, char** se);

static void freedtoa(char* s);

// Case-insensitive string match used for nan and inf detection; t should be
// lower-case.  Returns 1 for a successful match, 0 otherwise.
static bool caseInsensitiveMatch(const char* s, const char* t) {
  for (; *t != '\0'; s++, t++) {
    DCHECK('a' <= *t && *t <= 'z', "t must be lowercase letters");
    if (*t != *s && *t != (*s - 'A' + 'a')) {
      return false;
    }
  }
  return true;
}

// parseInfOrNan: Attempt to parse a string of the form "nan", "inf" or
// "infinity", with an optional leading sign of "+" or "-".  On success,
// return the NaN or Infinity as a double and set *endptr to point just beyond
// the successfully parsed portion of the string.  On failure, return -1.0 and
// set *endptr to point to the start of the string.
double parseInfOrNan(const char* p, char** endptr) {
  const char* s = p;
  bool negate = false;
  if (*s == '-') {
    negate = true;
    s++;
  } else if (*s == '+') {
    s++;
  }

  double retval;
  if (caseInsensitiveMatch(s, "inf")) {
    s += 3;
    if (caseInsensitiveMatch(s, "inity")) {
      s += 5;
    }
    retval = negate ? -std::numeric_limits<double>::infinity()
                    : std::numeric_limits<double>::infinity();
  } else if (caseInsensitiveMatch(s, "nan")) {
    s += 3;
    retval = negate ? -std::numeric_limits<double>::quiet_NaN()
                    : std::numeric_limits<double>::quiet_NaN();
  } else {
    s = p;
    retval = -1.0;
  }
  *endptr = const_cast<char*>(s);
  return retval;
}

// asciiStrtod:
// @nptr:    the string to convert to a numeric value.
// @endptr:  if non-%nullptr, it returns the character after
//           the last character used in the conversion.
//
// Converts a string to a #gdouble value.
// This function behaves like the standard strtod() function
// does in the C locale. It does this without actually
// changing the current locale, since that would not be
// thread-safe.
//
// This function is typically used when reading configuration
// files or other non-user input that should be locale independent.
// To handle input from the user you should normally use the
// locale-sensitive system strtod() function.
//
// If the correct value would cause overflow, plus or minus %HUGE_VAL
// is returned (according to the sign of the value), and %ERANGE is
// stored in %errno. If the correct value would cause underflow,
// zero is returned and %ERANGE is stored in %errno.
// If memory allocation fails, %ENOMEM is stored in %errno.
//
// This function resets %errno before calling strtod() so that
// you can reliably detect overflow and underflow.
//
// Return value: the #gdouble value.
static double asciiStrtod(const char* nptr, char** endptr) {
  double result = strtod(nptr, endptr);
  if (*endptr == nptr) {
    // string might represent an inf or nan
    result = parseInfOrNan(nptr, endptr);
  }
  return result;
}

// parseFloat converts a null-terminated byte string s (interpreted
// as a string of ASCII characters) to a float.  The string should not have
// leading or trailing whitespace.  The conversion is independent of the
// current locale.
//
// If endptr is nullptr, try to convert the whole string.  Raise ValueError and
// return -1.0 if the string is not a valid representation of a floating-point
// number.
//
// If endptr is non-nullptr, try to convert as much of the string as possible.
// If no initial segment of the string is the valid representation of a
// floating-point number then *endptr is set to point to the beginning of the
// string, -1.0 is returned and again ValueError is raised.
//
// On overflow (e.g., when trying to convert '1e500' on an IEEE 754 machine),
// if overflow_exception is nullptr then +-HUGE_VAL is returned, and no
// Python exception is raised.  Otherwise, overflow_exception should point to a
// Python exception, this exception will be raised, -1.0 will be returned, and
// *endptr will point just past the end of the converted value.
//
// If any other failure occurs (for example lack of memory), -1.0 is returned
// and the appropriate Python exception will have been set.
double parseFloat(const char* s, char** endptr,
                  ConversionResult* conversion_result) {
  errno = 0;
  char* fail_pos;
  double x = asciiStrtod(s, &fail_pos);

  double result = -1.0;
  if (errno == ENOMEM) {
    *conversion_result = ConversionResult::kOutOfMemory;
    fail_pos = const_cast<char*>(s);
  } else if (!endptr && (fail_pos == s || *fail_pos != '\0')) {
    *conversion_result = ConversionResult::kInvalid;
  } else if (fail_pos == s) {
    *conversion_result = ConversionResult::kInvalid;
  } else if (errno == ERANGE && std::fabs(x) >= 1.0) {
    *conversion_result = ConversionResult::kOverflow;
  } else {
    *conversion_result = ConversionResult::kSuccess;
    result = x;
  }

  if (endptr != nullptr) {
    *endptr = fail_pos;
  }
  return result;
}

// I'm using a lookup table here so that I don't have to invent a non-locale
// specific way to convert to uppercase.
static const int kOfsInf = 0;
static const int kOfsNan = 1;
static const int kOfsE = 2;

// The lengths of these are known to the code below, so don't change them.
static const char* const kLcFloatStrings[] = {
    "inf",
    "nan",
    "e",
};
static const char* const kUcFloatStrings[] = {
    "INF",
    "NAN",
    "E",
};

// Convert a double d to a string, and return a PyMem_Malloc'd block of
// memory contain the resulting string.
//
// Arguments:
//   d is the double to be converted
//   format_code is one of 'e', 'f', 'g', 'r'.  'e', 'f' and 'g'
//     correspond to '%e', '%f' and '%g';  'r' corresponds to repr.
//   mode is one of '0', '2' or '3', and is completely determined by
//     format_code: 'e' and 'g' use mode 2; 'f' mode 3, 'r' mode 0.
//   precision is the desired precision
//   skip_sign do not prepend any sign even for negative numbers.
//   add_dot_0_if_integer is nonzero if integers in non-exponential form
//     should have ".0" added.  Only applies to format codes 'r' and 'g'.
//   use_alt_formatting is nonzero if alternative formatting should be
//     used.  Only applies to format codes 'e', 'f' and 'g'.  For code 'g',
//     at most one of use_alt_formatting and add_dot_0_if_integer should
//     be nonzero.
//   type, if non-nullptr, will be set to one of these constants to identify
//     the type of the 'd' argument:
//   FormatResultKind::kFinite
//   FormatResultKind::kInfinite
//   FormatResultKind::kNan
//
// Returns a PyMem_Malloc'd block of memory containing the resulting string,
//  or nullptr on error. If nullptr is returned, the Python error has been set.
static char* formatFloatShort(double d, char format_code, int mode,
                              int precision, bool skip_sign,
                              bool always_add_sign, bool add_dot_0_if_integer,
                              bool use_alt_formatting,
                              const char* const* float_strings,
                              FormatResultKind* type) {
  // python::dtoa returns a digit string (no decimal point or exponent).
  // Must be matched by a call to python::freedtoa.
  char* digits_end;
  int decpt_as_int;
  int sign;
  char* digits = dtoa(d, mode, precision, &decpt_as_int, &sign, &digits_end);

  word decpt = decpt_as_int;
  if (digits == nullptr) {
    // The only failure mode is no memory.
    return nullptr;
  }
  DCHECK(digits_end != nullptr && digits_end >= digits, "unexpected result");
  word digits_len = digits_end - digits;

  word vdigits_end = digits_len;
  bool use_exp = false;

  word bufsize = 0;
  char* p = nullptr;
  char* buf = nullptr;
  if (digits_len && !('0' <= digits[0] && digits[0] <= '9')) {
    // Infinities and nans here; adapt Gay's output,
    // so convert Infinity to inf and NaN to nan, and
    // ignore sign of nan. Then return.

    // ignore the actual sign of a nan.
    if (digits[0] == 'n' || digits[0] == 'N') {
      sign = 0;
    }

    // We only need 5 bytes to hold the result "+inf\0".
    bufsize = 5;  // Used later in an assert.
    buf = static_cast<char*>(std::malloc(bufsize));
    if (buf == nullptr) {
      goto exit;
    }
    p = buf;

    if (!skip_sign) {
      if (sign == 1) {
        *p++ = '-';
      } else if (always_add_sign) {
        *p++ = '+';
      }
    }
    if (digits[0] == 'i' || digits[0] == 'I') {
      std::strncpy(p, float_strings[kOfsInf], 3);
      p += 3;

      if (type) {
        *type = FormatResultKind::kInfinite;
      }
    } else if (digits[0] == 'n' || digits[0] == 'N') {
      std::strncpy(p, float_strings[kOfsNan], 3);
      p += 3;

      if (type) {
        *type = FormatResultKind::kNan;
      }
    } else {
      // shouldn't get here: Gay's code should always return
      // something starting with a digit, an 'I',  or 'N'.
      UNIMPLEMENTED("should always return digit, I or N");
    }
    goto exit;
  }

  // The result must be finite (not inf or nan).
  if (type) {
    *type = FormatResultKind::kFinite;
  }

  // We got digits back, format them.  We may need to pad 'digits'
  // either on the left or right (or both) with extra zeros, so in
  // general the resulting string has the form

  //   [<sign>]<zeros><digits><zeros>[<exponent>]

  // where either of the <zeros> pieces could be empty, and there's a
  // decimal point that could appear either in <digits> or in the
  // leading or trailing <zeros>.

  // Imagine an infinite 'virtual' string vdigits, consisting of the
  // string 'digits' (starting at index 0) padded on both the left and
  // right with infinite strings of zeros.  We want to output a slice

  //   vdigits[vdigits_start : vdigits_end]

  // of this virtual string.  Thus if vdigits_start < 0 then we'll end
  // up producing some leading zeros; if vdigits_end > digits_len there
  // will be trailing zeros in the output.  The next section of code
  // determines whether to use an exponent or not, figures out the
  // position 'decpt' of the decimal point, and computes 'vdigits_start'
  // and 'vdigits_end'.
  switch (format_code) {
    case 'e':
      use_exp = true;
      vdigits_end = precision;
      break;
    case 'f':
      vdigits_end = decpt + precision;
      break;
    case 'g':
      if (decpt <= -4 ||
          decpt > (add_dot_0_if_integer ? precision - 1 : precision)) {
        use_exp = true;
      }
      if (use_alt_formatting) {
        vdigits_end = precision;
      }
      break;
    case 'r':
      // convert to exponential format at 1e16.  We used to convert
      // at 1e17, but that gives odd-looking results for some values
      // when a 16-digit 'shortest' repr is padded with bogus zeros.
      // For example, repr(2e16+8) would give 20000000000000010.0;
      // the true value is 20000000000000008.0.
      if (decpt <= -4 || decpt > 16) {
        use_exp = true;
      }
      break;
    default:
      goto exit;
  }

  // if using an exponent, reset decimal point position to 1 and adjust
  // exponent accordingly.
  {
    int exp = 0;
    if (use_exp) {
      exp = static_cast<int>(decpt) - 1;
      decpt = 1;
    }
    // ensure vdigits_start < decpt <= vdigits_end, or vdigits_start <
    // decpt < vdigits_end if add_dot_0_if_integer and no exponent.
    word vdigits_start = decpt <= 0 ? decpt - 1 : 0;
    if (!use_exp && add_dot_0_if_integer) {
      vdigits_end = vdigits_end > decpt ? vdigits_end : decpt + 1;
    } else {
      vdigits_end = vdigits_end > decpt ? vdigits_end : decpt;
    }

    // double check inequalities.
    DCHECK(vdigits_start <= 0 && 0 <= digits_len && digits_len <= vdigits_end,
           "failed consistency check");
    // decimal point should be in (vdigits_start, vdigits_end]
    DCHECK(vdigits_start < decpt && decpt <= vdigits_end,
           "failed decimal point check");

    // Compute an upper bound how much memory we need. This might be a few
    // chars too long, but no big deal.
    bufsize =
        // sign, decimal point and trailing 0 byte
        3 - skip_sign +

        // total digit count (including zero padding on both sides)
        (vdigits_end - vdigits_start) +

        // exponent "e+100", max 3 numerical digits
        (use_exp ? 5 : 0);

    // Now allocate the memory and initialize p to point to the start of it.
    buf = static_cast<char*>(std::malloc(bufsize));
    if (buf == nullptr) {
      goto exit;
    }
    p = buf;

    // Add a negative sign if negative, and a plus sign if non-negative
    // and always_add_sign is true.
    if (!skip_sign) {
      if (sign == 1) {
        *p++ = '-';
      } else if (always_add_sign) {
        *p++ = '+';
      }
    }

    // note that exactly one of the three 'if' conditions is true,
    // so we include exactly one decimal point.
    // Zero padding on left of digit string
    if (decpt <= 0) {
      std::memset(p, '0', decpt - vdigits_start);
      p += decpt - vdigits_start;
      *p++ = '.';
      std::memset(p, '0', 0 - decpt);
      p += 0 - decpt;
    } else {
      std::memset(p, '0', 0 - vdigits_start);
      p += 0 - vdigits_start;
    }

    // Digits, with included decimal point
    if (0 < decpt && decpt <= digits_len) {
      std::strncpy(p, digits, decpt - 0);
      p += decpt - 0;
      *p++ = '.';
      std::strncpy(p, digits + decpt, digits_len - decpt);
      p += digits_len - decpt;
    } else {
      std::strncpy(p, digits, digits_len);
      p += digits_len;
    }

    // And zeros on the right
    if (digits_len < decpt) {
      std::memset(p, '0', decpt - digits_len);
      p += decpt - digits_len;
      *p++ = '.';
      std::memset(p, '0', vdigits_end - decpt);
      p += vdigits_end - decpt;
    } else {
      std::memset(p, '0', vdigits_end - digits_len);
      p += vdigits_end - digits_len;
    }

    // Delete a trailing decimal pt unless using alternative formatting.
    if (p[-1] == '.' && !use_alt_formatting) {
      p--;
    }

    // Now that we've done zero padding, add an exponent if needed.
    if (use_exp) {
      *p++ = float_strings[kOfsE][0];
      int exp_len = std::sprintf(p, "%+.02d", exp);
      p += exp_len;
    }
  }

exit:
  if (buf) {
    *p = '\0';
    // It's too late if this fails, as we've already stepped on
    // memory that isn't ours. But it's an okay debugging test.
    DCHECK(p - buf < bufsize, "buffer overflow");
  }
  if (digits) {
    freedtoa(digits);
  }

  return buf;
}

char* formatFloat(double value, char format_code, int precision, bool skip_sign,
                  bool add_dot_0, bool use_alt_formatting,
                  FormatResultKind* type) {
  const char* const* float_strings = kLcFloatStrings;

  // Validate format_code, and map upper and lower case. Compute the
  // mode and make any adjustments as needed.
  int mode;
  switch (format_code) {
    // exponent
    case 'E':
      float_strings = kUcFloatStrings;
      format_code = 'e';
      // Fall through.
    case 'e':
      mode = 2;
      precision++;
      break;

    // fixed
    case 'F':
      float_strings = kUcFloatStrings;
      format_code = 'f';
      // Fall through.
    case 'f':
      mode = 3;
      break;

    // general
    case 'G':
      float_strings = kUcFloatStrings;
      format_code = 'g';
      // Fall through.
    case 'g':
      mode = 2;
      // precision 0 makes no sense for 'g' format; interpret as 1
      if (precision == 0) {
        precision = 1;
      }
      break;

    // repr format
    case 'r':
      mode = 0;
      // Supplied precision is unused, must be 0.
      if (precision != 0) {
        return nullptr;
      }
      break;

    default:
      return nullptr;
  }

  return formatFloatShort(value, format_code, mode, precision, skip_sign,
                          /*always_add_sign=*/false, add_dot_0,
                          use_alt_formatting, float_strings, type);
}

double doubleRoundDecimals(double value, int ndigits) {
  // Print value to a string with `ndigits` decimal digits.
  char* buf_end;
  int decpt;
  int sign;
  char* buf = dtoa(value, 3, ndigits, &decpt, &sign, &buf_end);
  CHECK(buf != nullptr, "out of memory");

  char shortbuf[100];
  unique_c_ptr<char> longbuf;
  size_t buflen = buf_end - buf;
  char* number_buf;
  size_t number_buf_len = buflen + 8;
  if (number_buf_len <= sizeof(shortbuf)) {
    number_buf = shortbuf;
  } else {
    longbuf.reset(static_cast<char*>(std::malloc(number_buf_len)));
    number_buf = longbuf.get();
  }
  std::snprintf(number_buf, number_buf_len, "%c0%se%d", (sign ? '-' : '+'), buf,
                decpt - static_cast<int>(buflen));
  freedtoa(buf);

  // Convert resulting string back to a double.
  return strtod(number_buf, nullptr);
}

/****************************************************************
 *
 * The author of this software is David M. Gay.
 *
 * Copyright (c) 1991, 2000, 2001 by Lucent Technologies.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHOR NOR LUCENT MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 ***************************************************************/

// This is dtoa.c by David M. Gay, downloaded from
// http://www.netlib.org/fp/dtoa.c on April 15, 2009 and modified for
// inclusion into the Python core by Mark E. T. Dickinson and Eric V. Smith.
//
// Please remember to check http://www.netlib.org/fp regularly (and especially
// before any Python release) for bugfixes and updates.
//
// The major modifications from Gay's original code are as follows:
//
//  0. The original code has been specialized to Python's needs by removing
//     many of the #ifdef'd sections.  In particular, code to support VAX and
//     IBM floating-point formats, hex NaNs, hex floats, locale-aware
//     treatment of the decimal point, and setting of the inexact flag have
//     been removed.
//
//  1. We use PyMem_Malloc and PyMem_Free in place of malloc and free.
//
//  2. The public functions strtod, dtoa and freedtoa all now have
//     a _Py_dg_ prefix.
//
//  3. Instead of assuming that PyMem_Malloc always succeeds, we thread
//     PyMem_Malloc failures through the code.  The functions
//
//       Balloc, multadd, s2b, i2b, mult, pow5mult, lshift, diff, d2b
//
//     of return type *Bigint all return nullptr to indicate a malloc failure.
//     Similarly, rv_alloc and nrv_alloc (return type char *) return nullptr on
//     failure.  bigcomp now has return type int (it used to be void) and
//     returns -1 on failure and 0 otherwise.  dtoa returns nullptr
//     on failure.  strtod indicates failure due to malloc failure
//     by returning -1.0, setting errno=ENOMEM and *se to s00.
//
//  4. The static variable dtoa_result has been removed.  Callers of
//     dtoa are expected to call freedtoa to free
//     the memory allocated by dtoa.
//
//  5. The code has been reformatted to better fit with Python's
//     C style guide (PEP 7).
//
//  6. A bug in the memory allocation has been fixed: to avoid FREEing memory
//     that hasn't been std::malloc'ed, private_mem should only be used when
//     k <= kKmax.
//
//  7. strtod has been modified so that it doesn't accept strings with
//     leading whitespace.
//

// Please send bug reports for the original dtoa.c code to David M. Gay (dmg
// at acm dot org, with " at " changed at "@" and " dot " changed to ".").
// Please report bugs for this modified version using the Python issue tracker
// (http://bugs.python.org).

// On a machine with IEEE extended-precision registers, it is
// necessary to specify double-precision (53-bit) rounding precision
// before invoking strtod or dtoa.  If the machine uses (the equivalent
// of) Intel 80x87 arithmetic, the call
//      _control87(PC_53, MCW_PC);
// does this with many compilers.  Whether this or another call is
// appropriate depends on the compiler; for this to work, it may be
// necessary to #include "float.h" or another system-dependent header
// file.

// strtod for IEEE-, VAX-, and IBM-arithmetic machines.
//
// This strtod returns a nearest machine number to the input decimal
// string (or sets errno to ERANGE).  With IEEE arithmetic, ties are
// broken by the IEEE round-even rule.  Otherwise ties are broken by
// biased rounding (add half and chop).
//
// Inspired loosely by William D. Clinger's paper "How to Read Floating
// Point Numbers Accurately" [Proc. ACM SIGPLAN '90, pp. 92-101].
//
// Modifications:
//
//      1. We only require IEEE, IBM, or VAX double-precision
//              arithmetic (not IEEE double-extended).
//      2. We get by with floating-point arithmetic in a case that
//              Clinger missed -- when we're computing d * 10^n
//              for a small integer d and the integer n is not too
//              much larger than 22 (the maximum integer k for which
//              we can represent 10^k exactly), we may be able to
//              compute (d*10^k) * 10^(e-k) with just one roundoff.
//      3. Rather than a bit-at-a-time adjustment of the binary
//              result in the hard case, we use floating-point
//              arithmetic to determine the adjustment to within
//              one bit; only in really hard cases do we need to
//              compute a second residual.
//      4. Because of 3., we don't need a large table of powers of 10
//              for ten-to-e (just some small tables, e.g. of 10^k
//              for 0 <= k <= 22).

static const int kPrivateMemBytes = 2304;
static const int kPrivateMemArraySize =
    ((kPrivateMemBytes + sizeof(double) - 1) / sizeof(double));
static double private_mem[kPrivateMemArraySize];
static double* pmem_next = private_mem;

typedef union {
  double d;
  uint32_t L[2];
} U;

#define word0(x) (x)->L[1]
#define word1(x) (x)->L[0]
#define dval(x) (x)->d

static const int kStrtodDiglim = 40;

// maximum permitted exponent value for strtod; exponents larger than
// kMaxAbsExp in absolute value get truncated to +-kMaxAbsExp.  kMaxAbsExp
// should fit into an int.
static const int kMaxAbsExp = 1100000000;

// Bound on length of pieces of input strings in strtod; specifically,
// this is used to bound the total number of digits ignoring leading zeros and
// the number of digits that follow the decimal point.  Ideally, kMaxDigits
// should satisfy kMaxDigits + 400 < kMaxAbsExp; that ensures that the
// exponent clipping in strtod can't affect the value of the output.
static const unsigned kMaxDigits = 1000000000U;

// kTenPmax = floor(P*log(2)/log(5))
// kBletch = (highest power of 2 < DBL_MAX_10_EXP) / 16
// kQuickMax = floor((P-1)*log(FLT_RADIX)/log(10) - 1)
// kIntMax = floor(P*log(FLT_RADIX)/log(10) - 1)

static_assert(sizeof(double) == 8, "Expected IEEE 754 binary64");
static_assert(DBL_MANT_DIG == 53, "Expected IEEE 754 binary64");
static const int kExpShift = 20;
static const int kExpShift1 = 20;
static const int kExpMsk1 = 0x100000;
static const uint32_t kExpMask = 0x7ff00000;
static const int kP = 53;
static const int kBias = 1023;
static const int kEtiny = -1074;  // smallest denormal is 2**kEtiny
static const uint32_t kExp1 = 0x3ff00000;
static const uint32_t kExp11 = 0x3ff00000;
static const int kEbits = 11;
static const uint32_t kFracMask = 0xfffff;
static const uint32_t kFracMask1 = 0xfffff;
static const int kTenPmax = 22;
static const int kBletch = 0x10;
static const uint32_t kBndryMask = 0xfffff;
static const uint32_t kBndryMask1 = 0xfffff;
static const uint32_t kSignBit = 0x80000000;
static const int kLog2P = 1;
static const int kTiny1 = 1;
static const int kQuickMax = 14;
static const int kIntMax = 14;

static const uint32_t kBig0 =
    (kFracMask1 | kExpMsk1 * (DBL_MAX_EXP + kBias - 1));
static const uint32_t kBig1 = 0xffffffff;

// struct BCinfo is used to pass information from strtod to bigcomp

struct BCinfo {
  int e0, nd, nd0, scale;
};

static const uint32_t kFfffffff = 0xffffffffU;

static const int kKmax = 7;

// struct Bigint is used to represent arbitrary-precision integers.  These
// integers are stored in sign-magnitude format, with the magnitude stored as
// an array of base 2**32 digits.  Bigints are always normalized: if x is a
// Bigint then x->wds >= 1, and either x->wds == 1 or x[wds-1] is nonzero.
//
// The Bigint fields are as follows:
//
//   - next is a header used by Balloc and Bfree to keep track of lists
//       of freed Bigints;  it's also used for the linked list of
//       powers of 5 of the form 5**2**i used by pow5mult.
//   - k indicates which pool this Bigint was allocated from
//   - maxwds is the maximum number of words space was allocated for
//     (usually maxwds == 2**k)
//   - sign is 1 for negative Bigints, 0 for positive.  The sign is unused
//     (ignored on inputs, set to 0 on outputs) in almost all operations
//     involving Bigints: a notable exception is the diff function, which
//     ignores signs on inputs but sets the sign of the output correctly.
//   - wds is the actual number of significant words
//   - x contains the vector of words (digits) for this Bigint, from least
//     significant (x[0]) to most significant (x[wds-1]).

struct Bigint {
  struct Bigint* next;
  int k, maxwds, sign, wds;
  uint32_t x[1];
};

// Memory management: memory is allocated from, and returned to, kKmax+1 pools
// of memory, where pool k (0 <= k <= kKmax) is for Bigints b with b->maxwds ==
// 1 << k.  These pools are maintained as linked lists, with freelist[k]
// pointing to the head of the list for pool k.
//
// On allocation, if there's no free slot in the appropriate pool, std::malloc
// is called to get more memory.  This memory is not returned to the system
// until Python quits.  There's also a private memory pool that's allocated from
// in preference to using std::malloc.
//
// For Bigints with more than (1 << kKmax) digits (which implies at least 1233
// decimal digits), memory is directly allocated using std::malloc, and freed
// using std::free.
//
// XXX: it would be easy to bypass this memory-management system and
// translate each call to Balloc into a call to PyMem_Malloc, and each
// Bfree to PyMem_Free.  Investigate whether this has any significant
// performance on impact.

static Bigint* freelist[kKmax + 1];

// Allocate space for a Bigint with up to 1<<k digits.
static Bigint* Balloc(int k) {
  Bigint* rv;
  if (k <= kKmax && (rv = freelist[k])) {
    freelist[k] = rv->next;
  } else {
    int x = 1 << k;
    unsigned len =
        (sizeof(Bigint) + (x - 1) * sizeof(uint32_t) + sizeof(double) - 1) /
        sizeof(double);
    if (k <= kKmax && pmem_next - private_mem + len <=
                          static_cast<long>(kPrivateMemArraySize)) {
      rv = reinterpret_cast<Bigint*>(pmem_next);
      pmem_next += len;
    } else {
      rv = static_cast<Bigint*>(std::malloc(len * sizeof(double)));
      if (rv == nullptr) {
        return nullptr;
      }
    }
    rv->k = k;
    rv->maxwds = x;
  }
  rv->sign = rv->wds = 0;
  return rv;
}

// Free a Bigint allocated with Balloc.
static void Bfree(Bigint* v) {
  if (v == nullptr) {
    return;
  }

  if (v->k > kKmax) {
    std::free(v);
  } else {
    v->next = freelist[v->k];
    freelist[v->k] = v;
  }
}

static inline void Bcopy(Bigint* dest, const Bigint* src) {
  std::memcpy(&dest->sign, &src->sign,
              src->wds * sizeof(int32_t) + 2 * sizeof(int));
}

// Multiply a Bigint b by m and add a.  Either modifies b in place and returns
// a pointer to the modified b, or Bfrees b and returns a pointer to a copy.
// On failure, return nullptr.  In this case, b will have been already freed.
static Bigint* multadd(Bigint* b, int m, int a)  // multiply by m and add a
{
  int wds = b->wds;
  uint32_t* x = b->x;
  int i = 0;
  uint64_t carry = a;
  do {
    uint64_t y = *x * static_cast<uint64_t>(m) + carry;
    carry = y >> 32;
    *x++ = static_cast<uint32_t>(y & kFfffffff);
  } while (++i < wds);

  if (carry) {
    if (wds >= b->maxwds) {
      Bigint* b1 = Balloc(b->k + 1);
      if (b1 == nullptr) {
        Bfree(b);
        return nullptr;
      }
      Bcopy(b1, b);
      Bfree(b);
      b = b1;
    }
    b->x[wds++] = static_cast<uint32_t>(carry);
    b->wds = wds;
  }
  return b;
}

// convert a string s containing nd decimal digits (possibly containing a
// decimal separator at position nd0, which is ignored) to a Bigint.  This
// function carries on where the parsing code in strtod leaves off: on
// entry, y9 contains the result of converting the first 9 digits.  Returns
// nullptr on failure.
static Bigint* s2b(const char* s, int nd0, int nd, uint32_t y9) {
  int32_t x = (nd + 8) / 9;
  int k = 0;
  for (int32_t y = 1; x > y; y <<= 1, k++) {
  }
  Bigint* b = Balloc(k);
  if (b == nullptr) {
    return nullptr;
  }
  b->x[0] = y9;
  b->wds = 1;

  if (nd <= 9) {
    return b;
  }

  s += 9;
  int i;
  for (i = 9; i < nd0; i++) {
    b = multadd(b, 10, *s++ - '0');
    if (b == nullptr) {
      return nullptr;
    }
  }
  s++;
  for (; i < nd; i++) {
    b = multadd(b, 10, *s++ - '0');
    if (b == nullptr) {
      return nullptr;
    }
  }
  return b;
}

// count leading 0 bits in the 32-bit integer x.
static int hi0bits(uint32_t x) {
  int k = 0;

  if (!(x & 0xffff0000)) {
    k = 16;
    x <<= 16;
  }
  if (!(x & 0xff000000)) {
    k += 8;
    x <<= 8;
  }
  if (!(x & 0xf0000000)) {
    k += 4;
    x <<= 4;
  }
  if (!(x & 0xc0000000)) {
    k += 2;
    x <<= 2;
  }
  if (!(x & 0x80000000)) {
    k++;
    if (!(x & 0x40000000)) {
      return 32;
    }
  }
  return k;
}

// count trailing 0 bits in the 32-bit integer y, and shift y right by that
// number of bits.
static int lo0bits(uint32_t* y) {
  uint32_t x = *y;

  if (x & 7) {
    if (x & 1) {
      return 0;
    }
    if (x & 2) {
      *y = x >> 1;
      return 1;
    }
    *y = x >> 2;
    return 2;
  }
  int k = 0;
  if (!(x & 0xffff)) {
    k = 16;
    x >>= 16;
  }
  if (!(x & 0xff)) {
    k += 8;
    x >>= 8;
  }
  if (!(x & 0xf)) {
    k += 4;
    x >>= 4;
  }
  if (!(x & 0x3)) {
    k += 2;
    x >>= 2;
  }
  if (!(x & 1)) {
    k++;
    x >>= 1;
    if (!x) {
      return 32;
    }
  }
  *y = x;
  return k;
}

// convert a small nonnegative integer to a Bigint
static Bigint* i2b(int i) {
  Bigint* b = Balloc(1);
  if (b == nullptr) {
    return nullptr;
  }
  b->x[0] = i;
  b->wds = 1;
  return b;
}

// multiply two Bigints.  Returns a new Bigint, or nullptr on failure.  Ignores
// the signs of a and b.
static Bigint* mult(Bigint* a, Bigint* b) {
  Bigint* c;
  if ((!a->x[0] && a->wds == 1) || (!b->x[0] && b->wds == 1)) {
    c = Balloc(0);
    if (c == nullptr) {
      return nullptr;
    }
    c->wds = 1;
    c->x[0] = 0;
    return c;
  }

  if (a->wds < b->wds) {
    c = a;
    a = b;
    b = c;
  }
  int k = a->k;
  int wa = a->wds;
  int wb = b->wds;
  int wc = wa + wb;
  if (wc > a->maxwds) {
    k++;
  }
  c = Balloc(k);
  if (c == nullptr) {
    return nullptr;
  }
  uint32_t* x;
  uint32_t* xa;
  for (x = c->x, xa = x + wc; x < xa; x++) {
    *x = 0;
  }
  xa = a->x;
  uint32_t* xae = xa + wa;
  uint32_t* xb = b->x;
  uint32_t* xbe = xb + wb;
  uint32_t* xc0 = c->x;
  uint32_t* xc;
  for (; xb < xbe; xc0++) {
    uint32_t y = *xb++;
    if (y) {
      x = xa;
      xc = xc0;
      uint64_t carry = 0;
      do {
        uint64_t z = *x++ * static_cast<uint64_t>(y) + *xc + carry;
        carry = z >> 32;
        *xc++ = static_cast<uint32_t>(z & kFfffffff);
      } while (x < xae);
      *xc = static_cast<uint32_t>(carry);
    }
  }
  for (xc0 = c->x, xc = xc0 + wc; wc > 0 && !*--xc; --wc) {
  }
  c->wds = wc;
  return c;
}

// p5s is a linked list of powers of 5 of the form 5**(2**i), i >= 2
static Bigint* p5s;

// multiply the Bigint b by 5**k.  Returns a pointer to the result, or nullptr
// on failure; if the returned pointer is distinct from b then the original
// Bigint b will have been Bfree'd.   Ignores the sign of b.
static Bigint* pow5mult(Bigint* b, int k) {
  static const int p05[3] = {5, 25, 125};

  int i = k & 3;
  if (i) {
    b = multadd(b, p05[i - 1], 0);
    if (b == nullptr) {
      return nullptr;
    }
  }

  if (!(k >>= 2)) {
    return b;
  }
  Bigint* p5 = p5s;
  if (!p5) {
    // first time
    p5 = i2b(625);
    if (p5 == nullptr) {
      Bfree(b);
      return nullptr;
    }
    p5s = p5;
    p5->next = nullptr;
  }
  for (;;) {
    if (k & 1) {
      Bigint* b1 = mult(b, p5);
      Bfree(b);
      b = b1;
      if (b == nullptr) {
        return nullptr;
      }
    }
    if (!(k >>= 1)) {
      break;
    }
    Bigint* p51 = p5->next;
    if (!p51) {
      p51 = mult(p5, p5);
      if (p51 == nullptr) {
        Bfree(b);
        return nullptr;
      }
      p51->next = nullptr;
      p5->next = p51;
    }
    p5 = p51;
  }
  return b;
}

// shift a Bigint b left by k bits.  Return a pointer to the shifted result,
// or nullptr on failure.  If the returned pointer is distinct from b then the
// original b will have been Bfree'd.   Ignores the sign of b.
static Bigint* lshift(Bigint* b, int k) {
  if (!k || (!b->x[0] && b->wds == 1)) {
    return b;
  }

  int n = k >> 5;
  int k1 = b->k;
  int n1 = n + b->wds + 1;
  for (int i = b->maxwds; n1 > i; i <<= 1) {
    k1++;
  }
  Bigint* b1 = Balloc(k1);
  if (b1 == nullptr) {
    Bfree(b);
    return nullptr;
  }
  uint32_t* x1 = b1->x;
  for (int i = 0; i < n; i++) {
    *x1++ = 0;
  }
  uint32_t* x = b->x;
  uint32_t* xe = x + b->wds;
  if (k &= 0x1f) {
    k1 = 32 - k;
    uint32_t z = 0;
    do {
      *x1++ = *x << k | z;
      z = *x++ >> k1;
    } while (x < xe);
    if ((*x1 = z)) {
      ++n1;
    }
  } else {
    do {
      *x1++ = *x++;
    } while (x < xe);
  }
  b1->wds = n1 - 1;
  Bfree(b);
  return b1;
}

// Do a three-way compare of a and b, returning -1 if a < b, 0 if a == b and
// 1 if a > b.  Ignores signs of a and b.
static int cmp(Bigint* a, Bigint* b) {
  uint32_t *xa, *xa0, *xb, *xb0;
  int i, j;

  i = a->wds;
  j = b->wds;
  DCHECK(i <= 1 || a->x[i - 1], "cmp called with a->x[a->wds-1] == 0");
  DCHECK(j <= 1 || b->x[j - 1], "cmp called with b->x[b->wds-1] == 0");
  if (i -= j) {
    return i;
  }
  xa0 = a->x;
  xa = xa0 + j;
  xb0 = b->x;
  xb = xb0 + j;
  for (;;) {
    if (*--xa != *--xb) {
      return *xa < *xb ? -1 : 1;
    }
    if (xa <= xa0) {
      break;
    }
  }
  return 0;
}

// Take the difference of Bigints a and b, returning a new Bigint.  Returns
// nullptr on failure.  The signs of a and b are ignored, but the sign of the
// result is set appropriately.
static Bigint* diff(Bigint* a, Bigint* b) {
  Bigint* c;
  int i, wa, wb;
  uint32_t *xa, *xae, *xb, *xbe, *xc;
  uint64_t borrow, y;

  i = cmp(a, b);
  if (!i) {
    c = Balloc(0);
    if (c == nullptr) {
      return nullptr;
    }
    c->wds = 1;
    c->x[0] = 0;
    return c;
  }
  if (i < 0) {
    c = a;
    a = b;
    b = c;
    i = 1;
  } else {
    i = 0;
  }
  c = Balloc(a->k);
  if (c == nullptr) {
    return nullptr;
  }
  c->sign = i;
  wa = a->wds;
  xa = a->x;
  xae = xa + wa;
  wb = b->wds;
  xb = b->x;
  xbe = xb + wb;
  xc = c->x;
  borrow = 0;
  do {
    y = static_cast<uint64_t>(*xa++) - *xb++ - borrow;
    borrow = y >> 32 & uint32_t{1};
    *xc++ = static_cast<uint32_t>(y & kFfffffff);
  } while (xb < xbe);
  while (xa < xae) {
    y = *xa++ - borrow;
    borrow = y >> 32 & uint32_t{1};
    *xc++ = static_cast<uint32_t>(y & kFfffffff);
  }
  while (!*--xc) {
    wa--;
  }
  c->wds = wa;
  return c;
}

// Given a positive normal double x, return the difference between x and the
// next double up.  Doesn't give correct results for subnormals.
static double ulp(U* x) {
  int32_t big_l = (word0(x) & kExpMask) - (kP - 1) * kExpMsk1;
  U u;
  word0(&u) = big_l;
  word1(&u) = 0;
  return dval(&u);
}

// Convert a Bigint to a double plus an exponent
static double b2d(Bigint* a, int* e) {
  uint32_t* xa0 = a->x;
  uint32_t* xa = xa0 + a->wds;
  uint32_t y = *--xa;
  DCHECK(y != 0, "zero y in b2d");
  int k = hi0bits(y);
  *e = 32 - k;
  U d;
  if (k < kEbits) {
    word0(&d) = kExp1 | y >> (kEbits - k);
    uint32_t w = xa > xa0 ? *--xa : 0;
    word1(&d) = y << ((32 - kEbits) + k) | w >> (kEbits - k);
    return dval(&d);
  }
  uint32_t z = xa > xa0 ? *--xa : 0;
  if (k -= kEbits) {
    word0(&d) = kExp1 | y << k | z >> (32 - k);
    y = xa > xa0 ? *--xa : 0;
    word1(&d) = z << k | y >> (32 - k);
  } else {
    word0(&d) = kExp1 | y;
    word1(&d) = z;
  }
  return dval(&d);
}

// Convert a scaled double to a Bigint plus an exponent.  Similar to d2b,
// except that it accepts the scale parameter used in strtod (which
// should be either 0 or 2*kP), and the normalization for the return value is
// different (see below).  On input, d should be finite and nonnegative, and d
// / 2**scale should be exactly representable as an IEEE 754 double.
//
// Returns a Bigint b and an integer e such that
//
//   dval(d) / 2**scale = b * 2**e.
//
// Unlike d2b, b is not necessarily odd: b and e are normalized so
// that either 2**(kP-1) <= b < 2**kP and e >= kEtiny, or b < 2**kP
// and e == kEtiny.  This applies equally to an input of 0.0: in that
// case the return values are b = 0 and e = kEtiny.
//
// The above normalization ensures that for all possible inputs d,
// 2**e gives ulp(d/2**scale).
//
// Returns nullptr on failure.
static Bigint* sd2b(U* d, int scale, int* e) {
  Bigint* b = Balloc(1);
  if (b == nullptr) {
    return nullptr;
  }

  // First construct b and e assuming that scale == 0.
  b->wds = 2;
  b->x[0] = word1(d);
  b->x[1] = word0(d) & kFracMask;
  *e = kEtiny - 1 + static_cast<int>((word0(d) & kExpMask) >> kExpShift);
  if (*e < kEtiny) {
    *e = kEtiny;
  } else {
    b->x[1] |= kExpMsk1;
  }

  // Now adjust for scale, provided that b != 0.
  if (scale && (b->x[0] || b->x[1])) {
    *e -= scale;
    if (*e < kEtiny) {
      scale = kEtiny - *e;
      *e = kEtiny;
      // We can't shift more than kP-1 bits without shifting out a 1.
      DCHECK(0 < scale && scale <= kP - 1, "unexpected scale");
      if (scale >= 32) {
        // The bits shifted out should all be zero.
        DCHECK(b->x[0] == 0, "unexpected bits");
        b->x[0] = b->x[1];
        b->x[1] = 0;
        scale -= 32;
      }
      if (scale) {
        // The bits shifted out should all be zero.
        DCHECK(b->x[0] << (32 - scale) == 0, "unexpected bits");
        b->x[0] = (b->x[0] >> scale) | (b->x[1] << (32 - scale));
        b->x[1] >>= scale;
      }
    }
  }
  // Ensure b is normalized.
  if (!b->x[1]) {
    b->wds = 1;
  }

  return b;
}

// Convert a double to a Bigint plus an exponent.  Return nullptr on failure.
//
// Given a finite nonzero double d, return an odd Bigint b and exponent *e
// such that fabs(d) = b * 2**e.  On return, *bbits gives the number of
// significant bits of b; that is, 2**(*bbits-1) <= b < 2**(*bbits).
//
// If d is zero, then b == 0, *e == -1010, *bbits = 0.
static Bigint* d2b(U* d, int* e, int* bits) {
  Bigint* b = Balloc(1);
  if (b == nullptr) {
    return nullptr;
  }
  uint32_t* x = b->x;

  uint32_t z = word0(d) & kFracMask;
  word0(d) &= 0x7fffffff;  // clear sign bit, which we ignore
  int de = static_cast<int>(word0(d) >> kExpShift);
  if (de) {
    z |= kExpMsk1;
  }
  uint32_t y = word1(d);
  int i;
  int k;
  if (y) {
    if ((k = lo0bits(&y))) {
      x[0] = y | z << (32 - k);
      z >>= k;
    } else {
      x[0] = y;
    }
    i = b->wds = (x[1] = z) ? 2 : 1;
  } else {
    k = lo0bits(&z);
    x[0] = z;
    i = b->wds = 1;
    k += 32;
  }
  if (de) {
    *e = de - kBias - (kP - 1) + k;
    *bits = kP - k;
  } else {
    *e = de - kBias - (kP - 1) + 1 + k;
    *bits = 32 * i - hi0bits(x[i - 1]);
  }
  return b;
}

// Compute the ratio of two Bigints, as a double.  The result may have an
// error of up to 2.5 ulps.
static double ratio(Bigint* a, Bigint* b) {
  U da;
  int ka;
  dval(&da) = b2d(a, &ka);
  U db;
  int kb;
  dval(&db) = b2d(b, &kb);
  int k = ka - kb + 32 * (a->wds - b->wds);
  if (k > 0) {
    word0(&da) += k * kExpMsk1;
  } else {
    k = -k;
    word0(&db) += k * kExpMsk1;
  }
  return dval(&da) / dval(&db);
}

static const double kTens[] = {1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,
                               1e8,  1e9,  1e10, 1e11, 1e12, 1e13, 1e14, 1e15,
                               1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};

static const double kBigTens[] = {1e16, 1e32, 1e64, 1e128, 1e256};
static const double kTinyTens[] = {
    1e-16, 1e-32, 1e-64, 1e-128, 9007199254740992. * 9007199254740992.e-256
    // = 2^106 * 1e-256
};
// The factor of 2^53 in kTinyTens[4] helps us avoid setting the underflow
// flag unnecessarily.  It leads to a song and dance at the end of strtod.
static const int kScaleBit = 0x10;

static const int kKmask = 31;

static int dshift(Bigint* b, int p2) {
  int rv = hi0bits(b->x[b->wds - 1]) - 4;
  if (p2 > 0) {
    rv -= p2;
  }
  return rv & kKmask;
}

// special case of Bigint division.  The quotient is always in the range 0 <=
// quotient < 10, and on entry the divisor big_s is normalized so that its top 4
// bits (28--31) are zero and bit 27 is set.
static int quorem(Bigint* b, Bigint* big_s) {
  int n = big_s->wds;
  DCHECK(b->wds <= n, "oversize b in quorem");
  if (b->wds < n) {
    return 0;
  }
  uint32_t* sx = big_s->x;
  uint32_t* sxe = sx + --n;
  uint32_t* bx = b->x;
  uint32_t* bxe = bx + n;
  uint32_t q = *bxe / (*sxe + 1);  // ensure q <= true quotient.
  DCHECK(q <= 9, "oversized quotient in quorem");
  if (q) {
    uint64_t borrow = 0;
    uint64_t carry = 0;
    do {
      uint64_t ys = *sx++ * static_cast<uint64_t>(q) + carry;
      carry = ys >> 32;
      uint64_t y = *bx - (ys & kFfffffff) - borrow;
      borrow = y >> 32 & uint32_t{1};
      *bx++ = static_cast<uint32_t>(y & kFfffffff);
    } while (sx <= sxe);
    if (!*bxe) {
      bx = b->x;
      while (--bxe > bx && !*bxe) {
        --n;
      }
      b->wds = n;
    }
  }
  if (cmp(b, big_s) >= 0) {
    q++;
    uint64_t borrow = 0;
    uint64_t carry = 0;
    bx = b->x;
    sx = big_s->x;
    do {
      uint64_t ys = *sx++ + carry;
      carry = ys >> 32;
      uint64_t y = *bx - (ys & kFfffffff) - borrow;
      borrow = y >> 32 & uint32_t{1};
      *bx++ = static_cast<uint32_t>(y & kFfffffff);
    } while (sx <= sxe);
    bx = b->x;
    bxe = bx + n;
    if (!*bxe) {
      while (--bxe > bx && !*bxe) {
        --n;
      }
      b->wds = n;
    }
  }
  return q;
}

// sulp(x) is a version of ulp(x) that takes bc.scale into account.
//
// Assuming that x is finite and nonnegative (positive zero is fine
// here) and x / 2^bc.scale is exactly representable as a double,
// sulp(x) is equivalent to 2^bc.scale * ulp(x / 2^bc.scale).
static double sulp(U* x, BCinfo* bc) {
  if (bc->scale &&
      2 * kP + 1 > static_cast<int>((word0(x) & kExpMask) >> kExpShift)) {
    // rv/2^bc->scale is subnormal
    U u;
    word0(&u) = (kP + 2) * kExpMsk1;
    word1(&u) = 0;
    return u.d;
  }
  DCHECK(word0(x) || word1(x), "should not be zero");  // x != 0.0
  return ulp(x);
}

// The bigcomp function handles some hard cases for strtod, for inputs
// with more than kStrtodDiglim digits.  It's called once an initial
// estimate for the double corresponding to the input string has
// already been obtained by the code in strtod.
//
// The bigcomp function is only called after strtod has found a
// double value rv such that either rv or rv + 1ulp represents the
// correctly rounded value corresponding to the original string.  It
// determines which of these two values is the correct one by
// computing the decimal digits of rv + 0.5ulp and comparing them with
// the corresponding digits of s0.
//
// In the following, write dv for the absolute value of the number represented
// by the input string.
//
// Inputs:
//
//   s0 points to the first significant digit of the input string.
//
//   rv is a (possibly scaled) estimate for the closest double value to the
//      value represented by the original input to strtod.  If
//      bc->scale is nonzero, then rv/2^(bc->scale) is the approximation to
//      the input value.
//
//   bc is a struct containing information gathered during the parsing and
//      estimation steps of strtod.  Description of fields follows:
//
//      bc->e0 gives the exponent of the input value, such that dv = (integer
//         given by the bd->nd digits of s0) * 10**e0
//
//      bc->nd gives the total number of significant digits of s0.  It will
//         be at least 1.
//
//      bc->nd0 gives the number of significant digits of s0 before the
//         decimal separator.  If there's no decimal separator, bc->nd0 ==
//         bc->nd.
//
//      bc->scale is the value used to scale rv to avoid doing arithmetic with
//         subnormal values.  It's either 0 or 2*kP (=106).
//
// Outputs:
//
//   On successful exit, rv/2^(bc->scale) is the closest double to dv.
//
//   Returns 0 on success, -1 on failure (e.g., due to a failed malloc call).
static int bigcomp(U* rv, const char* s0, BCinfo* bc) {
  int nd = bc->nd;
  int nd0 = bc->nd0;
  int p5 = nd + bc->e0;
  int p2;
  Bigint* b = sd2b(rv, bc->scale, &p2);
  if (b == nullptr) {
    return -1;
  }

  // record whether the lsb of rv/2^(bc->scale) is odd:  in the exact halfway
  // case, this is used for round to even.
  int odd = b->x[0] & 1;

  // left shift b by 1 bit and or a 1 into the least significant bit;
  // this gives us b * 2**p2 = rv/2^(bc->scale) + 0.5 ulp.
  b = lshift(b, 1);
  if (b == nullptr) {
    return -1;
  }
  b->x[0] |= 1;
  p2--;

  p2 -= p5;
  Bigint* d = i2b(1);
  if (d == nullptr) {
    Bfree(b);
    return -1;
  }
  // Arrange for convenient computation of quotients:
  // shift left if necessary so divisor has 4 leading 0 bits.
  if (p5 > 0) {
    d = pow5mult(d, p5);
    if (d == nullptr) {
      Bfree(b);
      return -1;
    }
  } else if (p5 < 0) {
    b = pow5mult(b, -p5);
    if (b == nullptr) {
      Bfree(d);
      return -1;
    }
  }
  int b2;
  int d2;
  if (p2 > 0) {
    b2 = p2;
    d2 = 0;
  } else {
    b2 = 0;
    d2 = -p2;
  }
  int i = dshift(d, d2);
  if ((b2 += i) > 0) {
    b = lshift(b, b2);
    if (b == nullptr) {
      Bfree(d);
      return -1;
    }
  }
  if ((d2 += i) > 0) {
    d = lshift(d, d2);
    if (d == nullptr) {
      Bfree(b);
      return -1;
    }
  }

  // Compare s0 with b/d: set dd to -1, 0, or 1 according as s0 < b/d, s0 ==
  // b/d, or s0 > b/d.  Here the digits of s0 are thought of as representing
  // a number in the range [0.1, 1).
  int dd;
  if (cmp(b, d) >= 0) {  // b/d >= 1
    dd = -1;
  } else {
    i = 0;
    for (;;) {
      b = multadd(b, 10, 0);
      if (b == nullptr) {
        Bfree(d);
        return -1;
      }
      dd = s0[i < nd0 ? i : i + 1] - '0' - quorem(b, d);
      i++;

      if (dd) {
        break;
      }
      if (!b->x[0] && b->wds == 1) {
        // b/d == 0
        dd = i < nd;
        break;
      }
      if (!(i < nd)) {
        // b/d != 0, but digits of s0 exhausted
        dd = -1;
        break;
      }
    }
  }
  Bfree(b);
  Bfree(d);
  if (dd > 0 || (dd == 0 && odd)) {
    dval(rv) += sulp(rv, bc);
  }
  return 0;
}

static double strtod(const char* s00, char** se) {
  int bb2, bb5, bbe, bd2, bd5, bs2, dsign, e1, error;
  int i, j, k, nd, nd0, odd;
  double aadj, aadj1;
  U aadj2, adj, rv0;
  uint32_t y, z;
  BCinfo bc;
  Bigint *bb, *bb1, *bd, *bd0, *bs, *delta;

  U rv;
  dval(&rv) = 0.;

  // Start parsing.
  const char* s = s00;
  int c = *s;

  // Parse optional sign, if present.
  bool sign = false;
  switch (c) {
    case '-':
      sign = true;
      // fall through
    case '+':
      c = *++s;
  }

  // Skip leading zeros: lz is true iff there were leading zeros.
  const char* s1 = s;
  while (c == '0') {
    c = *++s;
  }
  bool lz = s != s1;

  // Point s0 at the first nonzero digit (if any).  fraclen will be the
  // number of digits between the decimal point and the end of the
  // digit string.  ndigits will be the total number of digits ignoring
  // leading zeros.
  const char* s0 = s1 = s;
  while ('0' <= c && c <= '9') {
    c = *++s;
  }
  size_t ndigits = s - s1;
  size_t fraclen = 0;

  // Parse decimal point and following digits.
  if (c == '.') {
    c = *++s;
    if (!ndigits) {
      s1 = s;
      while (c == '0') {
        c = *++s;
      }
      lz = lz || s != s1;
      fraclen += (s - s1);
      s0 = s;
    }
    s1 = s;
    while ('0' <= c && c <= '9') {
      c = *++s;
    }
    ndigits += s - s1;
    fraclen += s - s1;
  }

  // Now lz is true if and only if there were leading zero digits, and
  // ndigits gives the total number of digits ignoring leading zeros.  A
  // valid input must have at least one digit.
  if (!ndigits && !lz) {
    if (se) {
      *se = const_cast<char*>(s00);
    }
    return 0.0;
  }

  // Range check ndigits and fraclen to make sure that they, and values
  // computed with them, can safely fit in an int.
  if (ndigits > kMaxDigits || fraclen > kMaxDigits) {
    if (se) {
      *se = const_cast<char*>(s00);
    }
    return 0.0;
  }
  nd = static_cast<int>(ndigits);
  nd0 = nd - static_cast<int>(fraclen);

  // Parse exponent.
  int e = 0;
  if (c == 'e' || c == 'E') {
    s00 = s;
    c = *++s;

    // Exponent sign.
    bool esign = false;
    switch (c) {
      case '-':
        esign = true;
        // fall through
      case '+':
        c = *++s;
    }

    // Skip zeros.  lz is true iff there are leading zeros.
    s1 = s;
    while (c == '0') {
      c = *++s;
    }
    lz = s != s1;

    // Get absolute value of the exponent.
    s1 = s;
    uint32_t abs_exp = 0;
    while ('0' <= c && c <= '9') {
      abs_exp = 10 * abs_exp + (c - '0');
      c = *++s;
    }

    // abs_exp will be correct modulo 2**32.  But 10**9 < 2**32, so if
    // there are at most 9 significant exponent digits then overflow is
    // impossible.
    if (s - s1 > 9 || abs_exp > kMaxAbsExp) {
      e = kMaxAbsExp;
    } else {
      e = static_cast<int>(abs_exp);
    }
    if (esign) {
      e = -e;
    }

    // A valid exponent must have at least one digit.
    if (s == s1 && !lz) {
      s = s00;
    }
  }

  // Adjust exponent to take into account position of the point.
  e -= nd - nd0;
  if (nd0 <= 0) {
    nd0 = nd;
  }

  // Finished parsing.  Set se to indicate how far we parsed
  if (se) {
    *se = const_cast<char*>(s);
  }

  // If all digits were zero, exit with return value +-0.0.  Otherwise,
  // strip trailing zeros: scan back until we hit a nonzero digit.
  if (!nd) {
    goto ret;
  }
  for (i = nd; i > 0;) {
    --i;
    if (s0[i < nd0 ? i : i + 1] != '0') {
      ++i;
      break;
    }
  }
  e += nd - i;
  nd = i;
  if (nd0 > nd) {
    nd0 = nd;
  }

  // Summary of parsing results.  After parsing, and dealing with zero
  // inputs, we have values s0, nd0, nd, e, sign, where:
  //
  //  - s0 points to the first significant digit of the input string
  //
  //  - nd is the total number of significant digits (here, and
  //    below, 'significant digits' means the set of digits of the
  //    significand of the input that remain after ignoring leading
  //    and trailing zeros).
  //
  //  - nd0 indicates the position of the decimal point, if present; it
  //    satisfies 1 <= nd0 <= nd.  The nd significant digits are in
  //    s0[0:nd0] and s0[nd0+1:nd+1] using the usual Python half-open slice
  //    notation.  (If nd0 < nd, then s0[nd0] contains a '.'  character; if
  //    nd0 == nd, then s0[nd0] could be any non-digit character.)
  //
  //  - e is the adjusted exponent: the absolute value of the number
  //    represented by the original input string is n * 10**e, where
  //    n is the integer represented by the concatenation of
  //    s0[0:nd0] and s0[nd0+1:nd+1]
  //
  //  - sign gives the sign of the input:  1 for negative, 0 for positive
  //
  //  - the first and last significant digits are nonzero

  // put first DBL_DIG+1 digits into integer y and z.
  //
  //  - y contains the value represented by the first min(9, nd)
  //    significant digits
  //
  //  - if nd > 9, z contains the value represented by significant digits
  //    with indices in [9, min(16, nd)).  So y * 10**(min(16, nd) - 9) + z
  //    gives the value represented by the first min(16, nd) sig. digits.

  bc.e0 = e1 = e;
  y = z = 0;
  for (i = 0; i < nd; i++) {
    if (i < 9) {
      y = 10 * y + s0[i < nd0 ? i : i + 1] - '0';
    } else if (i < DBL_DIG + 1) {
      z = 10 * z + s0[i < nd0 ? i : i + 1] - '0';
    } else {
      break;
    }
  }

  k = nd < DBL_DIG + 1 ? nd : DBL_DIG + 1;
  dval(&rv) = y;
  if (k > 9) {
    dval(&rv) = kTens[k - 9] * dval(&rv) + z;
  }
  bd0 = nullptr;
  if (nd <= DBL_DIG && FLT_ROUNDS == 1) {
    if (!e) {
      goto ret;
    }
    if (e > 0) {
      if (e <= kTenPmax) {
        dval(&rv) *= kTens[e];
        goto ret;
      }
      i = DBL_DIG - nd;
      if (e <= kTenPmax + i) {
        // A fancier test would sometimes let us do
        // this for larger i values.
        e -= i;
        dval(&rv) *= kTens[i];
        dval(&rv) *= kTens[e];
        goto ret;
      }
    } else if (e >= -kTenPmax) {
      dval(&rv) /= kTens[-e];
      goto ret;
    }
  }
  e1 += nd - k;

  bc.scale = 0;

  // Get starting approximation = rv * 10**e1

  if (e1 > 0) {
    if ((i = e1 & 15)) {
      dval(&rv) *= kTens[i];
    }
    if (e1 &= ~15) {
      if (e1 > DBL_MAX_10_EXP) {
        goto ovfl;
      }
      e1 >>= 4;
      for (j = 0; e1 > 1; j++, e1 >>= 1) {
        if (e1 & 1) {
          dval(&rv) *= kBigTens[j];
        }
      }
      // The last multiplication could overflow.
      word0(&rv) -= kP * kExpMsk1;
      dval(&rv) *= kBigTens[j];
      if ((z = word0(&rv) & kExpMask) > kExpMsk1 * (DBL_MAX_EXP + kBias - kP)) {
        goto ovfl;
      }
      if (z > kExpMsk1 * (DBL_MAX_EXP + kBias - 1 - kP)) {
        // set to largest number (Can't trust DBL_MAX)
        word0(&rv) = kBig0;
        word1(&rv) = kBig1;
      } else {
        word0(&rv) += kP * kExpMsk1;
      }
    }
  } else if (e1 < 0) {
    // The input decimal value lies in [10**e1, 10**(e1+16)).

    // If e1 <= -512, underflow immediately.
    // If e1 <= -256, set bc.scale to 2*kP.

    // So for input value < 1e-256, bc.scale is always set;
    // for input value >= 1e-240, bc.scale is never set.
    // For input values in [1e-256, 1e-240), bc.scale may or may
    // not be set.

    e1 = -e1;
    if ((i = e1 & 15)) {
      dval(&rv) /= kTens[i];
    }
    if (e1 >>= 4) {
      if (e1 >= 1 << ARRAYSIZE(kBigTens)) {
        goto undfl;
      }
      if (e1 & kScaleBit) {
        bc.scale = 2 * kP;
      }
      for (j = 0; e1 > 0; j++, e1 >>= 1) {
        if (e1 & 1) {
          dval(&rv) *= kTinyTens[j];
        }
      }
      if (bc.scale &&
          (j = 2 * kP + 1 - ((word0(&rv) & kExpMask) >> kExpShift)) > 0) {
        // scaled rv is denormal; clear j low bits
        if (j >= 32) {
          word1(&rv) = 0;
          if (j >= 53) {
            word0(&rv) = (kP + 2) * kExpMsk1;
          } else {
            word0(&rv) &= 0xffffffff << (j - 32);
          }
        } else {
          word1(&rv) &= 0xffffffff << j;
        }
      }
      if (!dval(&rv)) {
        goto undfl;
      }
    }
  }

  // Now the hard part -- adjusting rv to the correct value.

  // Put digits into bd: true value = bd * 10^e

  bc.nd = nd;
  bc.nd0 = nd0;  // Only needed if nd > kStrtodDiglim, but done here
                 // to silence an erroneous warning about bc.nd0
                 // possibly not being initialized.
  if (nd > kStrtodDiglim) {
    // ASSERT(kStrtodDiglim >= 18); 18 == one more than the
    // minimum number of decimal digits to distinguish double values
    // in IEEE arithmetic.

    // Truncate input to 18 significant digits, then discard any trailing
    // zeros on the result by updating nd, nd0, e and y suitably. (There's
    // no need to update z; it's not reused beyond this point.)
    for (i = 18; i > 0;) {
      // scan back until we hit a nonzero digit.  significant digit 'i'
      // is s0[i] if i < nd0, s0[i+1] if i >= nd0.
      --i;
      if (s0[i < nd0 ? i : i + 1] != '0') {
        ++i;
        break;
      }
    }
    e += nd - i;
    nd = i;
    if (nd0 > nd) {
      nd0 = nd;
    }
    if (nd < 9) {  // must recompute y
      y = 0;
      for (i = 0; i < nd0; ++i) {
        y = 10 * y + s0[i] - '0';
      }
      for (; i < nd; ++i) {
        y = 10 * y + s0[i + 1] - '0';
      }
    }
  }
  bd0 = s2b(s0, nd0, nd, y);
  if (bd0 == nullptr) {
    goto failed_malloc;
  }

  // Notation for the comments below.  Write:
  //
  //   - dv for the absolute value of the number represented by the original
  //     decimal input string.
  //
  //   - if we've truncated dv, write tdv for the truncated value.
  //     Otherwise, set tdv == dv.
  //
  //   - srv for the quantity rv/2^bc.scale; so srv is the current binary
  //     approximation to tdv (and dv).  It should be exactly representable
  //     in an IEEE 754 double.
  for (;;) {
    // This is the main correction loop for strtod.
    //
    // We've got a decimal value tdv, and a floating-point approximation
    // srv=rv/2^bc.scale to tdv.  The aim is to determine whether srv is
    // close enough (i.e., within 0.5 ulps) to tdv, and to compute a new
    // approximation if not.
    //
    // To determine whether srv is close enough to tdv, compute integers
    // bd, bb and bs proportional to tdv, srv and 0.5 ulp(srv)
    // respectively, and then use integer arithmetic to determine whether
    // |tdv - srv| is less than, equal to, or greater than 0.5 ulp(srv).

    bd = Balloc(bd0->k);
    if (bd == nullptr) {
      Bfree(bd0);
      goto failed_malloc;
    }
    Bcopy(bd, bd0);
    bb = sd2b(&rv, bc.scale, &bbe);  // srv = bb * 2^bbe
    if (bb == nullptr) {
      Bfree(bd);
      Bfree(bd0);
      goto failed_malloc;
    }
    // Record whether lsb of bb is odd, in case we need this
    // for the round-to-even step later.
    odd = bb->x[0] & 1;

    // tdv = bd * 10**e;  srv = bb * 2**bbe
    bs = i2b(1);
    if (bs == nullptr) {
      Bfree(bb);
      Bfree(bd);
      Bfree(bd0);
      goto failed_malloc;
    }

    if (e >= 0) {
      bb2 = bb5 = 0;
      bd2 = bd5 = e;
    } else {
      bb2 = bb5 = -e;
      bd2 = bd5 = 0;
    }
    if (bbe >= 0) {
      bb2 += bbe;
    } else {
      bd2 -= bbe;
    }
    bs2 = bb2;
    bb2++;
    bd2++;

    // At this stage bd5 - bb5 == e == bd2 - bb2 + bbe, bb2 - bs2 == 1,
    // and bs == 1, so:
    //
    //    tdv == bd * 10**e = bd * 2**(bbe - bb2 + bd2) * 5**(bd5 - bb5)
    //    srv == bb * 2**bbe = bb * 2**(bbe - bb2 + bb2)
    //    0.5 ulp(srv) == 2**(bbe-1) = bs * 2**(bbe - bb2 + bs2)
    //
    // It follows that:
    //
    //    M * tdv = bd * 2**bd2 * 5**bd5
    //    M * srv = bb * 2**bb2 * 5**bb5
    //    M * 0.5 ulp(srv) = bs * 2**bs2 * 5**bb5
    //
    // for some constant M.  (Actually, M == 2**(bb2 - bbe) * 5**bb5, but
    // this fact is not needed below.)

    // Remove factor of 2**i, where i = min(bb2, bd2, bs2).
    i = bb2 < bd2 ? bb2 : bd2;
    if (i > bs2) {
      i = bs2;
    }
    if (i > 0) {
      bb2 -= i;
      bd2 -= i;
      bs2 -= i;
    }

    // Scale bb, bd, bs by the appropriate powers of 2 and 5.
    if (bb5 > 0) {
      bs = pow5mult(bs, bb5);
      if (bs == nullptr) {
        Bfree(bb);
        Bfree(bd);
        Bfree(bd0);
        goto failed_malloc;
      }
      bb1 = mult(bs, bb);
      Bfree(bb);
      bb = bb1;
      if (bb == nullptr) {
        Bfree(bs);
        Bfree(bd);
        Bfree(bd0);
        goto failed_malloc;
      }
    }
    if (bb2 > 0) {
      bb = lshift(bb, bb2);
      if (bb == nullptr) {
        Bfree(bs);
        Bfree(bd);
        Bfree(bd0);
        goto failed_malloc;
      }
    }
    if (bd5 > 0) {
      bd = pow5mult(bd, bd5);
      if (bd == nullptr) {
        Bfree(bb);
        Bfree(bs);
        Bfree(bd0);
        goto failed_malloc;
      }
    }
    if (bd2 > 0) {
      bd = lshift(bd, bd2);
      if (bd == nullptr) {
        Bfree(bb);
        Bfree(bs);
        Bfree(bd0);
        goto failed_malloc;
      }
    }
    if (bs2 > 0) {
      bs = lshift(bs, bs2);
      if (bs == nullptr) {
        Bfree(bb);
        Bfree(bd);
        Bfree(bd0);
        goto failed_malloc;
      }
    }

    // Now bd, bb and bs are scaled versions of tdv, srv and 0.5 ulp(srv),
    // respectively.  Compute the difference |tdv - srv|, and compare
    // with 0.5 ulp(srv).

    delta = diff(bb, bd);
    if (delta == nullptr) {
      Bfree(bb);
      Bfree(bs);
      Bfree(bd);
      Bfree(bd0);
      goto failed_malloc;
    }
    dsign = delta->sign;
    delta->sign = 0;
    i = cmp(delta, bs);
    if (bc.nd > nd && i <= 0) {
      if (dsign) {
        break;  // Must use bigcomp().
      }

      // Here rv overestimates the truncated decimal value by at most
      // 0.5 ulp(rv).  Hence rv either overestimates the true decimal
      // value by <= 0.5 ulp(rv), or underestimates it by some small
      // amount (< 0.1 ulp(rv)); either way, rv is within 0.5 ulps of
      // the true decimal value, so it's possible to exit.
      //
      // Exception: if scaled rv is a normal exact power of 2, but not
      // DBL_MIN, then rv - 0.5 ulp(rv) takes us all the way down to the
      // next double, so the correctly rounded result is either rv - 0.5
      // ulp(rv) or rv; in this case, use bigcomp to distinguish.

      if (!word1(&rv) && !(word0(&rv) & kBndryMask)) {
        // rv can't be 0, since it's an overestimate for some
        // nonzero value.  So rv is a normal power of 2.
        j = static_cast<int>((word0(&rv) & kExpMask) >> kExpShift);
        // rv / 2^bc.scale = 2^(j - 1023 - bc.scale); use bigcomp if
        // rv / 2^bc.scale >= 2^-1021.
        if (j - bc.scale >= 2) {
          dval(&rv) -= 0.5 * sulp(&rv, &bc);
          break;  // Use bigcomp.
        }
      }

      {
        bc.nd = nd;
        i = -1;  // Discarded digits make delta smaller.
      }
    }

    if (i < 0) {
      // Error is less than half an ulp -- check for
      // special case of mantissa a power of two.
      if (dsign || word1(&rv) || word0(&rv) & kBndryMask ||
          (word0(&rv) & kExpMask) <= (2 * kP + 1) * kExpMsk1) {
        break;
      }
      if (!delta->x[0] && delta->wds <= 1) {
        // exact result
        break;
      }
      delta = lshift(delta, kLog2P);
      if (delta == nullptr) {
        Bfree(bb);
        Bfree(bs);
        Bfree(bd);
        Bfree(bd0);
        goto failed_malloc;
      }
      if (cmp(delta, bs) > 0) {
        goto drop_down;
      }
      break;
    }
    if (i == 0) {
      // exactly half-way between
      if (dsign) {
        if ((word0(&rv) & kBndryMask1) == kBndryMask1 &&
            word1(&rv) ==
                ((bc.scale && (y = word0(&rv) & kExpMask) <= 2 * kP * kExpMsk1)
                     ? (0xffffffff &
                        (0xffffffff << (2 * kP + 1 - (y >> kExpShift))))
                     : 0xffffffff)) {
          // boundary case -- increment exponent
          word0(&rv) = (word0(&rv) & kExpMask) + kExpMsk1;
          word1(&rv) = 0;
          // dsign = 0;
          break;
        }
      } else if (!(word0(&rv) & kBndryMask) && !word1(&rv)) {
      drop_down:
        // boundary case -- decrement exponent
        if (bc.scale) {
          int32_t big_l = word0(&rv) & kExpMask;
          if (big_l <= (2 * kP + 1) * kExpMsk1) {
            if (big_l > (kP + 2) * kExpMsk1) {  // round even ==>
              // accept rv
              break;
            }
            // rv = smallest denormal
            if (bc.nd > nd) {
              break;
            }
            goto undfl;
          }
        }
        int32_t big_l = (word0(&rv) & kExpMask) - kExpMsk1;
        word0(&rv) = big_l | kBndryMask1;
        word1(&rv) = 0xffffffff;
        break;
      }
      if (!odd) {
        break;
      }
      if (dsign) {
        dval(&rv) += sulp(&rv, &bc);
      } else {
        dval(&rv) -= sulp(&rv, &bc);
        if (!dval(&rv)) {
          if (bc.nd > nd) {
            break;
          }
          goto undfl;
        }
      }
      // dsign = 1 - dsign;
      break;
    }
    if ((aadj = ratio(delta, bs)) <= 2.) {
      if (dsign) {
        aadj = aadj1 = 1.;
      } else if (word1(&rv) || word0(&rv) & kBndryMask) {
        if (word1(&rv) == kTiny1 && !word0(&rv)) {
          if (bc.nd > nd) {
            break;
          }
          goto undfl;
        }
        aadj = 1.;
        aadj1 = -1.;
      } else {
        // special case -- power of FLT_RADIX to be
        // rounded down...

        if (aadj < 2. / FLT_RADIX) {
          aadj = 1. / FLT_RADIX;
        } else {
          aadj *= 0.5;
        }
        aadj1 = -aadj;
      }
    } else {
      aadj *= 0.5;
      aadj1 = dsign ? aadj : -aadj;
      if (FLT_ROUNDS == 0) {
        aadj1 += 0.5;
      }
    }
    y = word0(&rv) & kExpMask;

    // Check for overflow

    if (y == kExpMsk1 * (DBL_MAX_EXP + kBias - 1)) {
      dval(&rv0) = dval(&rv);
      word0(&rv) -= kP * kExpMsk1;
      adj.d = aadj1 * ulp(&rv);
      dval(&rv) += adj.d;
      if ((word0(&rv) & kExpMask) >= kExpMsk1 * (DBL_MAX_EXP + kBias - kP)) {
        if (word0(&rv0) == kBig0 && word1(&rv0) == kBig1) {
          Bfree(bb);
          Bfree(bd);
          Bfree(bs);
          Bfree(bd0);
          Bfree(delta);
          goto ovfl;
        }
        word0(&rv) = kBig0;
        word1(&rv) = kBig1;
        goto cont;
      } else {
        word0(&rv) += kP * kExpMsk1;
      }
    } else {
      if (bc.scale && y <= 2 * kP * kExpMsk1) {
        if (aadj <= 0x7fffffff) {
          if ((z = static_cast<uint32_t>(aadj)) <= 0) {
            z = 1;
          }
          aadj = z;
          aadj1 = dsign ? aadj : -aadj;
        }
        dval(&aadj2) = aadj1;
        word0(&aadj2) += (2 * kP + 1) * kExpMsk1 - y;
        aadj1 = dval(&aadj2);
      }
      adj.d = aadj1 * ulp(&rv);
      dval(&rv) += adj.d;
    }
    z = word0(&rv) & kExpMask;
    if (bc.nd == nd) {
      if (!bc.scale) {
        if (y == z) {
          // Can we stop now?
          int32_t big_l = static_cast<int32_t>(aadj);
          aadj -= big_l;
          // The tolerances below are conservative.
          if (dsign || word1(&rv) || word0(&rv) & kBndryMask) {
            if (aadj < .4999999 || aadj > .5000001) {
              break;
            }
          } else if (aadj < .4999999 / FLT_RADIX) {
            break;
          }
        }
      }
    }
  cont:
    Bfree(bb);
    Bfree(bd);
    Bfree(bs);
    Bfree(delta);
  }
  Bfree(bb);
  Bfree(bd);
  Bfree(bs);
  Bfree(bd0);
  Bfree(delta);
  if (bc.nd > nd) {
    error = bigcomp(&rv, s0, &bc);
    if (error) {
      goto failed_malloc;
    }
  }

  if (bc.scale) {
    word0(&rv0) = kExp1 - 2 * kP * kExpMsk1;
    word1(&rv0) = 0;
    dval(&rv) *= dval(&rv0);
  }

ret:
  return sign ? -dval(&rv) : dval(&rv);

failed_malloc:
  errno = ENOMEM;
  return -1.0;

undfl:
  return sign ? -0.0 : 0.0;

ovfl:
  errno = ERANGE;
  return sign ? -HUGE_VAL : HUGE_VAL;
}

static char* rv_alloc(int i) {
  int j = sizeof(uint32_t);
  int k;
  for (k = 0; sizeof(Bigint) - sizeof(uint32_t) - sizeof(int) + j <=
              static_cast<unsigned>(i);
       j <<= 1) {
    k++;
  }
  int* r = reinterpret_cast<int*>(Balloc(k));
  if (r == nullptr) {
    return nullptr;
  }
  *r = k;
  return reinterpret_cast<char*>(r + 1);
}

static char* nrv_alloc(const char* s, char** rve, int n) {
  char* rv = rv_alloc(n);
  if (rv == nullptr) {
    return nullptr;
  }
  char* t = rv;
  while ((*t = *s++)) {
    t++;
  }
  if (rve) {
    *rve = t;
  }
  return rv;
}

// freedtoa(s) must be used to free values s returned by dtoa
// when MULTIPLE_THREADS is #defined.  It should be used in all cases,
// but for consistency with earlier versions of dtoa, it is optional
// when MULTIPLE_THREADS is not defined.
void freedtoa(char* s) {
  Bigint* b = reinterpret_cast<Bigint*>(reinterpret_cast<int*>(s) - 1);
  b->maxwds = 1 << (b->k = *reinterpret_cast<int*>(b));
  Bfree(b);
}

// dtoa for IEEE arithmetic (dmg): convert double to ASCII string.
//
// Inspired by "How to Print Floating-Point Numbers Accurately" by
// Guy L. Steele, Jr. and Jon L. White [Proc. ACM SIGPLAN '90, pp. 112-126].
//
// Modifications:
//      1. Rather than iterating, we use a simple numeric overestimate
//         to determine k = floor(log10(d)).  We scale relevant
//         quantities using O(log2(k)) rather than O(k) multiplications.
//      2. For some modes > 2 (corresponding to ecvt and fcvt), we don't
//         try to generate digits strictly left to right.  Instead, we
//         compute with fewer bits and propagate the carry if necessary
//         when rounding the final digit up.  This is often faster.
//      3. Under the assumption that input will be rounded nearest,
//         mode 0 renders 1e23 as 1e23 rather than 9.999999999999999e22.
//         That is, we allow equality in stopping tests when the
//         round-nearest rule will give the same floating-point value
//         as would satisfaction of the stopping test with strict
//         inequality.
//      4. We remove common factors of powers of 2 from relevant
//         quantities.
//      5. When converting floating-point integers less than 1e16,
//         we use floating-point arithmetic rather than resorting
//         to multiple-precision integers.
//      6. When asked to produce fewer than 15 digits, we first try
//         to get by with floating-point arithmetic; we resort to
//         multiple-precision integer arithmetic only if we cannot
//         guarantee that the floating-point calculation has given
//         the correctly rounded result.  For k requested digits and
//         "uniformly" distributed input, the probability is
//         something like 10^(k-15) that we must resort to the int32_t
//         calculation.
//
// Additional notes (METD): (1) returns nullptr on failure.  (2) to avoid
// memory leakage, a successful call to dtoa should always be matched
// by a call to freedtoa.
static char* dtoa(double dd, int mode, int ndigits, int* decpt, int* sign,
                  char** rve) {
  //  Arguments ndigits, decpt, sign are similar to those
  //  of ecvt and fcvt; trailing zeros are suppressed from
  //  the returned string.  If not null, *rve is set to point
  //  to the end of the return value.  If d is +-Infinity or NaN,
  //  then *decpt is set to 9999.
  //
  //  mode:
  //  0 ==> shortest string that yields d when read in
  //  and rounded to nearest.
  //  1 ==> like 0, but with Steele & White stopping rule;
  //  e.g. with IEEE P754 arithmetic , mode 0 gives
  //  1e23 whereas mode 1 gives 9.999999999999999e22.
  //  2 ==> max(1,ndigits) significant digits.  This gives a
  //  return value similar to that of ecvt, except
  //  that trailing zeros are suppressed.
  //  3 ==> through ndigits past the decimal point.  This
  //  gives a return value similar to that from fcvt,
  //  except that trailing zeros are suppressed, and
  //  ndigits can be negative.
  //  4,5 ==> similar to 2 and 3, respectively, but (in
  //  round-nearest mode) with the tests of mode 0 to
  //  possibly return a shorter string that rounds to d.
  //  With IEEE arithmetic and compilation with
  //  -DHonor_FLT_ROUNDS, modes 4 and 5 behave the same
  //  as modes 2 and 3 when FLT_ROUNDS != 1.
  //  6-9 ==> Debugging modes similar to mode - 4:  don't try
  //  fast floating-point estimate (if applicable).
  //
  //  Values of mode other than 0-9 are treated as mode 0.
  //
  //  Sufficient space is allocated to the return value
  //  to hold the suppressed trailing zeros.

  int b2, b5, be, dig, i, ieps, ilim, ilim0, ilim1, j, j1, k, k0, k_check,
      leftright, m2, m5, s2, s5, spec_case, try_quick;
  U d2, eps;
  char* s;
  double ds;

  // set pointers to nullptr, to silence gcc compiler warnings and make
  // cleanup easier on error
  Bigint* mlo = nullptr;
  Bigint* mhi = nullptr;
  Bigint* big_s = nullptr;
  char* s0 = nullptr;

  U u;
  u.d = dd;
  if (word0(&u) & kSignBit) {
    // set sign for everything, including 0's and NaNs
    *sign = 1;
    word0(&u) &= ~kSignBit;  // clear sign bit
  } else {
    *sign = 0;
  }

  // quick return for Infinities, NaNs and zeros
  if ((word0(&u) & kExpMask) == kExpMask) {
    // Infinity or NaN
    *decpt = 9999;
    if (!word1(&u) && !(word0(&u) & 0xfffff)) {
      return nrv_alloc("Infinity", rve, 8);
    }
    return nrv_alloc("NaN", rve, 3);
  }
  if (!dval(&u)) {
    *decpt = 1;
    return nrv_alloc("0", rve, 1);
  }

  // compute k = floor(log10(d)).  The computation may leave k
  // one too large, but should never leave k too small.
  int bbits;
  Bigint* b = d2b(&u, &be, &bbits);
  if (b == nullptr) {
    goto failed_malloc;
  }
  bool denorm;
  i = static_cast<int>(word0(&u) >> kExpShift1 & (kExpMask >> kExpShift1));
  if (i) {
    dval(&d2) = dval(&u);
    word0(&d2) &= kFracMask1;
    word0(&d2) |= kExp11;

    // log(x)       ~=~ log(1.5) + (x-1.5)/1.5
    // log10(x)      =  log(x) / log(10)
    //              ~=~ log(1.5)/log(10) + (x-1.5)/(1.5*log(10))
    // log10(d) = (i-kBias)*log(2)/log(10) + log10(d2)
    //
    // This suggests computing an approximation k to log10(d) by
    //
    // k = (i - kBias)*0.301029995663981
    //      + ( (d2-1.5)*0.289529654602168 + 0.176091259055681 );
    //
    // We want k to be too large rather than too small.
    // The error in the first-order Taylor series approximation
    // is in our favor, so we just round up the constant enough
    // to compensate for any error in the multiplication of
    // (i - kBias) by 0.301029995663981; since |i - kBias| <= 1077,
    // and 1077 * 0.30103 * 2^-52 ~=~ 7.2e-14,
    // adding 1e-13 to the constant term more than suffices.
    // Hence we adjust the constant term to 0.1760912590558.
    // (We could get a more accurate k by invoking log10,
    //  but this is probably not worthwhile.)

    i -= kBias;
    denorm = false;
  } else {
    // d is denormalized

    i = bbits + be + (kBias + (kP - 1) - 1);
    uint32_t x = i > 32 ? word0(&u) << (64 - i) | word1(&u) >> (i - 32)
                        : word1(&u) << (32 - i);
    dval(&d2) = x;
    word0(&d2) -= 31 * kExpMsk1;  // adjust exponent
    i -= (kBias + (kP - 1) - 1) + 1;
    denorm = true;
  }
  ds = (dval(&d2) - 1.5) * 0.289529654602168 + 0.1760912590558 +
       i * 0.301029995663981;
  k = static_cast<int>(ds);
  if (ds < 0. && ds != k) k--;  // want k = floor(ds)
  k_check = 1;
  if (k >= 0 && k <= kTenPmax) {
    if (dval(&u) < kTens[k]) {
      k--;
    }
    k_check = 0;
  }
  j = bbits - i - 1;
  if (j >= 0) {
    b2 = 0;
    s2 = j;
  } else {
    b2 = -j;
    s2 = 0;
  }
  if (k >= 0) {
    b5 = 0;
    s5 = k;
    s2 += k;
  } else {
    b2 -= k;
    b5 = -k;
    s5 = 0;
  }
  if (mode < 0 || mode > 9) {
    mode = 0;
  }

  try_quick = 1;

  if (mode > 5) {
    mode -= 4;
    try_quick = 0;
  }
  leftright = 1;
  ilim = ilim1 = -1;  // Values for cases 0 and 1; done here to
  // silence erroneous "gcc -Wall" warning.
  switch (mode) {
    case 0:
    case 1:
      i = 18;
      ndigits = 0;
      break;
    case 2:
      leftright = 0;
      // fall through
    case 4:
      if (ndigits <= 0) {
        ndigits = 1;
      }
      ilim = ilim1 = i = ndigits;
      break;
    case 3:
      leftright = 0;
      // fall through
    case 5:
      i = ndigits + k + 1;
      ilim = i;
      ilim1 = i - 1;
      if (i <= 0) {
        i = 1;
      }
  }
  s0 = rv_alloc(i);
  if (s0 == nullptr) {
    goto failed_malloc;
  }
  s = s0;

  int32_t big_l;
  if (ilim >= 0 && ilim <= kQuickMax && try_quick) {
    // Try to get by with floating-point arithmetic.

    i = 0;
    dval(&d2) = dval(&u);
    k0 = k;
    ilim0 = ilim;
    ieps = 2;  // conservative
    if (k > 0) {
      ds = kTens[k & 0xf];
      j = k >> 4;
      if (j & kBletch) {
        // prevent overflows
        j &= kBletch - 1;
        dval(&u) /= kBigTens[ARRAYSIZE(kBigTens) - 1];
        ieps++;
      }
      for (; j; j >>= 1, i++) {
        if (j & 1) {
          ieps++;
          ds *= kBigTens[i];
        }
      }
      dval(&u) /= ds;
    } else if ((j1 = -k)) {
      dval(&u) *= kTens[j1 & 0xf];
      for (j = j1 >> 4; j; j >>= 1, i++) {
        if (j & 1) {
          ieps++;
          dval(&u) *= kBigTens[i];
        }
      }
    }
    if (k_check && dval(&u) < 1. && ilim > 0) {
      if (ilim1 <= 0) {
        goto fast_failed;
      }
      ilim = ilim1;
      k--;
      dval(&u) *= 10.;
      ieps++;
    }
    dval(&eps) = ieps * dval(&u) + 7.;
    word0(&eps) -= (kP - 1) * kExpMsk1;
    if (ilim == 0) {
      big_s = nullptr;
      mhi = nullptr;
      dval(&u) -= 5.;
      if (dval(&u) > dval(&eps)) {
        goto one_digit;
      }
      if (dval(&u) < -dval(&eps)) {
        goto no_digits;
      }
      goto fast_failed;
    }
    if (leftright) {
      // Use Steele & White method of only
      // generating digits needed.
      dval(&eps) = 0.5 / kTens[ilim - 1] - dval(&eps);
      for (i = 0;;) {
        big_l = static_cast<int32_t>(dval(&u));
        dval(&u) -= big_l;
        *s++ = '0' + big_l;
        if (dval(&u) < dval(&eps)) {
          goto ret1;
        }
        if (1. - dval(&u) < dval(&eps)) {
          goto bump_up;
        }
        if (++i >= ilim) {
          break;
        }
        dval(&eps) *= 10.;
        dval(&u) *= 10.;
      }
    } else {
      // Generate ilim digits, then fix them up.
      dval(&eps) *= kTens[ilim - 1];
      for (i = 1;; i++, dval(&u) *= 10.) {
        big_l = static_cast<int32_t>(dval(&u));
        if (!(dval(&u) -= big_l)) {
          ilim = i;
        }
        *s++ = '0' + big_l;
        if (i == ilim) {
          if (dval(&u) > 0.5 + dval(&eps)) {
            goto bump_up;
          } else if (dval(&u) < 0.5 - dval(&eps)) {
            while (*--s == '0') {
            }
            s++;
            goto ret1;
          }
          break;
        }
      }
    }
  fast_failed:
    s = s0;
    dval(&u) = dval(&d2);
    k = k0;
    ilim = ilim0;
  }

  // Do we have a "small" integer?

  if (be >= 0 && k <= kIntMax) {
    // Yes.
    ds = kTens[k];
    if (ndigits < 0 && ilim <= 0) {
      big_s = nullptr;
      mhi = nullptr;
      if (ilim < 0 || dval(&u) <= 5 * ds) {
        goto no_digits;
      }
      goto one_digit;
    }
    for (i = 1;; i++, dval(&u) *= 10.) {
      big_l = static_cast<int32_t>(dval(&u) / ds);
      dval(&u) -= big_l * ds;
      *s++ = '0' + big_l;
      if (!dval(&u)) {
        break;
      }
      if (i == ilim) {
        dval(&u) += dval(&u);
        if (dval(&u) > ds || (dval(&u) == ds && big_l & 1)) {
        bump_up:
          while (*--s == '9') {
            if (s == s0) {
              k++;
              *s = '0';
              break;
            }
          }
          ++*s++;
        }
        break;
      }
    }
    goto ret1;
  }

  m2 = b2;
  m5 = b5;
  if (leftright) {
    i = denorm ? be + (kBias + (kP - 1) - 1 + 1) : 1 + kP - bbits;
    b2 += i;
    s2 += i;
    mhi = i2b(1);
    if (mhi == nullptr) {
      goto failed_malloc;
    }
  }
  if (m2 > 0 && s2 > 0) {
    i = m2 < s2 ? m2 : s2;
    b2 -= i;
    m2 -= i;
    s2 -= i;
  }
  if (b5 > 0) {
    if (leftright) {
      if (m5 > 0) {
        mhi = pow5mult(mhi, m5);
        if (mhi == nullptr) {
          goto failed_malloc;
        }
        Bigint* b1 = mult(mhi, b);
        Bfree(b);
        b = b1;
        if (b == nullptr) {
          goto failed_malloc;
        }
      }
      if ((j = b5 - m5)) {
        b = pow5mult(b, j);
        if (b == nullptr) {
          goto failed_malloc;
        }
      }
    } else {
      b = pow5mult(b, b5);
      if (b == nullptr) {
        goto failed_malloc;
      }
    }
  }
  big_s = i2b(1);
  if (big_s == nullptr) {
    goto failed_malloc;
  }
  if (s5 > 0) {
    big_s = pow5mult(big_s, s5);
    if (big_s == nullptr) {
      goto failed_malloc;
    }
  }

  // Check for special case that d is a normalized power of 2.

  spec_case = 0;
  if (mode < 2 || leftright) {
    if (!word1(&u) && !(word0(&u) & kBndryMask) &&
        word0(&u) & (kExpMask & ~kExpMsk1)) {
      // The special case
      b2 += kLog2P;
      s2 += kLog2P;
      spec_case = 1;
    }
  }

  // Arrange for convenient computation of quotients:
  // shift left if necessary so divisor has 4 leading 0 bits.
  //
  // Perhaps we should just compute leading 28 bits of big_s once
  // and for all and pass them and a shift to quorem, so it
  // can do shifts and ors to compute the numerator for q.
  i = dshift(big_s, s2);
  b2 += i;
  m2 += i;
  s2 += i;
  if (b2 > 0) {
    b = lshift(b, b2);
    if (b == nullptr) {
      goto failed_malloc;
    }
  }
  if (s2 > 0) {
    big_s = lshift(big_s, s2);
    if (big_s == nullptr) {
      goto failed_malloc;
    }
  }
  if (k_check) {
    if (cmp(b, big_s) < 0) {
      k--;
      b = multadd(b, 10, 0);  // we botched the k estimate
      if (b == nullptr) {
        goto failed_malloc;
      }
      if (leftright) {
        mhi = multadd(mhi, 10, 0);
        if (mhi == nullptr) {
          goto failed_malloc;
        }
      }
      ilim = ilim1;
    }
  }
  if (ilim <= 0 && (mode == 3 || mode == 5)) {
    if (ilim < 0) {
      // no digits, fcvt style
    no_digits:
      k = -1 - ndigits;
      goto ret;
    } else {
      big_s = multadd(big_s, 5, 0);
      if (big_s == nullptr) {
        goto failed_malloc;
      }
      if (cmp(b, big_s) <= 0) {
        goto no_digits;
      }
    }
  one_digit:
    *s++ = '1';
    k++;
    goto ret;
  }
  if (leftright) {
    if (m2 > 0) {
      mhi = lshift(mhi, m2);
      if (mhi == nullptr) {
        goto failed_malloc;
      }
    }

    // Compute mlo -- check for special case
    // that d is a normalized power of 2.

    mlo = mhi;
    if (spec_case) {
      mhi = Balloc(mhi->k);
      if (mhi == nullptr) {
        goto failed_malloc;
      }
      Bcopy(mhi, mlo);
      mhi = lshift(mhi, kLog2P);
      if (mhi == nullptr) {
        goto failed_malloc;
      }
    }

    for (i = 1;; i++) {
      dig = quorem(b, big_s) + '0';
      // Do we yet have the shortest decimal string
      // that will round to d?
      j = cmp(b, mlo);
      Bigint* delta = diff(big_s, mhi);
      if (delta == nullptr) {
        goto failed_malloc;
      }
      j1 = delta->sign ? 1 : cmp(b, delta);
      Bfree(delta);
      if (j1 == 0 && mode != 1 && !(word1(&u) & 1)) {
        if (dig == '9') {
          goto round_9_up;
        }
        if (j > 0) {
          dig++;
        }
        *s++ = dig;
        goto ret;
      }
      if (j < 0 || (j == 0 && mode != 1 && !(word1(&u) & 1))) {
        if (!b->x[0] && b->wds <= 1) {
          goto accept_dig;
        }
        if (j1 > 0) {
          b = lshift(b, 1);
          if (b == nullptr) {
            goto failed_malloc;
          }
          j1 = cmp(b, big_s);
          if ((j1 > 0 || (j1 == 0 && dig & 1)) && dig++ == '9') {
            goto round_9_up;
          }
        }
      accept_dig:
        *s++ = dig;
        goto ret;
      }
      if (j1 > 0) {
        if (dig == '9') {  // possible if i == 1
        round_9_up:
          *s++ = '9';
          goto roundoff;
        }
        *s++ = dig + 1;
        goto ret;
      }
      *s++ = dig;
      if (i == ilim) {
        break;
      }
      b = multadd(b, 10, 0);
      if (b == nullptr) {
        goto failed_malloc;
      }
      if (mlo == mhi) {
        mlo = mhi = multadd(mhi, 10, 0);
        if (mlo == nullptr) {
          goto failed_malloc;
        }
      } else {
        mlo = multadd(mlo, 10, 0);
        if (mlo == nullptr) {
          goto failed_malloc;
        }
        mhi = multadd(mhi, 10, 0);
        if (mhi == nullptr) {
          goto failed_malloc;
        }
      }
    }
  } else {
    for (i = 1;; i++) {
      *s++ = dig = quorem(b, big_s) + '0';
      if (!b->x[0] && b->wds <= 1) {
        goto ret;
      }
      if (i >= ilim) {
        break;
      }
      b = multadd(b, 10, 0);
      if (b == nullptr) {
        goto failed_malloc;
      }
    }
  }

  // Round off last digit

  b = lshift(b, 1);
  if (b == nullptr) {
    goto failed_malloc;
  }
  j = cmp(b, big_s);
  if (j > 0 || (j == 0 && dig & 1)) {
  roundoff:
    while (*--s == '9') {
      if (s == s0) {
        k++;
        *s++ = '1';
        goto ret;
      }
    }
    ++*s++;
  } else {
    while (*--s == '0') {
    }
    s++;
  }
ret:
  Bfree(big_s);
  if (mhi) {
    if (mlo && mlo != mhi) {
      Bfree(mlo);
    }
    Bfree(mhi);
  }
ret1:
  Bfree(b);
  *s = 0;
  *decpt = k + 1;
  if (rve) {
    *rve = s;
  }
  return s0;
failed_malloc:
  if (big_s) {
    Bfree(big_s);
  }
  if (mlo && mlo != mhi) {
    Bfree(mlo);
  }
  if (mhi) {
    Bfree(mhi);
  }
  if (b) {
    Bfree(b);
  }
  if (s0) {
    freedtoa(s0);
  }
  return nullptr;
}

}  // namespace python
