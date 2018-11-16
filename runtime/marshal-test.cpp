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
  Handle<Object> result(&scope, reader.readObject());
  EXPECT_EQ(reader.numRefs(), 0);
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(Str::cast(*result), "testing123");

  // Shouldn't have interned the string during unmarshaling, so interning it
  // now should return the same string
  Handle<Object> str(&scope, runtime.newStrFromCStr("testing123"));
  EXPECT_EQ(runtime.internStr(str), *str);

  // Read a ref
  Marshal::Reader ref_reader(&scope, &runtime,
                             "\xe1\x0a\x00\x00\x00testing321");
  Handle<Object> ref_result(&scope, ref_reader.readObject());
  EXPECT_EQ(ref_reader.numRefs(), 1);
  ASSERT_TRUE(ref_result->isStr());
  EXPECT_PYSTRING_EQ(Str::cast(*ref_result), "testing321");

  // Shouldn't have interned the string during unmarshaling, so interning it
  // now should return the same string
  Handle<Object> str2(&scope, runtime.newStrFromCStr("testing321"));
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
  Handle<Object> result(&scope, reader.readObject());
  EXPECT_EQ(reader.numRefs(), 0);
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(Str::cast(*result), "testing123");

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Handle<Object> str(&scope, runtime.newStrFromCStr("testing123"));
  EXPECT_NE(runtime.internStr(str), *str);

  // Read a ref
  Marshal::Reader ref_reader(&scope, &runtime,
                             "\xc1\x0a\x00\x00\x00testing321");
  Handle<Object> ref_result(&scope, ref_reader.readObject());
  EXPECT_EQ(ref_reader.numRefs(), 1);
  ASSERT_TRUE(ref_result->isStr());
  EXPECT_PYSTRING_EQ(Str::cast(*ref_result), "testing321");

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Handle<Object> str2(&scope, runtime.newStrFromCStr("testing321"));
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
  Handle<Object> result(&scope, reader.readObject());
  EXPECT_EQ(reader.numRefs(), 0);
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(Str::cast(*result), "testing123");

  // Shouldn't have interned the string during unmarshaling, so interning it
  // now should return the same string
  Handle<Object> str(&scope, runtime.newStrFromCStr("testing123"));
  EXPECT_EQ(runtime.internStr(str), *str);

  // Read a ref
  Marshal::Reader ref_reader(&scope, &runtime,
                             "\xf5\x0a\x00\x00\x00testing321");
  Handle<Object> ref_result(&scope, ref_reader.readObject());
  EXPECT_EQ(ref_reader.numRefs(), 1);
  ASSERT_TRUE(ref_result->isStr());
  EXPECT_PYSTRING_EQ(Str::cast(*ref_result), "testing321");

  // Shouldn't have interned the string during unmarshaling, so interning it
  // now should return the same string
  Handle<Object> str2(&scope, runtime.newStrFromCStr("testing321"));
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
  Handle<Object> result(&scope, reader.readObject());
  EXPECT_EQ(reader.numRefs(), 0);
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(Str::cast(*result), "testing123");

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Handle<Object> str(&scope, runtime.newStrFromCStr("testing123"));
  EXPECT_NE(runtime.internStr(str), *str);

  // Read a ref
  Marshal::Reader ref_reader(&scope, &runtime,
                             "\xf4\x0a\x00\x00\x00testing321");
  Handle<Object> ref_result(&scope, ref_reader.readObject());
  EXPECT_EQ(ref_reader.numRefs(), 1);
  ASSERT_TRUE(ref_result->isStr());
  EXPECT_PYSTRING_EQ(Str::cast(*ref_result), "testing321");

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Handle<Object> str2(&scope, runtime.newStrFromCStr("testing321"));
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
  Handle<Object> result(&scope, reader.readObject());
  EXPECT_EQ(reader.numRefs(), 0);
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(Str::cast(*result), "testing123");

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Handle<Object> str(&scope, runtime.newStrFromCStr("testing123"));
  EXPECT_NE(runtime.internStr(str), *str);

  // Read a ref
  Marshal::Reader ref_reader(&scope, &runtime, "\xda\x0atesting321");
  Handle<Object> ref_result(&scope, ref_reader.readObject());
  EXPECT_EQ(ref_reader.numRefs(), 1);
  ASSERT_TRUE(ref_result->isStr());
  EXPECT_PYSTRING_EQ(Str::cast(*ref_result), "testing321");

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Handle<Object> str2(&scope, runtime.newStrFromCStr("testing321"));
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
  Object* result = reader.readObject();
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(SmallInt::cast(result)->value(), INT32_MIN);
  ASSERT_EQ(reader.numRefs(), 1);
  EXPECT_EQ(reader.getRef(0), result);

  // marshal.dumps(INT32_MIN)
  Marshal::Reader reader_norefs(&scope, &runtime, "\x69\x00\x00\x00\x80");
  result = reader_norefs.readObject();
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(SmallInt::cast(result)->value(), INT32_MIN);
  EXPECT_EQ(reader_norefs.numRefs(), 0);
}

TEST(MarshalReaderTest, ReadTypeIntMax) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(INT32_MAX)
  Marshal::Reader reader(&scope, &runtime, "\xe9\xff\xff\xff\x7f");
  Object* result = reader.readObject();
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(SmallInt::cast(result)->value(), INT32_MAX);
  ASSERT_EQ(reader.numRefs(), 1);
  EXPECT_EQ(reader.getRef(0), result);

  // marshal.dumps(INT32_MAX)
  Marshal::Reader reader_norefs(&scope, &runtime, "\x69\xff\xff\xff\x7f");
  result = reader_norefs.readObject();
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(SmallInt::cast(result)->value(), INT32_MAX);
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
  Object* integer = Marshal::Reader(&scope, &runtime, buf).readObject();
  ASSERT_TRUE(integer->isInt());
  EXPECT_EQ(Int::cast(integer)->asWord(), kMinInt64 + 1);

  // marshal.dumps(SmallInt::kMinValue)
  const char buf1[] =
      "\xec\xfb\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00";
  const word min_value = SmallInt::kMinValue;
  integer = Marshal::Reader(&scope, &runtime, buf1).readObject();
  ASSERT_TRUE(integer->isInt());
  EXPECT_EQ(Int::cast(integer)->asWord(), min_value);
}

TEST(MarshalReaderTest, ReadPositiveTypeLong) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(kMaxInt64)
  const char buf[] =
      "\xec\x05\x00\x00\x00\xff\x7f\xff\x7f\xff\x7f\xff\x7f\x07\x00";
  Object* integer = Marshal::Reader(&scope, &runtime, buf).readObject();
  ASSERT_TRUE(integer->isInt());
  EXPECT_EQ(Int::cast(integer)->asWord(), kMaxInt64);

  // marshal.dumps(SmallInt::kMaxValue)
  const char buf1[] =
      "\xec\x05\x00\x00\x00\xff\x7f\xff\x7f\xff\x7f\xff\x7f\x03\x00";
  const word max_value = SmallInt::kMaxValue;
  integer = Marshal::Reader(&scope, &runtime, buf1).readObject();
  ASSERT_TRUE(integer->isInt());
  EXPECT_EQ(Int::cast(integer)->asWord(), max_value);
}

TEST(MarshalReaderTest, ReadPositiveMultiDigitTypeLong) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(kMaxUint64)
  const char buf[] =
      "\xec\x05\x00\x00\x00\xff\x7f\xff\x7f\xff\x7f\xff\x7f\x0f\x00";

  Object* obj = Marshal::Reader(&scope, &runtime, buf).readObject();
  ASSERT_TRUE(obj->isLargeInt());
  LargeInt* integer = LargeInt::cast(obj);
  EXPECT_EQ(integer->numDigits(), 2);
  EXPECT_TRUE(integer->isPositive());
  EXPECT_EQ(integer->digitAt(0), kMaxUword);

  // marshal.dumps(kMaxUint64 << 1)
  const char buf1[] =
      "\xec\x05\x00\x00\x00\xfe\x7f\xff\x7f\xff\x7f\xff\x7f\x1f\x00";

  obj = Marshal::Reader(&scope, &runtime, buf1).readObject();
  ASSERT_TRUE(obj->isLargeInt());
  integer = LargeInt::cast(obj);
  EXPECT_EQ(integer->numDigits(), 2);
  EXPECT_TRUE(integer->isPositive());
  EXPECT_EQ(integer->digitAt(0), kMaxUint64 - 1);
  EXPECT_EQ(integer->digitAt(1), 1);

  // marshal.dumps(kMaxUint64 << 4)
  const char buf2[] =
      "\xec\x05\x00\x00\x00\xf0\x7f\xff\x7f\xff\x7f\xff\x7f\xff\x00";

  obj = Marshal::Reader(&scope, &runtime, buf2).readObject();
  ASSERT_TRUE(obj->isLargeInt());
  integer = LargeInt::cast(obj);
  EXPECT_EQ(integer->numDigits(), 2);
  EXPECT_TRUE(integer->isPositive());
  EXPECT_EQ(integer->digitAt(0), kMaxUint64 - 0xF);
  EXPECT_EQ(integer->digitAt(1), 0xF);

  // marshal.dumps(1 << 63)
  const char buf3[] =
      "\xec\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00";

  obj = Marshal::Reader(&scope, &runtime, buf3).readObject();
  ASSERT_TRUE(obj->isLargeInt());
  integer = LargeInt::cast(obj);
  ASSERT_EQ(integer->numDigits(), 2);
  EXPECT_EQ(integer->digitAt(0), 1ULL << (kBitsPerWord - 1));
  EXPECT_EQ(integer->digitAt(1), 0);
}

TEST(MarshalReaderTest, ReadNegativeMultiDigitTypeLong) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(-kMaxUint64)
  const char buf[] =
      "\xec\xfb\xff\xff\xff\xff\x7f\xff\x7f\xff\x7f\xff\x7f\x0f\x00";
  Object* obj = Marshal::Reader(&scope, &runtime, buf).readObject();
  ASSERT_TRUE(obj->isLargeInt());
  LargeInt* integer = LargeInt::cast(obj);
  EXPECT_EQ(integer->numDigits(), 2);
  EXPECT_TRUE(integer->isNegative());
  EXPECT_EQ(integer->digitAt(0), 1);
  EXPECT_EQ(integer->digitAt(1), kMaxUint64);

  // marshal.dumps(-(kMaxUint64 << 1))
  const char buf1[] =
      "\xec\xfb\xff\xff\xff\xfe\x7f\xff\x7f\xff\x7f\xff\x7f\x1f\x00";
  Object* obj1 = Marshal::Reader(&scope, &runtime, buf1).readObject();
  ASSERT_TRUE(obj1->isLargeInt());
  LargeInt* integer1 = LargeInt::cast(obj1);
  EXPECT_EQ(integer1->numDigits(), 2);
  EXPECT_TRUE(integer1->isNegative());
  EXPECT_EQ(integer1->digitAt(0), 2);               // ~(kMaxUint64 << 1) + 1
  EXPECT_EQ(integer1->digitAt(1), kMaxUint64 ^ 1);  // sign_extend(~1)

  // marshal.dumps(-(kMaxUint64 << 4))
  const char buf2[] =
      "\xec\xfb\xff\xff\xff\xf0\x7f\xff\x7f\xff\x7f\xff\x7f\xff\x00";
  Object* obj2 = Marshal::Reader(&scope, &runtime, buf2).readObject();
  ASSERT_TRUE(obj2->isLargeInt());
  LargeInt* integer2 = LargeInt::cast(obj2);
  EXPECT_EQ(integer2->numDigits(), 2);
  EXPECT_TRUE(integer2->isNegative());
  EXPECT_EQ(integer2->digitAt(0), 16);  // ~(kMaxUint64 << 4) + 1
  EXPECT_EQ(integer2->digitAt(1), ~(16ULL) + 1);

  // marshal.dumps(-(1 << 63))
  const char buf3[] =
      "\xec\xfb\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00";
  Object* obj3 = Marshal::Reader(&scope, &runtime, buf3).readObject();
  ASSERT_TRUE(obj3->isLargeInt());
  LargeInt* integer3 = LargeInt::cast(obj3);
  ASSERT_EQ(integer3->numDigits(), 1);
  EXPECT_EQ(integer3->digitAt(0), kMinWord);
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
  Object* a = Marshal::Reader(&scope, &runtime, "0").readObject();
  ASSERT_EQ(a, nullptr);
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

  Object* raw_object = reader.readObject();
  ASSERT_TRUE(raw_object->isCode());

  Code* code = Code::cast(raw_object);
  EXPECT_EQ(code->argcount(), 0);
  EXPECT_EQ(code->kwonlyargcount(), 0);
  EXPECT_EQ(code->nlocals(), 0);
  EXPECT_EQ(code->stacksize(), 1);
  EXPECT_EQ(code->cell2arg(), 0);
  EXPECT_EQ(code->flags(), Code::SIMPLE_CALL | Code::NOFREE);

  ASSERT_TRUE(code->code()->isBytes());
  EXPECT_NE(Bytes::cast(code->code())->length(), 0);

  ASSERT_TRUE(code->varnames()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->varnames())->length(), 0);

  ASSERT_TRUE(code->cellvars()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->cellvars())->length(), 0);

  ASSERT_TRUE(code->consts()->isObjectArray());
  ASSERT_EQ(ObjectArray::cast(code->consts())->length(), 1);
  EXPECT_EQ(ObjectArray::cast(code->consts())->at(0), None::object());

  ASSERT_TRUE(code->freevars()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->freevars())->length(), 0);

  ASSERT_TRUE(code->filename()->isStr());
  EXPECT_TRUE(Str::cast(code->filename())->equalsCStr("pass.py"));

  ASSERT_TRUE(code->name()->isStr());
  EXPECT_TRUE(Str::cast(code->name())->equalsCStr("<module>"));

  ASSERT_TRUE(code->names()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->names())->length(), 0);

  EXPECT_EQ(code->firstlineno(), 1);

  ASSERT_TRUE(code->lnotab()->isBytes());
  EXPECT_EQ(Bytes::cast(code->lnotab())->length(), 0);
}

}  // namespace python
