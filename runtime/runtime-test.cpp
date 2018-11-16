#include "gtest/gtest.h"

#include "runtime.h"

namespace python {

TEST(RuntimeTest, CollectGarbage) {
  Runtime runtime;
  ASSERT_TRUE(runtime.heap()->verify());
  runtime.collectGarbage();
  ASSERT_TRUE(runtime.heap()->verify());
}

TEST(RuntimeTest, BuiltinsModuleExists) {
  Runtime runtime;
  HandleScope scope;

  Handle<Dictionary> modules(&scope, runtime.modules());
  Handle<Object> name(&scope, runtime.newStringFromCString("builtins"));
  Handle<Object> hash(&scope, runtime.hash(*name));
  Object* builtins;
  bool found = runtime.dictionaryAt(modules, name, hash, &builtins);
  ASSERT_TRUE(found);
  ASSERT_TRUE(builtins->isModule());
}

TEST(RuntimeTest, NewByteArray) {
  Runtime runtime;
  HandleScope scope;

  Handle<ByteArray> empty0(&scope, runtime.newByteArray(0));
  EXPECT_EQ(empty0->length(), 0);

  Handle<ByteArray> empty1(&scope, runtime.newByteArray(0));
  EXPECT_EQ(*empty0, *empty1);

  Handle<ByteArray> b1(&scope, runtime.newByteArrayFromCString("\x42", 1));
  EXPECT_EQ(b1->length(), 1);
  EXPECT_EQ(b1->size(), Utils::roundUp(kPointerSize + 1, kPointerSize));
  EXPECT_EQ(b1->byteAt(0), 0x42);

  Handle<ByteArray> b3(
      &scope, runtime.newByteArrayFromCString("\xAA\xBB\xCC", 3));
  EXPECT_EQ(b3->length(), 3);
  EXPECT_EQ(b3->size(), Utils::roundUp(kPointerSize + 3, kPointerSize));
  EXPECT_EQ(b3->byteAt(0), 0xAA);
  EXPECT_EQ(b3->byteAt(1), 0xBB);
  EXPECT_EQ(b3->byteAt(2), 0xCC);

  Handle<ByteArray> b254(&scope, runtime.newByteArray(254));
  EXPECT_EQ(b254->length(), 254);
  EXPECT_EQ(b254->size(), Utils::roundUp(kPointerSize + 254, kPointerSize));

  Handle<ByteArray> b255(&scope, runtime.newByteArray(255));
  EXPECT_EQ(b255->length(), 255);
  EXPECT_EQ(b255->size(), Utils::roundUp(kPointerSize * 2 + 255, kPointerSize));
}

TEST(RuntimeTest, NewCode) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  EXPECT_EQ(code->argcount(), 0);
  EXPECT_EQ(code->cell2arg(), 0);
  ASSERT_TRUE(code->cellvars()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->cellvars())->length(), 0);
  EXPECT_TRUE(code->code()->isNone());
  EXPECT_TRUE(code->consts()->isNone());
  EXPECT_TRUE(code->filename()->isNone());
  EXPECT_EQ(code->firstlineno(), 0);
  EXPECT_EQ(code->flags(), 0);
  ASSERT_TRUE(code->freevars()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->freevars())->length(), 0);
  EXPECT_EQ(code->kwonlyargcount(), 0);
  EXPECT_TRUE(code->lnotab()->isNone());
  EXPECT_TRUE(code->name()->isNone());
  EXPECT_EQ(code->nlocals(), 0);
  EXPECT_EQ(code->stacksize(), 0);
  EXPECT_TRUE(code->varnames()->isNone());
}

TEST(RuntimeTest, NewObjectArray) {
  Runtime runtime;
  HandleScope scope;

  Handle<ObjectArray> a0(&scope, runtime.newObjectArray(0));
  EXPECT_EQ(a0->length(), 0);

  Handle<ObjectArray> a1(&scope, runtime.newObjectArray(1));
  ASSERT_EQ(a1->length(), 1);
  EXPECT_EQ(a1->at(0), None::object());
  a1->atPut(0, SmallInteger::fromWord(42));
  EXPECT_EQ(a1->at(0), SmallInteger::fromWord(42));

  Handle<ObjectArray> a300(&scope, runtime.newObjectArray(300));
  ASSERT_EQ(a300->length(), 300);
}

TEST(RuntimeTest, NewString) {
  Runtime runtime;
  HandleScope scope;

  Handle<String> empty0(&scope, runtime.newString(0));
  EXPECT_EQ(empty0->length(), 0);

  Handle<String> empty1(&scope, runtime.newString(0));
  EXPECT_EQ(*empty0, *empty1);

  Handle<String> empty2(&scope, runtime.newStringFromCString("\0"));
  EXPECT_EQ(*empty0, *empty2);

  Handle<String> s1(&scope, runtime.newString(1));
  EXPECT_EQ(s1->length(), 1);
  EXPECT_EQ(s1->size(), Utils::roundUp(kPointerSize + 1, kPointerSize));

  Handle<String> s254(&scope, runtime.newString(254));
  EXPECT_EQ(s254->length(), 254);
  EXPECT_EQ(s254->size(), Utils::roundUp(kPointerSize + 254, kPointerSize));

  Handle<String> s255(&scope, runtime.newString(255));
  EXPECT_EQ(s255->length(), 255);
  EXPECT_EQ(s255->size(), Utils::roundUp(kPointerSize * 2 + 255, kPointerSize));

  Handle<String> s300(&scope, runtime.newString(300));
  ASSERT_EQ(s300->length(), 300);
}

TEST(RuntimeTest, HashBooleans) {
  Runtime runtime;

  // In CPython, False hashes to 0 and True hashes to 1.
  SmallInteger* hash0 =
      SmallInteger::cast(runtime.hash(Boolean::fromBool(false)));
  EXPECT_EQ(hash0->value(), 0);
  SmallInteger* hash1 =
      SmallInteger::cast(runtime.hash(Boolean::fromBool(true)));
  EXPECT_EQ(hash1->value(), 1);
}

TEST(RuntimeTest, HashByteArrays) {
  Runtime runtime;
  HandleScope scope;

  // Strings have their hash codes computed lazily.
  const char src1[] = {0x1, 0x2, 0x3};
  Handle<ByteArray> arr1(
      &scope, runtime.newByteArrayFromCString(src1, ARRAYSIZE(src1)));
  EXPECT_EQ(arr1->header()->hashCode(), 0);
  word hash1 = SmallInteger::cast(runtime.hash(*arr1))->value();
  EXPECT_NE(arr1->header()->hashCode(), 0);
  EXPECT_EQ(arr1->header()->hashCode(), hash1);

  word code1 =
      runtime.siphash24(reinterpret_cast<const byte*>(src1), ARRAYSIZE(src1));
  EXPECT_EQ(code1 & Header::kHashCodeMask, hash1);

  // String with different values should (ideally) hash differently.
  const char src2[] = {0x3, 0x2, 0x1};
  Handle<ByteArray> arr2(
      &scope, runtime.newByteArrayFromCString(src2, ARRAYSIZE(src2)));
  word hash2 = SmallInteger::cast(runtime.hash(*arr2))->value();
  EXPECT_NE(hash1, hash2);

  word code2 =
      runtime.siphash24(reinterpret_cast<const byte*>(src2), ARRAYSIZE(src2));
  EXPECT_EQ(code2 & Header::kHashCodeMask, hash2);

  // Strings with the same value should hash the same.
  const char src3[] = {0x1, 0x2, 0x3};
  Handle<ByteArray> arr3(
      &scope, runtime.newByteArrayFromCString(src3, ARRAYSIZE(src3)));
  word hash3 = SmallInteger::cast(runtime.hash(*arr3))->value();
  EXPECT_EQ(hash1, hash3);

  word code3 =
      runtime.siphash24(reinterpret_cast<const byte*>(src3), ARRAYSIZE(src3));
  EXPECT_EQ(code3 & Header::kHashCodeMask, hash3);
}

TEST(RuntimeTest, HashSmallIntegers) {
  Runtime runtime;

  // In CPython, Integers hash to themselves.
  SmallInteger* hash123 =
      SmallInteger::cast(runtime.hash(SmallInteger::fromWord(123)));
  EXPECT_EQ(hash123->value(), 123);
  SmallInteger* hash456 =
      SmallInteger::cast(runtime.hash(SmallInteger::fromWord(456)));
  EXPECT_EQ(hash456->value(), 456);
}

TEST(RuntimeTest, HashSingletonImmediates) {
  Runtime runtime;

  // In CPython, these objects hash to arbitrary values.
  word none_value = reinterpret_cast<word>(None::object());
  SmallInteger* hash_none = SmallInteger::cast(runtime.hash(None::object()));
  EXPECT_EQ(hash_none->value(), none_value);

  word ellipsis_value = reinterpret_cast<word>(Ellipsis::object());
  SmallInteger* hash_ellipsis =
      SmallInteger::cast(runtime.hash(Ellipsis::object()));
  EXPECT_EQ(hash_ellipsis->value(), ellipsis_value);
}

TEST(RuntimeTest, HashStrings) {
  Runtime runtime;
  HandleScope scope;

  // Strings have their hash codes computed lazily.
  Handle<String> str1(&scope, runtime.newStringFromCString("testing 123"));
  EXPECT_EQ(str1->header()->hashCode(), 0);
  SmallInteger* hash1 = SmallInteger::cast(runtime.hash(*str1));
  EXPECT_NE(str1->header()->hashCode(), 0);
  EXPECT_EQ(str1->header()->hashCode(), hash1->value());

  // String with different values should (ideally) hash differently.
  Handle<String> str2(&scope, runtime.newStringFromCString("321 testing"));
  SmallInteger* hash2 = SmallInteger::cast(runtime.hash(*str2));
  EXPECT_NE(hash1, hash2);

  // Strings with the same value should hash the same.
  Handle<String> str3(&scope, runtime.newStringFromCString("testing 123"));
  SmallInteger* hash3 = SmallInteger::cast(runtime.hash(*str3));
  EXPECT_EQ(hash1, hash3);
}

TEST(RuntimeTest, Random) {
  Runtime runtime;
  uword r1 = runtime.random();
  uword r2 = runtime.random();
  EXPECT_NE(r1, r2);
  uword r3 = runtime.random();
  EXPECT_NE(r2, r3);
  uword r4 = runtime.random();
  EXPECT_NE(r3, r4);
}

TEST(RuntimeTest, EnsureCapacity) {
  Runtime runtime;
  HandleScope scope;

  // Check that empty arrays expand
  Handle<ObjectArray> empty(&scope, runtime.newObjectArray(0));
  Handle<ObjectArray> orig(&scope, runtime.ensureCapacity(empty, 0));
  ASSERT_NE(*empty, *orig);
  ASSERT_GT(orig->length(), 0);

  // We shouldn't grow the array if there is sufficient capacity
  Handle<ObjectArray> ensured0(
      &scope, runtime.ensureCapacity(orig, orig->length() - 1));
  ASSERT_EQ(*orig, *ensured0);

  // We should double the array if there is insufficient capacity
  Handle<ObjectArray> ensured1(
      &scope, runtime.ensureCapacity(orig, orig->length()));
  ASSERT_EQ(ensured1->length(), orig->length() * 2);
}

} // namespace python
