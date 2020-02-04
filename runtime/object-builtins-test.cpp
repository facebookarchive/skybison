#include "object-builtins.h"

#include "gtest/gtest.h"

#include "builtins.h"
#include "frame.h"
#include "ic.h"
#include "runtime.h"
#include "str-builtins.h"
#include "test-utils.h"

namespace py {

using namespace testing;

using NoneBuiltinsTest = RuntimeFixture;
using ObjectBuiltinsTest = RuntimeFixture;

TEST_F(ObjectBuiltinsTest, DunderReprReturnsTypeNameAndPointer) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  pass

a = object.__repr__(Foo())
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, mainModuleAt(runtime_, "a"));
  // Storage for the class name. It must be shorter than the length of the whole
  // string.
  char* c_str = a.toCStr();
  char* class_name = static_cast<char*>(std::malloc(a.charLength()));
  void* ptr = nullptr;
  int num_written = std::sscanf(c_str, "<%s object at %p>", class_name, &ptr);
  ASSERT_EQ(num_written, 2);
  EXPECT_STREQ(class_name, "Foo");
  // No need to check the actual pointer value, just make sure something was
  // there.
  EXPECT_NE(ptr, nullptr);
  free(class_name);
  free(c_str);
}

TEST_F(ObjectBuiltinsTest, DunderEqWithIdenticalObjectsReturnsTrue) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = object.__eq__(None, None)
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_EQ(*result, Bool::trueObj());
}

TEST_F(ObjectBuiltinsTest,
       DunderEqWithNonIdenticalObjectsReturnsNotImplemented) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = object.__eq__(object(), object())
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(ObjectBuiltinsTest, DunderGetattributeReturnsAttribute) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C: pass
i = C()
i.foo = 79
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));
  Object name(&scope, runtime_->newStrFromCStr("foo"));
  EXPECT_TRUE(
      isIntEqualsWord(runBuiltin(METH(object, __getattribute__), i, name), 79));
}

TEST_F(ObjectBuiltinsTest, DunderGetattributeWithNonStringNameRaisesTypeError) {
  HandleScope scope;
  Object object(&scope, NoneType::object());
  Object name(&scope, runtime_->newInt(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(METH(object, __getattribute__), object, name),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST_F(ObjectBuiltinsTest,
       DunderGetattributeWithMissingAttributeRaisesAttributeError) {
  HandleScope scope;
  Object object(&scope, NoneType::object());
  Object name(&scope, runtime_->newStrFromCStr("xxx"));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(METH(object, __getattribute__), object, name),
      LayoutId::kAttributeError, "'NoneType' object has no attribute 'xxx'"));
}

TEST_F(ObjectBuiltinsTest, DunderSetattrSetsValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C: pass
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object value(&scope, runtime_->newInt(42));
  EXPECT_TRUE(
      runBuiltin(METH(object, __setattr__), i, name, value).isNoneType());
  ASSERT_TRUE(i.isInstance());
  Instance i_instance(&scope, *i);
  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread_, i_instance, name), 42));
}

TEST_F(ObjectBuiltinsTest, DunderSetattrWithNonStringNameRaisesTypeError) {
  HandleScope scope;
  Object object(&scope, NoneType::object());
  Object name(&scope, runtime_->newInt(0));
  Object value(&scope, runtime_->newInt(1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(METH(object, __setattr__), object, name, value),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST_F(ObjectBuiltinsTest, DunderSetattrOnBuiltinTypeRaisesAttributeError) {
  HandleScope scope(thread_);
  Object object(&scope, NoneType::object());
  Object name(&scope, runtime_->newStrFromCStr("foo"));
  Object value(&scope, runtime_->newInt(1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(METH(object, __setattr__), object, name, value),
      LayoutId::kAttributeError, "'NoneType' object has no attribute 'foo'"));
}

TEST_F(ObjectBuiltinsTest,
       DunderSizeofWithNonHeapObjectReturnsSizeofRawObject) {
  HandleScope scope;
  Object small_int(&scope, SmallInt::fromWord(6));
  Object result(&scope, runBuiltin(METH(object, __sizeof__), small_int));
  EXPECT_TRUE(isIntEqualsWord(*result, kPointerSize));
}

TEST_F(ObjectBuiltinsTest, DunderSizeofWithLargeStrReturnsSizeofHeapObject) {
  HandleScope scope;
  HeapObject large_str(&scope, runtime_->heap()->createLargeStr(40));
  Object result(&scope, runBuiltin(METH(object, __sizeof__), large_str));
  EXPECT_TRUE(isIntEqualsWord(*result, large_str.size()));
}

TEST_F(
    ObjectBuiltinsTest,
    DunderNeWithSelfImplementingDunderEqReturningNotImplementedReturnsNotImplemented) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo():
  def __eq__(self, b): return NotImplemented

result = object.__ne__(Foo(), None)
)")
                   .isError());
  EXPECT_TRUE(mainModuleAt(runtime_, "result").isNotImplementedType());
}

TEST_F(ObjectBuiltinsTest,
       DunderNeWithSelfImplementingDunderEqReturningZeroReturnsTrue) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo():
  def __eq__(self, b): return 0

result = object.__ne__(Foo(), None)
)")
                   .isError());
  // 0 is converted to False, and flipped again for __ne__ from __eq__.
  EXPECT_EQ(mainModuleAt(runtime_, "result"), Bool::trueObj());
}

TEST_F(ObjectBuiltinsTest,
       DunderNeWithSelfImplementingDunderEqReturningOneReturnsFalse) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo():
  def __eq__(self, b): return 1

result = object.__ne__(Foo(), None)
)")
                   .isError());
  // 1 is converted to True, and flipped again for __ne__ from __eq__.
  EXPECT_EQ(mainModuleAt(runtime_, "result"), Bool::falseObj());
}

TEST_F(ObjectBuiltinsTest,
       DunderNeWithSelfImplementingDunderEqReturningFalseReturnsTrue) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo():
  def __eq__(self, b): return False

result = object.__ne__(Foo(), None)
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(runtime_, "result"), Bool::trueObj());
}

TEST_F(ObjectBuiltinsTest,
       DunderNeWithSelfImplementingDunderEqReturningTrueReturnsFalse) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo():
  def __eq__(self, b): return True

result = object.__ne__(Foo(), None)
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(runtime_, "result"), Bool::falseObj());
}

TEST_F(ObjectBuiltinsTest, DunderStrReturnsDunderRepr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  pass

f = Foo()
a = object.__str__(f)
b = object.__repr__(f)
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  EXPECT_TRUE(isStrEquals(a, b));
}

TEST_F(ObjectBuiltinsTest, UserDefinedTypeInheritsDunderStr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  pass

f = Foo()
a = object.__str__(f)
b = f.__str__()
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  EXPECT_TRUE(isStrEquals(a, b));
}

TEST_F(ObjectBuiltinsTest,
       DunderInitDoesNotRaiseIfNewIsDifferentButInitIsSame) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  def __new__(cls):
    return object.__new__(cls)

Foo.__init__(Foo(), 1)
)")
                   .isError());
  // It doesn't matter what the output is, just that it doesn't throw a
  // TypeError.
}

TEST_F(ObjectBuiltinsTest, DunderInitWithNonInstanceIsOk) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
object.__init__(object)
)")
                   .isError());
  // It doesn't matter what the output is, just that it doesn't throw a
  // TypeError.
}

TEST_F(ObjectBuiltinsTest, DunderInitWithNoArgsRaisesTypeError) {
  // Passing no args to object.__init__ should throw a type error.
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, R"(
object.__init__()
)"),
      LayoutId::kTypeError,
      "'object.__init__' takes min 1 positional arguments but 0 given"));
}

TEST_F(ObjectBuiltinsTest, DunderInitWithArgsRaisesTypeError) {
  // Passing extra args to object.__init__, without overwriting __new__,
  // should throw a type error.
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class Foo:
  pass

Foo.__init__(Foo(), 1)
)"),
                            LayoutId::kTypeError,
                            "object.__init__() takes no parameters"));
}

TEST_F(ObjectBuiltinsTest, DunderInitWithNewAndInitRaisesTypeError) {
  // Passing extra args to object.__init__, and overwriting only __init__,
  // should throw a type error.
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class Foo:
  def __init__(self):
    object.__init__(self, 1)

Foo()
)"),
                            LayoutId::kTypeError,
                            "object.__init__() takes no parameters"));
}

TEST_F(NoneBuiltinsTest, NewReturnsNone) {
  HandleScope scope;
  Type type(&scope, runtime_->typeAt(LayoutId::kNoneType));
  EXPECT_TRUE(runBuiltin(METH(NoneType, __new__), type).isNoneType());
}

TEST_F(NoneBuiltinsTest, NewWithExtraArgsRaisesTypeError) {
  EXPECT_TRUE(
      raised(runFromCStr(runtime_, "NoneType.__new__(NoneType, 1, 2, 3, 4, 5)"),
             LayoutId::kTypeError));
}

TEST_F(NoneBuiltinsTest, DunderReprIsBoundMethod) {
  ASSERT_FALSE(runFromCStr(runtime_, "a = None.__repr__").isError());
  HandleScope scope;
  Object a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_TRUE(a.isBoundMethod());
}

TEST_F(NoneBuiltinsTest, DunderReprReturnsNone) {
  ASSERT_FALSE(runFromCStr(runtime_, "a = None.__repr__()").isError());
  HandleScope scope;
  Object a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "None"));
}

TEST_F(NoneBuiltinsTest, BuiltinBaseIsNone) {
  HandleScope scope;
  Type none_type(&scope, runtime_->typeAt(LayoutId::kNoneType));
  EXPECT_EQ(none_type.builtinBase(), LayoutId::kNoneType);
}

TEST_F(ObjectBuiltinsTest,
       InstanceDelAttrWithInObjectAttributeDeletesAttribute) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self):
    self.foo = 42
instance = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Instance instance(&scope, mainModuleAt(runtime_, "instance"));
  Layout layout(&scope, runtime_->layoutOf(*instance));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  AttributeInfo info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, name, &info));
  ASSERT_TRUE(info.isInObject());

  EXPECT_TRUE(instanceDelAttr(thread_, instance, name).isNoneType());
  EXPECT_TRUE(instanceGetAttribute(thread_, instance, name).isErrorNotFound());
  EXPECT_TRUE(instanceDelAttr(thread_, instance, name).isErrorNotFound());
}

TEST_F(ObjectBuiltinsTest,
       InstanceDelAttrWithTupleOverflowAttributeDeletesAttribute) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C: pass
instance = C()
instance.foo = 42
)")
                   .isError());
  HandleScope scope(thread_);
  Instance instance(&scope, mainModuleAt(runtime_, "instance"));
  Layout layout(&scope, runtime_->layoutOf(*instance));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  AttributeInfo info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, name, &info));
  ASSERT_TRUE(info.isOverflow());

  EXPECT_TRUE(instanceDelAttr(thread_, instance, name).isNoneType());
  EXPECT_TRUE(instanceGetAttribute(thread_, instance, name).isErrorNotFound());
  EXPECT_TRUE(instanceDelAttr(thread_, instance, name).isErrorNotFound());
}

TEST_F(ObjectBuiltinsTest,
       InstanceDelAttrWithNonexistentAttributeReturnsErrorNotFound) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C: pass
instance = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Instance instance(&scope, mainModuleAt(runtime_, "instance"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "does_not_exist"));
  EXPECT_TRUE(instanceDelAttr(thread_, instance, name).isErrorNotFound());
}

TEST_F(ObjectBuiltinsTest,
       InstanceDelAttrWithTupleOverflowAttributeKeepsOtherAttributes) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C: pass
instance = C()
instance.y = 2
instance.z = 3
)")
                   .isError());
  HandleScope scope(thread_);
  Instance instance(&scope, mainModuleAt(runtime_, "instance"));
  Object y(&scope, Runtime::internStrFromCStr(thread_, "y"));
  Object result(&scope, instanceDelAttr(thread_, instance, y));
  EXPECT_TRUE(instanceGetAttribute(thread_, instance, y).isErrorNotFound());
  EXPECT_TRUE(result.isNoneType());
  Object z(&scope, Runtime::internStrFromCStr(thread_, "z"));
  EXPECT_TRUE(isIntEqualsWord(instanceGetAttribute(thread_, instance, z), 3));
}

TEST_F(ObjectBuiltinsTest,
       InstanceDelAttrWithReadonlyAttributeRaisesAttributeError) {
  HandleScope scope(thread_);
  BuiltinAttribute attrs[] = {
      {ID(__globals__), 0, AttributeFlags::kReadOnly},
      {SymbolId::kSentinelId, -1},
  };
  BuiltinMethod builtins[] = {
      {SymbolId::kSentinelId, nullptr},
  };
  LayoutId layout_id = LayoutId::kLastBuiltinId;
  Type type(&scope,
            runtime_->addBuiltinType(ID(version), layout_id, LayoutId::kObject,
                                     attrs, builtins));
  Layout layout(&scope, type.instanceLayout());
  runtime_->layoutAtPut(layout_id, *layout);
  Instance instance(&scope, runtime_->newInstance(layout));
  Str attribute_name(&scope,
                     Runtime::internStrFromCStr(thread_, "__globals__"));
  EXPECT_TRUE(raisedWithStr(instanceDelAttr(thread_, instance, attribute_name),
                            LayoutId::kAttributeError,
                            "'__globals__' attribute is read-only"));
  EXPECT_EQ(instance.layoutId(), layout.id());
}

TEST_F(ObjectBuiltinsTest,
       InstanceDelAttrWithDictOverflowAttributeDeletesAttribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def instance(): pass
instance.foo = 42
)")
                   .isError());
  Instance instance(&scope, mainModuleAt(runtime_, "instance"));
  Layout layout(&scope, runtime_->layoutOf(*instance));
  ASSERT_TRUE(layout.hasDictOverflow());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  AttributeInfo info;
  ASSERT_FALSE(Runtime::layoutFindAttribute(*layout, name, &info));

  EXPECT_TRUE(instanceDelAttr(thread_, instance, name).isNoneType());
  EXPECT_TRUE(instanceGetAttribute(thread_, instance, name).isErrorNotFound());
  EXPECT_TRUE(instanceDelAttr(thread_, instance, name).isErrorNotFound());
}

TEST_F(
    ObjectBuiltinsTest,
    InstanceDelAttrWithNonexistentAttributeDictOverflowReturnsErrorNotFound) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def instance(): pass
)")
                   .isError());
  Instance instance(&scope, mainModuleAt(runtime_, "instance"));
  Layout layout(&scope, runtime_->layoutOf(*instance));
  ASSERT_TRUE(layout.hasDictOverflow());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "does_not_exist"));
  EXPECT_TRUE(instanceDelAttr(thread_, instance, name).isErrorNotFound());
}

TEST_F(ObjectBuiltinsTest,
       InstanceGetAttributeWithInObjectAttributeReturnsValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self):
    self.foo = 42
instance = C()
)")
                   .isError());
  Instance instance(&scope, mainModuleAt(runtime_, "instance"));
  Layout layout(&scope, runtime_->layoutOf(*instance));
  ASSERT_TRUE(layout.hasTupleOverflow());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  AttributeInfo info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, name, &info));
  ASSERT_TRUE(info.isInObject());

  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread_, instance, name), 42));
}

TEST_F(ObjectBuiltinsTest,
       InstanceGetAttributeWithTupleOverflowAttributeReturnsValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C: pass
instance = C()
instance.foo = 42
)")
                   .isError());
  Instance instance(&scope, mainModuleAt(runtime_, "instance"));
  Layout layout(&scope, runtime_->layoutOf(*instance));
  ASSERT_TRUE(layout.hasTupleOverflow());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  AttributeInfo info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, name, &info));
  ASSERT_TRUE(info.isOverflow());

  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread_, instance, name), 42));
}

TEST_F(ObjectBuiltinsTest,
       InstanceGetAttributeWithDictOverflowAttributeReturnsValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def instance(): pass
instance.foo = 42
)")
                   .isError());
  Instance instance(&scope, mainModuleAt(runtime_, "instance"));
  Layout layout(&scope, runtime_->layoutOf(*instance));
  ASSERT_TRUE(layout.hasDictOverflow());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  AttributeInfo info;
  ASSERT_FALSE(Runtime::layoutFindAttribute(*layout, name, &info));

  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread_, instance, name), 42));
}

TEST_F(
    ObjectBuiltinsTest,
    InstanceGetAttributeWithNonExistentAttributeDictOverflowReturnsErrorNotFound) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def instance(): pass
)")
                   .isError());
  Instance instance(&scope, mainModuleAt(runtime_, "instance"));
  Layout layout(&scope, runtime_->layoutOf(*instance));
  ASSERT_TRUE(layout.hasDictOverflow());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "does_not_exist"));
  EXPECT_TRUE(instanceGetAttribute(thread_, instance, name).isErrorNotFound());
}

TEST_F(ObjectBuiltinsTest, InstanceSetAttrSetsInObjectAttribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self, set_value):
    if set_value:
      self.foo = 42
instance = C(False)
)")
                   .isError());
  Instance instance(&scope, mainModuleAt(runtime_, "instance"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  Object value(&scope, runtime_->newInt(-7));
  EXPECT_TRUE(instanceSetAttr(thread_, instance, name, value).isNoneType());

  Layout layout(&scope, runtime_->layoutOf(*instance));
  AttributeInfo info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, name, &info));
  ASSERT_TRUE(info.isInObject());

  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread_, instance, name), -7));
}

TEST_F(ObjectBuiltinsTest, InstanceSetAttrSetsNewTupleOverflowAttribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C: pass
instance = C()
)")
                   .isError());
  Instance instance(&scope, mainModuleAt(runtime_, "instance"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  Object value(&scope, runtime_->newInt(-14));
  EXPECT_TRUE(instanceSetAttr(thread_, instance, name, value).isNoneType());

  Layout layout(&scope, runtime_->layoutOf(*instance));
  Tuple overflow(&scope, instance.instanceVariableAt(layout.overflowOffset()));
  EXPECT_EQ(overflow.length(), 1);

  AttributeInfo info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, name, &info));
  ASSERT_TRUE(info.isOverflow());

  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread_, instance, name), -14));
}

TEST_F(ObjectBuiltinsTest, InstanceSetAttrSetsExistingTupleOverflowAttribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C: pass
instance = C()
instance.bar = 5000
)")
                   .isError());
  Instance instance(&scope, mainModuleAt(runtime_, "instance"));
  Layout layout(&scope, runtime_->layoutOf(*instance));
  Tuple overflow(&scope, instance.instanceVariableAt(layout.overflowOffset()));
  ASSERT_EQ(overflow.length(), 1);

  Object name(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  Object value(&scope, runtime_->newInt(-14));
  EXPECT_TRUE(instanceSetAttr(thread_, instance, name, value).isNoneType());
  ASSERT_EQ(overflow.length(), 1);

  AttributeInfo info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, name, &info));
  ASSERT_TRUE(info.isOverflow());

  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread_, instance, name), -14));
}

TEST_F(ObjectBuiltinsTest, InstanceSetAttrSetsDictOverflowAttribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def instance(): pass
)")
                   .isError());
  Instance instance(&scope, mainModuleAt(runtime_, "instance"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  Object value(&scope, runtime_->newInt(4711));
  Layout layout(&scope, runtime_->layoutOf(*instance));
  ASSERT_TRUE(layout.hasDictOverflow());

  EXPECT_TRUE(instanceSetAttr(thread_, instance, name, value).isNoneType());
  EXPECT_EQ(instance.layoutId(), layout.id());

  AttributeInfo info;
  ASSERT_FALSE(Runtime::layoutFindAttribute(*layout, name, &info));
  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread_, instance, name), 4711));
}

TEST_F(ObjectBuiltinsTest, ObjectGetAttributeReturnsInstanceValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C: pass
c = C()
c.__hash__ = 42
)")
                   .isError());
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "__hash__"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread_, c, name), 42));
}

TEST_F(ObjectBuiltinsTest, ObjectGetAttributeReturnsTypeValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  x = -11
c = C()
)")
                   .isError());
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "x"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread_, c, name), -11));
}

TEST_F(ObjectBuiltinsTest, ObjectGetAttributeWithNonExistentNameReturnsError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C: pass
c = C()
)")
                   .isError());
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "xxx"));
  EXPECT_TRUE(objectGetAttribute(thread_, c, name).isError());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(ObjectBuiltinsTest, ObjectGetAttributeCallsDunderGetOnDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): return 42
class A:
  foo = D()
a = A()
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread_, a, name), 42));
}

TEST_F(ObjectBuiltinsTest,
       ObjectGetAttributeCallsDunderGetOnNonDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class D:
  def __get__(self, instance, owner): return 42
class A:
  foo = D()
a = A()
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread_, a, name), 42));
}

TEST_F(ObjectBuiltinsTest,
       ObjectGetAttributePrefersDataDescriptorOverInstanceAttr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): return 42
class A:
  pass
a = A()
a.foo = 12
A.foo = D()
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread_, a, name), 42));
}

TEST_F(ObjectBuiltinsTest,
       ObjectGetAttributePrefersInstanceAttrOverNonDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class D:
  def __get__(self, instance, owner): return 42
class A:
  foo = D()
a = A()
a.foo = 12
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread_, a, name), 12));
}

TEST_F(ObjectBuiltinsTest, ObjectGetAttributePropagatesDunderGetException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): raise UserWarning()
class A:
  foo = D()
a = A()
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  EXPECT_TRUE(
      raised(objectGetAttribute(thread_, a, name), LayoutId::kUserWarning));
}

TEST_F(ObjectBuiltinsTest,
       ObjectGetAttributeOnNoneNonDataDescriptorReturnsBoundMethod) {
  HandleScope scope(thread_);
  Object none(&scope, NoneType::object());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "__repr__"));
  EXPECT_TRUE(objectGetAttribute(thread_, none, name).isBoundMethod());
}

TEST_F(ObjectBuiltinsTest,
       ObjectGetAttributeSetLocationReturnsBoundMethodAndCachesFunction) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def foo():
    pass
foo = C.foo
i = C()
)")
                   .isError());
  Object foo(&scope, mainModuleAt(runtime_, "foo"));
  Object i(&scope, mainModuleAt(runtime_, "i"));

  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object to_cache(&scope, NoneType::object());
  LoadAttrKind kind = LoadAttrKind::kUnknown;
  Object result_obj(&scope, objectGetAttributeSetLocation(thread_, i, name,
                                                          &to_cache, &kind));
  ASSERT_TRUE(result_obj.isBoundMethod());
  BoundMethod result(&scope, *result_obj);
  EXPECT_EQ(result.function(), foo);
  EXPECT_EQ(result.self(), i);
  EXPECT_EQ(to_cache, foo);
  EXPECT_EQ(kind, LoadAttrKind::kInstanceFunction);

  Object load_cached_result_obj(
      &scope, Interpreter::loadAttrWithLocation(thread_, *i, *to_cache));
  ASSERT_TRUE(load_cached_result_obj.isBoundMethod());
  BoundMethod load_cached_result(&scope, *load_cached_result_obj);
  EXPECT_EQ(load_cached_result.function(), foo);
  EXPECT_EQ(load_cached_result.self(), i);
}

TEST_F(ObjectBuiltinsTest,
       ObjectGetAttributeSetLocationReturnsInstanceVariableAndCachesOffset) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self):
    self.foo = 42
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));

  Layout layout(&scope, runtime_->layoutOf(*i));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  AttributeInfo info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, name, &info));
  ASSERT_TRUE(info.isInObject());

  Object to_cache(&scope, NoneType::object());
  LoadAttrKind kind = LoadAttrKind::kUnknown;
  EXPECT_TRUE(isIntEqualsWord(
      objectGetAttributeSetLocation(thread_, i, name, &to_cache, &kind), 42));
  EXPECT_TRUE(isIntEqualsWord(*to_cache, info.offset()));
  EXPECT_EQ(kind, LoadAttrKind::kInstanceOffset);

  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrWithLocation(thread_, *i, *to_cache), 42));
}

TEST_F(
    ObjectBuiltinsTest,
    ObjectGetAttributeSetLocationReturnsInstanceVariableAndCachesNegativeOffset) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  pass
i = C()
i.foo = 17
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));

  Layout layout(&scope, runtime_->layoutOf(*i));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  AttributeInfo info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, name, &info));
  ASSERT_TRUE(info.isOverflow());

  Object to_cache(&scope, NoneType::object());
  LoadAttrKind kind = LoadAttrKind::kUnknown;
  EXPECT_TRUE(isIntEqualsWord(
      objectGetAttributeSetLocation(thread_, i, name, &to_cache, &kind), 17));
  EXPECT_TRUE(isIntEqualsWord(*to_cache, -info.offset() - 1));

  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrWithLocation(thread_, *i, *to_cache), 17));
}

TEST_F(ObjectBuiltinsTest,
       ObjectGetAttributeSetLocationRaisesAttributeErrorAndDoesNotSetLocation) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  pass
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));

  Object name(&scope, Runtime::internStrFromCStr(thread_, "xxx"));
  Object to_cache(&scope, NoneType::object());
  LoadAttrKind kind = LoadAttrKind::kUnknown;
  EXPECT_TRUE(objectGetAttributeSetLocation(thread_, i, name, &to_cache, &kind)
                  .isError());
  EXPECT_TRUE(to_cache.isNoneType());
  EXPECT_EQ(kind, LoadAttrKind::kUnknown);
}

TEST_F(ObjectBuiltinsTest, ObjectSetAttrSetsInstanceValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C: pass
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object value(&scope, runtime_->newInt(47));
  EXPECT_TRUE(objectSetAttr(thread_, i, name, value).isNoneType());
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread_, i, name), 47));
}

TEST_F(ObjectBuiltinsTest, ObjectSetAttrOnDataDescriptorCallsDunderSet) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class D:
  def __set__(self, instance, value):
    global set_args
    set_args = (self, instance, value)
    return "ignored result"
  def __get__(self, instance, owner): pass
foo_descr = D()
class C:
  foo = foo_descr
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));
  Object foo_descr(&scope, mainModuleAt(runtime_, "foo_descr"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object value(&scope, runtime_->newInt(47));
  EXPECT_TRUE(objectSetAttr(thread_, i, name, value).isNoneType());
  Object set_args_obj(&scope, mainModuleAt(runtime_, "set_args"));
  ASSERT_TRUE(set_args_obj.isTuple());
  Tuple dunder_set_args(&scope, *set_args_obj);
  ASSERT_EQ(dunder_set_args.length(), 3);
  EXPECT_EQ(dunder_set_args.at(0), foo_descr);
  EXPECT_EQ(dunder_set_args.at(1), i);
  EXPECT_TRUE(isIntEqualsWord(dunder_set_args.at(2), 47));
}

TEST_F(ObjectBuiltinsTest, ObjectSetAttrPropagatesErrorsInDunderSet) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class D:
  def __set__(self, instance, value): raise UserWarning()
  def __get__(self, instance, owner): pass
class C:
  foo = D()
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object value(&scope, runtime_->newInt(1));
  EXPECT_TRUE(
      raised(objectSetAttr(thread_, i, name, value), LayoutId::kUserWarning));
}

TEST_F(ObjectBuiltinsTest, ObjectSetAttrOnNonHeapObjectRaisesAttributeError) {
  HandleScope scope(thread_);
  Object object(&scope, runtime_->newInt(42));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object value(&scope, runtime_->newInt(1));
  EXPECT_TRUE(raisedWithStr(objectSetAttr(thread_, object, name, value),
                            LayoutId::kAttributeError,
                            "'int' object has no attribute 'foo'"));
}

TEST_F(ObjectBuiltinsTest, ObjectSetAttrSetLocationSetsValueCachesOffset) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self):
    self.foo = 0
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));

  AttributeInfo info;
  Layout layout(&scope, runtime_->layoutOf(*i));
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, name, &info));
  ASSERT_TRUE(info.isInObject());

  Object value(&scope, runtime_->newInt(7));
  Object value2(&scope, runtime_->newInt(99));
  Object to_cache(&scope, NoneType::object());
  EXPECT_TRUE(objectSetAttrSetLocation(thread_, i, name, value, &to_cache)
                  .isNoneType());
  EXPECT_TRUE(isIntEqualsWord(*to_cache, info.offset()));
  ASSERT_TRUE(i.isInstance());
  Instance instance(&scope, *i);
  EXPECT_TRUE(isIntEqualsWord(instance.instanceVariableAt(info.offset()), 7));

  Interpreter::storeAttrWithLocation(thread_, *i, *to_cache, *value2);
  EXPECT_TRUE(isIntEqualsWord(instance.instanceVariableAt(info.offset()), 99));
}

TEST_F(ObjectBuiltinsTest,
       ObjectSetAttrSetLocationSetsOverflowValueCachesOffset) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C: pass
i = C()
i.foo = 0
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));

  AttributeInfo info;
  Layout layout(&scope, runtime_->layoutOf(*i));
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, name, &info));
  ASSERT_TRUE(info.isOverflow());

  Object value(&scope, runtime_->newInt(-8));
  Object value2(&scope, runtime_->newInt(11));
  Object to_cache(&scope, NoneType::object());
  EXPECT_TRUE(objectSetAttrSetLocation(thread_, i, name, value, &to_cache)
                  .isNoneType());
  EXPECT_TRUE(isIntEqualsWord(*to_cache, -info.offset() - 1));
  ASSERT_TRUE(i.isHeapObject());
  Instance instance(&scope, *i);
  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread_, instance, name), -8));

  Interpreter::storeAttrWithLocation(thread_, *i, *to_cache, *value2);
  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread_, instance, name), 11));
}

}  // namespace py
