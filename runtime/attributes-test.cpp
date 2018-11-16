#include "gtest/gtest.h"

#include <initializer_list>
#include <map>
#include <string>

#include "runtime.h"
#include "test-utils.h"

namespace python {
using namespace testing;

static Object* createClass(
    Runtime* runtime,
    std::initializer_list<ClassId> mro_tail) {
  HandleScope scope;
  Handle<Class> klass(&scope, runtime->newClass());
  Handle<ObjectArray> mro(&scope, runtime->newObjectArray(mro_tail.size() + 1));
  mro->atPut(0, *klass);
  word idx = 1;
  for (ClassId class_id : mro_tail) {
    mro->atPut(idx, runtime->classAt(class_id));
    idx++;
  }
  klass->setMro(*mro);
  return *klass;
}

static void setInClassDict(
    Runtime* runtime,
    const Handle<Object>& klass,
    const Handle<Object>& attr,
    const Handle<Object>& value) {
  HandleScope scope;
  Handle<Class> k(&scope, *klass);
  Handle<Dictionary> klass_dict(&scope, k->dictionary());
  runtime->dictionaryAtPutInValueCell(klass_dict, attr, value);
}

static void setInMetaclass(
    Runtime* runtime,
    const Handle<Object>& klass,
    const Handle<Object>& attr,
    const Handle<Object>& value) {
  HandleScope scope;
  Handle<Object> meta_klass(&scope, runtime->classOf(*klass));
  setInClassDict(runtime, meta_klass, attr, value);
}

// Get an attribute that corresponds to a function on the metaclass
TEST(ClassGetAttrTest, MetaClassFunction) {
  Runtime runtime;
  HandleScope scope;
  Handle<Object> klass(&scope, createClass(&runtime, {}));

  // Store the function on the metaclass
  Handle<Object> attr(&scope, runtime.newStringFromCString("test"));
  Handle<Object> value(&scope, runtime.newFunction());
  setInMetaclass(&runtime, klass, attr, value);

  // Fetch it from the class and ensure the bound method was created
  Object* result = runtime.attributeAt(Thread::currentThread(), klass, attr);
  ASSERT_TRUE(result->isBoundMethod());
  Handle<BoundMethod> bm(&scope, result);
  EXPECT_TRUE(Object::equals(bm->function(), *value));
  EXPECT_TRUE(Object::equals(bm->self(), *klass));
}

// Get an attribute that resides on the metaclass
TEST(ClassGetAttrTest, MetaClassAttr) {
  Runtime runtime;
  HandleScope scope;
  Handle<Object> klass(&scope, createClass(&runtime, {}));

  // Store the attribute on the metaclass
  Handle<Object> attr(&scope, runtime.newStringFromCString("test"));
  Handle<Object> value(&scope, SmallInteger::fromWord(100));
  setInMetaclass(&runtime, klass, attr, value);

  // Fetch it from the class
  Object* result = runtime.attributeAt(Thread::currentThread(), klass, attr);
  EXPECT_TRUE(Object::equals(result, *value));
}

// Get an attribute that resides on the class and shadows an attribute on
// the metaclass
TEST(ClassGetAttrTest, ShadowingAttr) {
  Runtime runtime;
  HandleScope scope;
  Handle<Object> klass(&scope, createClass(&runtime, {}));

  // Store the attribute on the metaclass
  Handle<Object> attr(&scope, runtime.newStringFromCString("test"));
  Handle<Object> meta_klass_value(&scope, SmallInteger::fromWord(100));
  setInMetaclass(&runtime, klass, attr, meta_klass_value);

  // Store the attribute on the class so that it shadows the attr
  // on the metaclass
  Handle<Object> klass_value(&scope, SmallInteger::fromWord(200));
  setInClassDict(&runtime, klass, attr, klass_value);

  // Fetch it from the class
  Object* result = runtime.attributeAt(Thread::currentThread(), klass, attr);
  EXPECT_TRUE(Object::equals(result, *klass_value));
}

struct IntrinsicClassSetAttrTestData {
  ClassId class_id;
  const char* name;
};

IntrinsicClassSetAttrTestData kIntrinsicClassSetAttrTests[] = {
// clang-format off
#define DEFINE_TEST(class_name) {ClassId::k##class_name, #class_name},
  INTRINSIC_CLASS_NAMES(DEFINE_TEST)
#undef DEFINE_TEST
    // clang-format on
};

static std::string intrinsicClassName(
    ::testing::TestParamInfo<IntrinsicClassSetAttrTestData> info) {
  return info.param.name;
}

class IntrinsicClassSetAttrTest
    : public ::testing::TestWithParam<IntrinsicClassSetAttrTestData> {};

TEST_P(IntrinsicClassSetAttrTest, SetAttr) {
  Runtime runtime;
  HandleScope scope;
  Handle<Object> klass(&scope, runtime.classAt(GetParam().class_id));
  Handle<Object> attr(&scope, runtime.newStringFromCString("test"));
  Handle<Object> value(&scope, SmallInteger::fromWord(100));
  Thread* thread = Thread::currentThread();

  Object* result = runtime.attributeAtPut(thread, klass, attr, value);

  EXPECT_TRUE(result->isError());
  ASSERT_TRUE(thread->pendingException()->isString());
  EXPECT_TRUE(
      String::cast(thread->pendingException())
          ->equalsCString("can't set attributes of built-in/extension type"));
}

INSTANTIATE_TEST_CASE_P(
    IntrinsicClasses,
    IntrinsicClassSetAttrTest,
    ::testing::ValuesIn(kIntrinsicClassSetAttrTests),
    intrinsicClassName);

// Set an attribute directly on the class
TEST(ClassAttributeTest, SetAttrOnClass) {
  Runtime runtime;
  HandleScope scope;

  Handle<Object> klass(&scope, runtime.newClass());
  Handle<Object> attr(&scope, runtime.newStringFromCString("test"));
  Handle<Object> value(&scope, SmallInteger::fromWord(100));

  Object* result =
      runtime.attributeAtPut(Thread::currentThread(), klass, attr, value);
  ASSERT_FALSE(result->isError());

  Handle<Dictionary> klass_dict(&scope, Class::cast(*klass)->dictionary());
  Handle<Object> value_cell(&scope, None::object());
  bool found = runtime.dictionaryAt(klass_dict, attr, value_cell.pointer());
  ASSERT_TRUE(found);
  EXPECT_EQ(ValueCell::cast(*value_cell)->value(), SmallInteger::fromWord(100));
}

TEST(ClassAttributeTest, Simple) {
  Runtime runtime;
  const char* src = R"(
class A:
  foo = 'hello'
print(A.foo)
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello\n");
}

TEST(ClassAttributeTest, SingleInheritance) {
  Runtime runtime;
  const char* src = R"(
class A:
  foo = 'hello'
class B(A): pass
class C(B): pass
print(A.foo, B.foo, C.foo)
B.foo = 123
print(A.foo, B.foo, C.foo)
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello hello hello\nhello 123 123\n");
}

TEST(ClassAttributeTest, MultipleInheritance) {
  Runtime runtime;
  const char* src = R"(
class A:
  foo = 'hello'
class B:
  bar = 'there'
class C(B, A): pass
print(C.foo, C.bar)
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello there\n");
}

TEST(ClassAttributeTest, GetMissingAttribute) {
  Runtime runtime;
  const char* src = R"(
class A: pass
print(A.foo)
)";
  ASSERT_DEATH(
      compileAndRunToString(&runtime, src),
      "aborting due to pending exception: missing attribute");
}

TEST(ClassAttributeTest, GetFunction) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  def bar(self):
    print(self)
Foo.bar('testing 123')
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "testing 123\n");
}

TEST(ClassAttributeTest, GetDataDescriptorOnMetaClass) {
  Runtime runtime;

  // Create the data descriptor class
  const char* src = R"(
class DataDescriptor:
  def __set__(self, instance, value):
    pass

  def __get__(self, instance, owner):
    pass
)";
  compileAndRunToString(&runtime, src);
  HandleScope scope;
  Handle<Module> main(&scope, runtime.findModule("__main__"));
  Handle<Class> descr_klass(
      &scope, findInModule(&runtime, main, "DataDescriptor"));

  // Create the class
  Handle<Object> klass(&scope, createClass(&runtime, {}));

  // Create an instance of the descriptor and store it on the metaclass
  Handle<Object> attr(&scope, runtime.newStringFromCString("test"));
  Handle<Object> descr(&scope, runtime.newInstance(descr_klass->id()));
  setInMetaclass(&runtime, klass, attr, descr);

  ASSERT_DEATH(
      runtime.attributeAt(Thread::currentThread(), klass, attr),
      "custom descriptors are unsupported");
}

TEST(ClassAttributeTest, GetNonDataDescriptorOnMetaClass) {
  Runtime runtime;

  // Create the non-data descriptor class
  const char* src = R"(
class DataDescriptor:
  def __get__(self, instance, owner):
    pass
)";
  compileAndRunToString(&runtime, src);
  HandleScope scope;
  Handle<Module> main(&scope, runtime.findModule("__main__"));
  Handle<Class> descr_klass(
      &scope, findInModule(&runtime, main, "DataDescriptor"));

  // Create the class
  Handle<Object> klass(&scope, createClass(&runtime, {}));

  // Create an instance of the descriptor and store it on the metaclass
  Handle<Object> attr(&scope, runtime.newStringFromCString("test"));
  Handle<Object> descr(&scope, runtime.newInstance(descr_klass->id()));
  setInMetaclass(&runtime, klass, attr, descr);

  ASSERT_DEATH(
      runtime.attributeAt(Thread::currentThread(), klass, attr),
      "custom descriptors are unsupported");
}

TEST(ClassAttributeTest, GetNonDataDescriptorOnClass) {
  Runtime runtime;

  // Create the non-data descriptor class
  const char* src = R"(
class DataDescriptor:
  def __get__(self, instance, owner):
    pass
)";
  compileAndRunToString(&runtime, src);
  HandleScope scope;
  Handle<Module> main(&scope, runtime.findModule("__main__"));
  Handle<Class> descr_klass(
      &scope, findInModule(&runtime, main, "DataDescriptor"));

  // Create the class
  Handle<Object> klass(&scope, createClass(&runtime, {}));

  // Create an instance of the descriptor and store it on the metaclass
  Handle<Object> attr(&scope, runtime.newStringFromCString("test"));
  Handle<Object> descr(&scope, runtime.newInstance(descr_klass->id()));
  setInClassDict(&runtime, klass, attr, descr);

  ASSERT_DEATH(
      runtime.attributeAt(Thread::currentThread(), klass, attr),
      "custom descriptors are unsupported");
}

// Fetch an unknown attribute
TEST(InstanceAttributeTest, GetMissing) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  pass

def test(x):
  print(x.foo)
)";
  compileAndRunToString(&runtime, src);
  HandleScope scope;
  Handle<Module> main(&scope, runtime.findModule("__main__"));
  Handle<Function> test(&scope, findInModule(&runtime, main, "test"));
  Handle<Class> klass(&scope, findInModule(&runtime, main, "Foo"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  args->atPut(0, runtime.newInstance(klass->id()));

  ASSERT_DEATH(callFunctionToString(test, args), "missing attribute");
}

// Fetch an attribute defined on the class
TEST(InstanceAttributeTest, GetClassAttribute) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  attr = 'testing 123'

def test(x):
  print(x.attr)
)";
  compileAndRunToString(&runtime, src);

  // Create the instance
  HandleScope scope;
  Handle<Module> main(&scope, runtime.findModule("__main__"));
  Handle<Function> test(&scope, findInModule(&runtime, main, "test"));
  Handle<Class> klass(&scope, findInModule(&runtime, main, "Foo"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  args->atPut(0, runtime.newInstance(klass->id()));

  EXPECT_EQ(callFunctionToString(test, args), "testing 123\n");
}

// Fetch an attribute defined in __init__
TEST(InstanceAttributeTest, GetInstanceAttribute) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  def __init__(self):
    self.attr = 'testing 123'

def test(x):
  Foo.__init__(x)
  print(x.attr)
)";
  compileAndRunToString(&runtime, src);

  // Create the instance
  HandleScope scope;
  Handle<Module> main(&scope, runtime.findModule("__main__"));
  Handle<Class> klass(&scope, findInModule(&runtime, main, "Foo"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  args->atPut(0, runtime.newInstance(klass->id()));

  // Run __init__
  Handle<Function> test(&scope, findInModule(&runtime, main, "test"));
  EXPECT_EQ(callFunctionToString(test, args), "testing 123\n");
}

// Set an attribute defined in __init__
TEST(InstanceAttributeTest, SetInstanceAttribute) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  def __init__(self):
    self.attr = 'testing 123'

def test(x):
  Foo.__init__(x)
  print(x.attr)
  x.attr = '321 testing'
  print(x.attr)
)";
  compileAndRunToString(&runtime, src);

  // Create the instance
  HandleScope scope;
  Handle<Module> main(&scope, runtime.findModule("__main__"));
  Handle<Class> klass(&scope, findInModule(&runtime, main, "Foo"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  args->atPut(0, runtime.newInstance(klass->id()));

  // Run __init__ then RMW the attribute
  Handle<Function> test(&scope, findInModule(&runtime, main, "test"));
  EXPECT_EQ(callFunctionToString(test, args), "testing 123\n321 testing\n");
}

// This is the real deal
TEST(InstanceAttributeTest, CallInstanceMethod) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  def __init__(self):
    self.attr = 'testing 123'

  def doit(self):
    print(self.attr)
    self.attr = '321 testing'
    print(self.attr)

def test(x):
  Foo.__init__(x)
  x.doit()
)";
  compileAndRunToString(&runtime, src);

  // Create the instance
  HandleScope scope;
  Handle<Module> main(&scope, runtime.findModule("__main__"));
  Handle<Class> klass(&scope, findInModule(&runtime, main, "Foo"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  args->atPut(0, runtime.newInstance(klass->id()));

  // Run __init__ then call the method
  Handle<Function> test(&scope, findInModule(&runtime, main, "test"));
  EXPECT_EQ(callFunctionToString(test, args), "testing 123\n321 testing\n");
}

TEST(InstanceAttributeTest, GetDataDescriptor) {
  Runtime runtime;
  const char* src = R"(
class DataDescr:
  def __set__(self, instance, value):
    pass

  def __get__(self, instance, owner):
    pass

class Foo:
  pass
)";
  compileAndRunToString(&runtime, src);

  // Create an instance of the descriptor and store it on the class
  HandleScope scope;
  Handle<Module> main(&scope, runtime.findModule("__main__"));
  Handle<Class> descr_klass(&scope, findInModule(&runtime, main, "DataDescr"));
  Handle<Object> klass(&scope, findInModule(&runtime, main, "Foo"));
  Handle<Object> attr(&scope, runtime.newStringFromCString("attr"));
  Handle<Object> descr(&scope, runtime.newInstance(descr_klass->id()));
  setInClassDict(&runtime, klass, attr, descr);

  // Fetch it from the instance
  Handle<Object> instance(
      &scope, runtime.newInstance(Class::cast(*klass)->id()));
  ASSERT_DEATH(
      runtime.attributeAt(Thread::currentThread(), instance, attr),
      "custom descriptors are unsupported");
}

TEST(InstanceAttributeTest, GetNonDataDescriptor) {
  Runtime runtime;
  const char* src = R"(
class Descr:
  def __get__(self, instance, owner):
    pass

class Foo:
  pass
)";
  compileAndRunToString(&runtime, src);

  // Create an instance of the descriptor and store it on the class
  HandleScope scope;
  Handle<Module> main(&scope, runtime.findModule("__main__"));
  Handle<Class> descr_klass(&scope, findInModule(&runtime, main, "Descr"));
  Handle<Object> klass(&scope, findInModule(&runtime, main, "Foo"));
  Handle<Object> attr(&scope, runtime.newStringFromCString("attr"));
  Handle<Object> descr(&scope, runtime.newInstance(descr_klass->id()));
  setInClassDict(&runtime, klass, attr, descr);

  // Fetch it from the instance
  Handle<Object> instance(
      &scope, runtime.newInstance(Class::cast(*klass)->id()));
  ASSERT_DEATH(
      runtime.attributeAt(Thread::currentThread(), instance, attr),
      "custom descriptors are unsupported");
}

} // namespace python
