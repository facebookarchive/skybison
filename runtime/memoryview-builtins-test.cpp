#include "gtest/gtest.h"

#include "handles.h"
#include "memoryview-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(MemoryViewBuiltinsTest, CastReturnsMemoryView) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0, 1, 2, 3};
  MemoryView view(&scope, newMemoryView(bytes, "f", ReadOnly::ReadWrite));
  Str new_format(&scope, runtime.newStrFromCStr("h"));
  Object result_obj(&scope,
                    runBuiltin(MemoryViewBuiltins::cast, view, new_format));
  ASSERT_TRUE(result_obj.isMemoryView());
  MemoryView result(&scope, *result_obj);
  EXPECT_NE(result, view);
  EXPECT_EQ(result.buffer(), view.buffer());
  EXPECT_TRUE(isStrEqualsCStr(view.format(), "f"));
  EXPECT_TRUE(isStrEqualsCStr(result.format(), "h"));
  EXPECT_EQ(view.readOnly(), result.readOnly());
}

TEST(MemoryViewBuiltinsTest, CastWithAtFormatReturnsMemoryView) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0, 1, 2, 3};
  MemoryView view(&scope, newMemoryView(bytes, "h", ReadOnly::ReadWrite));
  Str new_format(&scope, runtime.newStrFromCStr("@H"));
  Object result_obj(&scope,
                    runBuiltin(MemoryViewBuiltins::cast, view, new_format));
  ASSERT_TRUE(result_obj.isMemoryView());
  MemoryView result(&scope, *result_obj);
  EXPECT_NE(result, view);
  EXPECT_EQ(result.buffer(), view.buffer());
  EXPECT_TRUE(isStrEqualsCStr(view.format(), "h"));
  EXPECT_TRUE(isStrEqualsCStr(result.format(), "@H"));
  EXPECT_EQ(view.readOnly(), result.readOnly());
}

TEST(MemoryViewBuiltinsTest, CastWithBadLengthForFormatRaisesValueError) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0, 1, 2, 3, 4, 5};
  MemoryView view(&scope, newMemoryView(bytes, "B"));
  Str new_format(&scope, runtime.newStrFromCStr("f"));
  Object result(&scope, runBuiltin(MemoryViewBuiltins::cast, view, new_format));
  EXPECT_TRUE(
      raisedWithStr(*result, LayoutId::kValueError,
                    "memoryview: length is not a multiple of itemsize"));
}

TEST(MemoryViewBuiltinsTest, CastWithInvalidFormatRaisesValueError) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0, 1, 2, 3, 4, 5, 6, 7};
  MemoryView view(&scope, newMemoryView(bytes, "B"));
  Str new_format(&scope, runtime.newStrFromCStr(" "));
  Object result(&scope, runBuiltin(MemoryViewBuiltins::cast, view, new_format));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kValueError,
                            "memoryview: destination must be a native single "
                            "character format prefixed with an optional '@'"));
}

TEST(MemoryViewBuiltinsTest, CastWithNonStrFormatRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0, 1, 2, 3, 4, 5, 6, 7};
  MemoryView view(&scope, newMemoryView(bytes, "B"));
  Object not_str(&scope, NoneType::object());
  Object result(&scope, runBuiltin(MemoryViewBuiltins::cast, view, not_str));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kTypeError,
                            "format argument must be a string"));
}

TEST(MemoryViewBuiltinsTest, CastWithNonMemoryViewRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object none(&scope, NoneType::object());
  Str new_format(&scope, runtime.newStrFromCStr("I"));
  Object result(&scope, runBuiltin(MemoryViewBuiltins::cast, none, new_format));
  EXPECT_TRUE(raisedWithStr(
      *result, LayoutId::kTypeError,
      "'<anonymous>' requires a 'memoryview' object but got 'NoneType'"));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatbReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0xab, 0xc5};
  Object view(&scope, newMemoryView(bytes, "b"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -59));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatBReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0xee, 0xd8};
  Object view(&scope, newMemoryView(bytes, "B"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 216));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatcReturnsBytes) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0x03, 0x62};
  Object view(&scope, newMemoryView(bytes, "c"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  const byte expected_bytes[] = {0x62};
  EXPECT_TRUE(isBytesEqualsBytes(result, expected_bytes));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormathReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0xcd, 0x2c, 0x5c, 0xfc};
  Object view(&scope, newMemoryView(bytes, "h"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -932));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatHReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0xb2, 0x11, 0x94, 0xc0};
  Object view(&scope, newMemoryView(bytes, "H"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 49300));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatiReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0x30, 0x8A, 0x43, 0xF2, 0xE1, 0xD6, 0x56, 0xE4};
  Object view(&scope, newMemoryView(bytes, "i"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -464070943));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatAtiReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0x30, 0x8A, 0x43, 0xF2, 0xE1, 0xD6, 0x56, 0xE4};
  Object view(&scope, newMemoryView(bytes, "@i"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -464070943));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatIReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0x2, 0xBE, 0xA8, 0x3D, 0x74, 0x18, 0xEB, 0x8};
  Object view(&scope, newMemoryView(bytes, "I"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 149624948));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatlReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0xD8, 0x76, 0x97, 0xD1, 0x8B, 0xA1, 0xD2, 0x62,
                        0xD9, 0xD2, 0x50, 0x47, 0xC0, 0xA8, 0xB7, 0x81};
  Object view(&scope, newMemoryView(bytes, "l"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -9099618978295131431));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatLReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0x24, 0x37, 0x8B, 0x51, 0xCB, 0xB2, 0x16, 0xFB,
                        0xA6, 0xA9, 0x49, 0xB3, 0x59, 0x6A, 0x48, 0x62};
  Object view(&scope, newMemoryView(bytes, "L"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 7082027347532687782));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatqReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0x7,  0xE2, 0x42, 0x9E, 0x8F, 0xBF, 0xDB, 0x1B,
                        0x8C, 0x1C, 0x34, 0x40, 0x86, 0x41, 0x2B, 0x23};
  Object view(&scope, newMemoryView(bytes, "q"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 2534191260184616076));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatQReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0xD9, 0xC6, 0xD2, 0x40, 0xBD, 0x19, 0xA9, 0xC8,
                        0x8A, 0x1,  0x8B, 0xAF, 0x15, 0x36, 0xC7, 0xBD};
  Object view(&scope, newMemoryView(bytes, "Q"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  const uword expected_digits[] = {0xbdc73615af8b018aul, 0};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatnReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0xF2, 0x6F, 0xFA, 0x8B, 0x93, 0xC0, 0xED, 0x9D,
                        0x6D, 0x7C, 0xE3, 0xDC, 0x26, 0xEF, 0xB8, 0xEB};
  Object view(&scope, newMemoryView(bytes, "n"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -1461155128888034195l));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatNReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0x6B, 0x8F, 0x6,  0xA2, 0xE0, 0x13, 0x88, 0x47,
                        0x7E, 0xB6, 0x40, 0x7E, 0x6B, 0x2,  0x9,  0xC0};
  Object view(&scope, newMemoryView(bytes, "N"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  const uword expected_digits[] = {0xc009026b7e40b67eul, 0};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatfReturnsFloat) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0x67, 0x32, 0x23, 0x31, 0xB9, 0x70, 0xBC, 0x83};
  Object view(&scope, newMemoryView(bytes, "f"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(),
            std::strtod("-0x1.78e1720000000p-120", nullptr));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatdReturnsFloat) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0xEA, 0x43, 0xAD, 0x6F, 0x9D, 0x31, 0xE,  0x96,
                        0x28, 0x80, 0x1A, 0xD,  0x87, 0xC,  0xAC, 0x4B};
  Object view(&scope, newMemoryView(bytes, "d"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(),
            std::strtod("0x1.c0c870d1a8028p+187", nullptr));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatQuestionmarkReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0x92, 0xE1, 0x57, 0xEA, 0x81, 0xA8};
  Object view(&scope, newMemoryView(bytes, "?"));
  Int index(&scope, runtime.newInt(3));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_EQ(result, Bool::trueObj());
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatQuestionmarkReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0x92, 0xE1, 0, 0xEA, 0x81, 0xA8};
  Object view(&scope, newMemoryView(bytes, "?"));
  Int index(&scope, runtime.newInt(2));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_EQ(result, Bool::falseObj());
}

TEST(MemoryViewBuiltins, GetItemWithNegativeIndexReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0, 1, 2, 3, 4, 5, 6, 7};
  Object view(&scope, newMemoryView(bytes, "h"));
  Int index(&scope, runtime.newInt(-2));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 0x504));
}

TEST(MemoryViewBuiltinsTest, GetItemWithNonMemoryViewRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object none(&scope, NoneType::object());
  Int index(&scope, runtime.newInt(0));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, none, index));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(MemoryViewBuiltinsTest, GetItemWithTooBigIndexRaisesIndexError) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0, 1, 2, 3, 4, 5, 6, 7};
  Object view(&scope, newMemoryView(bytes, "I"));
  Int index(&scope, runtime.newInt(2));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(
      raisedWithStr(*result, LayoutId::kIndexError, "index out of bounds"));
}

TEST(MemoryViewBuiltins, GetItemWithOverflowingIndexRaisesIndexError) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0, 1, 2, 3, 4, 5, 6, 7};
  Object view(&scope, newMemoryView(bytes, "I"));
  Int index(&scope, runtime.newInt(kMaxWord / 2));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(
      raisedWithStr(*result, LayoutId::kIndexError, "index out of bounds"));
}

TEST(MemoryViewBuiltinsTest, DunderLenWithMemoryViewFormatBReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0, 1, 2};
  MemoryView view(&scope, newMemoryView(bytes, "B"));
  Object result(&scope, runBuiltin(MemoryViewBuiltins::dunderLen, view));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
}

TEST(MemoryViewBuiltinsTest, DunderLenWithMemoryViewFormatfReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[] = {0, 1, 2, 3, 4, 5, 6, 7};
  MemoryView view(&scope, newMemoryView(bytes, "f"));
  Object result(&scope, runBuiltin(MemoryViewBuiltins::dunderLen, view));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST(MemoryViewBuiltinsTest, DunderLenWithNonMemoryViewRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object none(&scope, NoneType::object());
  EXPECT_TRUE(raised(runBuiltin(MemoryViewBuiltins::dunderLen, none),
                     LayoutId::kTypeError));
}

TEST(MemoryViewBuiltinsTest, DunderNewWithBytesReturnsMemoryView) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes_array[] = {0xa9};
  Bytes bytes(&scope, runtime.newBytesWithAll(bytes_array));
  Type type(&scope, runtime.typeAt(LayoutId::kMemoryView));
  Object result_obj(&scope,
                    runBuiltin(MemoryViewBuiltins::dunderNew, type, bytes));
  ASSERT_TRUE(result_obj.isMemoryView());
  MemoryView view(&scope, *result_obj);
  EXPECT_EQ(view.buffer(), bytes);
  EXPECT_TRUE(isStrEqualsCStr(view.format(), "B"));
  EXPECT_TRUE(view.readOnly());
}

TEST(MemoryViewBuiltinsTest, DunderNewWithByteArrayReturnsMemoryView) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, runtime.typeAt(LayoutId::kMemoryView));
  ByteArray bytearray(&scope, runtime.newByteArray());
  const byte byte_array[] = {0xce};
  runtime.byteArrayExtend(thread, bytearray, byte_array);
  Object result_obj(&scope,
                    runBuiltin(MemoryViewBuiltins::dunderNew, type, bytearray));
  ASSERT_TRUE(result_obj.isMemoryView());
  MemoryView view(&scope, *result_obj);
  EXPECT_EQ(view.buffer(), bytearray.bytes());
  EXPECT_TRUE(isStrEqualsCStr(view.format(), "B"));
  EXPECT_FALSE(view.readOnly());
}

TEST(MemoryViewBuiltinsTest, DunderNewWithMemoryViewReturnsMemoryView) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kMemoryView));
  const byte bytes[] = {0x96, 0xfc};
  MemoryView view(&scope, newMemoryView(bytes, "H", ReadOnly::ReadWrite));
  Object result_obj(&scope,
                    runBuiltin(MemoryViewBuiltins::dunderNew, type, view));
  ASSERT_TRUE(result_obj.isMemoryView());
  MemoryView result(&scope, *result_obj);
  EXPECT_NE(result, view);
  EXPECT_EQ(view.buffer(), result.buffer());
  EXPECT_TRUE(Str::cast(view.format()).equals(result.format()));
  EXPECT_EQ(view.readOnly(), result.readOnly());
}

TEST(MemoryViewBuiltinsTest, DunderNewWithUnsupportedObjectRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kMemoryView));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(MemoryViewBuiltins::dunderNew, type, none));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kTypeError,
                            "memoryview: a bytes-like object is required"));
}

TEST(MemoryViewBuiltinsTest, DunderNewWithInvalidTypeRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object not_a_type(&scope, NoneType::object());
  Bytes bytes(&scope, runtime.newBytesWithAll(View<byte>(nullptr, 0)));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderNew, not_a_type, bytes));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kTypeError,
                            "memoryview.__new__(X): X is not 'memoryview'"));
}

}  // namespace python
