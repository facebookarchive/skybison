#include "memoryview-builtins.h"

#include "gtest/gtest.h"

#include "builtins.h"
#include "handles.h"
#include "test-utils.h"

namespace py {
namespace testing {

using MemoryViewBuiltinsTest = RuntimeFixture;

TEST_F(MemoryViewBuiltinsTest, CastReturnsMemoryView) {
  HandleScope scope(thread_);
  const byte bytes[] = {0, 1, 2, 3};
  MemoryView view(&scope, newMemoryView(bytes, "f", ReadOnly::ReadWrite));
  Str new_format(&scope, runtime_->newStrFromCStr("h"));
  Object result_obj(&scope,
                    runBuiltin(METH(memoryview, cast), view, new_format));
  ASSERT_TRUE(result_obj.isMemoryView());
  MemoryView result(&scope, *result_obj);
  EXPECT_NE(result, view);
  EXPECT_EQ(result.buffer(), view.buffer());
  EXPECT_TRUE(isStrEqualsCStr(view.format(), "f"));
  EXPECT_TRUE(isStrEqualsCStr(result.format(), "h"));
  EXPECT_EQ(view.readOnly(), result.readOnly());
}

TEST_F(MemoryViewBuiltinsTest, CastWithAtFormatReturnsMemoryView) {
  HandleScope scope(thread_);
  const byte bytes[] = {0, 1, 2, 3};
  MemoryView view(&scope, newMemoryView(bytes, "h", ReadOnly::ReadWrite));
  Str new_format(&scope, runtime_->newStrFromCStr("@H"));
  Object result_obj(&scope,
                    runBuiltin(METH(memoryview, cast), view, new_format));
  ASSERT_TRUE(result_obj.isMemoryView());
  MemoryView result(&scope, *result_obj);
  EXPECT_NE(result, view);
  EXPECT_EQ(result.buffer(), view.buffer());
  EXPECT_TRUE(isStrEqualsCStr(view.format(), "h"));
  EXPECT_TRUE(isStrEqualsCStr(result.format(), "@H"));
  EXPECT_EQ(view.readOnly(), result.readOnly());
}

TEST_F(MemoryViewBuiltinsTest, CastWithBadLengthForFormatRaisesValueError) {
  HandleScope scope(thread_);
  const byte bytes[] = {0, 1, 2, 3, 4, 5};
  MemoryView view(&scope, newMemoryView(bytes, "B"));
  Str new_format(&scope, runtime_->newStrFromCStr("f"));
  Object result(&scope, runBuiltin(METH(memoryview, cast), view, new_format));
  EXPECT_TRUE(
      raisedWithStr(*result, LayoutId::kValueError,
                    "memoryview: length is not a multiple of itemsize"));
}

TEST_F(MemoryViewBuiltinsTest, CastWithInvalidFormatRaisesValueError) {
  HandleScope scope(thread_);
  const byte bytes[] = {0, 1, 2, 3, 4, 5, 6, 7};
  MemoryView view(&scope, newMemoryView(bytes, "B"));
  Str new_format(&scope, runtime_->newStrFromCStr(" "));
  Object result(&scope, runBuiltin(METH(memoryview, cast), view, new_format));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kValueError,
                            "memoryview: destination must be a native single "
                            "character format prefixed with an optional '@'"));
}

TEST_F(MemoryViewBuiltinsTest, CastWithNonStrFormatRaisesTypeError) {
  HandleScope scope(thread_);
  const byte bytes[] = {0, 1, 2, 3, 4, 5, 6, 7};
  MemoryView view(&scope, newMemoryView(bytes, "B"));
  Object not_str(&scope, NoneType::object());
  Object result(&scope, runBuiltin(METH(memoryview, cast), view, not_str));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kTypeError,
                            "format argument must be a string"));
}

TEST_F(MemoryViewBuiltinsTest, CastWithNonMemoryViewRaisesTypeError) {
  HandleScope scope(thread_);
  Object none(&scope, NoneType::object());
  Str new_format(&scope, runtime_->newStrFromCStr("I"));
  Object result(&scope, runBuiltin(METH(memoryview, cast), none, new_format));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kTypeError,
                            "'<anonymous>' requires a 'memoryview' object but "
                            "received a 'NoneType'"));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatbSetsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xab};
  MemoryView view(&scope, newMemoryView(bytes, "b", ReadOnly::ReadWrite));
  word index = 0;
  Object value(&scope, runtime_->newInt(-59));
  EXPECT_EQ(memoryviewSetitem(thread_, view, index, value), NoneType::object());
  Object index_obj(&scope, runtime_->newInt(index));
  Object result(&scope, runBuiltin(FUNC(_builtins, _memoryview_getitem), view,
                                   index_obj));
  EXPECT_TRUE(isIntEqualsWord(*result, -59));
}

TEST_F(MemoryViewBuiltinsTest,
       SetitemWithFormatbAndOversizedValueRaisesValueError) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xab};
  MemoryView view(&scope, newMemoryView(bytes, "b", ReadOnly::ReadWrite));
  Object value(&scope, runtime_->newInt(0x101));
  Object result(&scope, memoryviewSetitem(thread_, view, 0, value));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kValueError,
                            "memoryview: invalid value for format 'b'"));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatBSetsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xee};
  MemoryView view(&scope, newMemoryView(bytes, "B", ReadOnly::ReadWrite));
  word index = 0;
  Object value(&scope, runtime_->newInt(0xd8));
  EXPECT_EQ(memoryviewSetitem(thread_, view, index, value), NoneType::object());
  Object index_obj(&scope, runtime_->newInt(index));
  Object result(&scope, runBuiltin(FUNC(_builtins, _memoryview_getitem), view,
                                   index_obj));
  EXPECT_TRUE(isIntEqualsWord(*result, 216));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatcSetsBytes) {
  HandleScope scope(thread_);
  const byte bytes[] = {97, 98};
  MemoryView view(&scope, newMemoryView(bytes, "c", ReadOnly::ReadWrite));
  word index = 0;
  Bytes value(&scope, runtime_->newBytes(1, 100));
  EXPECT_EQ(memoryviewSetitem(thread_, view, index, value), NoneType::object());
  Object index_obj(&scope, runtime_->newInt(index));
  Object result(&scope, runBuiltin(FUNC(_builtins, _memoryview_getitem), view,
                                   index_obj));
  const byte expected_bytes[] = {100};
  EXPECT_TRUE(isBytesEqualsBytes(result, expected_bytes));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormathSetsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xcd, 0x2c, 0xBE, 0xEF};
  MemoryView view(&scope, newMemoryView(bytes, "h", ReadOnly::ReadWrite));
  Object value(&scope, runtime_->newInt(-932));
  EXPECT_EQ(memoryviewSetitem(thread_, view, 2, value), NoneType::object());
  Object key(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, key));
  EXPECT_TRUE(isIntEqualsWord(*result, -932));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatHSetsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xb2, 0x11, 0xBE, 0xEF};
  MemoryView view(&scope, newMemoryView(bytes, "H", ReadOnly::ReadWrite));
  Object value(&scope, runtime_->newInt(49300));
  EXPECT_EQ(memoryviewSetitem(thread_, view, 2, value), NoneType::object());
  Object key(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, key));
  EXPECT_TRUE(isIntEqualsWord(*result, 49300));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatiSetsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x30, 0x8A, 0x43, 0xF2, 0xDE, 0xAD, 0xBE, 0xEF};
  MemoryView view(&scope, newMemoryView(bytes, "i", ReadOnly::ReadWrite));
  word index = 0;
  Object value(&scope, runtime_->newInt(-464070943));
  EXPECT_EQ(memoryviewSetitem(thread_, view, index, value), NoneType::object());
  Object index_obj(&scope, runtime_->newInt(index));
  Object result(&scope, runBuiltin(FUNC(_builtins, _memoryview_getitem), view,
                                   index_obj));
  EXPECT_TRUE(isIntEqualsWord(*result, -464070943));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatISetsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x30, 0x8A, 0x43, 0xF2, 0xDE, 0xAD, 0xBE, 0xEF};
  MemoryView view(&scope, newMemoryView(bytes, "I", ReadOnly::ReadWrite));
  Object value(&scope, runtime_->newInt(149624948));
  EXPECT_EQ(memoryviewSetitem(thread_, view, 4, value), NoneType::object());
  Object key(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, key));
  EXPECT_TRUE(isIntEqualsWord(*result, 149624948));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatlSetsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xD8, 0x76, 0x97, 0xD1, 0x8B, 0xA1, 0xD2, 0x62,
                        0xBA, 0xDC, 0x0F, 0xFE, 0xE0, 0xDD, 0xF0, 0x0D};
  MemoryView view(&scope, newMemoryView(bytes, "l", ReadOnly::ReadWrite));
  word index = 0;
  Object value(&scope, runtime_->newInt(-9099618978295131431));
  EXPECT_EQ(memoryviewSetitem(thread_, view, index, value), NoneType::object());
  Object index_obj(&scope, runtime_->newInt(index));
  Object result(&scope, runBuiltin(FUNC(_builtins, _memoryview_getitem), view,
                                   index_obj));
  EXPECT_TRUE(isIntEqualsWord(*result, -9099618978295131431));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatLSetsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xD8, 0x76, 0x97, 0xD1, 0x8B, 0xA1, 0xD2, 0x62,
                        0xBA, 0xDC, 0x0F, 0xFE, 0xE0, 0xDD, 0xF0, 0x0D};
  MemoryView view(&scope, newMemoryView(bytes, "L", ReadOnly::ReadWrite));
  Object value(&scope, runtime_->newIntFromUnsigned(7082027347532687782));
  EXPECT_EQ(memoryviewSetitem(thread_, view, 8, value), NoneType::object());
  Object key(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, key));
  EXPECT_TRUE(isIntEqualsWord(*result, 7082027347532687782));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatqSetsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x7,  0xE2, 0x42, 0x9E, 0x8F, 0xBF, 0xDB, 0x1B,
                        0xBA, 0xDC, 0x0F, 0xFE, 0xE0, 0xDD, 0xF0, 0x0D};
  MemoryView view(&scope, newMemoryView(bytes, "q", ReadOnly::ReadWrite));
  Object value(&scope, runtime_->newInt(2534191260184616076));
  EXPECT_EQ(memoryviewSetitem(thread_, view, 8, value), NoneType::object());
  Object key(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, key));
  EXPECT_TRUE(isIntEqualsWord(*result, 2534191260184616076));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatQSetsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xD9, 0xC6, 0xD2, 0x40, 0xBD, 0x19, 0xA9, 0xC8,
                        0xBA, 0xDC, 0x0F, 0xFE, 0xE0, 0xDD, 0xF0, 0x0D};
  MemoryView view(&scope, newMemoryView(bytes, "Q", ReadOnly::ReadWrite));
  Object value(&scope, runtime_->newIntFromUnsigned(0xbdc73615af8b018aul));
  EXPECT_EQ(memoryviewSetitem(thread_, view, 8, value), NoneType::object());
  Object key(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, key));
  const uword expected_digits[] = {0xbdc73615af8b018aul, 0};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatnSetsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xF2, 0x6F, 0xFA, 0x8B, 0x93, 0xC0, 0xED, 0x9D,
                        0xBA, 0xDC, 0x0F, 0xFE, 0xE0, 0xDD, 0xF0, 0x0D};
  MemoryView view(&scope, newMemoryView(bytes, "n", ReadOnly::ReadWrite));
  Object value(&scope, runtime_->newInt(-1461155128888034195l));
  EXPECT_EQ(memoryviewSetitem(thread_, view, 8, value), NoneType::object());
  Object key(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, key));
  EXPECT_TRUE(isIntEqualsWord(*result, -1461155128888034195l));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatNSetsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x6B, 0x8F, 0x6,  0xA2, 0xE0, 0x13, 0x88, 0x47,
                        0xBA, 0xDC, 0x0F, 0xFE, 0xE0, 0xDD, 0xF0, 0x0D};
  MemoryView view(&scope, newMemoryView(bytes, "N", ReadOnly::ReadWrite));
  Object value(&scope, runtime_->newIntFromUnsigned(0xc009026b7e40b67eul));
  EXPECT_EQ(memoryviewSetitem(thread_, view, 8, value), NoneType::object());
  Object key(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, key));
  const uword expected_digits[] = {0xc009026b7e40b67eul, 0};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatfSetsFloat) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x67, 0x32, 0x23, 0x31, 0xDE, 0xAD, 0xBE, 0xEF};
  MemoryView view(&scope, newMemoryView(bytes, "f", ReadOnly::ReadWrite));
  Float value(&scope, runtime_->newFloat(
                          std::strtof("-0x1.78e1720000000p-120", nullptr)));
  EXPECT_EQ(memoryviewSetitem(thread_, view, 4, value), NoneType::object());
  Object key(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, key));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(),
            std::strtof("-0x1.78e1720000000p-120", nullptr));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatdSetsFloat) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xEA, 0x43, 0xAD, 0x6F, 0x9D, 0x31, 0xE,  0x96,
                        0xBA, 0xDC, 0x0F, 0xFE, 0xE0, 0xDD, 0xF0, 0x0D};
  MemoryView view(&scope, newMemoryView(bytes, "d", ReadOnly::ReadWrite));
  Float value(&scope, runtime_->newFloat(
                          std::strtod("0x1.c0c870d1a8028p+187", nullptr)));
  EXPECT_EQ(memoryviewSetitem(thread_, view, 8, value), NoneType::object());
  Object key(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, key));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(),
            std::strtod("0x1.c0c870d1a8028p+187", nullptr));
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatQuestionmarkSetsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x92, 0xE1, 0x57, 0, 0x81, 0xA8};
  MemoryView view(&scope, newMemoryView(bytes, "?", ReadOnly::ReadWrite));
  word byte_index = 3;
  Bool value(&scope, Bool::trueObj());
  EXPECT_EQ(memoryviewSetitem(thread_, view, byte_index, value),
            NoneType::object());
  Object byte_index_obj(&scope, runtime_->newInt(byte_index));
  Object result(&scope, runBuiltin(FUNC(_builtins, _memoryview_getitem), view,
                                   byte_index_obj));
  EXPECT_EQ(result, Bool::trueObj());
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithFormatQuestionmarkSetsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x92, 0xE1, 0xAB, 0xEA, 0x81, 0xA8};
  MemoryView view(&scope, newMemoryView(bytes, "?", ReadOnly::ReadWrite));
  word byte_index = 2;
  Bool value(&scope, Bool::falseObj());
  EXPECT_EQ(memoryviewSetitem(thread_, view, byte_index, value),
            NoneType::object());
  Object byte_index_obj(&scope, runtime_->newInt(byte_index));
  Object result(&scope, runBuiltin(FUNC(_builtins, _memoryview_getitem), view,
                                   byte_index_obj));
  EXPECT_EQ(result, Bool::falseObj());
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithMemoryBufferWritesMemory) {
  HandleScope scope(thread_);
  const word length = 5;
  byte memory[length];
  MemoryView view(&scope, runtime_->newMemoryViewFromCPtr(
                              thread_, memory, length, ReadOnly::ReadWrite));
  Object value(&scope, SmallInt::fromWord(0));
  EXPECT_EQ(memoryviewSetitem(thread_, view, 0, value), NoneType::object());
  value = SmallInt::fromWord(1);
  EXPECT_EQ(memoryviewSetitem(thread_, view, 1, value), NoneType::object());
  value = SmallInt::fromWord(2);
  EXPECT_EQ(memoryviewSetitem(thread_, view, 2, value), NoneType::object());
  value = SmallInt::fromWord(3);
  EXPECT_EQ(memoryviewSetitem(thread_, view, 3, value), NoneType::object());
  value = SmallInt::fromWord(4);
  EXPECT_EQ(memoryviewSetitem(thread_, view, 4, value), NoneType::object());

  Object idx_obj(&scope, runtime_->newInt(0));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(FUNC(_builtins, _memoryview_getitem), view, idx_obj), 0));
  idx_obj = runtime_->newInt(1);
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(FUNC(_builtins, _memoryview_getitem), view, idx_obj), 1));
  idx_obj = runtime_->newInt(2);
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(FUNC(_builtins, _memoryview_getitem), view, idx_obj), 2));
  idx_obj = runtime_->newInt(3);
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(FUNC(_builtins, _memoryview_getitem), view, idx_obj), 3));
  idx_obj = runtime_->newInt(4);
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(FUNC(_builtins, _memoryview_getitem), view, idx_obj), 4));
  EXPECT_EQ(memory[0], 0);
  EXPECT_EQ(memory[1], 1);
  EXPECT_EQ(memory[2], 2);
  EXPECT_EQ(memory[3], 3);
  EXPECT_EQ(memory[4], 4);
}

TEST_F(MemoryViewBuiltinsTest, SetitemWithByteArraySetsMutableBytes) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, runtime_->typeAt(LayoutId::kMemoryView));
  ByteArray bytearray(&scope, runtime_->newByteArray());
  const byte byte_array[] = {0xCE};
  runtime_->byteArrayExtend(thread, bytearray, byte_array);
  EXPECT_EQ(bytearray.byteAt(0), 0xCE);

  Object result_obj(&scope,
                    runBuiltin(METH(memoryview, __new__), type, bytearray));
  ASSERT_TRUE(result_obj.isMemoryView());
  MemoryView view(&scope, *result_obj);
  word index = 0;
  Object value(&scope, runtime_->newInt(0xA5));
  EXPECT_EQ(memoryviewSetitem(thread_, view, index, value), NoneType::object());
  Object index_object(&scope, runtime_->newInt(index));
  Object result(&scope, runBuiltin(FUNC(_builtins, _memoryview_getitem), view,
                                   index_object));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xA5));
  EXPECT_EQ(bytearray.byteAt(0), 0xA5);
}

TEST_F(MemoryViewBuiltinsTest, DunderLenWithMemoryViewFormatBReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0, 1, 2};
  MemoryView view(&scope, newMemoryView(bytes, "B"));
  Object result(&scope, runBuiltin(METH(memoryview, __len__), view));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
}

TEST_F(MemoryViewBuiltinsTest, DunderLenWithMemoryViewFormatfReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0, 1, 2, 3, 4, 5, 6, 7};
  MemoryView view(&scope, newMemoryView(bytes, "f"));
  Object result(&scope, runBuiltin(METH(memoryview, __len__), view));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(MemoryViewBuiltinsTest, DunderLenWithNonMemoryViewRaisesTypeError) {
  HandleScope scope(thread_);
  Object none(&scope, NoneType::object());
  EXPECT_TRUE(raised(runBuiltin(METH(memoryview, __len__), none),
                     LayoutId::kTypeError));
}

TEST_F(MemoryViewBuiltinsTest, DunderNewWithBytesReturnsMemoryView) {
  HandleScope scope(thread_);
  const byte bytes_array[] = {0xa9};
  Bytes bytes(&scope, runtime_->newBytesWithAll(bytes_array));
  Type type(&scope, runtime_->typeAt(LayoutId::kMemoryView));
  Object result_obj(&scope, runBuiltin(METH(memoryview, __new__), type, bytes));
  ASSERT_TRUE(result_obj.isMemoryView());
  MemoryView view(&scope, *result_obj);
  EXPECT_EQ(view.buffer(), bytes);
  EXPECT_TRUE(isStrEqualsCStr(view.format(), "B"));
  EXPECT_TRUE(view.readOnly());
  EXPECT_EQ(view.start(), 0);
  Tuple shape(&scope, view.shape());
  ASSERT_EQ(shape.length(), 1);
  EXPECT_TRUE(isIntEqualsWord(shape.at(0), view.length()));
  Tuple strides(&scope, view.strides());
  ASSERT_EQ(strides.length(), 1);
  EXPECT_TRUE(isIntEqualsWord(strides.at(0), 1));
}

TEST_F(MemoryViewBuiltinsTest, DunderNewWithByteArrayReturnsMemoryView) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, runtime_->typeAt(LayoutId::kMemoryView));
  ByteArray bytearray(&scope, runtime_->newByteArray());
  const byte byte_array[] = {0xce};
  runtime_->byteArrayExtend(thread, bytearray, byte_array);
  Object result_obj(&scope,
                    runBuiltin(METH(memoryview, __new__), type, bytearray));
  ASSERT_TRUE(result_obj.isMemoryView());
  MemoryView view(&scope, *result_obj);
  EXPECT_EQ(view.buffer(), bytearray.items());
  EXPECT_EQ(view.length(), bytearray.numItems());
  EXPECT_TRUE(isStrEqualsCStr(view.format(), "B"));
  EXPECT_FALSE(view.readOnly());
  EXPECT_EQ(view.start(), 0);
  Tuple shape(&scope, view.shape());
  ASSERT_EQ(shape.length(), 1);
  EXPECT_TRUE(isIntEqualsWord(shape.at(0), view.length()));
  Tuple strides(&scope, view.strides());
  ASSERT_EQ(strides.length(), 1);
  EXPECT_TRUE(isIntEqualsWord(strides.at(0), 1));
}

TEST_F(MemoryViewBuiltinsTest, DunderNewWithMemoryViewReturnsMemoryView) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->typeAt(LayoutId::kMemoryView));
  const byte bytes[] = {0x96, 0xfc};
  MemoryView view(&scope, newMemoryView(bytes, "H", ReadOnly::ReadWrite));
  Object result_obj(&scope, runBuiltin(METH(memoryview, __new__), type, view));
  ASSERT_TRUE(result_obj.isMemoryView());
  MemoryView result(&scope, *result_obj);
  EXPECT_NE(result, view);
  EXPECT_EQ(view.buffer(), result.buffer());
  EXPECT_TRUE(Str::cast(view.format()).equals(result.format()));
  EXPECT_EQ(view.readOnly(), result.readOnly());
  EXPECT_EQ(result.start(), 0);
  Tuple shape(&scope, result.shape());
  ASSERT_EQ(shape.length(), 1);
  EXPECT_TRUE(isIntEqualsWord(shape.at(0), view.length()));
  Tuple strides(&scope, result.strides());
  ASSERT_EQ(strides.length(), 1);
  EXPECT_TRUE(isIntEqualsWord(strides.at(0), 1));
  EXPECT_EQ(strides.length(), 1);
}

TEST_F(MemoryViewBuiltinsTest, DunderNewWithUnsupportedObjectRaisesTypeError) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->typeAt(LayoutId::kMemoryView));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(METH(memoryview, __new__), type, none));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kTypeError,
                            "memoryview: a bytes-like object is required"));
}

TEST_F(MemoryViewBuiltinsTest, DunderNewWithInvalidTypeRaisesTypeError) {
  HandleScope scope(thread_);
  Object not_a_type(&scope, NoneType::object());
  Bytes bytes(&scope, runtime_->newBytesWithAll(View<byte>(nullptr, 0)));
  Object result(&scope,
                runBuiltin(METH(memoryview, __new__), not_a_type, bytes));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kTypeError,
                            "memoryview.__new__(X): X is not 'memoryview'"));
}

}  // namespace testing
}  // namespace py
