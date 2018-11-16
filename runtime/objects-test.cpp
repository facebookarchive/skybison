#include "gtest/gtest.h"

#include <cstdint>

#include "runtime.h"
#include "str-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(DoubleTest, DoubleTest) {
  Runtime runtime;
  Object* o = runtime.newFloat(3.14);
  ASSERT_TRUE(o->isFloat());
  Float* d = Float::cast(o);
  EXPECT_EQ(d->value(), 3.14);
}

TEST(ComplexTest, ComplexTest) {
  Runtime runtime;
  Object* o = runtime.newComplex(1.0, 2.0);
  ASSERT_TRUE(o->isComplex());
  Complex* c = Complex::cast(o);
  EXPECT_EQ(c->real(), 1.0);
  EXPECT_EQ(c->imag(), 2.0);
}

TEST(IntTest, IntTest) {
  Runtime runtime;
  Object* o1 = runtime.newInt(42);
  EXPECT_FALSE(o1->isLargeInt());
  Int* i1 = Int::cast(o1);
  EXPECT_EQ(i1->asWord(), 42);

  Object* o2 = runtime.newInt(9223372036854775807L);
  EXPECT_TRUE(o2->isLargeInt());
  Int* i2 = Int::cast(o2);
  EXPECT_EQ(i2->asWord(), 9223372036854775807L);

  int stack_val = 123;
  Object* o3 = runtime.newIntFromCPointer(&stack_val);
  Int* i3 = Int::cast(o3);
  EXPECT_EQ(*reinterpret_cast<int*>(i3->asCPointer()), 123);
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

TEST(ModulesTest, TestCreate) {
  Runtime runtime;
  HandleScope scope;
  Handle<Object> name(&scope, runtime.newStringFromCString("mymodule"));
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

  Object* obj = runtime.newStringFromCString("hello world!");
  ASSERT_TRUE(obj->isLargeStr());
  String* str = String::cast(obj);

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
  Object* obj0 = SmallStr::fromCString("AB");
  ASSERT_TRUE(obj0->isSmallStr());
  auto* str0 = String::cast(obj0);
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

  Handle<String> empty(&scope, runtime.newStringFromCString(""));
  char* c_empty = empty->toCString();
  ASSERT_NE(c_empty, nullptr);
  EXPECT_STREQ(c_empty, "");
  std::free(c_empty);

  Handle<String> length1(&scope, runtime.newStringFromCString("a"));
  char* c_length1 = length1->toCString();
  ASSERT_NE(c_length1, nullptr);
  EXPECT_STREQ(c_length1, "a");
  std::free(c_length1);

  Handle<String> length2(&scope, runtime.newStringFromCString("ab"));
  char* c_length2 = length2->toCString();
  ASSERT_NE(c_length2, nullptr);
  EXPECT_STREQ(c_length2, "ab");
  std::free(c_length2);

  Handle<String> length10(&scope, runtime.newStringFromCString("1234567890"));
  char* c_length10 = length10->toCString();
  ASSERT_NE(c_length10, nullptr);
  EXPECT_STREQ(c_length10, "1234567890");
  std::free(c_length10);

  Handle<String> nulchar(&scope, runtime.newStringFromCString("wx\0yz"));
  char* c_nulchar = nulchar->toCString();
  ASSERT_NE(c_nulchar, nullptr);
  EXPECT_STREQ(c_nulchar, "wx");
  std::free(c_nulchar);
}

TEST(StringTest, CompareSmallStr) {
  Runtime runtime;
  HandleScope scope;

  Handle<String> small(&scope, runtime.newStringFromCString("foo"));
  EXPECT_TRUE(small->isSmallStr());

  EXPECT_TRUE(small->equalsCString("foo"));
  // This apparently stupid test is in response to a bug where we assumed
  // that the c-string passed to SmallStr::equalsCString would always
  // be small itself.
  EXPECT_FALSE(small->equalsCString("123456789"));
}

TEST(WeakRefTest, EnqueueAndDequeue) {
  Runtime runtime;
  HandleScope scope;
  Object* list = None::object();
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

  EXPECT_EQ(list, None::object());
}

TEST(WeakRefTest, SpliceQueue) {
  Runtime runtime;
  HandleScope scope;
  Object* list1 = None::object();
  Object* list2 = None::object();
  EXPECT_EQ(WeakRef::spliceQueue(list1, list2), None::object());

  Object* list3 = runtime.newWeakRef();
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
  Object* list = WeakRef::spliceQueue(list1, list2);
  Handle<WeakRef> weak(&scope, WeakRef::dequeueReference(&list));
  EXPECT_EQ(SmallInt::cast(weak->referent())->value(), 0);

  weak = WeakRef::dequeueReference(&list);
  EXPECT_EQ(SmallInt::cast(weak->referent())->value(), 1);

  weak = WeakRef::dequeueReference(&list);
  EXPECT_EQ(SmallInt::cast(weak->referent())->value(), 2);

  weak = WeakRef::dequeueReference(&list);
  EXPECT_EQ(SmallInt::cast(weak->referent())->value(), 3);

  EXPECT_EQ(list, None::object());
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
