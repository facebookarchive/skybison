#include "gtest/gtest.h"

#include "runtime.h"

namespace python {

TEST(DictionaryTest, EmptyDictionaryInvariants) {
  Runtime runtime;
  Object* obj = runtime.newDictionary();
  ASSERT_NE(obj, nullptr);
  Dictionary* dict = Dictionary::cast(obj);

  ASSERT_EQ(dict->numItems(), 0);
  ASSERT_EQ(dict->capacity(), 8);
}

TEST(DictionaryTest, GetSet) {
  Runtime runtime;
  Object* obj = runtime.newDictionary();
  ASSERT_NE(obj, nullptr);
  Dictionary* dict = Dictionary::cast(obj);
  SmallInteger* key = SmallInteger::fromWord(12345);
  SmallInteger* hash = SmallInteger::fromWord(12345);
  Object* retrieved;

  // Looking up a key that doesn't exist should fail
  bool found = Dictionary::at(dict, key, hash, &retrieved);
  EXPECT_FALSE(found);

  // Store a value
  SmallInteger* stored = SmallInteger::fromWord(67890);
  Dictionary::atPut(dict, key, hash, stored, &runtime);

  // Retrieve the stored value
  found = Dictionary::at(dict, key, hash, &retrieved);
  ASSERT_TRUE(found);
  EXPECT_EQ(SmallInteger::cast(retrieved)->value(), stored->value());

  // Overwrite the stored value
  stored = SmallInteger::fromWord(5555);
  Dictionary::atPut(dict, key, hash, stored, &runtime);

  // Get the new value
  found = Dictionary::at(dict, key, hash, &retrieved);
  ASSERT_TRUE(found);
  EXPECT_EQ(SmallInteger::cast(retrieved)->value(), stored->value());
}

TEST(DictionaryTest, Remove) {
  Runtime runtime;
  Object* obj = runtime.newDictionary();
  ASSERT_NE(obj, nullptr);
  Dictionary* dict = Dictionary::cast(obj);
  SmallInteger* key = SmallInteger::fromWord(12345);
  Object* retrieved;
  SmallInteger* hash = SmallInteger::fromWord(12345);

  // Removing a key that doesn't exist should fail
  bool found = Dictionary::remove(dict, key, hash, &retrieved);
  EXPECT_FALSE(found);

  // Removing a key that exists should succeed and return the value that was
  // stored.
  SmallInteger* stored = SmallInteger::fromWord(54321);
  Dictionary::atPut(dict, key, hash, stored, &runtime);
  found = Dictionary::remove(dict, key, hash, &retrieved);
  ASSERT_TRUE(found);
  ASSERT_EQ(SmallInteger::cast(retrieved)->value(), stored->value());

  // Looking up a key that was deleted should fail
  found = Dictionary::at(dict, key, hash, &retrieved);
  ASSERT_FALSE(found);
}

TEST(DictionaryTest, Length) {
  Runtime runtime;
  Object* obj = runtime.newDictionary();
  ASSERT_NE(obj, nullptr);
  Dictionary* dict = Dictionary::cast(obj);

  // Add 10 items and make sure length reflects it
  for (int i = 0; i < 10; i++) {
    SmallInteger* key = SmallInteger::fromWord(i);
    Dictionary::atPut(dict, key, key, key, &runtime);
  }
  EXPECT_EQ(dict->numItems(), 10);

  // Remove half the items
  for (int i = 0; i < 5; i++) {
    SmallInteger* key = SmallInteger::fromWord(i);
    Object* retrieved;
    bool found = Dictionary::remove(dict, key, key, &retrieved);
    ASSERT_TRUE(found);
  }
  EXPECT_EQ(dict->numItems(), 5);
}

TEST(DictionaryTest, GrowWhenFull) {
  Runtime runtime;
  Object* obj = runtime.newDictionary();
  ASSERT_NE(obj, nullptr);
  Dictionary* dict = Dictionary::cast(obj);

  // Fill up the dict
  word initCap = dict->capacity();
  for (int i = 0; i < initCap; i++) {
    SmallInteger* key = SmallInteger::fromWord(i);
    Dictionary::atPut(dict, key, key, key, &runtime);
  }
  ASSERT_EQ(dict->capacity(), initCap);

  // Add another key which should force us to grow the underlying object array
  SmallInteger* straw = SmallInteger::fromWord(initCap);
  Dictionary::atPut(dict, straw, straw, straw, &runtime);
  ASSERT_GT(dict->capacity(), initCap);

  // Make sure we can still read all the stored keys/values
  for (int i = 0; i < initCap + 1; i++) {
    SmallInteger* key = SmallInteger::fromWord(i);
    Object* value;
    bool found = Dictionary::at(dict, key, key, &value);
    ASSERT_TRUE(found);
    EXPECT_EQ(SmallInteger::cast(value)->value(), i);
  }
}

TEST(DictionaryTest, CollidingKeys) {
  Runtime runtime;
  Object* obj = runtime.newDictionary();
  ASSERT_NE(obj, nullptr);
  Dictionary* dict = Dictionary::cast(obj);

  // Add two different keys with different values using the same hash
  SmallInteger* key1 = SmallInteger::fromWord(100);
  SmallInteger* hash = key1;
  Dictionary::atPut(dict, key1, hash, key1, &runtime);

  SmallInteger* key2 = SmallInteger::fromWord(200);
  Dictionary::atPut(dict, key2, hash, key2, &runtime);

  // Make sure we get both back
  Object* retrieved;
  bool found = Dictionary::at(dict, key1, hash, &retrieved);
  EXPECT_TRUE(found);
  EXPECT_EQ(SmallInteger::cast(retrieved)->value(), key1->value());

  found = Dictionary::at(dict, key2, hash, &retrieved);
  EXPECT_TRUE(found);
  EXPECT_EQ(SmallInteger::cast(retrieved)->value(), key2->value());
}

TEST(DictionaryTest, MixedKeys) {
  Runtime runtime;
  Object* obj = runtime.newDictionary();
  ASSERT_NE(obj, nullptr);
  Dictionary* dict = Dictionary::cast(obj);

  // Add keys of different type
  SmallInteger* intKey = SmallInteger::fromWord(100);
  SmallInteger* intHash = intKey;
  Dictionary::atPut(dict, intKey, intHash, intKey, &runtime);

  SmallInteger* strHash = SmallInteger::fromWord(200);
  String* strKey = String::cast(runtime.newStringFromCString("testing 123"));
  Dictionary::atPut(dict, strKey, strHash, strKey, &runtime);

  // Make sure we get the appropriate values back out
  Object* retrieved;
  bool found = Dictionary::at(dict, intKey, intHash, &retrieved);
  EXPECT_TRUE(found);
  EXPECT_EQ(SmallInteger::cast(retrieved)->value(), intKey->value());

  found = Dictionary::at(dict, strKey, strHash, &retrieved);
  EXPECT_TRUE(found);
  EXPECT_TRUE(Object::equals(strKey, retrieved));
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
    runtime.appendToList(list, value);
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
  Object* name = runtime.newStringFromCString("mymodule");
  ASSERT_NE(name, nullptr);
  Object* obj = runtime.newModule(name);
  ASSERT_NE(obj, nullptr);
  ASSERT_TRUE(obj->isModule());
  Module* mod = Module::cast(obj);
  EXPECT_EQ(mod->name(), name);
  obj = mod->dictionary();
  EXPECT_TRUE(obj->isDictionary());
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

} // namespace python
