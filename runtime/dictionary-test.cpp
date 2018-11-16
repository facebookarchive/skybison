#include "gtest/gtest.h"

#include "runtime.h"

namespace python {

TEST(DictionaryTest, EmptyDictionaryInvariants) {
  Runtime runtime;
  Object* obj = runtime.createDictionary();
  ASSERT_NE(obj, nullptr);
  Dictionary* dict = Dictionary::cast(obj);

  ASSERT_EQ(dict->numItems(), 0);
  ASSERT_EQ(dict->capacity(), 8);
}

TEST(DictionaryTest, GetSet) {
  Runtime runtime;
  Object* obj = runtime.createDictionary();
  ASSERT_NE(obj, nullptr);
  Dictionary* dict = Dictionary::cast(obj);
  SmallInteger* key = SmallInteger::fromWord(12345);
  word hash = 12345;
  Object* retrieved;

  // Looking up a key that doesn't exist should fail
  bool found = Dictionary::itemAt(dict, key, hash, &retrieved);
  EXPECT_FALSE(found);

  // Store a value
  SmallInteger* stored = SmallInteger::fromWord(67890);
  Dictionary::itemAtPut(dict, key, hash, stored, &runtime);

  // Retrieve the stored value
  found = Dictionary::itemAt(dict, key, hash, &retrieved);
  ASSERT_TRUE(found);
  EXPECT_EQ(SmallInteger::cast(retrieved)->value(), stored->value());

  // Overwrite the stored value
  stored = SmallInteger::fromWord(5555);
  Dictionary::itemAtPut(dict, key, hash, stored, &runtime);

  // Get the new value
  found = Dictionary::itemAt(dict, key, hash, &retrieved);
  ASSERT_TRUE(found);
  EXPECT_EQ(SmallInteger::cast(retrieved)->value(), stored->value());
}

TEST(DictionaryTest, Remove) {
  Runtime runtime;
  Object* obj = runtime.createDictionary();
  ASSERT_NE(obj, nullptr);
  Dictionary* dict = Dictionary::cast(obj);
  SmallInteger* key = SmallInteger::fromWord(12345);
  Object* retrieved;
  word hash = 12345;

  // Removing a key that doesn't exist should fail
  bool found = Dictionary::itemAtRemove(dict, key, hash, &retrieved);
  EXPECT_FALSE(found);

  // Removing a key that exists should succeed and return the value that was
  // stored.
  SmallInteger* stored = SmallInteger::fromWord(54321);
  Dictionary::itemAtPut(dict, key, hash, stored, &runtime);
  found = Dictionary::itemAtRemove(dict, key, hash, &retrieved);
  ASSERT_TRUE(found);
  ASSERT_EQ(SmallInteger::cast(retrieved)->value(), stored->value());

  // Looking up a key that was deleted should fail
  found = Dictionary::itemAt(dict, key, hash, &retrieved);
  ASSERT_FALSE(found);
}

TEST(DictionaryTest, Length) {
  Runtime runtime;
  Object* obj = runtime.createDictionary();
  ASSERT_NE(obj, nullptr);
  Dictionary* dict = Dictionary::cast(obj);

  // Add 10 items and make sure length reflects it
  for (int i = 0; i < 10; i++) {
    SmallInteger* key = SmallInteger::fromWord(i);
    Dictionary::itemAtPut(dict, key, i, key, &runtime);
  }
  EXPECT_EQ(dict->numItems(), 10);

  // Remove half the items
  for (int i = 0; i < 5; i++) {
    SmallInteger* key = SmallInteger::fromWord(i);
    Object* retrieved;
    bool found = Dictionary::itemAtRemove(dict, key, i, &retrieved);
    ASSERT_TRUE(found);
  }
  EXPECT_EQ(dict->numItems(), 5);
}

TEST(DictionaryTest, GrowWhenFull) {
  Runtime runtime;
  Object* obj = runtime.createDictionary();
  ASSERT_NE(obj, nullptr);
  Dictionary* dict = Dictionary::cast(obj);

  // Fill up the dict
  word initCap = dict->capacity();
  for (int i = 0; i < initCap; i++) {
    SmallInteger* key = SmallInteger::fromWord(i);
    Dictionary::itemAtPut(dict, key, i, key, &runtime);
  }
  ASSERT_EQ(dict->capacity(), initCap);

  // Add another key which should force us to grow the underlying object array
  SmallInteger* straw = SmallInteger::fromWord(initCap);
  Dictionary::itemAtPut(dict, straw, initCap, straw, &runtime);
  ASSERT_GT(dict->capacity(), initCap);

  // Make sure we can still read all the stored keys/values
  for (int i = 0; i < initCap + 1; i++) {
    SmallInteger* key = SmallInteger::fromWord(i);
    Object* value;
    bool found = Dictionary::itemAt(dict, key, i, &value);
    ASSERT_TRUE(found);
    EXPECT_EQ(SmallInteger::cast(value)->value(), i);
  }
}

TEST(DictionaryTest, CollidingKeys) {
  Runtime runtime;
  Object* obj = runtime.createDictionary();
  ASSERT_NE(obj, nullptr);
  Dictionary* dict = Dictionary::cast(obj);

  // Add two different keys with different values using the same hash
  word hash = 100;
  SmallInteger* key1 = SmallInteger::fromWord(100);
  Dictionary::itemAtPut(dict, key1, hash, key1, &runtime);

  SmallInteger* key2 = SmallInteger::fromWord(200);
  Dictionary::itemAtPut(dict, key2, hash, key2, &runtime);

  // Make sure we get both back
  Object* retrieved;
  bool found = Dictionary::itemAt(dict, key1, hash, &retrieved);
  EXPECT_TRUE(found);
  EXPECT_EQ(SmallInteger::cast(retrieved)->value(), key1->value());

  found = Dictionary::itemAt(dict, key2, hash, &retrieved);
  EXPECT_TRUE(found);
  EXPECT_EQ(SmallInteger::cast(retrieved)->value(), key2->value());
}

TEST(DictionaryTest, MixedKeys) {
  Runtime runtime;
  Object* obj = runtime.createDictionary();
  ASSERT_NE(obj, nullptr);
  Dictionary* dict = Dictionary::cast(obj);

  // Add keys of different type
  word intHash = 100;
  SmallInteger* intKey = SmallInteger::fromWord(100);
  Dictionary::itemAtPut(dict, intKey, intHash, intKey, &runtime);

  word strHash = 200;
  String* strKey = String::cast(runtime.createStringFromCString("testing 123"));
  Dictionary::itemAtPut(dict, strKey, strHash, strKey, &runtime);

  // Make sure we get the appropriate values back out
  Object* retrieved;
  bool found = Dictionary::itemAt(dict, intKey, intHash, &retrieved);
  EXPECT_TRUE(found);
  EXPECT_EQ(SmallInteger::cast(retrieved)->value(), intKey->value());

  found = Dictionary::itemAt(dict, strKey, strHash, &retrieved);
  EXPECT_TRUE(found);
  EXPECT_TRUE(Object::equals(strKey, retrieved));
}

} // namespace python
