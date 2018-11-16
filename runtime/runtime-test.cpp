#include "gtest/gtest.h"

#include "bytecode.h"
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
  Object* builtins;
  bool found = runtime.dictionaryAt(modules, name, &builtins);
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

  const byte src1[] = {0x42};
  Handle<ByteArray> b1(&scope, runtime.newByteArrayWithAll(src1));
  EXPECT_EQ(b1->length(), 1);
  EXPECT_EQ(b1->size(), Utils::roundUp(kPointerSize + 1, kPointerSize));
  EXPECT_EQ(b1->byteAt(0), 0x42);

  const byte src3[] = {0xAA, 0xBB, 0xCC};
  Handle<ByteArray> b3(&scope, runtime.newByteArrayWithAll(src3));
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
  ASSERT_TRUE(s1->isLargeString());
  EXPECT_EQ(
      LargeString::cast(*s1)->size(),
      Utils::roundUp(kPointerSize + 1, kPointerSize));

  Handle<LargeString> s254(&scope, runtime.newString(254));
  EXPECT_EQ(s254->length(), 254);
  ASSERT_TRUE(s1->isLargeString());
  EXPECT_EQ(
      LargeString::cast(*s254)->size(),
      Utils::roundUp(kPointerSize + 254, kPointerSize));

  Handle<LargeString> s255(&scope, runtime.newString(255));
  EXPECT_EQ(s255->length(), 255);
  ASSERT_TRUE(s1->isLargeString());
  EXPECT_EQ(
      LargeString::cast(*s255)->size(),
      Utils::roundUp(kPointerSize * 2 + 255, kPointerSize));

  Handle<String> s300(&scope, runtime.newString(300));
  ASSERT_EQ(s300->length(), 300);
}

TEST(RuntimeTest, NewStringWithAll) {
  Runtime runtime;
  HandleScope scope;

  Handle<String> string0(
      &scope, runtime.newStringWithAll(View<byte>(nullptr, 0)));
  EXPECT_EQ(string0->length(), 0);
  EXPECT_TRUE(string0->equalsCString(""));

  const byte bytes3[] = {'A', 'B', 'C'};
  Handle<String> string3(&scope, runtime.newStringWithAll(bytes3));
  EXPECT_EQ(string3->length(), 3);
  EXPECT_TRUE(string3->equalsCString("ABC"));

  const byte bytes10[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'};
  Handle<String> string10(&scope, runtime.newStringWithAll(bytes10));
  EXPECT_EQ(string10->length(), 10);
  EXPECT_TRUE(string10->equalsCString("ABCDEFGHIJ"));
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
  const byte src1[] = {0x1, 0x2, 0x3};
  Handle<ByteArray> arr1(&scope, runtime.newByteArrayWithAll(src1));
  EXPECT_EQ(arr1->header()->hashCode(), 0);
  word hash1 = SmallInteger::cast(runtime.hash(*arr1))->value();
  EXPECT_NE(arr1->header()->hashCode(), 0);
  EXPECT_EQ(arr1->header()->hashCode(), hash1);

  word code1 = runtime.siphash24(src1);
  EXPECT_EQ(code1 & Header::kHashCodeMask, static_cast<uword>(hash1));

  // String with different values should (ideally) hash differently.
  const byte src2[] = {0x3, 0x2, 0x1};
  Handle<ByteArray> arr2(&scope, runtime.newByteArrayWithAll(src2));
  word hash2 = SmallInteger::cast(runtime.hash(*arr2))->value();
  EXPECT_NE(hash1, hash2);

  word code2 = runtime.siphash24(src2);
  EXPECT_EQ(code2 & Header::kHashCodeMask, static_cast<uword>(hash2));

  // Strings with the same value should hash the same.
  const byte src3[] = {0x1, 0x2, 0x3};
  Handle<ByteArray> arr3(&scope, runtime.newByteArrayWithAll(src3));
  word hash3 = SmallInteger::cast(runtime.hash(*arr3))->value();
  EXPECT_EQ(hash1, hash3);

  word code3 = runtime.siphash24(src3);
  EXPECT_EQ(code3 & Header::kHashCodeMask, static_cast<uword>(hash3));
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

  word error_value = reinterpret_cast<word>(Error::object());
  SmallInteger* hash_error = SmallInteger::cast(runtime.hash(Error::object()));
  EXPECT_EQ(hash_error->value(), error_value);
}

TEST(RuntimeTest, HashStrings) {
  Runtime runtime;
  HandleScope scope;

  // Strings have their hash codes computed lazily.
  Handle<LargeString> str1(&scope, runtime.newStringFromCString("testing 123"));
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

TEST(RuntimeTest, InternString) {
  Runtime runtime;
  HandleScope scope;
  Object* ignore;

  Handle<Dictionary> interned(&scope, runtime.interned());

  Handle<Object> str1(&scope, runtime.newStringFromCString("hello, world"));
  EXPECT_FALSE(runtime.dictionaryAt(interned, str1, &ignore));

  Handle<Object> sym1(&scope, runtime.internString(str1));
  EXPECT_TRUE(runtime.dictionaryAt(interned, str1, &ignore));
  EXPECT_EQ(*sym1, *str1);

  Handle<Object> str2(&scope, runtime.newStringFromCString("goodbye, world"));
  EXPECT_NE(*str1, *str2);

  Handle<Object> sym2(&scope, runtime.internString(str2));
  EXPECT_TRUE(runtime.dictionaryAt(interned, str2, &ignore));
  EXPECT_EQ(*sym2, *str2);
  EXPECT_NE(*sym1, *sym2);

  Handle<Object> str3(&scope, runtime.newStringFromCString("hello, world"));
  EXPECT_TRUE(runtime.dictionaryAt(interned, str3, &ignore));

  Handle<Object> sym3(&scope, runtime.internString(str3));
  EXPECT_NE(*sym3, *str3);
  EXPECT_EQ(*sym3, *sym1);
}

TEST(RuntimeTest, CollectAttributes) {
  Runtime runtime;
  HandleScope scope;

  Handle<Object> foo(&scope, runtime.newStringFromCString("foo"));
  Handle<Object> bar(&scope, runtime.newStringFromCString("bar"));
  Handle<Object> baz(&scope, runtime.newStringFromCString("baz"));

  Handle<ObjectArray> names(&scope, runtime.newObjectArray(3));
  names->atPut(0, *foo);
  names->atPut(1, *bar);
  names->atPut(2, *baz);

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(4));
  consts->atPut(0, SmallInteger::fromWord(100));
  consts->atPut(1, SmallInteger::fromWord(200));
  consts->atPut(2, SmallInteger::fromWord(300));
  consts->atPut(3, None::object());

  Handle<Code> code(&scope, runtime.newCode());
  code->setNames(*names);
  // Bytecode for the snippet:
  //
  //   def __init__(self):
  //       self.foo = 100
  //       self.foo = 200
  //
  // The assignment to self.foo is intentionally duplicated to ensure that we
  // only record a single attribute name.
  const byte bc[] = {LOAD_CONST,
                     0,
                     LOAD_FAST,
                     0,
                     STORE_ATTR,
                     0,
                     LOAD_CONST,
                     1,
                     LOAD_FAST,
                     0,
                     STORE_ATTR,
                     0,
                     RETURN_VALUE,
                     0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  Handle<Dictionary> attributes(&scope, runtime.newDictionary());
  runtime.collectAttributes(code, attributes);

  // We should have collected a single attribute: 'foo'
  EXPECT_EQ(attributes->numItems(), 1);

  // Check that we collected 'foo'
  Object* result;
  EXPECT_TRUE(runtime.dictionaryAt(attributes, foo, &result));
  ASSERT_TRUE(result->isString());
  EXPECT_TRUE(String::cast(result)->equals(*foo));

  // Bytecode for the snippet:
  //
  //   def __init__(self):
  //       self.bar = 200
  //       self.baz = 300
  const byte bc2[] = {LOAD_CONST,
                      1,
                      LOAD_FAST,
                      0,
                      STORE_ATTR,
                      1,
                      LOAD_CONST,
                      2,
                      LOAD_FAST,
                      0,
                      STORE_ATTR,
                      2,
                      RETURN_VALUE,
                      0};
  code->setCode(runtime.newByteArrayWithAll(bc2));
  runtime.collectAttributes(code, attributes);

  // We should have collected a two more attributes: 'bar' and 'baz'
  EXPECT_EQ(attributes->numItems(), 3);

  // Check that we collected 'bar'
  EXPECT_TRUE(runtime.dictionaryAt(attributes, bar, &result));
  ASSERT_TRUE(result->isString());
  EXPECT_TRUE(String::cast(result)->equals(*bar));

  // Check that we collected 'baz'
  EXPECT_TRUE(runtime.dictionaryAt(attributes, baz, &result));
  ASSERT_TRUE(result->isString());
  EXPECT_TRUE(String::cast(result)->equals(*baz));
}

TEST(RuntimeTest, CollectAttributesWithExtendedArg) {
  Runtime runtime;
  HandleScope scope;

  Handle<Object> foo(&scope, runtime.newStringFromCString("foo"));
  Handle<Object> bar(&scope, runtime.newStringFromCString("bar"));

  Handle<ObjectArray> names(&scope, runtime.newObjectArray(2));
  names->atPut(0, *foo);
  names->atPut(1, *bar);

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(1));
  consts->atPut(0, None::object());

  Handle<Code> code(&scope, runtime.newCode());
  code->setNames(*names);
  // Bytecode for the snippet:
  //
  //   def __init__(self):
  //       self.foo = None
  //
  // There is an additional LOAD_FAST that is preceded by an EXTENDED_ARG
  // that must be skipped.
  const byte bc[] = {LOAD_CONST,
                     0,
                     EXTENDED_ARG,
                     10,
                     LOAD_FAST,
                     0,
                     STORE_ATTR,
                     1,
                     LOAD_CONST,
                     0,
                     LOAD_FAST,
                     0,
                     STORE_ATTR,
                     0,
                     RETURN_VALUE,
                     0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  Handle<Dictionary> attributes(&scope, runtime.newDictionary());
  runtime.collectAttributes(code, attributes);

  // We should have collected a single attribute: 'foo'
  EXPECT_EQ(attributes->numItems(), 1);

  // Check that we collected 'foo'
  Object* result;
  EXPECT_TRUE(runtime.dictionaryAt(attributes, foo, &result));
  ASSERT_TRUE(result->isString());
  EXPECT_TRUE(String::cast(result)->equals(*foo));
}

TEST(RuntimeTest, GetClassConstructor) {
  Runtime runtime;
  HandleScope scope;
  Handle<Class> klass(&scope, runtime.newClass());
  Handle<Dictionary> klass_dict(&scope, runtime.newDictionary());
  klass->setDictionary(*klass_dict);

  EXPECT_EQ(runtime.classConstructor(klass), None::object());

  Handle<Object> init(&scope, runtime.newStringFromCString("__init__"));
  Handle<ValueCell> value_cell(
      &scope,
      runtime.dictionaryAtIfAbsentPut(
          klass_dict, init, runtime.newValueCellCallback()));
  Handle<Function> func(&scope, runtime.newFunction());
  value_cell->setValue(*func);

  EXPECT_EQ(runtime.classConstructor(klass), *func);
}

TEST(RuntimeTest, ComputeInstanceSize) {
  Runtime runtime;
  // Template bytecode for:
  //
  //   def __init__(self):
  //       self.<name0> = None
  //       self.<name1> = None
  const byte bc[] = {LOAD_CONST,
                     0,
                     LOAD_FAST,
                     0,
                     STORE_ATTR,
                     0,
                     LOAD_CONST,
                     0,
                     LOAD_FAST,
                     0,
                     STORE_ATTR,
                     1,
                     RETURN_VALUE,
                     0};

  // Creates a new class with a constructor that contains the bytecode
  // defined above.
  auto createClass = [&](const char* name0, const char* name1) {
    HandleScope scope;

    Handle<Object> attr0(&scope, runtime.newStringFromCString(name0));
    Handle<Object> attr1(&scope, runtime.newStringFromCString(name1));

    Handle<ObjectArray> names(&scope, runtime.newObjectArray(2));
    names->atPut(0, *attr0);
    names->atPut(1, *attr1);

    Handle<ObjectArray> consts(&scope, runtime.newObjectArray(1));
    consts->atPut(0, None::object());

    Handle<Code> code(&scope, runtime.newCode());
    code->setNames(*names);
    code->setConsts(*consts);
    code->setCode(runtime.newByteArrayWithAll(bc));

    Handle<Function> func(&scope, runtime.newFunction());
    func->setCode(*code);

    Handle<Dictionary> klass_dict(&scope, runtime.newDictionary());
    Handle<Object> init(&scope, runtime.newStringFromCString("__init__"));
    Handle<ValueCell> value_cell(
        &scope,
        runtime.dictionaryAtIfAbsentPut(
            klass_dict, init, runtime.newValueCellCallback()));
    value_cell->setValue(*func);

    Handle<Class> klass(&scope, runtime.newClass());
    klass->setDictionary(*klass_dict);

    Handle<ObjectArray> mro(&scope, runtime.newObjectArray(1));
    mro->atPut(0, *klass);
    klass->setMro(*mro);

    return *klass;
  };

  // Create the following classes:
  //
  // class A:
  //   def __init__(self):
  //     self.attr0 = None
  //     self.attr1 = None
  //
  // class B
  //   def __init__(self):
  //     self.attr0 = None
  //     self.attr2 = None
  //
  // class C(A, B):
  //   def __init__(self):
  //     self.attr3 = None
  //     self.attr4 = None
  HandleScope scope;
  Handle<Class> klass_a(&scope, createClass("attr0", "attr1"));
  EXPECT_EQ(runtime.computeInstanceSize(klass_a), 2);

  Handle<Class> klass_b(&scope, createClass("attr0", "attr2"));
  EXPECT_EQ(runtime.computeInstanceSize(klass_b), 2);

  Handle<Class> klass_c(&scope, createClass("attr3", "attr4"));
  Handle<ObjectArray> mro(&scope, runtime.newObjectArray(3));
  mro->atPut(0, *klass_c);
  mro->atPut(1, *klass_a);
  mro->atPut(2, *klass_b);
  klass_c->setMro(*mro);
  // Both A and B have "attr0" which should only be counted once
  EXPECT_EQ(runtime.computeInstanceSize(klass_c), 5);
}

} // namespace python
