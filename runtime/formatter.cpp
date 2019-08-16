#include "formatter.h"

#include "runtime.h"

namespace python {

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
  word length = spec.charLength();
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
        /* fall through */
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

RawObject formatStr(Thread* thread, const Str& str, FormatSpec* format) {
  if (format->positive_sign != '\0') {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "Sign not allowed in string format specifier");
  }
  if (format->alternate) {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "Alternate form (#) not allowed in string format specifier");
  }
  if (format->alignment == '=') {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "'=' alignment not allowed in string format specifier");
  }
  word width = format->width;
  word precision = format->precision;
  if (width == -1 && precision == -1) {
    return *str;
  }

  word char_length = str.charLength();
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
  word fill_char_length = fill_char.charLength();
  word padding_char_length = padding * fill_char_length;
  word result_char_length = str_end_index + padding_char_length;
  MutableBytes result(
      &scope, runtime->newMutableBytesUninitialized(result_char_length));
  word index = 0;
  word early_padding;
  if (format->alignment == '>') {
    early_padding = padding;
    padding = 0;
  } else if (format->alignment == '^') {
    word half = padding / 2;
    early_padding = half;
    padding = half + (padding % 2);
  } else {
    early_padding = 0;
    DCHECK(format->alignment == '<', "remaining assignment must be '<'");
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
// digits upfront. Because of this the function takes a `buf_end` argument and
// writes the digit before it. Returns a pointer to the last byte written.
static byte* uwordToDecimal(uword num, byte* buf_end) {
  byte* start = buf_end;
  do {
    *--start = '0' + num % 10;
    num /= 10;
  } while (num > 0);
  return start;
}

RawObject formatIntDecimalSimple(Thread* thread, const Int& value_int) {
  if (value_int.numDigits() == 1) {
    word value = value_int.asWord();
    uword magnitude = value >= 0 ? value : -static_cast<uword>(value);
    byte buffer[kUwordDigits10 + 1];
    byte* end = buffer + sizeof(buffer);
    byte* start = uwordToDecimal(magnitude, end);
    if (value < 0) *--start = '-';
    DCHECK(start >= buffer, "buffer underflow");
    return thread->runtime()->newStrWithAll(View<byte>(start, end - start));
  }
  HandleScope scope(thread);
  LargeInt large_int(&scope, *value_int);

  // Allocate space for intermediate results. We also convert a negative number
  // to a positive number of the same magnitude here.
  word num_digits = large_int.numDigits();
  std::unique_ptr<uword[]> temp_digits(new uword[num_digits]);
  bool negative = large_int.isNegative();
  if (!negative) {
    for (word i = 0; i < num_digits; ++i) {
      temp_digits[i] = large_int.digitAt(i);
    }
  } else {
    uword carry = 1;
    for (word i = 0; i < num_digits; ++i) {
      uword digit = large_int.digitAt(i);
      carry = __builtin_uaddl_overflow(~digit, carry, &temp_digits[i]);
    }
    // The complement of the highest bit in a negative number must be 0 so we
    // cannot overflow.
    DCHECK(carry == 0, "overflow");
  }
  word num_temp_digits = num_digits;

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
  word bit_length = large_int.bitLength();
  word max_chars = 1 + negative + bit_length * 309 / 1024;
  std::unique_ptr<byte[]> buffer(new byte[max_chars]);

  // The strategy here is to divide the large integer by continually dividing it
  // by `kUwordDigits10Pow`. `uwordToDecimal` can convert those remainders to
  // decimal digits.
  //
  // TODO(matthiasb): Future optimization ideas:
  // It seems cpythons algorithm is faster (for big numbers) in practive.
  // Their source claims it is (Knuth TAOCP, vol 2, section 4.4, method 1b).
  byte* end = buffer.get() + max_chars;
  byte* start = end;
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

  if (negative) *--start = '-';

  DCHECK(start >= buffer.get(), "buffer underflow");
  return thread->runtime()->newStrWithAll(View<byte>(start, end - start));
}

static word numBinaryDigits(const Int& value) {
  return value.isZero() ? 1 : value.bitLength();
}

static word numHexadecimalDigits(const Int& value) {
  return value.isZero() ? 1 : (value.bitLength() + 3) >> 2;
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

static void putHexadecimalDigits(Thread* thread, const MutableBytes& dest,
                                 word at, const Int& value, word num_digits) {
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
        dest.byteAtPut(--idx, "0123456789abcdef"[digit & 0xf]);
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
    dest.byteAtPut(--idx, "0123456789abcdef"[last_digit & 0xf]);
    last_digit >>= kBitsPerHexDigit;
  } while (last_digit != 0);
  DCHECK(idx == at, "unexpected number of digits");
}

RawObject formatIntSimpleBinary(Thread* thread, const Int& value) {
  word result_n_digits = numBinaryDigits(value);
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
  result.byteAtPut(index++, 'b');
  putBinaryDigits(thread, result, index, value, result_n_digits);
  return result.becomeStr();
}

RawObject formatIntSimpleHexadecimal(Thread* thread, const Int& value) {
  word result_n_digits = numHexadecimalDigits(value);
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
  result.byteAtPut(index++, 'x');
  putHexadecimalDigits(thread, result, index, value, result_n_digits);
  return result.becomeStr();
}

}  // namespace python
