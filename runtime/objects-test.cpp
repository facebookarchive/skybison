#include "gtest/gtest.h"

#include "runtime.h"

namespace python {

void expectHash(Object* object, word expectedHash) {
  Object* hash = object->hash();
  EXPECT_EQ(SmallInteger::cast(hash)->value(), expectedHash);
}

TEST(HashTest, TestHashBoolean) {
  // These are consistent with CPython
  expectHash(Boolean::fromBool(true), 1);
  expectHash(Boolean::fromBool(false), 0);
}

TEST(HashTest, TestHashIntegers) {
  // Integers hash to themselves in CPython
  expectHash(SmallInteger::fromWord(100), 100);
  expectHash(SmallInteger::fromWord(200), 200);
}

TEST(HashTest, TestHashImmediateObjects) {
  word expected = reinterpret_cast<word>(None::object());
  expectHash(None::object(), expected);

  expected = reinterpret_cast<word>(Ellipsis::object());
  expectHash(Ellipsis::object(), expected);
}

TEST(HashTest, TestHashStrings) {
  Runtime runtime;
  Object* obj = runtime.createStringFromCString("testing 123");
  ASSERT_NE(obj, nullptr);
  String* str1 = String::cast(obj);

  // Strings with different values should have different hashes
  obj = runtime.createStringFromCString("321 testing");
  ASSERT_NE(obj, nullptr);
  String* str2 = String::cast(obj);
  EXPECT_NE(str1->hash(), str2->hash());

  // Strings with the same value should have the same hash
  obj = runtime.createStringFromCString("testing 123");
  ASSERT_NE(obj, nullptr);
  String* str3 = String::cast(obj);
  EXPECT_EQ(str1->hash(), str3->hash());
}

} // namespace python
