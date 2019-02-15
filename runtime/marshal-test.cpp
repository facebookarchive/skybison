#include "gtest/gtest.h"

#include <cmath>
#include <cstdint>

#include "globals.h"
#include "marshal.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(MarshalReaderTest, ReadBytes) {
  Runtime runtime;
  HandleScope scope;
  Marshal::Reader reader(&scope, &runtime, "hello, world");

  const byte* s1 = reader.readBytes(1);
  ASSERT_NE(s1, nullptr);
  EXPECT_EQ(*s1, 'h');

  const byte* s2 = reader.readBytes(2);
  ASSERT_NE(s2, nullptr);
  EXPECT_EQ(s2[0], 'e');
  EXPECT_EQ(s2[1], 'l');
}

TEST(MarshalReaderTest, ReadTypeAscii) {
  Runtime runtime;
  HandleScope scope;

  // Read a non ref
  Marshal::Reader reader(&scope, &runtime, "\x61\x0a\x00\x00\x00testing123");
  Object result(&scope, reader.readObject());
  EXPECT_EQ(reader.numRefs(), 0);
  EXPECT_TRUE(isStrEqualsCStr(*result, "testing123"));

  // Shouldn't have interned the string during unmarshaling, so interning it
  // now should return the same string
  Object str(&scope, runtime.newStrFromCStr("testing123"));
  EXPECT_EQ(runtime.internStr(str), *str);

  // Read a ref
  Marshal::Reader ref_reader(&scope, &runtime,
                             "\xe1\x0a\x00\x00\x00testing321");
  Object ref_result(&scope, ref_reader.readObject());
  EXPECT_EQ(ref_reader.numRefs(), 1);
  EXPECT_TRUE(isStrEqualsCStr(*ref_result, "testing321"));

  // Shouldn't have interned the string during unmarshaling, so interning it
  // now should return the same string
  Object str2(&scope, runtime.newStrFromCStr("testing321"));
  EXPECT_EQ(runtime.internStr(str2), *str2);

  // Read an ascii string with negative length
  Marshal::Reader neg_reader(&scope, &runtime,
                             "\x61\xf6\xff\xff\xfftesting123");
  EXPECT_EQ(neg_reader.readObject(), Error::object());
}

TEST(MarshalReaderTest, ReadTypeAsciiInterned) {
  Runtime runtime;
  HandleScope scope;

  // Read a non ref
  Marshal::Reader reader(&scope, &runtime, "\x41\x0a\x00\x00\x00testing123");
  Object result(&scope, reader.readObject());
  EXPECT_EQ(reader.numRefs(), 0);
  EXPECT_TRUE(isStrEqualsCStr(*result, "testing123"));

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Object str(&scope, runtime.newStrFromCStr("testing123"));
  EXPECT_NE(runtime.internStr(str), *str);

  // Read a ref
  Marshal::Reader ref_reader(&scope, &runtime,
                             "\xc1\x0a\x00\x00\x00testing321");
  Object ref_result(&scope, ref_reader.readObject());
  EXPECT_EQ(ref_reader.numRefs(), 1);
  EXPECT_TRUE(isStrEqualsCStr(*ref_result, "testing321"));

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Object str2(&scope, runtime.newStrFromCStr("testing321"));
  EXPECT_NE(runtime.internStr(str2), *str2);

  // Read an ascii string with negative length
  Marshal::Reader neg_reader(&scope, &runtime,
                             "\x41\xf6\xff\xff\xfftesting123");
  EXPECT_EQ(neg_reader.readObject(), Error::object());
}

TEST(MarshalReaderTest, ReadTypeUnicode) {
  Runtime runtime;
  HandleScope scope;

  // Read a non ref
  Marshal::Reader reader(&scope, &runtime, "\x75\x0a\x00\x00\x00testing123");
  Object result(&scope, reader.readObject());
  EXPECT_EQ(reader.numRefs(), 0);
  EXPECT_TRUE(isStrEqualsCStr(*result, "testing123"));

  // Shouldn't have interned the string during unmarshaling, so interning it
  // now should return the same string
  Object str(&scope, runtime.newStrFromCStr("testing123"));
  EXPECT_EQ(runtime.internStr(str), *str);

  // Read a ref
  Marshal::Reader ref_reader(&scope, &runtime,
                             "\xf5\x0a\x00\x00\x00testing321");
  Object ref_result(&scope, ref_reader.readObject());
  EXPECT_EQ(ref_reader.numRefs(), 1);
  EXPECT_TRUE(isStrEqualsCStr(*ref_result, "testing321"));

  // Shouldn't have interned the string during unmarshaling, so interning it
  // now should return the same string
  Object str2(&scope, runtime.newStrFromCStr("testing321"));
  EXPECT_EQ(runtime.internStr(str2), *str2);

  // Read an unicode string with negative length
  Marshal::Reader neg_reader(&scope, &runtime,
                             "\x75\xf6\xff\xff\xfftesting123");
  EXPECT_EQ(neg_reader.readObject(), Error::object());
}

TEST(MarshalReaderTest, ReadTypeInterned) {
  Runtime runtime;
  HandleScope scope;

  // Read a non ref
  Marshal::Reader reader(&scope, &runtime, "\x74\x0a\x00\x00\x00testing123");
  Object result(&scope, reader.readObject());
  EXPECT_EQ(reader.numRefs(), 0);
  EXPECT_TRUE(isStrEqualsCStr(*result, "testing123"));

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Object str(&scope, runtime.newStrFromCStr("testing123"));
  EXPECT_NE(runtime.internStr(str), *str);

  // Read a ref
  Marshal::Reader ref_reader(&scope, &runtime,
                             "\xf4\x0a\x00\x00\x00testing321");
  Object ref_result(&scope, ref_reader.readObject());
  EXPECT_EQ(ref_reader.numRefs(), 1);
  EXPECT_TRUE(isStrEqualsCStr(*ref_result, "testing321"));

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Object str2(&scope, runtime.newStrFromCStr("testing321"));
  EXPECT_NE(runtime.internStr(str2), *str2);

  // Read an interned string with negative length
  Marshal::Reader neg_reader(&scope, &runtime,
                             "\x74\xf6\xff\xff\xfftesting123");
  EXPECT_EQ(neg_reader.readObject(), Error::object());
}

TEST(MarshalReaderTest, ReadTypeShortAsciiInterned) {
  Runtime runtime;
  HandleScope scope;

  // Read a non ref
  Marshal::Reader reader(&scope, &runtime, "\x5a\x0atesting123");
  Object result(&scope, reader.readObject());
  EXPECT_EQ(reader.numRefs(), 0);
  EXPECT_TRUE(isStrEqualsCStr(*result, "testing123"));

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Object str(&scope, runtime.newStrFromCStr("testing123"));
  EXPECT_NE(runtime.internStr(str), *str);

  // Read a ref
  Marshal::Reader ref_reader(&scope, &runtime, "\xda\x0atesting321");
  Object ref_result(&scope, ref_reader.readObject());
  EXPECT_EQ(ref_reader.numRefs(), 1);
  EXPECT_TRUE(isStrEqualsCStr(*ref_result, "testing321"));

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Object str2(&scope, runtime.newStrFromCStr("testing321"));
  EXPECT_NE(runtime.internStr(str2), *str2);
}

TEST(MarshalReaderTest, ReadLong) {
  Runtime runtime;
  HandleScope scope;

  int32 a = Marshal::Reader(&scope, &runtime, "\x01\x00\x00\x00").readLong();
  EXPECT_EQ(a, 1);

  int32 b = Marshal::Reader(&scope, &runtime, "\x01\x02\x00\x00").readLong();
  ASSERT_EQ(b, 0x0201);

  int32 c = Marshal::Reader(&scope, &runtime, "\x01\x02\x03\x00").readLong();
  ASSERT_EQ(c, 0x030201);

  int32 d = Marshal::Reader(&scope, &runtime, "\x01\x02\x03\x04").readLong();
  ASSERT_EQ(d, 0x04030201);

  int32 e = Marshal::Reader(&scope, &runtime, "\x00\x00\x00\x80").readLong();
  ASSERT_EQ(e, -2147483648);  // INT32_MIN
}

TEST(MarshalReaderTest, ReadTypeIntMin) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(INT32_MIN)
  Marshal::Reader reader(&scope, &runtime, "\xe9\x00\x00\x00\x80");
  RawObject result = reader.readObject();
  EXPECT_TRUE(isIntEqualsWord(result, INT32_MIN));
  ASSERT_EQ(reader.numRefs(), 1);
  EXPECT_EQ(reader.getRef(0), result);

  // marshal.dumps(INT32_MIN)
  Marshal::Reader reader_norefs(&scope, &runtime, "\x69\x00\x00\x00\x80");
  result = reader_norefs.readObject();
  EXPECT_TRUE(isIntEqualsWord(result, INT32_MIN));
  EXPECT_EQ(reader_norefs.numRefs(), 0);
}

TEST(MarshalReaderTest, ReadTypeIntMax) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(INT32_MAX)
  Marshal::Reader reader(&scope, &runtime, "\xe9\xff\xff\xff\x7f");
  RawObject result = reader.readObject();
  EXPECT_TRUE(isIntEqualsWord(result, INT32_MAX));
  ASSERT_EQ(reader.numRefs(), 1);
  EXPECT_EQ(reader.getRef(0), result);

  // marshal.dumps(INT32_MAX)
  Marshal::Reader reader_norefs(&scope, &runtime, "\x69\xff\xff\xff\x7f");
  result = reader_norefs.readObject();
  EXPECT_TRUE(isIntEqualsWord(result, INT32_MAX));
  EXPECT_EQ(reader_norefs.numRefs(), 0);
}

TEST(MarshalReaderTest, ReadBinaryFloat) {
  Runtime runtime;
  HandleScope scope;

  double a =
      Marshal::Reader(&scope, &runtime, "\x00\x00\x00\x00\x00\x00\x00\x00")
          .readBinaryFloat();
  ASSERT_EQ(a, 0.0);

  double b = Marshal::Reader(&scope, &runtime, "\x00\x00\x00\x00\x00\x00\xf0?")
                 .readBinaryFloat();
  ASSERT_EQ(b, 1.0);

  double c =
      Marshal::Reader(&scope, &runtime, "\x00\x00\x00\x00\x00\x00\xf0\x7f")
          .readBinaryFloat();
  ASSERT_TRUE(std::isinf(c));

  double d =
      Marshal::Reader(&scope, &runtime, "\x00\x00\x00\x00\x00\x00\xf8\x7f")
          .readBinaryFloat();
  ASSERT_TRUE(std::isnan(d));
}

TEST(MarshalReaderTest, ReadNegativeTypeLong) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(kMinInt64) + 1
  const char buf[] =
      "\xec\xfb\xff\xff\xff\xff\x7f\xff\x7f\xff\x7f\xff\x7f\x07\x00";
  RawObject integer = Marshal::Reader(&scope, &runtime, buf).readObject();
  EXPECT_TRUE(isIntEqualsWord(integer, kMinInt64 + 1));

  // marshal.dumps(SmallInt::kMinValue)
  const char buf1[] =
      "\xec\xfb\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00";
  const word min_value = RawSmallInt::kMinValue;
  integer = Marshal::Reader(&scope, &runtime, buf1).readObject();
  EXPECT_TRUE(isIntEqualsWord(integer, min_value));
}

TEST(MarshalReaderTest, ReadPositiveTypeLong) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(kMaxInt64)
  const char buf[] =
      "\xec\x05\x00\x00\x00\xff\x7f\xff\x7f\xff\x7f\xff\x7f\x07\x00";
  RawObject integer = Marshal::Reader(&scope, &runtime, buf).readObject();
  EXPECT_TRUE(isIntEqualsWord(integer, kMaxInt64));

  // marshal.dumps(SmallInt::kMaxValue)
  const char buf1[] =
      "\xec\x05\x00\x00\x00\xff\x7f\xff\x7f\xff\x7f\xff\x7f\x03\x00";
  const word max_value = RawSmallInt::kMaxValue;
  integer = Marshal::Reader(&scope, &runtime, buf1).readObject();
  EXPECT_TRUE(isIntEqualsWord(integer, max_value));
}

TEST(MarshalReaderTest, ReadPositiveMultiDigitTypeLong) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(kMaxUint64)
  const char buf[] =
      "\xec\x05\x00\x00\x00\xff\x7f\xff\x7f\xff\x7f\xff\x7f\x0f\x00";

  RawObject obj = Marshal::Reader(&scope, &runtime, buf).readObject();
  ASSERT_TRUE(obj->isLargeInt());
  RawLargeInt integer = RawLargeInt::cast(obj);
  EXPECT_EQ(integer->numDigits(), 2);
  EXPECT_TRUE(integer->isPositive());
  EXPECT_EQ(integer->digitAt(0), kMaxUint64);

  // marshal.dumps(kMaxUint64 << 1)
  const char buf1[] =
      "\xec\x05\x00\x00\x00\xfe\x7f\xff\x7f\xff\x7f\xff\x7f\x1f\x00";

  obj = Marshal::Reader(&scope, &runtime, buf1).readObject();
  ASSERT_TRUE(obj->isLargeInt());
  integer = RawLargeInt::cast(obj);
  EXPECT_EQ(integer->numDigits(), 2);
  EXPECT_TRUE(integer->isPositive());
  EXPECT_EQ(integer->digitAt(0), uword{kMaxUint64 - 0x1});
  EXPECT_EQ(integer->digitAt(1), uword{1});

  // marshal.dumps(kMaxUint64 << 4)
  const char buf2[] =
      "\xec\x05\x00\x00\x00\xf0\x7f\xff\x7f\xff\x7f\xff\x7f\xff\x00";

  obj = Marshal::Reader(&scope, &runtime, buf2).readObject();
  ASSERT_TRUE(obj->isLargeInt());
  integer = RawLargeInt::cast(obj);
  EXPECT_EQ(integer->numDigits(), 2);
  EXPECT_TRUE(integer->isPositive());
  EXPECT_EQ(integer->digitAt(0), uword{kMaxUint64 - 0xF});
  EXPECT_EQ(integer->digitAt(1), uword{15});

  // marshal.dumps(1 << 63)
  const char buf3[] =
      "\xec\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00";

  obj = Marshal::Reader(&scope, &runtime, buf3).readObject();
  ASSERT_TRUE(obj->isLargeInt());
  integer = RawLargeInt::cast(obj);
  ASSERT_EQ(integer->numDigits(), 2);
  EXPECT_EQ(integer->digitAt(0), uword{1} << (kBitsPerWord - 1));
  EXPECT_EQ(integer->digitAt(1), uword{0});
}

TEST(MarshalReaderTest, ReadNegativeMultiDigitTypeLong) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(-kMaxUint64)
  const char buf[] =
      "\xec\xfb\xff\xff\xff\xff\x7f\xff\x7f\xff\x7f\xff\x7f\x0f\x00";
  RawObject obj = Marshal::Reader(&scope, &runtime, buf).readObject();
  ASSERT_TRUE(obj->isLargeInt());
  RawLargeInt integer = RawLargeInt::cast(obj);
  EXPECT_EQ(integer->numDigits(), 2);
  EXPECT_TRUE(integer->isNegative());
  EXPECT_EQ(integer->digitAt(0), uword{1});
  EXPECT_EQ(integer->digitAt(1), uword{kMaxUint64});

  // marshal.dumps(-(kMaxUint64 << 1))
  const char buf1[] =
      "\xec\xfb\xff\xff\xff\xfe\x7f\xff\x7f\xff\x7f\xff\x7f\x1f\x00";
  RawObject obj1 = Marshal::Reader(&scope, &runtime, buf1).readObject();
  ASSERT_TRUE(obj1->isLargeInt());
  RawLargeInt integer1 = RawLargeInt::cast(obj1);
  EXPECT_EQ(integer1->numDigits(), 2);
  EXPECT_TRUE(integer1->isNegative());
  EXPECT_EQ(integer1->digitAt(0), uword{2});  // ~(kMaxUint64 << 1) + 1
  EXPECT_EQ(integer1->digitAt(1), uword{kMaxUint64 ^ 1});  // sign_extend(~1)

  // marshal.dumps(-(kMaxUint64 << 4))
  const char buf2[] =
      "\xec\xfb\xff\xff\xff\xf0\x7f\xff\x7f\xff\x7f\xff\x7f\xff\x00";
  RawObject obj2 = Marshal::Reader(&scope, &runtime, buf2).readObject();
  ASSERT_TRUE(obj2->isLargeInt());
  RawLargeInt integer2 = RawLargeInt::cast(obj2);
  EXPECT_EQ(integer2->numDigits(), 2);
  EXPECT_TRUE(integer2->isNegative());
  EXPECT_EQ(integer2->digitAt(0), uword{16});  // ~(kMaxUint64 << 4) + 1
  EXPECT_EQ(integer2->digitAt(1), ~uword{16} + 1);

  // marshal.dumps(-(1 << 63))
  const char buf3[] =
      "\xec\xfb\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00";
  RawObject obj3 = Marshal::Reader(&scope, &runtime, buf3).readObject();
  ASSERT_TRUE(obj3->isLargeInt());
  RawLargeInt integer3 = RawLargeInt::cast(obj3);
  ASSERT_EQ(integer3->numDigits(), 1);
  EXPECT_EQ(integer3->digitAt(0), uword{1} << (kBitsPerWord - 1));
}

TEST(MarshalReaderDeathTest, ReadUnknownTypeCode) {
  Runtime runtime;
  HandleScope scope;
  const char buf[] = "\xff";
  EXPECT_DEATH(Marshal::Reader(&scope, &runtime, buf).readObject(),
               "unreachable: unknown type");
}

TEST(MarshalReaderTest, ReadShort) {
  Runtime runtime;
  HandleScope scope;

  int16 a = Marshal::Reader(&scope, &runtime, "\x01\x00").readShort();
  EXPECT_EQ(a, 1);

  int16 b = Marshal::Reader(&scope, &runtime, "\x01\x02").readShort();
  ASSERT_EQ(b, 0x0201);

  int16 c = Marshal::Reader(&scope, &runtime, "\x00\x80").readShort();
  ASSERT_EQ(c, kMinInt16);
}

TEST(MarshalReaderTest, ReadObjectNull) {
  Runtime runtime;
  HandleScope scope;
  RawObject a = Marshal::Reader(&scope, &runtime, "0").readObject();
  ASSERT_EQ(a, RawObject{0});
}

TEST(MarshalReaderTest, ReadObjectCode) {
  Runtime runtime;
  HandleScope scope;
  const char* buffer =
      "\x33\x0D\x0D\x0A\x3B\x5B\xB8\x59\x05\x00\x00\x00\xE3\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x40\x00\x00\x00\x73\x04\x00"
      "\x00\x00\x64\x00\x53\x00\x29\x01\x4E\xA9\x00\x72\x01\x00\x00\x00\x72\x01"
      "\x00\x00\x00\x72\x01\x00\x00\x00\xFA\x07\x70\x61\x73\x73\x2E\x70\x79\xDA"
      "\x08\x3C\x6D\x6F\x64\x75\x6C\x65\x3E\x01\x00\x00\x00\x73\x00\x00\x00"
      "\x00";
  Marshal::Reader reader(&scope, &runtime, buffer);

  int32 magic = reader.readLong();
  EXPECT_EQ(magic, 0x0A0D0D33);
  int32 mtime = reader.readLong();
  EXPECT_EQ(mtime, 0x59B85B3B);
  int32 size = reader.readLong();
  EXPECT_EQ(size, 0x05);

  RawObject raw_object = reader.readObject();
  ASSERT_TRUE(raw_object->isCode());

  RawCode code = RawCode::cast(raw_object);
  EXPECT_EQ(code->argcount(), 0);
  EXPECT_EQ(code->kwonlyargcount(), 0);
  EXPECT_EQ(code->nlocals(), 0);
  EXPECT_EQ(code->stacksize(), 1);
  EXPECT_TRUE(code->cell2arg()->isNoneType());
  EXPECT_EQ(code->flags(), Code::SIMPLE_CALL | Code::NOFREE);

  ASSERT_TRUE(code->code()->isBytes());
  EXPECT_NE(RawBytes::cast(code->code())->length(), 0);

  ASSERT_TRUE(code->varnames()->isTuple());
  EXPECT_EQ(RawTuple::cast(code->varnames())->length(), 0);

  ASSERT_TRUE(code->cellvars()->isTuple());
  EXPECT_EQ(RawTuple::cast(code->cellvars())->length(), 0);

  ASSERT_TRUE(code->consts()->isTuple());
  ASSERT_EQ(RawTuple::cast(code->consts())->length(), 1);
  EXPECT_EQ(RawTuple::cast(code->consts())->at(0), NoneType::object());

  ASSERT_TRUE(code->freevars()->isTuple());
  EXPECT_EQ(RawTuple::cast(code->freevars())->length(), 0);

  ASSERT_TRUE(code->filename()->isStr());
  EXPECT_TRUE(RawStr::cast(code->filename())->equalsCStr("pass.py"));

  ASSERT_TRUE(code->name()->isStr());
  EXPECT_TRUE(RawStr::cast(code->name())->equalsCStr("<module>"));

  ASSERT_TRUE(code->names()->isTuple());
  EXPECT_EQ(RawTuple::cast(code->names())->length(), 0);

  EXPECT_EQ(code->firstlineno(), 1);

  ASSERT_TRUE(code->lnotab()->isBytes());
  EXPECT_EQ(RawBytes::cast(code->lnotab())->length(), 0);
}

TEST(MarshalReaderTest, ReadObjectSetOnEmptySetReturnsEmptySet) {
  Runtime runtime;
  HandleScope scope;
  // marshal.dumps(set())
  Marshal::Reader reader(&scope, &runtime, "\xbc\x00\x00\x00\x00");
  Object obj(&scope, reader.readObject());
  ASSERT_TRUE(obj.isSet());
  EXPECT_EQ(RawSet::cast(*obj).numItems(), 0);
}

TEST(MarshalReaderTest, ReadObjectSetOnNonEmptySetReturnsCorrectNonEmptySet) {
  Runtime runtime;
  HandleScope scope;
  // marshal.dumps(set([1,2,3]))
  const char* buffer =
      "\xbc\x03\x00\x00\x00\xe9\x01\x00\x00\x00\xe9\x02\x00\x00\x00\xe9\x03\x00"
      "\x00\x00";
  Marshal::Reader reader(&scope, &runtime, buffer);
  Object obj(&scope, reader.readObject());
  ASSERT_TRUE(obj.isSet());
  Set set(&scope, *obj);
  EXPECT_EQ(set.numItems(), 3);
  Int one(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(runtime.setIncludes(set, one));
  Int two(&scope, SmallInt::fromWord(2));
  EXPECT_TRUE(runtime.setIncludes(set, two));
  Int three(&scope, SmallInt::fromWord(3));
  EXPECT_TRUE(runtime.setIncludes(set, three));
}

TEST(MarshalReaderTest, ReadObjectFrozenSetOnEmptySetReturnsEmptyFrozenSet) {
  Runtime runtime;
  HandleScope scope;
  // marshal.dumps(frozenset())
  Marshal::Reader reader(&scope, &runtime, "\xbe\x00\x00\x00\x00");
  Object obj(&scope, reader.readObject());
  ASSERT_TRUE(obj.isFrozenSet());
  EXPECT_EQ(RawFrozenSet::cast(*obj).numItems(), 0);
}

TEST(MarshalReaderTest,
     ReadObjectFrozenSetOnEmptySetReturnsEmptyFrozenSetSingleton) {
  Runtime runtime;
  HandleScope scope;
  // marshal.dumps(frozenset())
  Marshal::Reader reader(&scope, &runtime, "\xbe\x00\x00\x00\x00");
  Object obj(&scope, reader.readObject());
  ASSERT_EQ(*obj, runtime.emptyFrozenSet());
}

TEST(MarshalReaderTest,
     ReadObjectFrozenSetOnNonEmptySetReturnsCorrectNonEmptyFrozenSet) {
  Runtime runtime;
  HandleScope scope;
  // marshal.dumps(frozenset([1,2,3]))
  const char* buffer =
      "\xbe\x03\x00\x00\x00\xe9\x01\x00\x00\x00\xe9\x02\x00\x00\x00\xe9\x03\x00"
      "\x00\x00";
  Marshal::Reader reader(&scope, &runtime, buffer);
  Object obj(&scope, reader.readObject());
  ASSERT_TRUE(obj.isFrozenSet());
  FrozenSet set(&scope, *obj);
  EXPECT_EQ(set.numItems(), 3);
  Int one(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(runtime.setIncludes(set, one));
  Int two(&scope, SmallInt::fromWord(2));
  EXPECT_TRUE(runtime.setIncludes(set, two));
  Int three(&scope, SmallInt::fromWord(3));
  EXPECT_TRUE(runtime.setIncludes(set, three));
}

}  // namespace python
