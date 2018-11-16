#include "gtest/gtest.h"

#include <memory>

#include "bytecode.h"
#include "runtime.h"
#include "symbols.h"
#include "test-utils.h"

namespace python {
using namespace testing;

TEST(RuntimeTest, CollectGarbage) {
  Runtime runtime;
  ASSERT_TRUE(runtime.heap()->verify());
  runtime.collectGarbage();
  ASSERT_TRUE(runtime.heap()->verify());
}

TEST(RuntimeTest, AllocateAndCollectGarbage) {
  const word heap_size = 32 * MiB;
  const word array_length = 1024;
  const word allocation_size = ByteArray::allocationSize(array_length);
  const word total_allocation_size = heap_size * 10;
  Runtime runtime(heap_size);
  ASSERT_TRUE(runtime.heap()->verify());
  for (word i = 0; i < total_allocation_size; i += allocation_size) {
    runtime.newByteArray(array_length, 0);
  }
  ASSERT_TRUE(runtime.heap()->verify());
}

TEST(RuntimeTest, BuiltinsModuleExists) {
  Runtime runtime;
  HandleScope scope;

  Handle<Dictionary> modules(&scope, runtime.modules());
  Handle<Object> name(&scope, runtime.newStringFromCString("builtins"));
  ASSERT_TRUE(runtime.dictionaryAt(modules, name)->isModule());
}

TEST(RuntimeTest, NewByteArray) {
  Runtime runtime;
  HandleScope scope;

  Handle<ByteArray> len0(&scope, runtime.newByteArray(0, 0));
  EXPECT_EQ(len0->length(), 0);

  Handle<ByteArray> len3(&scope, runtime.newByteArray(3, 9));
  EXPECT_EQ(len3->length(), 3);
  EXPECT_EQ(len3->byteAt(0), 9);
  EXPECT_EQ(len3->byteAt(1), 9);
  EXPECT_EQ(len3->byteAt(2), 9);

  Handle<ByteArray> len254(&scope, runtime.newByteArray(254, 0));
  EXPECT_EQ(len254->length(), 254);
  EXPECT_EQ(len254->size(), Utils::roundUp(kPointerSize + 254, kPointerSize));

  Handle<ByteArray> len255(&scope, runtime.newByteArray(255, 0));
  EXPECT_EQ(len255->length(), 255);
  EXPECT_EQ(
      len255->size(), Utils::roundUp(kPointerSize * 2 + 255, kPointerSize));
}

TEST(RuntimeTest, NewByteArrayWithAll) {
  Runtime runtime;
  HandleScope scope;

  Handle<ByteArray> len0(
      &scope, runtime.newByteArrayWithAll(View<byte>(nullptr, 0)));
  EXPECT_EQ(len0->length(), 0);

  const byte src1[] = {0x42};
  Handle<ByteArray> len1(&scope, runtime.newByteArrayWithAll(src1));
  EXPECT_EQ(len1->length(), 1);
  EXPECT_EQ(len1->size(), Utils::roundUp(kPointerSize + 1, kPointerSize));
  EXPECT_EQ(len1->byteAt(0), 0x42);

  const byte src3[] = {0xAA, 0xBB, 0xCC};
  Handle<ByteArray> len3(&scope, runtime.newByteArrayWithAll(src3));
  EXPECT_EQ(len3->length(), 3);
  EXPECT_EQ(len3->size(), Utils::roundUp(kPointerSize + 3, kPointerSize));
  EXPECT_EQ(len3->byteAt(0), 0xAA);
  EXPECT_EQ(len3->byteAt(1), 0xBB);
  EXPECT_EQ(len3->byteAt(2), 0xCC);
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
  const byte bytes[400]{0};
  Handle<String> empty0(&scope, runtime.newStringWithAll(View<byte>(bytes, 0)));
  ASSERT_TRUE(empty0->isSmallString());
  EXPECT_EQ(empty0->length(), 0);

  Handle<String> empty1(&scope, runtime.newStringWithAll(View<byte>(bytes, 0)));
  ASSERT_TRUE(empty1->isSmallString());
  EXPECT_EQ(*empty0, *empty1);

  Handle<String> empty2(&scope, runtime.newStringFromCString("\0"));
  ASSERT_TRUE(empty2->isSmallString());
  EXPECT_EQ(*empty0, *empty2);

  Handle<String> s1(&scope, runtime.newStringWithAll(View<byte>(bytes, 1)));
  ASSERT_TRUE(s1->isSmallString());
  EXPECT_EQ(s1->length(), 1);

  Handle<String> s254(&scope, runtime.newStringWithAll(View<byte>(bytes, 254)));
  EXPECT_EQ(s254->length(), 254);
  ASSERT_TRUE(s254->isLargeString());
  EXPECT_EQ(
      HeapObject::cast(*s254)->size(),
      Utils::roundUp(kPointerSize + 254, kPointerSize));

  Handle<String> s255(&scope, runtime.newStringWithAll(View<byte>(bytes, 255)));
  EXPECT_EQ(s255->length(), 255);
  ASSERT_TRUE(s255->isLargeString());
  EXPECT_EQ(
      HeapObject::cast(*s255)->size(),
      Utils::roundUp(kPointerSize * 2 + 255, kPointerSize));

  Handle<String> s300(&scope, runtime.newStringWithAll(View<byte>(bytes, 300)));
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

  // LargeStrings have their hash codes computed lazily.
  Handle<Object> str1(&scope, runtime.newStringFromCString("testing 123"));
  EXPECT_EQ(HeapObject::cast(*str1)->header()->hashCode(), 0);
  SmallInteger* hash1 = SmallInteger::cast(runtime.hash(*str1));
  EXPECT_NE(HeapObject::cast(*str1)->header()->hashCode(), 0);
  EXPECT_EQ(HeapObject::cast(*str1)->header()->hashCode(), hash1->value());

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

TEST(RuntimeTest, HashCodeSizeCheck) {
  Runtime runtime;
  Object* code = runtime.newCode();
  ASSERT_TRUE(code->isHeapObject());
  EXPECT_EQ(HeapObject::cast(code)->header()->hashCode(), 0);
  // Verify that large-magnitude random numbers are properly
  // truncated to somethat which fits in a SmallInteger

  // Conspire based on knoledge of the random number genrated to
  // create a high-magnitude result from Runtime::random
  // which is truncated to 0 for storage in the header and
  // replaced with "1" so no hash code has value 0.
  uword high = static_cast<uword>(1) << (8 * sizeof(uword) - 1);
  uword state[2] = {0, high};
  uword secret[2] = {0, 0};
  runtime.seedRandom(state, secret);
  uword first = runtime.random();
  EXPECT_EQ(first, high);
  runtime.seedRandom(state, secret);
  word hash1 = SmallInteger::cast(runtime.hash(code))->value();
  EXPECT_EQ(hash1, 1);
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

TEST(RuntimeTest, InternLargeString) {
  Runtime runtime;
  HandleScope scope;

  Handle<Set> interned(&scope, runtime.interned());

  // Creating an ordinary large string should not affect on the intern table.
  word num_interned_strings = interned->numItems();
  Handle<Object> str1(&scope, runtime.newStringFromCString("hello, world"));
  ASSERT_TRUE(str1->isLargeString());
  EXPECT_EQ(num_interned_strings, interned->numItems());
  EXPECT_FALSE(runtime.setIncludes(interned, str1));

  // Interning the string should add it to the intern table and increase the
  // size of the intern table by one.
  num_interned_strings = interned->numItems();
  Handle<Object> sym1(&scope, runtime.internString(str1));
  EXPECT_TRUE(runtime.setIncludes(interned, str1));
  EXPECT_EQ(*sym1, *str1);
  EXPECT_EQ(num_interned_strings + 1, interned->numItems());

  Handle<Object> str2(&scope, runtime.newStringFromCString("goodbye, world"));
  ASSERT_TRUE(str2->isLargeString());
  EXPECT_NE(*str1, *str2);

  // Intern another string and make sure we get it back (as opposed to the
  // previously interned string).
  num_interned_strings = interned->numItems();
  Handle<Object> sym2(&scope, runtime.internString(str2));
  EXPECT_EQ(num_interned_strings + 1, interned->numItems());
  EXPECT_TRUE(runtime.setIncludes(interned, str2));
  EXPECT_EQ(*sym2, *str2);
  EXPECT_NE(*sym1, *sym2);

  // Create a unique copy of a previously created string.
  Handle<Object> str3(&scope, runtime.newStringFromCString("hello, world"));
  ASSERT_TRUE(str3->isLargeString());
  EXPECT_NE(*str1, *str3);
  EXPECT_TRUE(runtime.setIncludes(interned, str3));

  // Interning a duplicate string should not affecct the intern table.
  num_interned_strings = interned->numItems();
  Handle<Object> sym3(&scope, runtime.internString(str3));
  EXPECT_EQ(num_interned_strings, interned->numItems());
  EXPECT_NE(*sym3, *str3);
  EXPECT_EQ(*sym3, *sym1);
}

TEST(RuntimeTest, InternSmallString) {
  Runtime runtime;
  HandleScope scope;

  Handle<Set> interned(&scope, runtime.interned());

  // Creating a small string should not affect the intern table.
  word num_interned_strings = interned->numItems();
  Handle<Object> str(&scope, runtime.newStringFromCString("a"));
  ASSERT_TRUE(str->isSmallString());
  EXPECT_FALSE(runtime.setIncludes(interned, str));
  EXPECT_EQ(num_interned_strings, interned->numItems());

  // Interning a small string should have no affect on the intern table.
  Handle<Object> sym(&scope, runtime.internString(str));
  EXPECT_TRUE(sym->isSmallString());
  EXPECT_FALSE(runtime.setIncludes(interned, str));
  EXPECT_EQ(num_interned_strings, interned->numItems());
  EXPECT_EQ(*sym, *str);
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
  Handle<Object> result(&scope, runtime.dictionaryAt(attributes, foo));
  ASSERT_TRUE(result->isString());
  EXPECT_TRUE(String::cast(*result)->equals(*foo));

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
  result = runtime.dictionaryAt(attributes, bar);
  ASSERT_TRUE(result->isString());
  EXPECT_TRUE(String::cast(*result)->equals(*bar));

  // Check that we collected 'baz'
  result = runtime.dictionaryAt(attributes, baz);
  ASSERT_TRUE(result->isString());
  EXPECT_TRUE(String::cast(*result)->equals(*baz));
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
  Handle<Object> result(&scope, runtime.dictionaryAt(attributes, foo));
  ASSERT_TRUE(result->isString());
  EXPECT_TRUE(String::cast(*result)->equals(*foo));
}

TEST(RuntimeTest, GetClassConstructor) {
  Runtime runtime;
  HandleScope scope;
  Handle<Class> klass(&scope, runtime.newClass());
  Handle<Dictionary> klass_dict(&scope, runtime.newDictionary());
  klass->setDictionary(*klass_dict);

  EXPECT_EQ(runtime.classConstructor(klass), None::object());

  Handle<Object> init(&scope, runtime.symbols()->DunderInit());
  Handle<Object> func(&scope, runtime.newFunction());
  runtime.dictionaryAtPutInValueCell(klass_dict, init, func);

  EXPECT_EQ(runtime.classConstructor(klass), *func);
}

TEST(RuntimeTest, NewInstanceEmptyClass) {
  Runtime runtime;
  HandleScope scope;

  const char* src = "class MyEmptyClass: pass";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  runtime.run(buffer.get());

  word layout_id = IntrinsicLayoutId::kLastId + 1;
  Handle<Layout> layout(&scope, runtime.layoutAt(layout_id));
  EXPECT_EQ(layout->instanceSize(), 1);

  Handle<Class> cls(&scope, layout->describedClass());
  EXPECT_TRUE(String::cast(cls->name())->equalsCString("MyEmptyClass"));

  Handle<Instance> instance(&scope, runtime.newInstance(layout));
  EXPECT_TRUE(instance->isInstance());
  EXPECT_TRUE(instance->header()->layoutId() == layout_id);
}

TEST(RuntimeTest, NewInstanceManyAttributes) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class MyClassWithAttributes():
  def __init__(self):
    self.a = 1
    self.b = 2
    self.c = 3
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  runtime.run(buffer.get());

  word layout_id = IntrinsicLayoutId::kLastId + 1;
  Handle<Layout> layout(&scope, runtime.layoutAt(layout_id));
  ASSERT_EQ(layout->instanceSize(), 4);

  Handle<Class> cls(&scope, layout->describedClass());
  ASSERT_TRUE(
      String::cast(cls->name())->equalsCString("MyClassWithAttributes"));

  Handle<Instance> instance(&scope, runtime.newInstance(layout));
  EXPECT_TRUE(instance->isInstance());
  EXPECT_EQ(instance->header()->layoutId(), layout_id);
}

TEST(RuntimeTest, VerifySymbols) {
  Runtime runtime;
  Symbols* symbols = runtime.symbols();
  for (word i = 0; i < Symbols::kMaxSymbolId; i++) {
    Symbols::SymbolId id = static_cast<Symbols::SymbolId>(i);
    Object* value = symbols->at(id);
    ASSERT_TRUE(value->isString());
    const char* expected = symbols->literalAt(id);
    EXPECT_TRUE(String::cast(value)->equalsCString(expected))
        << "Incorrect symbol value for " << expected;
  }
}

static String* className(Runtime* runtime, Object* o) {
  auto cls = Class::cast(runtime->classOf(o));
  auto name = String::cast(cls->name());
  return name;
}

TEST(RuntimeTest, ClassIds) {
  Runtime runtime;
  HandleScope scope;

  EXPECT_PYSTRING_EQ(className(&runtime, Boolean::fromBool(true)), "bool");
  EXPECT_PYSTRING_EQ(className(&runtime, None::object()), "NoneType");
  EXPECT_PYSTRING_EQ(
      className(&runtime, runtime.newStringFromCString("abc")), "smallstr");

  for (word i = 0; i < 16; i++) {
    auto small_int = SmallInteger::fromWord(i);
    EXPECT_PYSTRING_EQ(className(&runtime, small_int), "smallint");
  }
}

TEST(RuntimeTest, StringConcat) {
  Runtime runtime;
  HandleScope scope;

  Handle<String> str1(&scope, runtime.newStringFromCString("abc"));
  Handle<String> str2(&scope, runtime.newStringFromCString("def"));

  // Large strings.
  Handle<String> str3(&scope, runtime.newStringFromCString("0123456789abcdef"));
  Handle<String> str4(&scope, runtime.newStringFromCString("fedbca9876543210"));

  Handle<String> concat12(&scope, runtime.stringConcat(str1, str2));
  Handle<String> concat34(&scope, runtime.stringConcat(str3, str4));

  Handle<String> concat13(&scope, runtime.stringConcat(str1, str3));
  Handle<String> concat31(&scope, runtime.stringConcat(str3, str1));

  // Test that we don't make large strings when small srings would suffice.
  EXPECT_PYSTRING_EQ(*concat12, "abcdef");
  EXPECT_PYSTRING_EQ(*concat34, "0123456789abcdeffedbca9876543210");
  EXPECT_PYSTRING_EQ(*concat13, "abc0123456789abcdef");
  EXPECT_PYSTRING_EQ(*concat31, "0123456789abcdefabc");

  EXPECT_TRUE(concat12->isSmallString());
  EXPECT_TRUE(concat34->isLargeString());
  EXPECT_TRUE(concat13->isLargeString());
  EXPECT_TRUE(concat31->isLargeString());
}

struct LookupNameInMroData {
  const char* test_name;
  const char* name;
  Object* expected;
};

static std::string lookupNameInMroTestName(
    ::testing::TestParamInfo<LookupNameInMroData> info) {
  return info.param.test_name;
}

class LookupNameInMroTest
    : public ::testing::TestWithParam<LookupNameInMroData> {};

TEST_P(LookupNameInMroTest, Lookup) {
  Runtime runtime;
  HandleScope scope;

  auto createClassWithAttr = [&](const char* attr, word value) {
    Handle<Class> klass(&scope, runtime.newClass());
    Handle<Dictionary> dict(&scope, klass->dictionary());
    Handle<Object> key(&scope, runtime.newStringFromCString(attr));
    Handle<Object> val(&scope, SmallInteger::fromWord(value));
    runtime.dictionaryAtPutInValueCell(dict, key, val);
    return *klass;
  };

  Handle<ObjectArray> mro(&scope, runtime.newObjectArray(3));
  mro->atPut(0, createClassWithAttr("foo", 2));
  mro->atPut(1, createClassWithAttr("bar", 4));
  mro->atPut(2, createClassWithAttr("baz", 8));

  Handle<Class> klass(&scope, mro->at(0));
  klass->setMro(*mro);

  auto param = GetParam();
  Handle<Object> key(&scope, runtime.newStringFromCString(param.name));
  Object* result = runtime.lookupNameInMro(Thread::currentThread(), klass, key);
  EXPECT_EQ(result, param.expected);
}

INSTANTIATE_TEST_CASE_P(
    LookupNameInMro,
    LookupNameInMroTest,
    ::testing::Values(
        LookupNameInMroData{"OnInstance", "foo", SmallInteger::fromWord(2)},
        LookupNameInMroData{"OnParent", "bar", SmallInteger::fromWord(4)},
        LookupNameInMroData{"OnGrandParent", "baz", SmallInteger::fromWord(8)},
        LookupNameInMroData{"NonExistent", "xxx", Error::object()}),
    lookupNameInMroTestName);

TEST(RuntimeTest, TypeCallNoInitMethod) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class MyClassWithNoInitMethod():
  def m(self):
    pass

c = MyClassWithNoInitMethod()
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  runtime.run(buffer.get());

  word layout_id = IntrinsicLayoutId::kLastId + 1;
  Handle<Layout> layout(&scope, runtime.layoutAt(layout_id));
  EXPECT_EQ(layout->instanceSize(), 1);

  Handle<Class> cls(&scope, layout->describedClass());
  EXPECT_PYSTRING_EQ(String::cast(cls->name()), "MyClassWithNoInitMethod");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> key(&scope, runtime.newStringFromCString("c"));
  Handle<Object> instance(&scope, runtime.moduleAt(main, key));
  ASSERT_TRUE(instance->isInstance());
  EXPECT_EQ(instance->layoutId(), layout_id);
}

TEST(RuntimeTest, TypeCallEmptyInitMethod) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class MyClassWithEmptyInitMethod():
  def __init__(self):
    pass
  def m(self):
    pass

c = MyClassWithEmptyInitMethod()
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  runtime.run(buffer.get());

  word layout_id = IntrinsicLayoutId::kLastId + 1;
  Handle<Layout> layout(&scope, runtime.layoutAt(layout_id));
  EXPECT_EQ(layout->instanceSize(), 1);

  Handle<Class> cls(&scope, layout->describedClass());
  EXPECT_PYSTRING_EQ(String::cast(cls->name()), "MyClassWithEmptyInitMethod");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> key(&scope, runtime.newStringFromCString("c"));
  Handle<Object> instance(&scope, runtime.moduleAt(main, key));
  ASSERT_TRUE(instance->isInstance());
  EXPECT_EQ(instance->layoutId(), layout_id);
}

TEST(RuntimeTest, TypeCallWithArguments) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class MyClassWithAttributes():
  def __init__(self, x):
    self.x = x
  def m(self):
    pass

c = MyClassWithAttributes(1)
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  runtime.run(buffer.get());

  word layout_id = IntrinsicLayoutId::kLastId + 1;
  Handle<Layout> layout(&scope, runtime.layoutAt(layout_id));
  ASSERT_EQ(layout->instanceSize(), 2);

  Handle<Class> cls(&scope, layout->describedClass());
  EXPECT_PYSTRING_EQ(String::cast(cls->name()), "MyClassWithAttributes");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> key(&scope, runtime.newStringFromCString("c"));
  Handle<Object> instance(&scope, runtime.moduleAt(main, key));
  ASSERT_TRUE(instance->isInstance());
  ASSERT_EQ(instance->layoutId(), layout_id);

  Handle<Object> name(&scope, runtime.newStringFromCString("x"));
  Handle<Object> value(
      &scope, runtime.attributeAt(Thread::currentThread(), instance, name));
  EXPECT_FALSE(value->isError());
  EXPECT_EQ(*value, SmallInteger::fromWord(1));
}

TEST(RuntimeTest, ComputeLineNumberForBytecodeOffset) {
  Runtime runtime;
  const char* src = R"(
def func():
  a = 1
  b = 2
  print(a, b)
)";
  compileAndRunToString(&runtime, src);
  HandleScope scope;
  Handle<Object> dunder_main(&scope, runtime.symbols()->DunderMain());
  Handle<Module> main(&scope, runtime.findModule(dunder_main));

  // The bytecode for func is roughly:
  // LOAD_CONST     # a = 1
  // STORE_FAST
  //
  // LOAD_CONST     # b = 2
  // STORE_FAST
  //
  // LOAD_GLOBAL    # print(a, b)
  // LOAD_FAST
  // LOAD_FAST
  // CALL_FUNCTION

  Handle<Object> name(&scope, runtime.newStringFromCString("func"));
  Handle<Function> func(&scope, runtime.moduleAt(main, name));
  Handle<Code> code(&scope, func->code());
  ASSERT_EQ(code->firstlineno(), 2);

  // a = 1
  Thread* thread = Thread::currentThread();
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 0), 3);
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 2), 3);

  // b = 2
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 4), 4);
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 6), 4);

  // print(a, b)
  for (word i = 8; i < ByteArray::cast(code->code())->length(); i++) {
    EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, i), 5);
  }
}

} // namespace python
