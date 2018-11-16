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
  ASSERT_TRUE(result->isString());
  EXPECT_PYSTRING_EQ(String::cast(*result), "testing123");

  // Shouldn't have interned the string during unmarshaling, so interning it
  // now should return the same string
  Handle<Object> str(&scope, runtime.newStringFromCString("testing123"));
  EXPECT_EQ(runtime.internString(str), *str);

  // Read a ref
  Marshal::Reader ref_reader(
      &scope, &runtime, "\xe1\x0a\x00\x00\x00testing321");
  Handle<Object> ref_result(&scope, ref_reader.readObject());
  EXPECT_EQ(ref_reader.numRefs(), 1);
  ASSERT_TRUE(ref_result->isString());
  EXPECT_PYSTRING_EQ(String::cast(*ref_result), "testing321");

  // Shouldn't have interned the string during unmarshaling, so interning it
  // now should return the same string
  Handle<Object> str2(&scope, runtime.newStringFromCString("testing321"));
  EXPECT_EQ(runtime.internString(str2), *str2);

  // Read an ascii string with negative length
  Marshal::Reader neg_reader(
      &scope, &runtime, "\x61\xf6\xff\xff\xfftesting123");
  EXPECT_EQ(neg_reader.readObject(), Error::object());
}

TEST(MarshalReaderTest, ReadTypeAsciiInterned) {
  Runtime runtime;
  HandleScope scope;

  // Read a non ref
  Marshal::Reader reader(&scope, &runtime, "\x41\x0a\x00\x00\x00testing123");
  Handle<Object> result(&scope, reader.readObject());
  EXPECT_EQ(reader.numRefs(), 0);
  ASSERT_TRUE(result->isString());
  EXPECT_PYSTRING_EQ(String::cast(*result), "testing123");

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Handle<Object> str(&scope, runtime.newStringFromCString("testing123"));
  EXPECT_NE(runtime.internString(str), *str);

  // Read a ref
  Marshal::Reader ref_reader(
      &scope, &runtime, "\xc1\x0a\x00\x00\x00testing321");
  Handle<Object> ref_result(&scope, ref_reader.readObject());
  EXPECT_EQ(ref_reader.numRefs(), 1);
  ASSERT_TRUE(ref_result->isString());
  EXPECT_PYSTRING_EQ(String::cast(*ref_result), "testing321");

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Handle<Object> str2(&scope, runtime.newStringFromCString("testing321"));
  EXPECT_NE(runtime.internString(str2), *str2);

  // Read an ascii string with negative length
  Marshal::Reader neg_reader(
      &scope, &runtime, "\x41\xf6\xff\xff\xfftesting123");
  EXPECT_EQ(neg_reader.readObject(), Error::object());
}

TEST(MarshalReaderTest, ReadTypeUnicode) {
  Runtime runtime;
  HandleScope scope;

  // Read a non ref
  Marshal::Reader reader(&scope, &runtime, "\x75\x0a\x00\x00\x00testing123");
  Handle<Object> result(&scope, reader.readObject());
  EXPECT_EQ(reader.numRefs(), 0);
  ASSERT_TRUE(result->isString());
  EXPECT_PYSTRING_EQ(String::cast(*result), "testing123");

  // Shouldn't have interned the string during unmarshaling, so interning it
  // now should return the same string
  Handle<Object> str(&scope, runtime.newStringFromCString("testing123"));
  EXPECT_EQ(runtime.internString(str), *str);

  // Read a ref
  Marshal::Reader ref_reader(
      &scope, &runtime, "\xf5\x0a\x00\x00\x00testing321");
  Handle<Object> ref_result(&scope, ref_reader.readObject());
  EXPECT_EQ(ref_reader.numRefs(), 1);
  ASSERT_TRUE(ref_result->isString());
  EXPECT_PYSTRING_EQ(String::cast(*ref_result), "testing321");

  // Shouldn't have interned the string during unmarshaling, so interning it
  // now should return the same string
  Handle<Object> str2(&scope, runtime.newStringFromCString("testing321"));
  EXPECT_EQ(runtime.internString(str2), *str2);

  // Read an unicode string with negative length
  Marshal::Reader neg_reader(
      &scope, &runtime, "\x75\xf6\xff\xff\xfftesting123");
  EXPECT_EQ(neg_reader.readObject(), Error::object());
}

TEST(MarshalReaderTest, ReadTypeInterned) {
  Runtime runtime;
  HandleScope scope;

  // Read a non ref
  Marshal::Reader reader(&scope, &runtime, "\x74\x0a\x00\x00\x00testing123");
  Handle<Object> result(&scope, reader.readObject());
  EXPECT_EQ(reader.numRefs(), 0);
  ASSERT_TRUE(result->isString());
  EXPECT_PYSTRING_EQ(String::cast(*result), "testing123");

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Handle<Object> str(&scope, runtime.newStringFromCString("testing123"));
  EXPECT_NE(runtime.internString(str), *str);

  // Read a ref
  Marshal::Reader ref_reader(
      &scope, &runtime, "\xf4\x0a\x00\x00\x00testing321");
  Handle<Object> ref_result(&scope, ref_reader.readObject());
  EXPECT_EQ(ref_reader.numRefs(), 1);
  ASSERT_TRUE(ref_result->isString());
  EXPECT_PYSTRING_EQ(String::cast(*ref_result), "testing321");

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Handle<Object> str2(&scope, runtime.newStringFromCString("testing321"));
  EXPECT_NE(runtime.internString(str2), *str2);

  // Read an interned string with negative length
  Marshal::Reader neg_reader(
      &scope, &runtime, "\x74\xf6\xff\xff\xfftesting123");
  EXPECT_EQ(neg_reader.readObject(), Error::object());
}

TEST(MarshalReaderTest, ReadTypeShortAsciiInterned) {
  Runtime runtime;
  HandleScope scope;

  // Read a non ref
  Marshal::Reader reader(&scope, &runtime, "\x5a\x0atesting123");
  Handle<Object> result(&scope, reader.readObject());
  EXPECT_EQ(reader.numRefs(), 0);
  ASSERT_TRUE(result->isString());
  EXPECT_PYSTRING_EQ(String::cast(*result), "testing123");

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Handle<Object> str(&scope, runtime.newStringFromCString("testing123"));
  EXPECT_NE(runtime.internString(str), *str);

  // Read a ref
  Marshal::Reader ref_reader(&scope, &runtime, "\xda\x0atesting321");
  Handle<Object> ref_result(&scope, ref_reader.readObject());
  EXPECT_EQ(ref_reader.numRefs(), 1);
  ASSERT_TRUE(ref_result->isString());
  EXPECT_PYSTRING_EQ(String::cast(*ref_result), "testing321");

  // Should have interned the string during unmarshaling, so interning it
  // now should return the canonical value.
  Handle<Object> str2(&scope, runtime.newStringFromCString("testing321"));
  EXPECT_NE(runtime.internString(str2), *str2);
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
  ASSERT_EQ(e, -2147483648); // INT32_MIN
}

TEST(MarshalReaderTest, ReadTypeIntMin) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(INT32_MIN)
  Marshal::Reader reader(&scope, &runtime, "\xe9\x00\x00\x00\x80");
  Object* result = reader.readObject();
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), INT32_MIN);
  ASSERT_EQ(reader.numRefs(), 1);
  EXPECT_EQ(reader.getRef(0), result);

  // marshal.dumps(INT32_MIN)
  Marshal::Reader reader_norefs(&scope, &runtime, "\x69\x00\x00\x00\x80");
  result = reader_norefs.readObject();
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), INT32_MIN);
  EXPECT_EQ(reader_norefs.numRefs(), 0);
}

TEST(MarshalReaderTest, ReadTypeIntMax) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(INT32_MAX)
  Marshal::Reader reader(&scope, &runtime, "\xe9\xff\xff\xff\x7f");
  Object* result = reader.readObject();
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), INT32_MAX);
  ASSERT_EQ(reader.numRefs(), 1);
  EXPECT_EQ(reader.getRef(0), result);

  // marshal.dumps(INT32_MAX)
  Marshal::Reader reader_norefs(&scope, &runtime, "\x69\xff\xff\xff\x7f");
  result = reader_norefs.readObject();
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), INT32_MAX);
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

TEST(MarshalReaderDeathTest, ReadNegativeTypeLong) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(INT32_MIN - 1)
  const char buf[] = "\xec\xfd\xff\xff\xff\x01\x00\x00\x00\x02\x00";
  EXPECT_DEATH(
      Marshal::Reader(&scope, &runtime, buf).readObject(),
      "unimplemented: TYPE_LONG");
}

TEST(MarshalReaderDeathTest, ReadPositiveTypeLong) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(INT32_MAX + 1)
  const char buf[] = "\xec\x03\x00\x00\x00\x00\x00\x00\x00\x02\x00";
  EXPECT_DEATH(
      Marshal::Reader(&scope, &runtime, buf).readObject(),
      "unimplemented: TYPE_LONG");
}

TEST(MarshalReaderDeathTest, ReadUnknownTypeCode) {
  Runtime runtime;
  HandleScope scope;
  const char buf[] = "\xff";
  EXPECT_DEATH(
      Marshal::Reader(&scope, &runtime, buf).readObject(),
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
  ASSERT_EQ(c, -32768); // INT16_MIN
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
      "\x08\x3C\x6D\x6F\x64\x75\x6C\x65\x3E\x01\x00\x00\x00\x73\x00\x00\x00\x00";
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

  ASSERT_TRUE(code->code()->isByteArray());
  EXPECT_NE(ByteArray::cast(code->code())->length(), 0);

  ASSERT_TRUE(code->varnames()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->varnames())->length(), 0);

  ASSERT_TRUE(code->cellvars()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->cellvars())->length(), 0);

  ASSERT_TRUE(code->consts()->isObjectArray());
  ASSERT_EQ(ObjectArray::cast(code->consts())->length(), 1);
  EXPECT_EQ(ObjectArray::cast(code->consts())->at(0), None::object());

  ASSERT_TRUE(code->freevars()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->freevars())->length(), 0);

  ASSERT_TRUE(code->filename()->isString());
  EXPECT_TRUE(String::cast(code->filename())->equalsCString("pass.py"));

  ASSERT_TRUE(code->name()->isString());
  EXPECT_TRUE(String::cast(code->name())->equalsCString("<module>"));

  ASSERT_TRUE(code->names()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->names())->length(), 0);

  EXPECT_EQ(code->firstlineno(), 1);

  ASSERT_TRUE(code->lnotab()->isByteArray());
  EXPECT_EQ(ByteArray::cast(code->lnotab())->length(), 0);
}

} // namespace python
