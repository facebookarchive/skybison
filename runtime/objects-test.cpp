#include "gtest/gtest.h"

#include <cstdint>

#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(DoubleTest, DoubleTest) {
  Runtime runtime;
  Object* o = runtime.newDouble(3.14);
  ASSERT_TRUE(o->isDouble());
  Double* d = Double::cast(o);
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

TEST(IntegerTest, IntegerTest) {
  Runtime runtime;
  Object* o1 = runtime.newInteger(42);
  EXPECT_FALSE(o1->isLargeInteger());
  Integer* i1 = Integer::cast(o1);
  EXPECT_EQ(i1->asWord(), 42);

  Object* o2 = runtime.newInteger(9223372036854775807L);
  EXPECT_TRUE(o2->isLargeInteger());
  Integer* i2 = Integer::cast(o2);
  EXPECT_EQ(i2->asWord(), 9223372036854775807L);

  int stack_val = 123;
  Object* o3 = runtime.newIntegerFromCPointer(&stack_val);
  Integer* i3 = Integer::cast(o3);
  EXPECT_EQ(*reinterpret_cast<int*>(i3->asCPointer()), 123);
}

TEST(IntegerTest, IsPositive) {
  Runtime runtime;
  HandleScope scope;

  Handle<Integer> zero(&scope, runtime.newInteger(0));
  EXPECT_FALSE(zero->isPositive());

  Handle<Integer> one(&scope, runtime.newInteger(1));
  EXPECT_TRUE(one->isPositive());

  Handle<Integer> neg_one(&scope, runtime.newInteger(-1));
  EXPECT_FALSE(neg_one->isPositive());

  Handle<Integer> max_small_int(&scope,
                                runtime.newInteger(SmallInteger::kMaxValue));
  EXPECT_TRUE(max_small_int->isPositive());

  Handle<Integer> min_small_int(&scope,
                                runtime.newInteger(SmallInteger::kMinValue));
  EXPECT_FALSE(min_small_int->isPositive());

  Handle<Integer> max_word(&scope, runtime.newInteger(kMaxWord));
  EXPECT_TRUE(max_word->isPositive());

  Handle<Integer> min_word(&scope, runtime.newInteger(kMinWord));
  EXPECT_FALSE(min_word->isPositive());
}

TEST(IntegerTest, IsNegative) {
  Runtime runtime;
  HandleScope scope;

  Handle<Integer> zero(&scope, runtime.newInteger(0));
  EXPECT_FALSE(zero->isNegative());

  Handle<Integer> one(&scope, runtime.newInteger(1));
  EXPECT_FALSE(one->isNegative());

  Handle<Integer> neg_one(&scope, runtime.newInteger(-1));
  EXPECT_TRUE(neg_one->isNegative());

  Handle<Integer> max_small_int(&scope,
                                runtime.newInteger(SmallInteger::kMaxValue));
  EXPECT_FALSE(max_small_int->isNegative());

  Handle<Integer> min_small_int(&scope,
                                runtime.newInteger(SmallInteger::kMinValue));
  EXPECT_TRUE(min_small_int->isNegative());

  Handle<Integer> max_word(&scope, runtime.newInteger(kMaxWord));
  EXPECT_FALSE(max_word->isNegative());

  Handle<Integer> min_word(&scope, runtime.newInteger(kMinWord));
  EXPECT_TRUE(min_word->isNegative());
}

TEST(IntegerTest, IsZero) {
  Runtime runtime;
  HandleScope scope;

  Handle<Integer> zero(&scope, runtime.newInteger(0));
  EXPECT_TRUE(zero->isZero());

  Handle<Integer> one(&scope, runtime.newInteger(1));
  EXPECT_FALSE(one->isZero());

  Handle<Integer> neg_one(&scope, runtime.newInteger(-1));
  EXPECT_FALSE(neg_one->isZero());

  Handle<Integer> max_small_int(&scope,
                                runtime.newInteger(SmallInteger::kMaxValue));
  EXPECT_FALSE(max_small_int->isZero());

  Handle<Integer> min_small_int(&scope,
                                runtime.newInteger(SmallInteger::kMinValue));
  EXPECT_FALSE(min_small_int->isZero());

  Handle<Integer> max_word(&scope, runtime.newInteger(kMaxWord));
  EXPECT_FALSE(max_word->isZero());

  Handle<Integer> min_word(&scope, runtime.newInteger(kMinWord));
  EXPECT_FALSE(min_word->isZero());
}

TEST(ModulesTest, TestCreate) {
  Runtime runtime;
  HandleScope scope;
  Handle<Object> name(&scope, runtime.newStringFromCString("mymodule"));
  Handle<Module> module(&scope, runtime.newModule(name));
  EXPECT_EQ(module->name(), *name);
  EXPECT_TRUE(module->dictionary()->isDictionary());
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

TEST(SliceTest, sliceList) {
  Runtime runtime;
  HandleScope scope;

  // list = [1,2,3,4,5]
  Handle<List> list(&scope, runtime.newList());
  for (int i = 1; i < 6; i++) {
    Handle<Object> value(&scope, SmallInteger::fromWord(i));
    runtime.listAdd(list, value);
  }

  // Test: [::]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test1(&scope, runtime.listSlice(list, slice));
  ASSERT_EQ(test1->allocated(), 5);
  ASSERT_EQ(SmallInteger::cast(test1->at(0))->value(), 1);
  ASSERT_EQ(SmallInteger::cast(test1->at(4))->value(), 5);

  /// consts->atPut(0, SmallInteger::fromWord(0));
  // Test: [2:]
  start = SmallInteger::fromWord(2);
  stop = None::object();
  step = None::object();
  slice = runtime.newSlice(start, stop, step);
  Handle<List> test2(&scope, runtime.listSlice(list, slice));
  ASSERT_EQ(test2->allocated(), 3);
  ASSERT_EQ(SmallInteger::cast(test2->at(0))->value(), 3);
  ASSERT_EQ(SmallInteger::cast(test2->at(2))->value(), 5);

  // Test: [-2:]
  start = SmallInteger::fromWord(-2);
  stop = None::object();
  step = None::object();
  slice = runtime.newSlice(start, stop, step);
  Handle<List> test3(&scope, runtime.listSlice(list, slice));
  ASSERT_EQ(test3->allocated(), 2);
  ASSERT_EQ(SmallInteger::cast(test3->at(0))->value(), 4);
  ASSERT_EQ(SmallInteger::cast(test3->at(1))->value(), 5);

  // Test [:2]
  start = None::object();
  stop = SmallInteger::fromWord(2);
  step = None::object();
  slice = runtime.newSlice(start, stop, step);
  Handle<List> test4(&scope, runtime.listSlice(list, slice));
  ASSERT_EQ(test4->allocated(), 2);
  ASSERT_EQ(SmallInteger::cast(test4->at(0))->value(), 1);
  ASSERT_EQ(SmallInteger::cast(test4->at(1))->value(), 2);

  // Test [:-2]
  start = None::object();
  stop = SmallInteger::fromWord(-2);
  step = None::object();
  slice = runtime.newSlice(start, stop, step);
  Handle<List> test5(&scope, runtime.listSlice(list, slice));
  ASSERT_EQ(test5->allocated(), 3);
  ASSERT_EQ(SmallInteger::cast(test5->at(0))->value(), 1);
  ASSERT_EQ(SmallInteger::cast(test5->at(2))->value(), 3);

  // Test [::2]
  start = None::object();
  stop = None::object();
  step = SmallInteger::fromWord(2);
  slice = runtime.newSlice(start, stop, step);
  Handle<List> test6(&scope, runtime.listSlice(list, slice));
  ASSERT_EQ(test6->allocated(), 3);
  ASSERT_EQ(SmallInteger::cast(test6->at(0))->value(), 1);
  ASSERT_EQ(SmallInteger::cast(test6->at(2))->value(), 5);

  // Test [::-2]
  start = None::object();
  stop = None::object();
  step = SmallInteger::fromWord(-2);
  slice = runtime.newSlice(start, stop, step);
  Handle<List> test7(&scope, runtime.listSlice(list, slice));
  ASSERT_EQ(test7->allocated(), 3);
  ASSERT_EQ(SmallInteger::cast(test7->at(0))->value(), 5);
  ASSERT_EQ(SmallInteger::cast(test7->at(2))->value(), 1);

  // Out of bounds:

  // Test [10:]
  start = SmallInteger::fromWord(10);
  stop = None::object();
  step = None::object();
  slice = runtime.newSlice(start, stop, step);
  Handle<List> test8(&scope, runtime.listSlice(list, slice));
  ASSERT_EQ(test8->allocated(), 0);

  // Test [:10]
  start = None::object();
  stop = SmallInteger::fromWord(10);
  step = None::object();
  slice = runtime.newSlice(start, stop, step);
  Handle<List> test9(&scope, runtime.listSlice(list, slice));
  ASSERT_EQ(test9->allocated(), 5);
  ASSERT_EQ(SmallInteger::cast(test9->at(0))->value(), 1);
  ASSERT_EQ(SmallInteger::cast(test9->at(4))->value(), 5);

  // Test [::10]
  start = None::object();
  stop = None::object();
  step = SmallInteger::fromWord(10);
  slice = runtime.newSlice(start, stop, step);
  Handle<List> test10(&scope, runtime.listSlice(list, slice));
  ASSERT_EQ(test10->allocated(), 1);
  ASSERT_EQ(SmallInteger::cast(test10->at(0))->value(), 1);
}

TEST(SmallIntegerTest, IsValid) {
  EXPECT_TRUE(SmallInteger::isValid(0));
  EXPECT_TRUE(SmallInteger::isValid(1));
  EXPECT_TRUE(SmallInteger::isValid(-1));

  EXPECT_FALSE(SmallInteger::isValid(INTPTR_MAX));
  EXPECT_DEBUG_DEATH(SmallInteger::fromWord(INTPTR_MAX), "invalid cast");
  EXPECT_FALSE(SmallInteger::isValid(INTPTR_MIN));
  EXPECT_DEBUG_DEATH(SmallInteger::fromWord(INTPTR_MIN), "invalid cast");

  EXPECT_TRUE(SmallInteger::isValid(SmallInteger::kMaxValue));
  EXPECT_TRUE(SmallInteger::isValid(SmallInteger::kMaxValue - 1));
  EXPECT_FALSE(SmallInteger::isValid(SmallInteger::kMaxValue + 1));

  EXPECT_TRUE(SmallInteger::isValid(SmallInteger::kMinValue));
  EXPECT_FALSE(SmallInteger::isValid(SmallInteger::kMinValue - 1));
  EXPECT_TRUE(SmallInteger::isValid(SmallInteger::kMinValue + 1));
}

TEST(LargeStringTest, CopyTo) {
  Runtime runtime;

  Object* obj = runtime.newStringFromCString("hello world!");
  ASSERT_TRUE(obj->isLargeString());
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

TEST(SmallStringTest, Tests) {
  Object* obj0 = SmallString::fromCString("AB");
  ASSERT_TRUE(obj0->isSmallString());
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
  EXPECT_TRUE(small->isSmallString());

  EXPECT_TRUE(small->equalsCString("foo"));
  // This apparently stupid test is in response to a bug where we assumed
  // that the c-string passed to SmallString::equalsCString would always
  // be small itself.
  EXPECT_FALSE(small->equalsCString("123456789"));
}

TEST(WeakRefTest, EnqueueAndDequeue) {
  Runtime runtime;
  HandleScope scope;
  Object* list = None::object();
  for (int i = 0; i < 3; i++) {
    Handle<WeakRef> weak(&scope, runtime.newWeakRef());
    weak->setReferent(SmallInteger::fromWord(i));
    WeakRef::enqueueReference(*weak, &list);
  }
  Handle<WeakRef> weak(&scope, WeakRef::dequeueReference(&list));
  EXPECT_EQ(SmallInteger::cast(weak->referent())->value(), 0);

  weak = WeakRef::dequeueReference(&list);
  EXPECT_EQ(SmallInteger::cast(weak->referent())->value(), 1);

  weak = WeakRef::dequeueReference(&list);
  EXPECT_EQ(SmallInteger::cast(weak->referent())->value(), 2);

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
    weak1->setReferent(SmallInteger::fromWord(i));
    WeakRef::enqueueReference(*weak1, &list1);

    Handle<WeakRef> weak2(&scope, runtime.newWeakRef());
    weak2->setReferent(SmallInteger::fromWord(i + 2));
    WeakRef::enqueueReference(*weak2, &list2);
  }
  Object* list = WeakRef::spliceQueue(list1, list2);
  Handle<WeakRef> weak(&scope, WeakRef::dequeueReference(&list));
  EXPECT_EQ(SmallInteger::cast(weak->referent())->value(), 0);

  weak = WeakRef::dequeueReference(&list);
  EXPECT_EQ(SmallInteger::cast(weak->referent())->value(), 1);

  weak = WeakRef::dequeueReference(&list);
  EXPECT_EQ(SmallInteger::cast(weak->referent())->value(), 2);

  weak = WeakRef::dequeueReference(&list);
  EXPECT_EQ(SmallInteger::cast(weak->referent())->value(), 3);

  EXPECT_EQ(list, None::object());
}

}  // namespace python
