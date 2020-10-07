#include "builtins.h"
#include "handles-decl.h"
#include "layout.h"
#include "module-builtins.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "symbols.h"
#include "thread.h"
#include "type-builtins.h"
#include "unicode-db.h"
#include "unicode.h"

namespace py {

void FUNC(unicodedata, __init_module__)(Thread* thread, const Module& module,
                                        View<byte> bytecode) {
  executeFrozenModule(thread, module, bytecode);

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type ucd_type(&scope, moduleAtById(thread, module, ID(UCD)));
  Layout ucd_layout(&scope, ucd_type.instanceLayout());
  Object old_ucd(&scope, runtime->newInstance(ucd_layout));
  moduleAtPutById(thread, module, ID(ucd_3_2_0), old_ucd);
}

static int32_t getCodePoint(const Str& src) {
  word length = src.length();
  if (length == 0) {
    return -1;
  }
  word char_length;
  int32_t result = src.codePointAt(0, &char_length);
  return (length == char_length) ? result : -1;
}

RawObject FUNC(unicodedata, bidirectional)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*obj)) {
    return thread->raiseRequiresType(obj, ID(str));
  }
  Str src(&scope, strUnderlying(*obj));
  int32_t code_point = getCodePoint(src);
  if (code_point == -1) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "bidirectional() argument must be a unicode character");
  }
  return kBidirectionalNames[databaseRecord(code_point)->bidirectional];
}

RawObject FUNC(unicodedata, category)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*obj)) {
    return thread->raiseRequiresType(obj, ID(str));
  }
  Str src(&scope, strUnderlying(*obj));
  int32_t code_point = getCodePoint(src);
  if (code_point == -1) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "category() argument must be a unicode character");
  }
  return kCategoryNames[databaseRecord(code_point)->category];
}

RawObject FUNC(unicodedata, decimal)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*obj)) {
    return thread->raiseRequiresType(obj, ID(str));
  }
  Str src(&scope, strUnderlying(*obj));
  int32_t code_point = getCodePoint(src);
  if (code_point == -1) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "decimal() argument must be a unicode character");
  }

  int8_t decimal = Unicode::toDecimal(code_point);
  if (decimal != -1) {
    return SmallInt::fromWord(decimal);
  }

  Object default_value(&scope, args.get(1));
  if (default_value.isUnbound()) {
    return thread->raiseWithFmt(LayoutId::kValueError, "not a decimal");
  }
  return *default_value;
}

static void writeDecomposition(UnicodeDecomposition decomp,
                               const MutableBytes& out) {
  word prefix_length = std::strlen(decomp.prefix);
  char* dst = reinterpret_cast<char*>(out.address());
  std::memcpy(dst, decomp.prefix, prefix_length);

  word i = prefix_length;
  for (word j = 0; j < decomp.count; j++) {
    if (i > 0) {
      dst[i++] = ' ';
    }
    std::sprintf(&dst[i], "%04X", decomp.code_points[j]);
    i += 4;
  }
  DCHECK(i == out.length(), "expected %d bytes, wrote %d", out.length(), i);
}

RawObject FUNC(unicodedata, decomposition)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*obj)) {
    return thread->raiseRequiresType(obj, ID(str));
  }
  Str src(&scope, strUnderlying(*obj));
  int32_t code_point = getCodePoint(src);
  if (code_point == -1) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "decomposition() argument must be a unicode character");
  }

  UnicodeDecomposition decomp = decomposeCodePoint(code_point);
  if (decomp.count == 0) {
    return Str::empty();
  }

  word prefix_length = std::strlen(decomp.prefix);
  word result_length = prefix_length + 5 * decomp.count;
  MutableBytes result(&scope,
                      runtime->newMutableBytesUninitialized(result_length));
  writeDecomposition(decomp, result);
  return result.becomeStr();
}

RawObject FUNC(unicodedata, digit)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*obj)) {
    return thread->raiseRequiresType(obj, ID(str));
  }
  Str src(&scope, strUnderlying(*obj));
  int32_t code_point = getCodePoint(src);
  if (code_point == -1) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "digit() argument must be a unicode character");
  }

  int8_t digit = Unicode::toDigit(code_point);
  if (digit != -1) {
    return SmallInt::fromWord(digit);
  }

  Object default_value(&scope, args.get(1));
  if (default_value.isUnbound()) {
    return thread->raiseWithFmt(LayoutId::kValueError, "not a digit");
  }
  return *default_value;
}

static RawObject copyName(Thread* thread, const Object& name_obj, byte* buffer,
                          word size) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfStr(*name_obj)) {
    Str name(&scope, strUnderlying(*name_obj));
    word length = name.length();
    if (length > size) {
      return thread->raiseWithFmt(LayoutId::kKeyError, "name too long");
    }
    name.copyTo(buffer, length);
    return SmallInt::fromWord(length);
  }
  if (runtime->isInstanceOfBytes(*name_obj)) {
    Bytes name(&scope, bytesUnderlying(*name_obj));
    word length = name.length();
    if (length > size) {
      return thread->raiseWithFmt(LayoutId::kKeyError, "name too long");
    }
    name.copyTo(buffer, length);
    return SmallInt::fromWord(length);
  }
  if (runtime->isByteslike(*name_obj)) {
    UNIMPLEMENTED("bytes-like other than bytes");
  }
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "a bytes-like object is required, not '%T'",
                              &name_obj);
}

RawObject FUNC(unicodedata, lookup)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object name(&scope, args.get(0));
  Runtime* runtime = thread->runtime();

  byte buffer[kMaxNameLength + 1];
  Object copy_result(&scope, copyName(thread, name, buffer, kMaxNameLength));
  if (copy_result.isErrorException()) {
    return *copy_result;
  }
  word length = SmallInt::cast(*copy_result).value();

  int32_t code_point = codePointFromNameOrNamedSequence(buffer, length);
  if (code_point < 0) {
    buffer[length] = '\0';
    return thread->raiseWithFmt(LayoutId::kKeyError,
                                "undefined character name '%s'", buffer);
  }
  if (Unicode::isNamedSequence(code_point)) {
    const UnicodeNamedSequence* seq = namedSequence(code_point);
    return runtime->newStrFromUTF32({seq->code_points, seq->length});
  }
  DCHECK_BOUND(code_point, kMaxUnicode);
  return SmallStr::fromCodePoint(code_point);
}

static NormalizationForm getForm(const Str& str) {
  if (str.equalsCStr("NFC")) {
    return NormalizationForm::kNFC;
  }
  if (str.equalsCStr("NFKC")) {
    return NormalizationForm::kNFKC;
  }
  if (str.equalsCStr("NFD")) {
    return NormalizationForm::kNFD;
  }
  if (str.equalsCStr("NFKD")) {
    return NormalizationForm::kNFKD;
  }
  return NormalizationForm::kInvalid;
}

static bool isNormalized(const Str& str, NormalizationForm form) {
  byte prev_combining = 0;
  for (word i = 0, length = str.length(), char_length; i < length;
       i += char_length) {
    int32_t code_point = str.codePointAt(i, &char_length);
    const UnicodeDatabaseRecord* record = databaseRecord(code_point);
    if ((record->quick_check & form) != 0) {
      return false;
    }
    byte combining = record->combining;
    if (combining != 0 && combining < prev_combining) {
      return false;
    }
    prev_combining = combining;
  }
  return true;
}

static void decomposeHangul(Thread* thread, const StrArray& buffer,
                            int32_t code_point) {
  int32_t offset = code_point - Unicode::kHangulSyllableStart;
  int32_t lead = Unicode::kHangulLeadStart + offset / Unicode::kHangulCodaCount;
  int32_t vowel =
      Unicode::kHangulVowelStart +
      (offset % Unicode::kHangulCodaCount) / Unicode::kHangulTrailCount;
  int32_t trail =
      Unicode::kHangulTrailStart + offset % Unicode::kHangulTrailCount;

  Runtime* runtime = thread->runtime();
  runtime->strArrayAddCodePoint(thread, buffer, lead);
  runtime->strArrayAddCodePoint(thread, buffer, vowel);
  if (trail != Unicode::kHangulTrailStart) {
    runtime->strArrayAddCodePoint(thread, buffer, trail);
  }
}

static void sortCanonical(const StrArray& buffer) {
  word char_length;
  int32_t code_point = buffer.codePointAt(0, &char_length);
  byte prev_combining = databaseRecord(code_point)->combining;
  word result_length = buffer.numItems();
  for (word i = char_length; i < result_length; i += char_length) {
    code_point = buffer.codePointAt(i, &char_length);
    byte combining = databaseRecord(code_point)->combining;
    if (combining == 0 || prev_combining <= combining) {
      prev_combining = combining;
      continue;
    }

    // Non-canonical order. Insert the code point in order.
    word first = 0;
    for (word j = buffer.offsetByCodePoints(i, -2); j >= 0;
         j = buffer.offsetByCodePoints(j, -1)) {
      word other_len;
      int32_t other = buffer.codePointAt(j, &other_len);
      byte other_combining = databaseRecord(other)->combining;
      if (other_combining == 0 || other_combining <= combining) {
        first = j + other_len;
        break;
      }
    }
    buffer.rotateCodePoint(first, i);
  }
}

static word skipIndex(word index, int32_t* skipped, word num_skipped) {
  for (word i = 0; i < num_skipped; i++) {
    if (skipped[i] == index) {
      skipped[i] = skipped[num_skipped - 1];
      return true;
    }
  }
  return false;
}

static RawObject compose(Thread* thread, const StrArray& decomposition) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  StrArray result(&scope, runtime->newStrArray());
  word decomp_length = decomposition.numItems();

  int32_t skipped[kMaxDecomposition];
  for (word char_length, i = 0, num_skipped = 0; i < decomp_length;
       i += char_length) {
    int32_t code_point = decomposition.codePointAt(i, &char_length);
    if (skipIndex(i, skipped, num_skipped)) {
      num_skipped--;
      continue;
    }

    // Hangul Composition
    if (Unicode::isHangulLead(code_point) && i + char_length < decomp_length) {
      word vowel_length;
      int32_t vowel = decomposition.codePointAt(i + char_length, &vowel_length);
      if (Unicode::isHangulVowel(vowel)) {
        int32_t lead = code_point - Unicode::kHangulLeadStart;
        vowel -= Unicode::kHangulVowelStart;
        code_point = Unicode::kHangulSyllableStart +
                     (lead * Unicode::kHangulVowelCount + vowel) *
                         Unicode::kHangulTrailCount;
        char_length += vowel_length;

        if (i + char_length < decomp_length) {
          word trail_length;
          int32_t trail =
              decomposition.codePointAt(i + char_length, &trail_length);
          if (Unicode::isHangulTrail(trail)) {
            code_point += trail - Unicode::kHangulTrailStart;
            char_length += trail_length;
          }
        }
        runtime->strArrayAddCodePoint(thread, result, code_point);
        continue;
      }
    }

    int32_t first = findNFCFirst(code_point);
    if (first == -1) {
      runtime->strArrayAddCodePoint(thread, result, code_point);
      continue;
    }

    // Find next unblocked character.
    byte combining = 0;
    for (word j = i + char_length, next_len; j < decomp_length; j += next_len) {
      int32_t next = decomposition.codePointAt(j, &next_len);
      byte next_combining = databaseRecord(next)->combining;
      if (combining != 0) {
        if (next_combining == 0) {
          break;
        }
        if (next_combining <= combining) {
          continue;
        }
      }

      int32_t last = findNFCLast(next);
      next = (last == -1) ? 0 : composeCodePoint(first, last);
      if (next == 0) {
        if (next_combining == 0) {
          break;
        }
        combining = next_combining;
        continue;
      }

      // Replace the original character
      code_point = next;
      DCHECK_INDEX(num_skipped, kMaxDecomposition);
      skipped[num_skipped++] = j;
      first = findNFCFirst(code_point);
      if (first == -1) {
        break;
      }
    }

    // Write the output character
    runtime->strArrayAddCodePoint(thread, result, code_point);
  }

  return runtime->strFromStrArray(result);
}

RawObject FUNC(unicodedata, normalize)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object form_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*form_obj)) {
    return thread->raiseRequiresType(form_obj, ID(str));
  }
  Object src_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*src_obj)) {
    return thread->raiseRequiresType(src_obj, ID(str));
  }

  Str src(&scope, strUnderlying(*src_obj));
  if (src.length() == 0) {
    return *src_obj;
  }

  Str form_str(&scope, strUnderlying(*form_obj));
  NormalizationForm form = getForm(form_str);
  if (form == NormalizationForm::kInvalid) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "invalid normalization form");
  }

  if (isNormalized(src, form)) {
    return *src_obj;
  }

  // Decomposition
  StrArray buffer(&scope, runtime->newStrArray());
  word src_length = src.length();
  runtime->strArrayEnsureCapacity(thread, buffer, src_length);
  bool canonical =
      form == NormalizationForm::kNFC || form == NormalizationForm::kNFD;
  for (word i = 0, char_length; i < src_length; i += char_length) {
    int32_t stack[kMaxDecomposition];
    stack[0] = src.codePointAt(i, &char_length);
    for (word depth = 1; depth > 0;) {
      int32_t code_point = stack[--depth];
      if (Unicode::isHangulSyllable(code_point)) {
        decomposeHangul(thread, buffer, code_point);
        continue;
      }

      UnicodeDecomposition decomp = decomposeCodePoint(code_point);
      if (decomp.count == 0 || (std::strlen(decomp.prefix) > 0 && canonical)) {
        runtime->strArrayAddCodePoint(thread, buffer, code_point);
        continue;
      }

      for (word j = decomp.count - 1; j >= 0; j--) {
        stack[depth++] = decomp.code_points[j];
      }
    }
  }

  sortCanonical(buffer);
  if (form == NormalizationForm::kNFD || form == NormalizationForm::kNFKD) {
    return runtime->strFromStrArray(buffer);
  }

  return compose(thread, buffer);
}

RawObject FUNC(unicodedata, numeric)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*obj)) {
    return thread->raiseRequiresType(obj, ID(str));
  }
  Str src(&scope, strUnderlying(*obj));
  int32_t code_point = getCodePoint(src);
  if (code_point == -1) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "numeric() argument must be a unicode character");
  }

  double value = Unicode::toNumeric(code_point);
  if (value != -1.0) {
    return runtime->newFloat(value);
  }

  Object default_value(&scope, args.get(1));
  if (default_value.isUnbound()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "not a numeric character");
  }
  return *default_value;
}

RawObject METH(UCD, bidirectional)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  Type self_type(&scope, runtime->typeOf(*self));
  Type ucd_type(&scope,
                runtime->lookupNameInModule(thread, ID(unicodedata), ID(UCD)));
  if (!typeIsSubclass(self_type, ucd_type)) {
    return thread->raiseRequiresType(self, ID(UCD));
  }
  Object obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*obj)) {
    return thread->raiseRequiresType(obj, ID(str));
  }
  Str src(&scope, strUnderlying(*obj));
  int32_t code_point = getCodePoint(src);
  if (code_point == -1) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "bidirectional() argument must be a unicode character");
  }
  const UnicodeChangeRecord* record = changeRecord(code_point);
  if (record->category == 0) {
    return kBidirectionalNames[0];
  }
  if (record->bidirectional != 0xff) {
    return kBidirectionalNames[record->bidirectional];
  }
  return kBidirectionalNames[databaseRecord(code_point)->bidirectional];
}

RawObject METH(UCD, category)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  Type self_type(&scope, runtime->typeOf(*self));
  Type ucd_type(&scope,
                runtime->lookupNameInModule(thread, ID(unicodedata), ID(UCD)));
  if (!typeIsSubclass(self_type, ucd_type)) {
    return thread->raiseRequiresType(self, ID(UCD));
  }
  Object obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*obj)) {
    return thread->raiseRequiresType(obj, ID(str));
  }
  Str src(&scope, strUnderlying(*obj));
  int32_t code_point = getCodePoint(src);
  if (code_point == -1) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "category() argument must be a unicode character");
  }
  byte category = changeRecord(code_point)->category;
  if (category != 0xff) {
    return kCategoryNames[category];
  }
  return kCategoryNames[databaseRecord(code_point)->category];
}

RawObject METH(UCD, decomposition)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  Type self_type(&scope, runtime->typeOf(*self));
  Type ucd_type(&scope,
                runtime->lookupNameInModule(thread, ID(unicodedata), ID(UCD)));
  if (!typeIsSubclass(self_type, ucd_type)) {
    return thread->raiseRequiresType(self, ID(UCD));
  }
  Object obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*obj)) {
    return thread->raiseRequiresType(obj, ID(str));
  }
  Str src(&scope, strUnderlying(*obj));
  int32_t code_point = getCodePoint(src);
  if (code_point == -1) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "decomposition() argument must be a unicode character");
  }

  if (changeRecord(code_point)->category == 0) {
    return Str::empty();
  }

  UnicodeDecomposition decomp = decomposeCodePoint(code_point);
  if (decomp.count == 0) {
    return Str::empty();
  }

  word prefix_length = std::strlen(decomp.prefix);
  word result_length = prefix_length + 5 * decomp.count;
  MutableBytes result(&scope,
                      runtime->newMutableBytesUninitialized(result_length));
  writeDecomposition(decomp, result);
  return result.becomeStr();
}

RawObject METH(UCD, decimal)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  Type self_type(&scope, runtime->typeOf(*self));
  Type ucd_type(&scope,
                runtime->lookupNameInModule(thread, ID(unicodedata), ID(UCD)));
  if (!typeIsSubclass(self_type, ucd_type)) {
    return thread->raiseRequiresType(self, ID(UCD));
  }
  Object obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*obj)) {
    return thread->raiseRequiresType(obj, ID(str));
  }
  Str src(&scope, strUnderlying(*obj));
  int32_t code_point = getCodePoint(src);
  if (code_point == -1) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "decimal() argument must be a unicode character");
  }

  word decimal;
  const UnicodeChangeRecord* record = changeRecord(code_point);
  if (record->category == 0) {
    decimal = -1;
  } else if (record->decimal != kMaxByte) {
    decimal = record->decimal;
  } else {
    decimal = Unicode::toDecimal(code_point);
  }

  if (decimal != -1) {
    return SmallInt::fromWord(decimal);
  }

  Object default_value(&scope, args.get(2));
  if (default_value.isUnbound()) {
    return thread->raiseWithFmt(LayoutId::kValueError, "not a decimal");
  }
  return *default_value;
}

RawObject METH(UCD, digit)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  Type self_type(&scope, runtime->typeOf(*self));
  Type ucd_type(&scope,
                runtime->lookupNameInModule(thread, ID(unicodedata), ID(UCD)));
  if (!typeIsSubclass(self_type, ucd_type)) {
    return thread->raiseRequiresType(self, ID(UCD));
  }
  Object obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*obj)) {
    return thread->raiseRequiresType(obj, ID(str));
  }
  Str src(&scope, strUnderlying(*obj));
  int32_t code_point = getCodePoint(src);
  if (code_point == -1) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "digit() argument must be a unicode character");
  }

  int8_t digit = Unicode::toDigit(code_point);
  if (digit != -1) {
    return SmallInt::fromWord(digit);
  }

  Object default_value(&scope, args.get(2));
  if (default_value.isUnbound()) {
    return thread->raiseWithFmt(LayoutId::kValueError, "not a digit");
  }
  return *default_value;
}

RawObject METH(UCD, normalize)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Object self(&scope, args.get(0));
  Type self_type(&scope, runtime->typeOf(*self));
  Type ucd_type(&scope,
                runtime->lookupNameInModule(thread, ID(unicodedata), ID(UCD)));
  if (!typeIsSubclass(self_type, ucd_type)) {
    return thread->raiseRequiresType(self, ID(UCD));
  }
  Object form_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*form_obj)) {
    return thread->raiseRequiresType(form_obj, ID(str));
  }
  Object src_obj(&scope, args.get(2));
  if (!runtime->isInstanceOfStr(*src_obj)) {
    return thread->raiseRequiresType(src_obj, ID(str));
  }

  Str src(&scope, strUnderlying(*src_obj));
  if (src.length() == 0) {
    return *src_obj;
  }

  Str form_str(&scope, strUnderlying(*form_obj));
  NormalizationForm form = getForm(form_str);
  if (form == NormalizationForm::kInvalid) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "invalid normalization form");
  }

  // Decomposition
  StrArray buffer(&scope, runtime->newStrArray());
  word src_length = src.length();
  runtime->strArrayEnsureCapacity(thread, buffer, src_length);
  bool canonical =
      form == NormalizationForm::kNFC || form == NormalizationForm::kNFD;
  for (word i = 0, char_length; i < src_length; i += char_length) {
    // longest decomposition in Unicode 3.2: U+FDFA
    int32_t stack[kMaxDecomposition];
    stack[0] = src.codePointAt(i, &char_length);
    for (word depth = 1; depth > 0;) {
      int32_t code_point = stack[--depth];
      if (Unicode::isHangulSyllable(code_point)) {
        decomposeHangul(thread, buffer, code_point);
        continue;
      }

      int32_t normalization = normalizeOld(code_point);
      if (normalization >= 0) {
        stack[depth++] = normalization;
        continue;
      }

      if (changeRecord(code_point)->category == 0) {
        runtime->strArrayAddCodePoint(thread, buffer, code_point);
        continue;
      }

      UnicodeDecomposition decomp = decomposeCodePoint(code_point);
      if (decomp.count == 0 || (std::strlen(decomp.prefix) > 0 && canonical)) {
        runtime->strArrayAddCodePoint(thread, buffer, code_point);
        continue;
      }

      for (word j = decomp.count - 1; j >= 0; j--) {
        stack[depth++] = decomp.code_points[j];
      }
    }
  }

  sortCanonical(buffer);
  if (form == NormalizationForm::kNFD || form == NormalizationForm::kNFKD) {
    return runtime->strFromStrArray(buffer);
  }

  return compose(thread, buffer);
}

}  // namespace py
