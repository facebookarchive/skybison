#include "gtest/gtest.h"

#include <cstdint>

#include "runtime.h"
#include "str-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(DoubleTest, DoubleTest) {
  Runtime runtime;
  RawObject o = runtime.newFloat(3.14);
  ASSERT_TRUE(o->isFloat());
  RawFloat d = Float::cast(o);
  EXPECT_EQ(d->value(), 3.14);
}

TEST(ComplexTest, ComplexTest) {
  Runtime runtime;
  RawObject o = runtime.newComplex(1.0, 2.0);
  ASSERT_TRUE(o->isComplex());
  RawComplex c = Complex::cast(o);
  EXPECT_EQ(c->real(), 1.0);
  EXPECT_EQ(c->imag(), 2.0);
}

TEST(IntTest, IntTest) {
  Runtime runtime;
  HandleScope scope;

  Handle<Object> o1(&scope, runtime.newInt(42));
  ASSERT_TRUE(o1->isSmallInt());
  Handle<Int> i1(o1);
  EXPECT_EQ(i1->asWord(), 42);

  Handle<Object> o2(&scope, runtime.newInt(9223372036854775807L));
  ASSERT_TRUE(o2->isLargeInt());
  Handle<Int> i2(o2);
  EXPECT_EQ(i2->asWord(), 9223372036854775807L);

  int stack_val = 123;
  Handle<Object> o3(&scope, runtime.newIntFromCPtr(&stack_val));
  ASSERT_TRUE(o3->isInt());
  Handle<Int> i3(o3);
  EXPECT_EQ(*static_cast<int*>(i3->asCPtr()), 123);

  Handle<Object> o4(&scope, runtime.newInt(kMinWord));
  ASSERT_TRUE(o4->isLargeInt());
  EXPECT_EQ(LargeInt::cast(*o4)->numDigits(), 1);
  EXPECT_EQ(LargeInt::cast(*o4)->asWord(), kMinWord);

  uword digits[] = {kMaxUword, 0};
  Handle<Int> o5(&scope, runtime.newIntWithDigits(digits));
  EXPECT_TRUE(o5->isLargeInt());
  EXPECT_EQ(o5->bitLength(), kBitsPerWord);

  uword digits2[] = {kMaxUword, 1};
  Handle<Int> o6(&scope, runtime.newIntWithDigits(digits2));
  EXPECT_TRUE(o6->isLargeInt());
  EXPECT_EQ(o6->bitLength(), kBitsPerWord + 1);
}

TEST(IntTest, IsPositive) {
  Runtime runtime;
  HandleScope scope;

  Handle<Int> zero(&scope, runtime.newInt(0));
  EXPECT_FALSE(zero->isPositive());

  Handle<Int> one(&scope, runtime.newInt(1));
  EXPECT_TRUE(one->isPositive());

  Handle<Int> neg_one(&scope, runtime.newInt(-1));
  EXPECT_FALSE(neg_one->isPositive());

  Handle<Int> max_small_int(&scope, runtime.newInt(SmallInt::kMaxValue));
  EXPECT_TRUE(max_small_int->isPositive());

  Handle<Int> min_small_int(&scope, runtime.newInt(SmallInt::kMinValue));
  EXPECT_FALSE(min_small_int->isPositive());

  Handle<Int> max_word(&scope, runtime.newInt(kMaxWord));
  EXPECT_TRUE(max_word->isPositive());

  Handle<Int> min_word(&scope, runtime.newInt(kMinWord));
  EXPECT_FALSE(min_word->isPositive());
}

TEST(IntTest, IsNegative) {
  Runtime runtime;
  HandleScope scope;

  Handle<Int> zero(&scope, runtime.newInt(0));
  EXPECT_FALSE(zero->isNegative());

  Handle<Int> one(&scope, runtime.newInt(1));
  EXPECT_FALSE(one->isNegative());

  Handle<Int> neg_one(&scope, runtime.newInt(-1));
  EXPECT_TRUE(neg_one->isNegative());

  Handle<Int> max_small_int(&scope, runtime.newInt(SmallInt::kMaxValue));
  EXPECT_FALSE(max_small_int->isNegative());

  Handle<Int> min_small_int(&scope, runtime.newInt(SmallInt::kMinValue));
  EXPECT_TRUE(min_small_int->isNegative());

  Handle<Int> max_word(&scope, runtime.newInt(kMaxWord));
  EXPECT_FALSE(max_word->isNegative());

  Handle<Int> min_word(&scope, runtime.newInt(kMinWord));
  EXPECT_TRUE(min_word->isNegative());
}

TEST(IntTest, IsZero) {
  Runtime runtime;
  HandleScope scope;

  Handle<Int> zero(&scope, runtime.newInt(0));
  EXPECT_TRUE(zero->isZero());

  Handle<Int> one(&scope, runtime.newInt(1));
  EXPECT_FALSE(one->isZero());

  Handle<Int> neg_one(&scope, runtime.newInt(-1));
  EXPECT_FALSE(neg_one->isZero());

  Handle<Int> max_small_int(&scope, runtime.newInt(SmallInt::kMaxValue));
  EXPECT_FALSE(max_small_int->isZero());

  Handle<Int> min_small_int(&scope, runtime.newInt(SmallInt::kMinValue));
  EXPECT_FALSE(min_small_int->isZero());

  Handle<Int> max_word(&scope, runtime.newInt(kMaxWord));
  EXPECT_FALSE(max_word->isZero());

  Handle<Int> min_word(&scope, runtime.newInt(kMinWord));
  EXPECT_FALSE(min_word->isZero());
}

TEST(IntTest, Compare) {
  Runtime runtime;
  HandleScope scope;

  Handle<Int> zero(&scope, runtime.newInt(0));
  Handle<Int> one(&scope, runtime.newInt(1));
  Handle<Int> neg_one(&scope, runtime.newInt(-1));

  EXPECT_EQ(zero->compare(*zero), 0);
  EXPECT_GE(one->compare(*neg_one), 1);
  EXPECT_LE(neg_one->compare(*one), -1);

  Handle<Int> min_small_int(&scope, runtime.newInt(SmallInt::kMinValue));
  Handle<Int> max_small_int(&scope, runtime.newInt(SmallInt::kMaxValue));

  EXPECT_GE(max_small_int->compare(*min_small_int), 1);
  EXPECT_LE(min_small_int->compare(*max_small_int), -1);
  EXPECT_EQ(min_small_int->compare(*min_small_int), 0);
  EXPECT_EQ(max_small_int->compare(*max_small_int), 0);

  Handle<Int> min_word(&scope, runtime.newInt(kMinWord));
  Handle<Int> max_word(&scope, runtime.newInt(kMaxWord));

  EXPECT_GE(max_word->compare(*min_word), 1);
  EXPECT_LE(min_word->compare(*max_word), -1);
  EXPECT_EQ(min_word->compare(*min_word), 0);
  EXPECT_EQ(max_word->compare(*max_word), 0);

  EXPECT_GE(max_word->compare(*max_small_int), 1);
  EXPECT_LE(min_word->compare(*min_small_int), -1);
}

#define EXPECT_VALID(expr, expected_value)                                     \
  {                                                                            \
    auto const result = (expr);                                                \
    EXPECT_EQ(result.error, CastError::None);                                  \
    EXPECT_EQ(result.value, expected_value);                                   \
  }

TEST(IntTest, AsInt) {
  Runtime runtime;
  HandleScope scope;

  Handle<Int> zero(&scope, runtime.newInt(0));
  EXPECT_VALID(zero->asInt<int>(), 0);
  EXPECT_VALID(zero->asInt<unsigned>(), 0);
  EXPECT_VALID(zero->asInt<unsigned long>(), 0);
  EXPECT_VALID(zero->asInt<unsigned long long>(), 0);

  Handle<Int> num(&scope, runtime.newInt(1234));
  EXPECT_EQ(num->asInt<byte>().error, CastError::Overflow);
  EXPECT_EQ(num->asInt<sbyte>().error, CastError::Overflow);
  EXPECT_VALID(num->asInt<int>(), 1234);
  EXPECT_VALID(num->asInt<long>(), 1234);
  EXPECT_VALID(num->asInt<unsigned>(), 1234);
  EXPECT_VALID(num->asInt<unsigned long>(), 1234);

  Handle<Int> neg_num(&scope, runtime.newInt(-4567));
  EXPECT_EQ(neg_num->asInt<unsigned>().error, CastError::Underflow);
  EXPECT_EQ(neg_num->asInt<sbyte>().error, CastError::Underflow);
  EXPECT_VALID(neg_num->asInt<int16>(), -4567);

  Handle<Int> neg_one(&scope, runtime.newInt(-1));
  EXPECT_VALID(neg_one->asInt<int>(), -1);
  EXPECT_EQ(neg_one->asInt<unsigned>().error, CastError::Underflow);

  Handle<Int> int_max(&scope, runtime.newInt(kMaxInt32));
  EXPECT_VALID(int_max->asInt<int32>(), kMaxInt32);
  EXPECT_EQ(int_max->asInt<int16>().error, CastError::Overflow);

  Handle<Int> uword_max(&scope, runtime.newIntFromUnsigned(kMaxUword));
  EXPECT_VALID(uword_max->asInt<uword>(), kMaxUword);
  EXPECT_EQ(uword_max->asInt<word>().error, CastError::Overflow);

  Handle<Int> word_max(&scope, runtime.newInt(kMaxWord));
  EXPECT_VALID(word_max->asInt<word>(), kMaxWord);
  EXPECT_VALID(word_max->asInt<uword>(), uword{kMaxWord});
  EXPECT_EQ(word_max->asInt<int32>().error, CastError::Overflow);

  Handle<Int> word_min(&scope, runtime.newInt(kMinWord));
  EXPECT_VALID(word_min->asInt<word>(), kMinWord);
  EXPECT_EQ(word_min->asInt<uword>().error, CastError::Underflow);
  EXPECT_EQ(word_min->asInt<int32>().error, CastError::Overflow);

  uword digits[] = {0, 0xffff000000000000LL};
  Handle<Int> negative(&scope, runtime.newIntWithDigits(digits));
  EXPECT_EQ(negative->asInt<word>().error, CastError::Underflow);
  EXPECT_EQ(negative->asInt<uword>().error, CastError::Underflow);
}

#undef EXPECT_VALID

TEST(ModulesTest, TestCreate) {
  Runtime runtime;
  HandleScope scope;
  Handle<Object> name(&scope, runtime.newStrFromCStr("mymodule"));
  Handle<Module> module(&scope, runtime.newModule(name));
  EXPECT_EQ(module->name(), *name);
  EXPECT_TRUE(module->dict()->isDict());
}

TEST(SliceTest, adjustIndices) {
  // Test: 0:10:1 on len: 10
  word length = 10;
  word start = 0;
  word stop = 10;
  word step = 1;
  word new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 10);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 10);

  // Test: 2:10:1 on len: 10
  start = 2;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 8);
  ASSERT_EQ(start, 2);
  ASSERT_EQ(stop, 10);

  // Test: -4:10:1 on len: 10
  start = -4;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 4);
  ASSERT_EQ(start, 6);
  ASSERT_EQ(stop, 10);

  // Test: 0:2:1 on len: 10
  start = 0;
  stop = 2;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 2);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 2);

  // Test: 0:-2:1 on len: 10
  start = 0;
  stop = -2;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 8);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 8);

  // Test: 0:10:2 on len: 10
  start = 0;
  stop = 10;
  step = 2;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 5);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 10);

  // Test: 0:10:-2 on len: 10
  start = 0;
  stop = 10;
  step = -2;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 0);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 9);
}

TEST(SliceTest, adjustIndicesOutOfBounds) {
  // Test: 10:5:1 on len: 5
  word length = 5;
  word start = 10;
  word stop = 5;
  word step = 1;
  word new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 0);
  ASSERT_EQ(start, 5);
  ASSERT_EQ(stop, 5);

  // Test: -10:5:1 on len: 5
  start = -10;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 5);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 5);

  // Test: 0:10:1 on len: 5
  start = 0;
  stop = 10;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 5);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 5);

  // Test: 0:-10:1 on len: 5
  stop = -10;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 0);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 0);

  // Test: 0:5:10 on len: 5
  stop = 5;
  step = 10;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 1);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 5);

  // Test: 0:5:-10 on len: 5
  step = -10;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 0);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 4);
}

TEST(LargeStrTest, CopyTo) {
  Runtime runtime;

  RawObject obj = runtime.newStrFromCStr("hello world!");
  ASSERT_TRUE(obj->isLargeStr());
  RawStr str = Str::cast(obj);

  byte array[5];
  memset(array, 'a', ARRAYSIZE(array));
  str->copyTo(array, 0);
  EXPECT_EQ(array[0], 'a');
  EXPECT_EQ(array[1], 'a');
  EXPECT_EQ(array[2], 'a');
  EXPECT_EQ(array[3], 'a');
  EXPECT_EQ(array[4], 'a');

  memset(array, 'b', ARRAYSIZE(array));
  str->copyTo(array, 1);
  EXPECT_EQ(array[0], 'h');
  EXPECT_EQ(array[1], 'b');
  EXPECT_EQ(array[2], 'b');
  EXPECT_EQ(array[3], 'b');
  EXPECT_EQ(array[4], 'b');

  memset(array, 'c', ARRAYSIZE(array));
  str->copyTo(array, 5);
  EXPECT_EQ(array[0], 'h');
  EXPECT_EQ(array[1], 'e');
  EXPECT_EQ(array[2], 'l');
  EXPECT_EQ(array[3], 'l');
  EXPECT_EQ(array[4], 'o');
}

TEST(SmallStrTest, Tests) {
  RawObject obj0 = SmallStr::fromCStr("AB");
  ASSERT_TRUE(obj0->isSmallStr());
  auto* str0 = Str::cast(obj0);
  EXPECT_EQ(str0->length(), 2);
  EXPECT_EQ(str0->charAt(0), 'A');
  EXPECT_EQ(str0->charAt(1), 'B');
  byte array[3]{0, 0, 0};
  str0->copyTo(array, 2);
  EXPECT_EQ(array[0], 'A');
  EXPECT_EQ(array[1], 'B');
  EXPECT_EQ(array[2], 0);
}

TEST(StringTest, ToCString) {
  Runtime runtime;
  HandleScope scope;

  Handle<Str> empty(&scope, runtime.newStrFromCStr(""));
  char* c_empty = empty->toCStr();
  ASSERT_NE(c_empty, nullptr);
  EXPECT_STREQ(c_empty, "");
  std::free(c_empty);

  Handle<Str> length1(&scope, runtime.newStrFromCStr("a"));
  char* c_length1 = length1->toCStr();
  ASSERT_NE(c_length1, nullptr);
  EXPECT_STREQ(c_length1, "a");
  std::free(c_length1);

  Handle<Str> length2(&scope, runtime.newStrFromCStr("ab"));
  char* c_length2 = length2->toCStr();
  ASSERT_NE(c_length2, nullptr);
  EXPECT_STREQ(c_length2, "ab");
  std::free(c_length2);

  Handle<Str> length10(&scope, runtime.newStrFromCStr("1234567890"));
  char* c_length10 = length10->toCStr();
  ASSERT_NE(c_length10, nullptr);
  EXPECT_STREQ(c_length10, "1234567890");
  std::free(c_length10);

  Handle<Str> nulchar(&scope, runtime.newStrFromCStr("wx\0yz"));
  char* c_nulchar = nulchar->toCStr();
  ASSERT_NE(c_nulchar, nullptr);
  EXPECT_STREQ(c_nulchar, "wx");
  std::free(c_nulchar);
}

TEST(StringTest, CompareSmallStr) {
  Runtime runtime;
  HandleScope scope;

  Handle<Str> small(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(small->isSmallStr());

  EXPECT_TRUE(small->equalsCStr("foo"));
  // This apparently stupid test is in response to a bug where we assumed
  // that the c-string passed to SmallStr::equalsCStr would always
  // be small itself.
  EXPECT_FALSE(small->equalsCStr("123456789"));
}

TEST(WeakRefTest, EnqueueAndDequeue) {
  Runtime runtime;
  HandleScope scope;
  RawObject list = NoneType::object();
  for (int i = 0; i < 3; i++) {
    Handle<WeakRef> weak(&scope, runtime.newWeakRef());
    weak->setReferent(SmallInt::fromWord(i));
    WeakRef::enqueueReference(*weak, &list);
  }
  Handle<WeakRef> weak(&scope, WeakRef::dequeueReference(&list));
  EXPECT_EQ(SmallInt::cast(weak->referent())->value(), 0);

  weak = WeakRef::dequeueReference(&list);
  EXPECT_EQ(SmallInt::cast(weak->referent())->value(), 1);

  weak = WeakRef::dequeueReference(&list);
  EXPECT_EQ(SmallInt::cast(weak->referent())->value(), 2);

  EXPECT_EQ(list, NoneType::object());
}

TEST(WeakRefTest, SpliceQueue) {
  Runtime runtime;
  HandleScope scope;
  RawObject list1 = NoneType::object();
  RawObject list2 = NoneType::object();
  EXPECT_EQ(WeakRef::spliceQueue(list1, list2), NoneType::object());

  RawObject list3 = runtime.newWeakRef();
  WeakRef::cast(list3)->setLink(list3);
  EXPECT_EQ(WeakRef::spliceQueue(list1, list3), list3);
  EXPECT_EQ(WeakRef::spliceQueue(list3, list2), list3);

  for (int i = 0; i < 2; i++) {
    Handle<WeakRef> weak1(&scope, runtime.newWeakRef());
    weak1->setReferent(SmallInt::fromWord(i));
    WeakRef::enqueueReference(*weak1, &list1);

    Handle<WeakRef> weak2(&scope, runtime.newWeakRef());
    weak2->setReferent(SmallInt::fromWord(i + 2));
    WeakRef::enqueueReference(*weak2, &list2);
  }
  RawObject list = WeakRef::spliceQueue(list1, list2);
  Handle<WeakRef> weak(&scope, WeakRef::dequeueReference(&list));
  EXPECT_EQ(SmallInt::cast(weak->referent())->value(), 0);

  weak = WeakRef::dequeueReference(&list);
  EXPECT_EQ(SmallInt::cast(weak->referent())->value(), 1);

  weak = WeakRef::dequeueReference(&list);
  EXPECT_EQ(SmallInt::cast(weak->referent())->value(), 2);

  weak = WeakRef::dequeueReference(&list);
  EXPECT_EQ(SmallInt::cast(weak->referent())->value(), 3);

  EXPECT_EQ(list, NoneType::object());
}

TEST(ClassTest, SetFlagThenHasFlagReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Handle<Type> type(&scope, runtime.newClass());
  type->setFlag(Type::Flag::kDictSubclass);
  type->setFlag(Type::Flag::kListSubclass);
  EXPECT_TRUE(type->hasFlag(Type::Flag::kDictSubclass));
  EXPECT_TRUE(type->hasFlag(Type::Flag::kListSubclass));
  EXPECT_EQ(SmallInt::cast(type->flags())->value(),
            Type::Flag::kDictSubclass | Type::Flag::kListSubclass);
}

}  // namespace python
