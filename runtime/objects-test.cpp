#include "gtest/gtest.h"

#include <cstdint>

#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(DictionaryTest, EmptyDictionaryInvariants) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> dict(&scope, runtime.newDictionary());

  EXPECT_EQ(dict->numItems(), 0);
  ASSERT_TRUE(dict->data()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(dict->data())->length(), 0);
}

TEST(DictionaryTest, GetSet) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> dict(&scope, runtime.newDictionary());
  Handle<Object> key(&scope, SmallInteger::fromWord(12345));
  Object* retrieved;

  // Looking up a key that doesn't exist should fail
  EXPECT_TRUE(runtime.dictionaryAt(dict, key)->isError());

  // Store a value
  Handle<Object> stored(&scope, SmallInteger::fromWord(67890));
  runtime.dictionaryAtPut(dict, key, stored);
  EXPECT_EQ(dict->numItems(), 1);

  // Retrieve the stored value
  retrieved = runtime.dictionaryAt(dict, key);
  ASSERT_TRUE(retrieved->isSmallInteger());
  EXPECT_EQ(
      SmallInteger::cast(retrieved)->value(),
      SmallInteger::cast(*stored)->value());

  // Overwrite the stored value
  Handle<Object> newValue(&scope, SmallInteger::fromWord(5555));
  runtime.dictionaryAtPut(dict, key, newValue);
  EXPECT_EQ(dict->numItems(), 1);

  // Get the new value
  retrieved = runtime.dictionaryAt(dict, key);
  ASSERT_TRUE(retrieved->isSmallInteger());
  EXPECT_EQ(
      SmallInteger::cast(retrieved)->value(),
      SmallInteger::cast(*newValue)->value());
}

TEST(DictionaryTest, Remove) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> dict(&scope, runtime.newDictionary());
  Handle<Object> key(&scope, SmallInteger::fromWord(12345));
  Object* retrieved;

  // Removing a key that doesn't exist should fail
  bool found = runtime.dictionaryRemove(dict, key, &retrieved);
  EXPECT_FALSE(found);

  // Removing a key that exists should succeed and return the value that was
  // stored.
  Handle<Object> stored(&scope, SmallInteger::fromWord(54321));

  runtime.dictionaryAtPut(dict, key, stored);
  EXPECT_EQ(dict->numItems(), 1);

  found = runtime.dictionaryRemove(dict, key, &retrieved);
  ASSERT_TRUE(found);
  ASSERT_EQ(
      SmallInteger::cast(retrieved)->value(),
      SmallInteger::cast(*stored)->value());

  // Looking up a key that was deleted should fail
  EXPECT_TRUE(runtime.dictionaryAt(dict, key)->isError());
  EXPECT_EQ(dict->numItems(), 0);
}

TEST(DictionaryTest, Length) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> dict(&scope, runtime.newDictionary());

  // Add 10 items and make sure length reflects it
  for (int i = 0; i < 10; i++) {
    Handle<Object> key(&scope, SmallInteger::fromWord(i));
    runtime.dictionaryAtPut(dict, key, key);
  }
  EXPECT_EQ(dict->numItems(), 10);

  // Remove half the items
  for (int i = 0; i < 5; i++) {
    Handle<Object> key(&scope, SmallInteger::fromWord(i));
    Object* retrieved;
    bool found = runtime.dictionaryRemove(dict, key, &retrieved);
    ASSERT_TRUE(found);
  }
  EXPECT_EQ(dict->numItems(), 5);
}

TEST(DictionaryTest, AtIfAbsentPutLength) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> dict(&scope, runtime.newDictionary());

  Handle<Object> k1(&scope, SmallInteger::fromWord(1));
  Handle<Object> v1(&scope, SmallInteger::fromWord(111));
  runtime.dictionaryAtPut(dict, k1, v1);
  EXPECT_EQ(dict->numItems(), 1);

  class SmallIntegerCallback : public Callback<Object*> {
   public:
    explicit SmallIntegerCallback(int i) : i_(i) {}
    Object* call() override {
      return SmallInteger::fromWord(i_);
    }

   private:
    int i_;
  };

  // Add new item
  Handle<Object> k2(&scope, SmallInteger::fromWord(2));
  SmallIntegerCallback cb(222);
  Handle<Object> entry2(&scope, runtime.dictionaryAtIfAbsentPut(dict, k2, &cb));
  EXPECT_EQ(dict->numItems(), 2);
  Object* retrieved = runtime.dictionaryAt(dict, k2);
  EXPECT_TRUE(retrieved->isSmallInteger());
  EXPECT_EQ(retrieved, SmallInteger::fromWord(222));

  // Don't overrwite existing item 1 -> v1
  Handle<Object> k3(&scope, SmallInteger::fromWord(1));
  SmallIntegerCallback cb3(333);
  Handle<Object> entry3(
      &scope, runtime.dictionaryAtIfAbsentPut(dict, k3, &cb3));
  EXPECT_EQ(dict->numItems(), 2);
  retrieved = runtime.dictionaryAt(dict, k3);
  EXPECT_TRUE(retrieved->isSmallInteger());
  EXPECT_EQ(retrieved, *v1);
}

TEST(DictionaryTest, GrowWhenFull) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> dict(&scope, runtime.newDictionary());

  // Fill up the dict - we insert an initial key to force the allocation of the
  // backing ObjectArray.
  Handle<Object> initKey(&scope, SmallInteger::fromWord(0));
  runtime.dictionaryAtPut(dict, initKey, initKey);
  ASSERT_TRUE(dict->data()->isObjectArray());
  word initDataSize = ObjectArray::cast(dict->data())->length();

  auto makeKey = [&runtime](int i) {
    byte text[]{"0123456789abcdeghiklmn"};
    return runtime.newStringWithAll(View<byte>(text + i % 10, 10));
  };
  auto makeValue = [](int i) { return SmallInteger::fromWord(i); };

  // Fill in one fewer keys than would require growing the underlying object
  // array again
  word numKeys = Runtime::kInitialDictionaryCapacity;
  for (int i = 1; i < numKeys; i++) {
    Handle<Object> key(&scope, makeKey(i));
    Handle<Object> value(&scope, makeValue(i));
    runtime.dictionaryAtPut(dict, key, value);
  }

  // Add another key which should force us to double the capacity
  Handle<Object> straw(&scope, makeKey(numKeys));
  Handle<Object> strawValue(&scope, makeValue(numKeys));
  runtime.dictionaryAtPut(dict, straw, strawValue);
  ASSERT_TRUE(dict->data()->isObjectArray());
  word newDataSize = ObjectArray::cast(dict->data())->length();
  EXPECT_EQ(newDataSize, Runtime::kDictionaryGrowthFactor * initDataSize);

  // Make sure we can still read all the stored keys/values
  for (int i = 1; i <= numKeys; i++) {
    Handle<Object> key(&scope, makeKey(i));
    Object* value = runtime.dictionaryAt(dict, key);
    ASSERT_FALSE(value->isError());
    EXPECT_TRUE(Object::equals(value, makeValue(i)));
  }
}

TEST(DictionaryTest, CollidingKeys) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> dict(&scope, runtime.newDictionary());

  // Add two different keys with different values using the same hash
  Handle<Object> key1(&scope, SmallInteger::fromWord(1));
  runtime.dictionaryAtPut(dict, key1, key1);

  Handle<Object> key2(&scope, Boolean::trueObj());
  runtime.dictionaryAtPut(dict, key2, key2);

  // Make sure we get both back
  Object* retrieved = runtime.dictionaryAt(dict, key1);
  ASSERT_TRUE(retrieved->isSmallInteger());
  EXPECT_EQ(
      SmallInteger::cast(retrieved)->value(),
      SmallInteger::cast(*key1)->value());

  retrieved = runtime.dictionaryAt(dict, key2);
  ASSERT_TRUE(retrieved->isBoolean());
  EXPECT_EQ(Boolean::cast(retrieved)->value(), Boolean::cast(*key2)->value());
}

TEST(DictionaryTest, MixedKeys) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> dict(&scope, runtime.newDictionary());

  // Add keys of different type
  Handle<Object> intKey(&scope, SmallInteger::fromWord(100));
  runtime.dictionaryAtPut(dict, intKey, intKey);

  Handle<Object> strKey(&scope, runtime.newStringFromCString("testing 123"));
  runtime.dictionaryAtPut(dict, strKey, strKey);

  // Make sure we get the appropriate values back out
  Object* retrieved = runtime.dictionaryAt(dict, intKey);
  ASSERT_TRUE(retrieved->isSmallInteger());
  EXPECT_EQ(
      SmallInteger::cast(retrieved)->value(),
      SmallInteger::cast(*intKey)->value());

  retrieved = runtime.dictionaryAt(dict, strKey);
  ASSERT_TRUE(retrieved->isString());
  EXPECT_TRUE(Object::equals(*strKey, retrieved));
}

TEST(DictionaryTest, GetKeys) {
  Runtime runtime;
  HandleScope scope;

  // Create keys
  Handle<ObjectArray> keys(&scope, runtime.newObjectArray(4));
  keys->atPut(0, SmallInteger::fromWord(100));
  keys->atPut(1, runtime.newStringFromCString("testing 123"));
  keys->atPut(2, Boolean::trueObj());
  keys->atPut(3, None::object());

  // Add keys to dictionary
  Handle<Dictionary> dict(&scope, runtime.newDictionary());
  for (word i = 0; i < keys->length(); i++) {
    Handle<Object> key(&scope, keys->at(i));
    runtime.dictionaryAtPut(dict, key, key);
  }

  // Grab the keys and verify everything is there
  Handle<ObjectArray> retrieved(&scope, runtime.dictionaryKeys(dict));
  ASSERT_EQ(retrieved->length(), keys->length());
  for (word i = 0; i < keys->length(); i++) {
    Handle<Object> key(&scope, keys->at(i));
    EXPECT_TRUE(objectArrayContains(retrieved, key)) << " missing key " << i;
  }
}

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

  Handle<Integer> max_small_int(
      &scope, runtime.newInteger(SmallInteger::kMaxValue));
  EXPECT_TRUE(max_small_int->isPositive());

  Handle<Integer> min_small_int(
      &scope, runtime.newInteger(SmallInteger::kMinValue));
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

  Handle<Integer> max_small_int(
      &scope, runtime.newInteger(SmallInteger::kMaxValue));
  EXPECT_FALSE(max_small_int->isNegative());

  Handle<Integer> min_small_int(
      &scope, runtime.newInteger(SmallInteger::kMinValue));
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

  Handle<Integer> max_small_int(
      &scope, runtime.newInteger(SmallInteger::kMaxValue));
  EXPECT_FALSE(max_small_int->isZero());

  Handle<Integer> min_small_int(
      &scope, runtime.newInteger(SmallInteger::kMinValue));
  EXPECT_FALSE(min_small_int->isZero());

  Handle<Integer> max_word(&scope, runtime.newInteger(kMaxWord));
  EXPECT_FALSE(max_word->isZero());

  Handle<Integer> min_word(&scope, runtime.newInteger(kMinWord));
  EXPECT_FALSE(min_word->isZero());
}

TEST(ListTest, EmptyListInvariants) {
  Runtime runtime;
  List* list = List::cast(runtime.newList());
  ASSERT_EQ(list->capacity(), 0);
  ASSERT_EQ(list->allocated(), 0);
}

TEST(ListTest, AppendToList) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());

  // Check that list capacity grows according to a doubling schedule
  word expectedCapacity[] = {
      4, 4, 4, 4, 8, 8, 8, 8, 16, 16, 16, 16, 16, 16, 16, 16};
  for (int i = 0; i < 16; i++) {
    Handle<Object> value(&scope, SmallInteger::fromWord(i));
    runtime.listAdd(list, value);
    ASSERT_EQ(list->capacity(), expectedCapacity[i]);
    ASSERT_EQ(list->allocated(), i + 1);
  }

  // Sanity check list contents
  for (int i = 0; i < 16; i++) {
    SmallInteger* elem = SmallInteger::cast(list->at(i));
    ASSERT_EQ(elem->value(), i);
  }
}

TEST(ListTest, InsertToList) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());

  for (int i = 0; i < 16; i++) {
    if (i == 2 || i == 12)
      continue;
    Handle<Object> value(&scope, SmallInteger::fromWord(i));
    runtime.listAdd(list, value);
  }
  ASSERT_NE(SmallInteger::cast(list->at(2))->value(), 2);
  ASSERT_NE(SmallInteger::cast(list->at(12))->value(), 12);

  Handle<Object> value2(&scope, SmallInteger::fromWord(2));
  runtime.listInsert(list, value2, 2);
  Handle<Object> value12(&scope, SmallInteger::fromWord(12));
  runtime.listInsert(list, value12, 12);

  ASSERT_EQ(SmallInteger::cast(list->at(2))->value(), 2);
  ASSERT_EQ(SmallInteger::cast(list->at(12))->value(), 12);
  for (int i = 0; i < 16; i++) {
    SmallInteger* elem = SmallInteger::cast(list->at(i));
    ASSERT_EQ(elem->value(), i);
  }
}

TEST(ListTest, InsertToListBounds) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  for (int i = 0; i < 10; i++) {
    Handle<Object> value(&scope, SmallInteger::fromWord(i));
    runtime.listAdd(list, value);
  }
  ASSERT_EQ(list->allocated(), 10);

  Handle<Object> value100(&scope, SmallInteger::fromWord(100));
  runtime.listInsert(list, value100, 100);
  ASSERT_EQ(list->allocated(), 11);
  ASSERT_EQ(SmallInteger::cast(list->at(10))->value(), 100);

  Handle<Object> value0(&scope, SmallInteger::fromWord(400));
  runtime.listInsert(list, value0, 0);
  ASSERT_EQ(list->allocated(), 12);
  ASSERT_EQ(SmallInteger::cast(list->at(0))->value(), 400);

  Handle<Object> valueN(&scope, SmallInteger::fromWord(-10));
  runtime.listInsert(list, valueN, -10);
  ASSERT_EQ(list->allocated(), 13);
  ASSERT_EQ(SmallInteger::cast(list->at(0))->value(), -10);
}

TEST(ListTest, PopList) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  for (int i = 0; i < 16; i++) {
    Handle<Object> value(&scope, SmallInteger::fromWord(i));
    runtime.listAdd(list, value);
  }
  ASSERT_EQ(list->allocated(), 16);

  // Pop from the end
  Object* res1 = runtime.listPop(list, 15);
  ASSERT_EQ(list->allocated(), 15);
  ASSERT_EQ(SmallInteger::cast(list->at(14))->value(), 14);
  ASSERT_EQ(SmallInteger::cast(res1)->value(), 15);

  // Pop elements from 5 - 10
  for (int i = 0; i < 5; i++) {
    Object* res5 = runtime.listPop(list, 5);
    ASSERT_EQ(SmallInteger::cast(res5)->value(), i + 5);
  }
  ASSERT_EQ(list->allocated(), 10);
  for (int i = 0; i < 5; i++) {
    SmallInteger* elem = SmallInteger::cast(list->at(i));
    ASSERT_EQ(elem->value(), i);
  }
  for (int i = 5; i < 10; i++) {
    SmallInteger* elem = SmallInteger::cast(list->at(i));
    ASSERT_EQ(elem->value(), i + 5);
  }

  // Pop element 0
  Object* res0 = runtime.listPop(list, 0);
  ASSERT_EQ(list->allocated(), 9);
  ASSERT_EQ(SmallInteger::cast(list->at(0))->value(), 1);
  ASSERT_EQ(SmallInteger::cast(res0)->value(), 0);
}

TEST(ListTest, ListExtendList) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  Handle<List> list1(&scope, runtime.newList());
  for (int i = 0; i < 16; i++) {
    Handle<Object> value(&scope, SmallInteger::fromWord(i));
    Handle<Object> value1(&scope, SmallInteger::fromWord(i + 16));
    runtime.listAdd(list, value);
    runtime.listAdd(list1, value1);
  }
  EXPECT_EQ(list->allocated(), 16);
  Handle<Object> list1_handle(&scope, *list1);
  runtime.listExtend(list, list1_handle);
  ASSERT_EQ(list->allocated(), 32);
  for (int i = 0; i < 32; i++) {
    SmallInteger* elem = SmallInteger::cast(list->at(i));
    EXPECT_EQ(elem->value(), i);
  }
}

TEST(ListTest, ListExtendListIterator) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  Handle<List> list1(&scope, runtime.newList());
  for (int i = 0; i < 16; i++) {
    Handle<Object> value(&scope, SmallInteger::fromWord(i));
    Handle<Object> value1(&scope, SmallInteger::fromWord(i + 16));
    runtime.listAdd(list, value);
    runtime.listAdd(list1, value1);
  }
  EXPECT_EQ(list->allocated(), 16);
  Handle<Object> list1_handle(&scope, *list1);
  Handle<Object> list1_iterator(&scope, runtime.newListIterator(list1_handle));
  runtime.listExtend(list, list1_iterator);
  ASSERT_EQ(list->allocated(), 32);
  for (int i = 0; i < 32; i++) {
    SmallInteger* elem = SmallInteger::cast(list->at(i));
    EXPECT_EQ(elem->value(), i);
  }
}

TEST(ListTest, ListExtendObjectArray) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  Handle<Object> object_array0(&scope, runtime.newObjectArray(0));
  Handle<ObjectArray> object_array1(&scope, runtime.newObjectArray(1));
  Handle<ObjectArray> object_array2(&scope, runtime.newObjectArray(16));

  for (int i = 0; i < 16; i++) {
    Handle<Object> value(&scope, SmallInteger::fromWord(i));
    runtime.listAdd(list, value);
  }
  runtime.listExtend(list, object_array0);
  EXPECT_EQ(list->allocated(), 16);

  Handle<Object> object_array1_handle(&scope, *object_array1);
  object_array1->atPut(0, None::object());
  runtime.listExtend(list, object_array1_handle);
  ASSERT_GE(list->allocated(), 17);
  ASSERT_TRUE(list->at(16)->isNone());

  for (word i = 0; i < 16; i++)
    object_array2->atPut(i, SmallInteger::fromWord(i));

  Handle<Object> object_array2_handle(&scope, *object_array2);
  runtime.listExtend(list, object_array2_handle);
  ASSERT_GE(list->allocated(), 16 + 1 + 16);
  for (word i = 0; i < 16; i++)
    EXPECT_EQ(list->at(i + 17), SmallInteger::fromWord(i));
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

TEST(ModulesTest, TestCreate) {
  Runtime runtime;
  HandleScope scope;
  Handle<Object> name(&scope, runtime.newStringFromCString("mymodule"));
  Handle<Module> module(&scope, runtime.newModule(name));
  EXPECT_EQ(module->name(), *name);
  EXPECT_TRUE(module->dictionary()->isDictionary());
}

TEST(ObjectArrayTest, Create) {
  Runtime runtime;

  Object* obj0 = runtime.newObjectArray(0);
  ASSERT_TRUE(obj0->isObjectArray());
  ObjectArray* array0 = ObjectArray::cast(obj0);
  EXPECT_EQ(array0->length(), 0);

  Object* obj1 = runtime.newObjectArray(1);
  ASSERT_TRUE(obj1->isObjectArray());
  ObjectArray* array1 = ObjectArray::cast(obj1);
  EXPECT_EQ(array1->length(), 1);

  Object* obj7 = runtime.newObjectArray(7);
  ASSERT_TRUE(obj7->isObjectArray());
  ObjectArray* array7 = ObjectArray::cast(obj7);
  EXPECT_EQ(array7->length(), 7);

  Object* obj8 = runtime.newObjectArray(8);
  ASSERT_TRUE(obj8->isObjectArray());
  ObjectArray* array8 = ObjectArray::cast(obj8);
  EXPECT_EQ(array8->length(), 8);
}

TEST(SmallInteger, IsValid) {
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

TEST(LargeString, CopyTo) {
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

TEST(String, ToCString) {
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

TEST(SetTest, EmptySetInvariants) {
  Runtime runtime;
  HandleScope scope;
  Handle<Set> set(&scope, runtime.newSet());

  EXPECT_EQ(set->numItems(), 0);
  ASSERT_TRUE(set->isSet());
  ASSERT_TRUE(set->data()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(set->data())->length(), 0);
}

TEST(SetTest, Add) {
  Runtime runtime;
  HandleScope scope;
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Object> value(&scope, SmallInteger::fromWord(12345));

  // Store a value
  runtime.setAdd(set, value);
  EXPECT_EQ(set->numItems(), 1);

  // Retrieve the stored value
  ASSERT_TRUE(runtime.setIncludes(set, value));

  // Add a new value
  Handle<Object> newValue(&scope, SmallInteger::fromWord(5555));
  runtime.setAdd(set, newValue);
  EXPECT_EQ(set->numItems(), 2);

  // Get the new value
  ASSERT_TRUE(runtime.setIncludes(set, newValue));

  // Add a existing value
  Handle<Object> same_value(&scope, SmallInteger::fromWord(12345));
  Object* old_value = runtime.setAdd(set, same_value);
  EXPECT_EQ(set->numItems(), 2);
  EXPECT_EQ(old_value, *value);
}

TEST(SetTest, Remove) {
  Runtime runtime;
  HandleScope scope;
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Object> value(&scope, SmallInteger::fromWord(12345));

  // Removing a key that doesn't exist should fail
  EXPECT_FALSE(runtime.setRemove(set, value));

  runtime.setAdd(set, value);
  EXPECT_EQ(set->numItems(), 1);

  ASSERT_TRUE(runtime.setRemove(set, value));
  EXPECT_EQ(set->numItems(), 0);

  // Looking up a key that was deleted should fail
  ASSERT_FALSE(runtime.setIncludes(set, value));
}

TEST(SetTest, Grow) {
  Runtime runtime;
  HandleScope scope;
  Handle<Set> set(&scope, runtime.newSet());

  // Fill up the dict - we insert an initial key to force the allocation of the
  // backing ObjectArray.
  Handle<Object> initKey(&scope, SmallInteger::fromWord(0));
  runtime.setAdd(set, initKey);
  ASSERT_TRUE(set->data()->isObjectArray());
  word initDataSize = ObjectArray::cast(set->data())->length();

  auto makeKey = [&runtime](int i) {
    byte text[]{"0123456789abcdeghiklmn"};
    return runtime.newStringWithAll(View<byte>(text + i % 10, 10));
  };

  // Fill in one fewer keys than would require growing the underlying object
  // array again
  word numKeys = Runtime::kInitialSetCapacity;
  for (int i = 1; i < numKeys; i++) {
    Handle<Object> key(&scope, makeKey(i));
    runtime.setAdd(set, key);
  }

  // Add another key which should force us to double the capacity
  Handle<Object> straw(&scope, makeKey(numKeys));
  runtime.setAdd(set, straw);
  ASSERT_TRUE(set->data()->isObjectArray());
  word newDataSize = ObjectArray::cast(set->data())->length();
  EXPECT_EQ(newDataSize, Runtime::kSetGrowthFactor * initDataSize);

  // Make sure we can still read all the stored keys
  for (int i = 1; i <= numKeys; i++) {
    Handle<Object> key(&scope, makeKey(i));
    bool found = runtime.setIncludes(set, key);
    ASSERT_TRUE(found);
  }
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

} // namespace python
