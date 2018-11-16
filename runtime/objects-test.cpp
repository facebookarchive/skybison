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
    return runtime.newStringWithAll(view(text + i % 10, 10));
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

  Handle<Object> key2(&scope, Boolean::fromBool(true));
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
  keys->atPut(2, Boolean::fromBool(true));
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
  EXPECT_TRUE(o->isDouble());
  Double* d = Double::cast(o);
  EXPECT_EQ(d->value(), 3.14);
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
    return runtime.newStringWithAll(view(text + i % 10, 10));
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

} // namespace python
