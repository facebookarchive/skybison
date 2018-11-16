#include "gtest/gtest.h"

#include <cstdint>

#include "runtime.h"

namespace python {

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
  bool found = runtime.dictionaryAt(dict, key, &retrieved);
  EXPECT_FALSE(found);

  // Store a value
  Handle<Object> stored(&scope, SmallInteger::fromWord(67890));
  runtime.dictionaryAtPut(dict, key, stored);

  // Retrieve the stored value
  found = runtime.dictionaryAt(dict, key, &retrieved);
  ASSERT_TRUE(found);
  EXPECT_EQ(
      SmallInteger::cast(retrieved)->value(),
      SmallInteger::cast(*stored)->value());

  // Overwrite the stored value
  Handle<Object> newValue(&scope, SmallInteger::fromWord(5555));
  runtime.dictionaryAtPut(dict, key, newValue);

  // Get the new value
  found = runtime.dictionaryAt(dict, key, &retrieved);
  ASSERT_TRUE(found);
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
  found = runtime.dictionaryRemove(dict, key, &retrieved);
  ASSERT_TRUE(found);
  ASSERT_EQ(
      SmallInteger::cast(retrieved)->value(),
      SmallInteger::cast(*stored)->value());

  // Looking up a key that was deleted should fail
  found = runtime.dictionaryAt(dict, key, &retrieved);
  ASSERT_FALSE(found);
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

  // Fill in one fewer keys than would require growing the underlying object
  // array again
  word numKeys = Runtime::kInitialDictionaryCapacity + 1;
  for (int i = 1; i < numKeys; i++) {
    Handle<Object> key(&scope, SmallInteger::fromWord(i));
    runtime.dictionaryAtPut(dict, key, key);
  }

  // Add another key which should force us to double the capacity
  Handle<Object> straw(&scope, SmallInteger::fromWord(numKeys));
  runtime.dictionaryAtPut(dict, straw, straw);
  ASSERT_TRUE(dict->data()->isObjectArray());
  word newDataSize = ObjectArray::cast(dict->data())->length();
  EXPECT_EQ(newDataSize, Runtime::kDictionaryGrowthFactor * initDataSize);

  // Make sure we can still read all the stored keys/values
  for (int i = 0; i < numKeys; i++) {
    Object* value;
    Handle<Object> key(&scope, SmallInteger::fromWord(i));
    bool found = runtime.dictionaryAt(dict, key, &value);
    ASSERT_TRUE(found);
    EXPECT_EQ(SmallInteger::cast(value)->value(), i);
  }
}

TEST(DictionaryTest, CollidingKeys) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> dict(&scope, runtime.newDictionary());

  // Add two different keys with different values using the same hash
  Handle<Object> key1(&scope, SmallInteger::fromWord(1));
  runtime.dictionaryAtPut(dict, key1, key1);

  Handle<Object> key2(&scope, Boolean::fromBool(true));
  runtime.dictionaryAtPut(dict, key2, key2);

  // Make sure we get both back
  Object* retrieved;
  bool found = runtime.dictionaryAt(dict, key1, &retrieved);
  EXPECT_TRUE(found);
  EXPECT_EQ(
      SmallInteger::cast(retrieved)->value(),
      SmallInteger::cast(*key1)->value());

  found = runtime.dictionaryAt(dict, key2, &retrieved);
  EXPECT_TRUE(found);
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
  Object* retrieved;
  bool found = runtime.dictionaryAt(dict, intKey, &retrieved);
  EXPECT_TRUE(found);
  EXPECT_EQ(
      SmallInteger::cast(retrieved)->value(),
      SmallInteger::cast(*intKey)->value());

  found = runtime.dictionaryAt(dict, strKey, &retrieved);
  EXPECT_TRUE(found);
  EXPECT_TRUE(Object::equals(*strKey, retrieved));
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
  EXPECT_DEBUG_DEATH(SmallInteger::fromWord(INTPTR_MAX), "fromWord");
  EXPECT_FALSE(SmallInteger::isValid(INTPTR_MIN));
  EXPECT_DEBUG_DEATH(SmallInteger::fromWord(INTPTR_MIN), "fromWord");

  EXPECT_TRUE(SmallInteger::isValid(SmallInteger::kMaxValue));
  EXPECT_TRUE(SmallInteger::isValid(SmallInteger::kMaxValue - 1));
  EXPECT_FALSE(SmallInteger::isValid(SmallInteger::kMaxValue + 1));

  EXPECT_TRUE(SmallInteger::isValid(SmallInteger::kMinValue));
  EXPECT_FALSE(SmallInteger::isValid(SmallInteger::kMinValue - 1));
  EXPECT_TRUE(SmallInteger::isValid(SmallInteger::kMinValue + 1));
}

} // namespace python
