#include "gtest/gtest.h"

#include "handles.h"
#include "memoryview-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(MemoryViewBuiltinsTest, GetItemWithFormatbReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope, newMemoryView({0xab, 0xc5}, "b"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -59));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatBReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope, newMemoryView({0xee, 0xd8}, "B"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 216));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatcReturnsBytes) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope, newMemoryView({0x03, 0x62}, "c"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isBytesEqualsBytes(result, {0x62}));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormathReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope, newMemoryView({0xcd, 0x2c, 0x5c, 0xfc}, "h"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -932));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatHReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope, newMemoryView({0xb2, 0x11, 0x94, 0xc0}, "H"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 49300));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatiReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Object view(
      &scope,
      newMemoryView({0x30, 0x8A, 0x43, 0xF2, 0xE1, 0xD6, 0x56, 0xE4}, "i"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -464070943));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatAtiReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Object view(
      &scope,
      newMemoryView({0x30, 0x8A, 0x43, 0xF2, 0xE1, 0xD6, 0x56, 0xE4}, "@i"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -464070943));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatIReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope, newMemoryView(
                          {0x2, 0xBE, 0xA8, 0x3D, 0x74, 0x18, 0xEB, 0x8}, "I"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 149624948));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatlReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope,
              newMemoryView({0xD8, 0x76, 0x97, 0xD1, 0x8B, 0xA1, 0xD2, 0x62,
                             0xD9, 0xD2, 0x50, 0x47, 0xC0, 0xA8, 0xB7, 0x81},
                            "l"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -9099618978295131431));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatLReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope,
              newMemoryView({0x24, 0x37, 0x8B, 0x51, 0xCB, 0xB2, 0x16, 0xFB,
                             0xA6, 0xA9, 0x49, 0xB3, 0x59, 0x6A, 0x48, 0x62},
                            "L"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 7082027347532687782));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatqReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope,
              newMemoryView({0x7, 0xE2, 0x42, 0x9E, 0x8F, 0xBF, 0xDB, 0x1B,
                             0x8C, 0x1C, 0x34, 0x40, 0x86, 0x41, 0x2B, 0x23},
                            "q"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 2534191260184616076));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatQReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope,
              newMemoryView({0xD9, 0xC6, 0xD2, 0x40, 0xBD, 0x19, 0xA9, 0xC8,
                             0x8A, 0x1, 0x8B, 0xAF, 0x15, 0x36, 0xC7, 0xBD},
                            "Q"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsDigits(*result, {0xbdc73615af8b018aul, 0}));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatnReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope,
              newMemoryView({0xF2, 0x6F, 0xFA, 0x8B, 0x93, 0xC0, 0xED, 0x9D,
                             0x6D, 0x7C, 0xE3, 0xDC, 0x26, 0xEF, 0xB8, 0xEB},
                            "n"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -1461155128888034195l));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatNReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope,
              newMemoryView({0x6B, 0x8F, 0x6, 0xA2, 0xE0, 0x13, 0x88, 0x47,
                             0x7E, 0xB6, 0x40, 0x7E, 0x6B, 0x2, 0x9, 0xC0},
                            "N"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(isIntEqualsDigits(*result, {0xc009026b7e40b67eul, 0}));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatfReturnsFloat) {
  Runtime runtime;
  HandleScope scope;
  Object view(
      &scope,
      newMemoryView({0x67, 0x32, 0x23, 0x31, 0xB9, 0x70, 0xBC, 0x83}, "f"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(RawFloat::cast(*result).value(),
            std::strtod("-0x1.78e1720000000p-120", nullptr));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatdReturnsFloat) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope,
              newMemoryView({0xEA, 0x43, 0xAD, 0x6F, 0x9D, 0x31, 0xE, 0x96,
                             0x28, 0x80, 0x1A, 0xD, 0x87, 0xC, 0xAC, 0x4B},
                            "d"));
  Int index(&scope, runtime.newInt(1));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(RawFloat::cast(*result).value(),
            std::strtod("0x1.c0c870d1a8028p+187", nullptr));
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatQuestionmarkReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope, newMemoryView({0x92, 0xE1, 0x57, 0xEA, 0x81, 0xA8}, "?"));
  Int index(&scope, runtime.newInt(3));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_EQ(result, Bool::trueObj());
}

TEST(MemoryViewBuiltinsTest, GetItemWithFormatQuestionmarkReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope, newMemoryView({0x92, 0xE1, 0, 0xEA, 0x81, 0xA8}, "?"));
  Int index(&scope, runtime.newInt(2));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_EQ(result, Bool::falseObj());
}

TEST(MemoryViewBuiltins, GetItemWithNegativeIndexReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope, newMemoryView({0, 1, 2, 3, 4, 5, 6, 7}, "h"));
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
  Object view(&scope, newMemoryView({0, 1, 2, 3, 4, 5, 6, 7}, "I"));
  Int index(&scope, runtime.newInt(2));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(
      raisedWithStr(*result, LayoutId::kIndexError, "index out of bounds"));
}

TEST(MemoryViewBuiltins, GetItemWithOverflowingIndexRaisesIndexError) {
  Runtime runtime;
  HandleScope scope;
  Object view(&scope, newMemoryView({0, 1, 2, 3, 4, 5, 6, 7}, "I"));
  Int index(&scope, runtime.newInt(kMaxWord / 2));
  Object result(&scope,
                runBuiltin(MemoryViewBuiltins::dunderGetItem, view, index));
  EXPECT_TRUE(
      raisedWithStr(*result, LayoutId::kIndexError, "index out of bounds"));
}

}  // namespace python
