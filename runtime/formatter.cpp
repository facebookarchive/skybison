#include "formatter.h"

#include <climits>

#include "float-builtins.h"
#include "float-conversion.h"
#include "globals.h"
#include "handles-decl.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "unicode.h"

namespace py {

struct FloatWidths {
  word left_padding;
  byte sign;
  word sign_padding;
  word grouped_digits;
  bool has_decimal;
  word remainder;
  word right_padding;
};

static word calculateFloatWidths(const FormatSpec* format, const char* buf,
                                 word length, FloatWidths* result) {
  result->left_padding = 0;
  result->sign = '\0';
  result->sign_padding = 0;
  result->grouped_digits = length;
  result->has_decimal = false;
  result->remainder = 0;
  result->right_padding = 0;

  // Check for a sign character
  word index = 0;
  word total_length = 0;
  if (buf[index] == '-') {
    result->sign = '-';
    index++;
    total_length++;
  }

  // Parse the digits for a remainder
  for (; index < length; index++) {
    char c = buf[index];
    if (!ASCII::isDigit(c)) {
      word remainder = length - index;
      if (c == '.') {
        result->has_decimal = true;
        result->remainder = remainder - 1;
        total_length += remainder;  // TODO(T52759101): use locale for decimal
      } else {
        result->remainder = remainder;
        total_length += remainder;
      }
      break;
    }
  }

  if (format->positive_sign != '\0' && result->sign == '\0') {
    result->sign = format->positive_sign;
    total_length++;
  }

  // TODO(T52759101): use locale for thousands separator and grouping
  word digits = result->sign == '-' ? index - 1 : index;
  if (format->thousands_separator != '\0') {
    digits += (digits - 1) / 3;
  }
  result->grouped_digits = digits;
  total_length += digits;

  word padding = format->width - total_length;
  if (padding > 0) {
    total_length +=
        padding * SmallStr::fromCodePoint(format->fill_char).length();
    switch (format->alignment) {
      case '<':
        result->right_padding = padding;
        break;
      case '=':
        result->sign_padding = padding;
        break;
      case '>':
        result->left_padding = padding;
        break;
      case '^':
        result->left_padding = padding >> 1;
        result->right_padding = padding - result->left_padding;
        break;
      default:
        UNREACHABLE("unexpected alignment %c", format->alignment);
    }
  }

  return total_length;
}

static bool isAlignmentSpec(int32_t cp) {
  switch (cp) {
    case '<':
    case '>':
    case '=':
    case '^':
      return true;
    default:
      return false;
  }
}

static inline int32_t nextCodePoint(const Str& spec, word length, word* index) {
  if (*index >= length) {
    return 0;
  }
  word cp_length;
  int32_t cp = spec.codePointAt(*index, &cp_length);
  *index += cp_length;
  return cp;
}

RawObject parseFormatSpec(Thread* thread, const Str& spec, int32_t default_type,
                          char default_align, FormatSpec* result) {
  result->alignment = default_align;
  result->positive_sign = 0;
  result->thousands_separator = 0;
  result->type = default_type;
  result->alternate = false;
  result->fill_char = ' ';
  result->width = -1;
  result->precision = -1;

  word index = 0;
  word length = spec.length();
  int32_t cp = nextCodePoint(spec, length, &index);

  bool fill_char_specified = false;
  bool alignment_specified = false;
  word old_index = index;
  int32_t c_next = nextCodePoint(spec, length, &index);
  if (isAlignmentSpec(c_next)) {
    result->alignment = static_cast<char>(c_next);
    result->fill_char = cp;
    fill_char_specified = true;
    alignment_specified = true;

    cp = nextCodePoint(spec, length, &index);
  } else if (!alignment_specified && isAlignmentSpec(cp)) {
    result->alignment = static_cast<char>(cp);
    alignment_specified = true;
    cp = c_next;
  } else {
    index = old_index;
  }

  switch (cp) {
    case '+':
    case ' ':
      result->positive_sign = static_cast<char>(cp);
      cp = nextCodePoint(spec, length, &index);
      break;
    case '-':
      cp = nextCodePoint(spec, length, &index);
      break;
  }

  if (!fill_char_specified && cp == '0') {
    result->fill_char = '0';
    if (!alignment_specified) {
      result->alignment = '=';
    }
  }

  if (cp == '#') {
    result->alternate = true;
    cp = nextCodePoint(spec, length, &index);
  }

  if ('0' <= cp && cp <= '9') {
    word width = 0;
    for (;;) {
      width += cp - '0';
      cp = nextCodePoint(spec, length, &index);
      if ('0' > cp || cp > '9') break;
      if (__builtin_mul_overflow(width, 10, &width)) {
        return thread->raiseWithFmt(LayoutId::kValueError,
                                    "Too many decimal digits in format string");
      }
    }
    result->width = width;
  }

  if (cp == ',') {
    result->thousands_separator = ',';
    cp = nextCodePoint(spec, length, &index);
  }
  if (cp == '_') {
    if (result->thousands_separator != 0) {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "Cannot specify both ',' and '_'.");
    }
    result->thousands_separator = '_';
    cp = nextCodePoint(spec, length, &index);
  }
  if (cp == ',') {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "Cannot specify both ',' and '_'.");
  }

  if (cp == '.') {
    cp = nextCodePoint(spec, length, &index);
    if ('0' > cp || cp > '9') {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "Format specifier missing precision");
    }

    word precision = 0;
    for (;;) {
      precision += cp - '0';
      cp = nextCodePoint(spec, length, &index);
      if ('0' > cp || cp > '9') break;
      if (__builtin_mul_overflow(precision, 10, &precision)) {
        return thread->raiseWithFmt(LayoutId::kValueError,
                                    "Too many decimal digits in format string");
      }
    }
    result->precision = precision;
  }

  if (cp != 0) {
    result->type = cp;
    // This was the last step: No need to call `nextCodePoint()` here.
  }
  if (index < length) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "Invalid format specifier");
  }

  if (result->thousands_separator) {
    switch (result->type) {
      case 'd':
      case 'e':
      case 'f':
      case 'g':
      case 'E':
      case 'G':
      case '%':
      case 'F':
      case '\0':
        // These are allowed. See PEP 378.
        break;
      case 'b':
      case 'o':
      case 'x':
      case 'X':
        // Underscores are allowed in bin/oct/hex. See PEP 515.
        if (result->thousands_separator == '_') {
          break;
        }
        FALLTHROUGH;
      default:
        if (32 < result->type && result->type <= kMaxASCII) {
          return thread->raiseWithFmt(
              LayoutId::kValueError, "Cannot specify '%c' with '%c'.",
              result->thousands_separator, static_cast<char>(result->type));
        }
        return thread->raiseWithFmt(
            LayoutId::kValueError, "Cannot specify '%c' with '\\x%x'.",
            result->thousands_separator, static_cast<unsigned>(result->type));
    }
  }
  return NoneType::object();
}

static word putFloat(const MutableBytes& dest, const byte* buf,
                     const FormatSpec* format, const FloatWidths* widths) {
  word at = 0;
  RawSmallStr fill = SmallStr::fromCodePoint(format->fill_char);
  word fill_length = fill.length();
  for (word i = 0; i < widths->left_padding; i++, at += fill_length) {
    dest.replaceFromWithStr(at, Str::cast(fill), fill_length);
  }
  switch (widths->sign) {
    case '\0':
      break;
    case '-':
      buf++;
      FALLTHROUGH;
    case '+':
    case ' ':
      dest.byteAtPut(at++, widths->sign);
      break;
    default:
      UNREACHABLE("unexpected sign char %c", widths->sign);
  }
  for (word i = 0; i < widths->sign_padding; i++, at += fill_length) {
    dest.replaceFromWithStr(at, Str::cast(fill), fill_length);
  }
  // TODO(T52759101): use thousands separator from locale
  if (format->thousands_separator == '\0') {
    dest.replaceFromWithAll(at, {buf, widths->grouped_digits});
    at += widths->grouped_digits;
    buf += widths->grouped_digits;
  } else {
    // TODO(T52759101): use locale for grouping
    word prefix = widths->grouped_digits % 4;
    dest.replaceFromWithAll(at, {buf, prefix});
    buf += prefix;
    for (word i = prefix; i < widths->grouped_digits; i += 4, buf += 3) {
      dest.byteAtPut(at + i, format->thousands_separator);
      dest.replaceFromWithAll(at + i + 1, {buf, 3});
    }
    at += widths->grouped_digits;
  }
  if (widths->has_decimal) {
    // TODO(T52759101): use decimal from locale
    dest.byteAtPut(at++, '.');
    buf++;
  }
  dest.replaceFromWithAll(at, {buf, widths->remainder});
  at += widths->remainder;
  for (word i = 0; i < widths->right_padding; i++, at += fill_length) {
    dest.replaceFromWithStr(at, Str::cast(fill), fill_length);
  }
  return at;
}

RawObject raiseUnknownFormatError(Thread* thread, int32_t format_code,
                                  const Object& object) {
  if (32 < format_code && format_code < kMaxASCII) {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "Unknown format code '%c' for object of type '%T'",
        static_cast<char>(format_code), &object);
  }
  return thread->raiseWithFmt(
      LayoutId::kValueError,
      "Unknown format code '\\x%x' for object of type '%T'",
      static_cast<unsigned>(format_code), &object);
}

RawObject formatFloat(Thread* thread, double value, FormatSpec* format) {
  static const word default_precision = 6;
  word precision = format->precision;
  if (precision > INT_MAX) {
    return thread->raiseWithFmt(LayoutId::kValueError, "precision too big");
  }

  int32_t type = format->type;
  bool add_dot_0 = false;
  bool add_percent = false;
  switch (type) {
    case '\0':
      add_dot_0 = true;
      type = 'r';
      break;
    case 'n':
      type = 'g';
      break;
    case '%':
      type = 'f';
      value *= 100;
      add_percent = true;
      break;
  }

  if (precision < 0) {
    precision = add_dot_0 ? 0 : default_precision;
  } else if (type == 'r') {
    type = 'g';
  }

  FormatResultKind kind;
  unique_c_ptr<char> buf(doubleToString(value, type,
                                        static_cast<int>(precision), false,
                                        add_dot_0, format->alternate, &kind));
  word length = std::strlen(buf.get());
  if (add_percent) {
    // Overwrite the terminating null character
    buf.get()[length++] = '%';
  }

  Runtime* runtime = thread->runtime();
  if (format->positive_sign == '\0' && format->width <= length &&
      format->type != 'n' && format->thousands_separator == '\0') {
    return runtime->newStrWithAll({reinterpret_cast<byte*>(buf.get()), length});
  }

  // TODO(T52759101): use locale for grouping, separator, and decimal point
  FloatWidths widths;
  word result_length = calculateFloatWidths(format, buf.get(), length, &widths);

  HandleScope scope(thread);
  MutableBytes result(&scope,
                      runtime->newMutableBytesUninitialized(result_length));
  word written =
      putFloat(result, reinterpret_cast<byte*>(buf.get()), format, &widths);
  DCHECK(written == result_length, "expected to write %ld bytes, wrote %ld",
         result_length, written);
  return result.becomeStr();
}

RawObject formatStr(Thread* thread, const Str& str, FormatSpec* format) {
  DCHECK(format->positive_sign == '\0', "must no have sign specified");
  DCHECK(!format->alternate, "must not have alternative format");
  word width = format->width;
  word precision = format->precision;
  if (width == -1 && precision == -1) {
    return *str;
  }

  word char_length = str.length();
  word codepoint_length;
  word str_end_index;
  if (precision >= 0) {
    str_end_index = str.offsetByCodePoints(0, precision);
    if (str_end_index < char_length) {
      codepoint_length = precision;
    } else {
      codepoint_length = str.codePointLength();
    }
  } else {
    str_end_index = char_length;
    codepoint_length = str.codePointLength();
  }

  Runtime* runtime = thread->runtime();
  word padding = width - codepoint_length;
  if (padding <= 0) {
    return runtime->strSubstr(thread, str, 0, str_end_index);
  }

  // Construct result.
  HandleScope scope(thread);
  Str fill_char(&scope, SmallStr::fromCodePoint(format->fill_char));
  word fill_char_length = fill_char.length();
  word padding_char_length = padding * fill_char_length;
  word result_char_length = str_end_index + padding_char_length;
  MutableBytes result(
      &scope, runtime->newMutableBytesUninitialized(result_char_length));
  word index = 0;
  word early_padding;
  if (format->alignment == '<') {
    early_padding = 0;
  } else if (format->alignment == '^') {
    word half = padding / 2;
    early_padding = half;
    padding = half + (padding % 2);
  } else {
    DCHECK(format->alignment == '>' || format->alignment == '=',
           "remaining cases must be '>' or '='");
    early_padding = padding;
    padding = 0;
  }
  for (word i = 0; i < early_padding; i++) {
    result.replaceFromWithStr(index, *fill_char, fill_char_length);
    index += fill_char_length;
  }
  result.replaceFromWithStr(index, *str, str_end_index);
  index += str_end_index;
  if (padding > 0) {
    DCHECK(format->alignment == '<' || format->alignment == '^',
           "unexpected alignment");
    for (word i = 0; i < padding; i++) {
      result.replaceFromWithStr(index, *fill_char, fill_char_length);
      index += fill_char_length;
    }
  }
  DCHECK(index == result_char_length, "overflow or underflow in result");
  return result.becomeStr();
}

// Returns the quotient of a double word number and a single word.
// Assumes the result will fit in a single uword: `dividend_high < divisor`.
static uword dwordUDiv(uword dividend_low, uword dividend_high, uword divisor,
                       uword* remainder) {
  // TODO(matthiasb): Future optimization idea:
  // This whole function is a single `divq` instruction on x86_64, we could use
  // inline assembly for it (there doesn't seem to be a builtin).

  // The code is based on Hacker's Delight chapter 9-4 Unsigned Long Division.
  DCHECK(divisor != 0, "division by zero");
  DCHECK(dividend_high < divisor, "overflow");

  // Performs some arithmetic with no more than half the bits of a `uword`.
  int half_bits = kBitsPerWord / 2;
  uword half_mask = (uword{1} << half_bits) - 1;

  // Normalize divisor by shifting the highest bit left as much as possible.
  static_assert(sizeof(divisor) == sizeof(long), "choose right builtin");
  int s = __builtin_clzl(divisor);
  uword divisor_n = divisor << s;
  uword divisor_n_high_half = divisor_n >> half_bits;
  uword divisor_n_low_half = divisor_n & half_mask;

  // Normalize dividend by shifting it by the same amount as the divisor.
  uword dividend_high_n =
      (s == 0) ? dividend_high
               : (dividend_high << s) | (dividend_low >> (kBitsPerWord - s));
  uword dividend_low_n = dividend_low << s;
  uword dividend_low_n_high_half = dividend_low_n >> half_bits;
  uword dividend_low_n_low_half = dividend_low_n & half_mask;

  uword quot_high_half = dividend_high_n / divisor_n_high_half;
  uword remainder_high_half = dividend_high_n % divisor_n_high_half;
  while (quot_high_half > half_mask ||
         quot_high_half * divisor_n_low_half >
             ((remainder_high_half << half_bits) | dividend_low_n_high_half)) {
    quot_high_half--;
    remainder_high_half += divisor_n_high_half;
    if (remainder_high_half > half_mask) break;
  }

  uword dividend_middle =
      ((dividend_high_n << half_bits) | dividend_low_n_high_half) -
      quot_high_half * divisor_n;

  uword quot_low_half = dividend_middle / divisor_n_high_half;
  uword remainder_low_half = dividend_middle % divisor_n_high_half;
  while (quot_low_half > half_mask ||
         quot_low_half * divisor_n_low_half >
             ((remainder_low_half << half_bits) | dividend_low_n_low_half)) {
    quot_low_half--;
    remainder_low_half += divisor_n_high_half;
    if (remainder_low_half > half_mask) break;
  }

  uword result = (quot_high_half << half_bits) | quot_low_half;
  *remainder = dividend_low - result * divisor;
  return result;
}

// Divide a large integer formed by an array of int digits by a single digit and
// return the remainder. `digits_in` and `digits_out` may be the same pointer
// for in-place operation.
static uword divIntSingleDigit(uword* digits_out, const uword* digits_in,
                               word num_digits, uword divisor) {
  // TODO(matthiasb): Future optimization idea:
  // Instead of dividing by a constant, multiply with a precomputed inverse
  // (see Hackers Delight, chapter 10). The compiler doesn't catch this case
  // for double word arithmetic as in dwordUDiv.
  uword remainder = 0;
  for (word i = num_digits - 1; i >= 0; i--) {
    // Compute `remainder:digit / divisor`.
    digits_out[i] = dwordUDiv(digits_in[i], remainder, divisor, &remainder);
  }
  return remainder;
}

// Converts an uword to ascii decimal digits. The digits can only be efficiently
// produced from least to most significant without knowing the exact number of
// digits upfront. Because of this the function takes a `buffer_end` argument
// and writes the digit before it. Returns a pointer to the last byte written.
static byte* uwordToDecimal(uword num, byte* buffer_end) {
  byte* start = buffer_end;
  do {
    *--start = '0' + num % 10;
    num /= 10;
  } while (num > 0);
  return start;
}

// Return upper bound on number of decimal digits to format large int `value`.
static word estimateNumDecimalDigits(RawLargeInt value) {
  // Compute an upper bound on the number of decimal digits required for a
  // number with n bits:
  //   ceil(log10(2**n - 1))
  // We over-approximate this with:
  //   ceil(log10(2**n - 1))
  //   == ceil(log2(2**n - 1)/log2(10))
  //   <= 1 + n * (1/log2(10))
  //   <= 1 + n * 0.30102999566398114
  //   <= 1 + n * 309 / 1024
  // This isn't off by more than 1 digit for all one binary numbers up to
  // 1425 bits.
  word bit_length = value.bitLength();
  return 1 + bit_length * 309 / 1024;
}

static byte* writeLargeIntDecimalDigits(byte* buffer_end, RawLargeInt value) {
  // Allocate space for intermediate results. We also convert a negative number
  // to a positive number of the same magnitude here.
  word num_digits = value.numDigits();
  std::unique_ptr<uword[]> temp_digits(new uword[num_digits]);
  bool negative = value.isNegative();
  if (!negative) {
    for (word i = 0; i < num_digits; ++i) {
      temp_digits[i] = value.digitAt(i);
    }
  } else {
    uword carry = 1;
    for (word i = 0; i < num_digits; ++i) {
      uword digit = value.digitAt(i);
      carry = __builtin_uaddl_overflow(~digit, carry, &temp_digits[i]);
    }
    // The complement of the highest bit in a negative number must be 0 so we
    // cannot overflow.
    DCHECK(carry == 0, "overflow");
  }
  word num_temp_digits = num_digits;

  // The strategy here is to divide the large integer by continually dividing it
  // by `kUwordDigits10Pow`. `uwordToDecimal` can convert those remainders to
  // decimal digits.
  //
  // TODO(matthiasb): Future optimization ideas:
  // It seems cpythons algorithm is faster (for big numbers) in practive.
  // Their source claims it is (Knuth TAOCP, vol 2, section 4.4, method 1b).
  byte* start = buffer_end;
  do {
    uword remainder = divIntSingleDigit(temp_digits.get(), temp_digits.get(),
                                        num_temp_digits, kUwordDigits10Pow);
    byte* new_start = uwordToDecimal(remainder, start);

    while (num_temp_digits > 0 && temp_digits[num_temp_digits - 1] == 0) {
      num_temp_digits--;
    }
    // Produce leading zeros if this wasn't the last round.
    if (num_temp_digits > 0) {
      for (word i = 0, n = kUwordDigits10 - (start - new_start); i < n; i++) {
        *--new_start = '0';
      }
    }
    start = new_start;
  } while (num_temp_digits > 0);
  return start;
}

RawObject formatIntDecimalSimple(Thread* thread, const Int& value) {
  if (!value.isLargeInt()) {
    word value_word = value.asWord();
    uword magnitude =
        value_word >= 0 ? value_word : -static_cast<uword>(value_word);
    byte buffer[kUwordDigits10 + 1];
    byte* end = buffer + sizeof(buffer);
    byte* start = uwordToDecimal(magnitude, end);
    if (value_word < 0) *--start = '-';
    DCHECK(start >= buffer, "buffer underflow");
    return thread->runtime()->newStrWithAll(View<byte>(start, end - start));
  }

  RawLargeInt value_large = LargeInt::cast(*value);
  bool is_negative = value_large.isNegative();
  word max_chars =
      estimateNumDecimalDigits(value_large) + (is_negative ? 1 : 0);
  std::unique_ptr<byte[]> buffer(new byte[max_chars]);
  byte* end = buffer.get() + max_chars;
  byte* start = writeLargeIntDecimalDigits(end, value_large);
  if (is_negative) {
    *--start = '-';
  }
  DCHECK(start >= buffer.get(), "buffer underflow");
  return thread->runtime()->newStrWithAll(View<byte>(start, end - start));
}

static word numBinaryDigits(const Int& value) {
  return value.isZero() ? 1 : value.bitLength();
}

static word numHexadecimalDigits(const Int& value) {
  return value.isZero() ? 1 : (value.bitLength() + 3) >> 2;
}

static word numOctalDigits(const Int& value) {
  return value.isZero() ? 1 : (value.bitLength() + 2) / 3;
}

static void putBinaryDigits(Thread* thread, const MutableBytes& dest, word at,
                            const Int& value, word num_digits) {
  static const char* const quads[16] = {
      "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111",
      "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111",
  };

  word idx = at + num_digits;
  uword last_digit;
  if (value.isLargeInt()) {
    HandleScope scope(thread);
    LargeInt value_large(&scope, *value);
    word d_last = (num_digits - 1) / kBitsPerWord;
    bool is_negative = value_large.isNegative();
    uword carry = 1;
    for (word d = 0; d < d_last; d++) {
      uword digit = value_large.digitAt(d);
      if (is_negative) {
        digit = ~digit + carry;
        carry = carry & (digit == 0);
      }
      static_assert(kBitsPerWord % 4 == 0, "bits per word divisible by 4");
      for (word i = 0; i < kBitsPerWord / 4; i++) {
        const char* quad = quads[digit & 0xf];
        dest.byteAtPut(--idx, quad[3]);
        dest.byteAtPut(--idx, quad[2]);
        dest.byteAtPut(--idx, quad[1]);
        dest.byteAtPut(--idx, quad[0]);
        digit >>= 4;
      }
    }
    last_digit = value_large.digitAt(d_last);
    if (is_negative) {
      last_digit = ~last_digit + carry;
    }
  } else {
    last_digit = static_cast<uword>(std::abs(value.asWord()));
  }

  do {
    dest.byteAtPut(--idx, '0' + (last_digit & 1));
    last_digit >>= 1;
  } while (last_digit != 0);
  DCHECK(idx == at, "unexpected number of digits");
}

static void putHexadecimalDigitsImpl(Thread* thread, const MutableBytes& dest,
                                     word at, const Int& value, word num_digits,
                                     const char* hex_digits) {
  word idx = at + num_digits;
  uword last_digit;
  if (value.isLargeInt()) {
    HandleScope scope(thread);
    LargeInt value_large(&scope, *value);
    const word hexdigits_per_word = kBitsPerWord / kBitsPerHexDigit;
    word d_last = (num_digits - 1) / hexdigits_per_word;
    bool is_negative = value_large.isNegative();
    uword carry = 1;
    for (word d = 0; d < d_last; d++) {
      uword digit = value_large.digitAt(d);
      if (is_negative) {
        digit = ~digit + carry;
        carry = carry & (digit == 0);
      }
      for (word i = 0; i < hexdigits_per_word; i++) {
        dest.byteAtPut(--idx, hex_digits[digit & 0xf]);
        digit >>= kBitsPerHexDigit;
      }
    }
    last_digit = value_large.digitAt(d_last);
    if (is_negative) {
      last_digit = ~last_digit + carry;
    }
  } else {
    last_digit = static_cast<uword>(std::abs(value.asWord()));
  }

  do {
    dest.byteAtPut(--idx, hex_digits[last_digit & 0xf]);
    last_digit >>= kBitsPerHexDigit;
  } while (last_digit != 0);
  DCHECK(idx == at, "unexpected number of digits");
}

static void putHexadecimalLowerCaseDigits(Thread* thread,
                                          const MutableBytes& dest, word at,
                                          const Int& value, word num_digits) {
  putHexadecimalDigitsImpl(thread, dest, at, value, num_digits,
                           "0123456789abcdef");
}

static void putHexadecimalUpperCaseDigits(Thread* thread,
                                          const MutableBytes& dest, word at,
                                          const Int& value, word num_digits) {
  putHexadecimalDigitsImpl(thread, dest, at, value, num_digits,
                           "0123456789ABCDEF");
}

static void putOctalDigits(Thread* thread, const MutableBytes& dest, word at,
                           const Int& value, word num_result_digits) {
  word idx = at + num_result_digits;
  if (value.isLargeInt()) {
    HandleScope scope(thread);
    LargeInt value_large(&scope, *value);
    bool is_negative = value_large.isNegative();

    // We have to negate negative numbers which produces carry between
    // digits.
    uword negate_carry = 1;
    // We have carry over when the three bits for an octal digits are between
    // large-int digits (this has nothing to do with `negate_carry`).
    uword prev_digit_carry = 0;
    word prev_digit_carry_num_bits = 0;
    for (word d = 0, num_digits = value_large.numDigits(); d < num_digits;
         d++) {
      uword digit = value_large.digitAt(d);
      if (is_negative) {
        digit = ~digit + negate_carry;
        negate_carry &= (digit == 0);
      }

      word num_oct_digits = kBitsPerWord / kBitsPerOctDigit;
      word next_carry_num_bits = kBitsPerWord % kBitsPerOctDigit;
      if (prev_digit_carry_num_bits != 0) {
        word combined = digit << prev_digit_carry_num_bits | prev_digit_carry;
        dest.byteAtPut(--idx, '0' + (combined & 7));
        digit >>= (kBitsPerOctDigit - prev_digit_carry_num_bits);
        if (idx == at) {
          DCHECK(d == num_digits - 1 && digit == 0, "rest must be zero");
          break;
        }
        num_oct_digits--;

        next_carry_num_bits += prev_digit_carry_num_bits;
        if (next_carry_num_bits == kBitsPerOctDigit) {
          num_oct_digits++;
          next_carry_num_bits = 0;
        }
      }
      for (word i = 0; i < num_oct_digits; i++) {
        dest.byteAtPut(--idx, '0' + (digit & 7));
        digit >>= kBitsPerOctDigit;
        // Stop before we start outputting leading zeros.
        if (idx == at) {
          DCHECK(d == num_digits - 1 && digit == 0, "rest must be zero");
          break;
        }
      }
      DCHECK(digit < static_cast<uword>(1 << next_carry_num_bits),
             "too many bits left");
      prev_digit_carry_num_bits = next_carry_num_bits;
      prev_digit_carry = digit;
    }
    // Output leftover carry bits.
    if (idx > at) {
      DCHECK(prev_digit_carry_num_bits > 0, "should have carry bits");
      dest.byteAtPut(--idx, '0' + prev_digit_carry);
    }
  } else {
    uword value_uword = static_cast<uword>(std::abs(value.asWord()));
    do {
      dest.byteAtPut(--idx, '0' + (value_uword & 7));
      value_uword >>= kBitsPerOctDigit;
    } while (value_uword != 0);
  }
  DCHECK(idx == at, "unexpected number of digits");
}

using numDigitsFunc = word (*)(const Int&);
using putDigitsFunc = void (*)(Thread*, const MutableBytes&, word, const Int&,
                               word);

static inline RawObject formatIntSimpleImpl(Thread* thread, const Int& value,
                                            char format_prefix,
                                            numDigitsFunc num_digits,
                                            putDigitsFunc put_digits) {
  word result_n_digits = num_digits(value);
  word result_size = 2 + (value.isNegative() ? 1 : 0) + result_n_digits;

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  MutableBytes result(&scope,
                      runtime->newMutableBytesUninitialized(result_size));
  word index = 0;
  if (value.isNegative()) {
    result.byteAtPut(index++, '-');
  }
  result.byteAtPut(index++, '0');
  result.byteAtPut(index++, format_prefix);
  put_digits(thread, result, index, value, result_n_digits);
  return result.becomeStr();
}

RawObject formatIntBinarySimple(Thread* thread, const Int& value) {
  return formatIntSimpleImpl(thread, value, 'b', numBinaryDigits,
                             putBinaryDigits);
}

RawObject formatIntHexadecimalSimple(Thread* thread, const Int& value) {
  return formatIntSimpleImpl(thread, value, 'x', numHexadecimalDigits,
                             putHexadecimalLowerCaseDigits);
}

RawObject formatIntOctalSimple(Thread* thread, const Int& value) {
  return formatIntSimpleImpl(thread, value, 'o', numOctalDigits,
                             putOctalDigits);
}

RawObject formatIntDecimal(Thread* thread, const Int& value,
                           FormatSpec* format) {
  if (format->precision >= 0) {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "Precision not allowed in integer format specifier");
  }
  if (format->thousands_separator != 0) {
    UNIMPLEMENTED("thousands separator");
  }

  // We cannot easily predict how many digits are necessary. So we overestimate
  // and printing into a temporary buffer.
  byte fixed_buffer[kUwordDigits10];
  byte* buffer;
  byte* digits;
  word result_n_digits;
  if (value.isLargeInt()) {
    word max_chars = estimateNumDecimalDigits(LargeInt::cast(*value));
    buffer = new byte[max_chars];
    byte* end = buffer + max_chars;
    digits = writeLargeIntDecimalDigits(end, LargeInt::cast(*value));
    result_n_digits = end - digits;
  } else {
    buffer = fixed_buffer;
    word value_word = value.asWord();
    uword magnitude =
        value_word >= 0 ? value_word : -static_cast<uword>(value_word);
    byte* end = buffer + ARRAYSIZE(fixed_buffer);
    digits = uwordToDecimal(magnitude, end);
    result_n_digits = end - digits;
  }

  bool is_negative = value.isNegative();
  word number_chars =
      (is_negative || format->positive_sign != '\0') + result_n_digits;

  HandleScope scope(thread);
  Str fill_cp(&scope, SmallStr::fromCodePoint(format->fill_char));
  word fill_cp_chars = fill_cp.length();
  word result_chars = number_chars;

  word padding = format->width - number_chars;
  word left_padding = 0;
  word sign_padding = 0;
  word right_padding = 0;
  if (padding > 0) {
    result_chars += padding * fill_cp_chars;
    switch (format->alignment) {
      case '<':
        right_padding = padding;
        break;
      case '=':
        sign_padding = padding;
        break;
      case '>':
        left_padding = padding;
        break;
      case '^':
        left_padding = padding >> 1;
        right_padding = padding - left_padding;
        break;
      default:
        UNREACHABLE("unexpected alignment %c", format->alignment);
    }
  }

  word index = 0;
  MutableBytes result(
      &scope, thread->runtime()->newMutableBytesUninitialized(result_chars));
  for (word i = 0; i < left_padding; i++) {
    result.replaceFromWithStr(index, *fill_cp, fill_cp_chars);
    index += fill_cp_chars;
  }
  if (is_negative) {
    result.byteAtPut(index++, '-');
  } else if (format->positive_sign != '\0') {
    result.byteAtPut(index++, format->positive_sign);
  }
  for (word i = 0; i < sign_padding; i++) {
    result.replaceFromWithStr(index, *fill_cp, fill_cp_chars);
    index += fill_cp_chars;
  }
  result.replaceFromWithAll(index, {digits, result_n_digits});
  index += result_n_digits;
  for (word i = 0; i < right_padding; i++) {
    result.replaceFromWithStr(index, *fill_cp, fill_cp_chars);
    index += fill_cp_chars;
  }
  DCHECK(index == result.length(), "wrong number of bytes written");

  if (value.isLargeInt()) {
    delete[] buffer;
  }
  return result.becomeStr();
}

static inline RawObject formatIntImpl(Thread* thread, const Int& value,
                                      FormatSpec* format, char format_prefix,
                                      numDigitsFunc num_digits,
                                      putDigitsFunc put_digits) {
  if (format->precision >= 0) {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "Precision not allowed in integer format specifier");
  }
  if (format->thousands_separator != 0) {
    UNIMPLEMENTED("thousands separator");
  }

  bool is_negative = value.isNegative();
  word result_n_digits = num_digits(value);
  word number_chars = (is_negative || format->positive_sign != '\0') +
                      (format->alternate ? 2 : 0) + result_n_digits;

  HandleScope scope(thread);
  Str fill_cp(&scope, SmallStr::fromCodePoint(format->fill_char));
  word fill_cp_chars = fill_cp.length();
  word result_chars = number_chars;

  word padding = format->width - number_chars;
  word left_padding = 0;
  word sign_padding = 0;
  word right_padding = 0;
  if (padding > 0) {
    result_chars += padding * fill_cp_chars;
    switch (format->alignment) {
      case '<':
        right_padding = padding;
        break;
      case '=':
        sign_padding = padding;
        break;
      case '>':
        left_padding = padding;
        break;
      case '^':
        left_padding = padding >> 1;
        right_padding = padding - left_padding;
        break;
      default:
        UNREACHABLE("unexpected alignment %c", format->alignment);
    }
  }

  word index = 0;
  MutableBytes result(
      &scope, thread->runtime()->newMutableBytesUninitialized(result_chars));
  for (word i = 0; i < left_padding; i++) {
    result.replaceFromWithStr(index, *fill_cp, fill_cp_chars);
    index += fill_cp_chars;
  }
  if (is_negative) {
    result.byteAtPut(index++, '-');
  } else if (format->positive_sign != '\0') {
    result.byteAtPut(index++, format->positive_sign);
  }
  if (format->alternate) {
    result.byteAtPut(index++, '0');
    result.byteAtPut(index++, format_prefix);
  }
  for (word i = 0; i < sign_padding; i++) {
    result.replaceFromWithStr(index, *fill_cp, fill_cp_chars);
    index += fill_cp_chars;
  }
  put_digits(thread, result, index, value, result_n_digits);
  index += result_n_digits;

  for (word i = 0; i < right_padding; i++) {
    result.replaceFromWithStr(index, *fill_cp, fill_cp_chars);
    index += fill_cp_chars;
  }
  DCHECK(index == result.length(), "wrong number of bytes written");
  return result.becomeStr();
}

RawObject formatIntBinary(Thread* thread, const Int& value,
                          FormatSpec* format) {
  return formatIntImpl(thread, value, format, 'b', numBinaryDigits,
                       putBinaryDigits);
}

RawObject formatIntHexadecimalLowerCase(Thread* thread, const Int& value,
                                        FormatSpec* format) {
  return formatIntImpl(thread, value, format, 'x', numHexadecimalDigits,
                       putHexadecimalLowerCaseDigits);
}

RawObject formatIntHexadecimalUpperCase(Thread* thread, const Int& value,
                                        FormatSpec* format) {
  return formatIntImpl(thread, value, format, 'X', numHexadecimalDigits,
                       putHexadecimalUpperCaseDigits);
}

RawObject formatIntOctal(Thread* thread, const Int& value, FormatSpec* format) {
  return formatIntImpl(thread, value, format, 'o', numOctalDigits,
                       putOctalDigits);
}

RawObject formatDoubleHexadecimalSimple(Runtime* runtime, double value) {
  const int mantissa_hex_digits = (kDoubleMantissaBits / 4) + 1;
  const int exp_bits = kBitsPerDouble - kDoubleMantissaBits - 1;
  const int max_exp = 1 << (exp_bits - 1);
  const int min_exp = -(1 << (exp_bits - 1)) + 1;

  bool is_negative;
  int exponent;
  int64_t mantissa;

  decodeDouble(value, &is_negative, &exponent, &mantissa);
  if (exponent == max_exp) {
    if (mantissa == 0) {
      return is_negative ? runtime->newStrFromCStr("-inf")
                         : runtime->newStrFromCStr("inf");
    }
    return runtime->newStrFromCStr("nan");
  }

  if (exponent == min_exp && mantissa == 0) {
    return is_negative ? runtime->newStrFromCStr("-0x0.0p+0")
                       : runtime->newStrFromCStr("0x0.0p+0");
  }

  int exponent_sign;
  if (exponent < 0) {
    exponent_sign = '-';
    exponent = -exponent;
  } else {
    exponent_sign = '+';
  }

  // Output buffer_size mantissaHexDigits + 1 char for '-', 2 chars for '0x', 2
  // chars for '1.', 1 char for 'p', 1 char for exponent sign '+' or '-', and 4
  // chars for the exponent.
  const int buffer_size = mantissa_hex_digits + 11;
  char output[buffer_size];
  byte* end = reinterpret_cast<byte*>(output) + buffer_size;
  byte* poutput = uwordToDecimal(exponent, end);

  *--poutput = exponent_sign;
  *--poutput = 'p';
  for (int k = 0; k < kDoubleMantissaBits; k += 4) {
    *--poutput = Utils::kHexDigits[(mantissa >> k) & 0xf];
  }

  *--poutput = '.';
  *--poutput = '1';
  *--poutput = 'x';
  *--poutput = '0';
  if (is_negative) {
    *--poutput = '-';
  }
  return runtime->newStrWithAll(View<byte>(poutput, end - poutput));
}

}  // namespace py
