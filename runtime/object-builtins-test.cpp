#include "gtest/gtest.h"

#include "frame.h"
#include "ic.h"
#include "object-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

using NoneBuiltinsTest = RuntimeFixture;
using ObjectBuiltinsTest = RuntimeFixture;

TEST_F(ObjectBuiltinsTest, DunderReprReturnsTypeNameAndPointer) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  pass

a = object.__repr__(Foo())
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));
  // Storage for the class name. It must be shorter than the length of the whole
  // string.
  char* c_str = a.toCStr();
  char* class_name = static_cast<char*>(std::malloc(a.length()));
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
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = object.__eq__(None, None)
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_EQ(*result, Bool::trueObj());
}

TEST_F(ObjectBuiltinsTest,
       DunderEqWithNonIdenticalObjectsReturnsNotImplemented) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = object.__eq__(object(), object())
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(ObjectBuiltinsTest, DunderGetattributeReturnsAttribute) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C: pass
i = C()
i.foo = 79
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime_, "__main__", "i"));
  Object name(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(ObjectBuiltins::dunderGetattribute, i, name), 79));
}

TEST_F(ObjectBuiltinsTest, DunderGetattributeWithNonStringNameRaisesTypeError) {
  HandleScope scope;
  Object object(&scope, NoneType::object());
  Object name(&scope, runtime_.newInt(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ObjectBuiltins::dunderGetattribute, object, name),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST_F(ObjectBuiltinsTest,
       DunderGetattributeWithMissingAttributeRaisesAttributeError) {
  HandleScope scope;
  Object object(&scope, NoneType::object());
  Object name(&scope, runtime_.newStrFromCStr("xxx"));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ObjectBuiltins::dunderGetattribute, object, name),
      LayoutId::kAttributeError, "'NoneType' object has no attribute 'xxx'"));
}

TEST_F(ObjectBuiltinsTest, DunderSetattrSetsValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C: pass
i = C()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime_, "__main__", "i"));
  Object name(&scope, runtime_.newStrFromCStr("foo"));
  Object value(&scope, runtime_.newInt(42));
  EXPECT_TRUE(
      runBuiltin(ObjectBuiltins::dunderSetattr, i, name, value).isNoneType());
  ASSERT_TRUE(i.isHeapObject());
  HeapObject i_heap_object(&scope, *i);
  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread_, i_heap_object, name), 42));
}

TEST_F(ObjectBuiltinsTest, DunderSetattrWithNonStringNameRaisesTypeError) {
  HandleScope scope;
  Object object(&scope, NoneType::object());
  Object name(&scope, runtime_.newInt(0));
  Object value(&scope, runtime_.newInt(1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ObjectBuiltins::dunderSetattr, object, name, value),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST_F(ObjectBuiltinsTest, DunderSetattrOnBuiltinTypeRaisesAttributeError) {
  HandleScope scope(thread_);
  Object object(&scope, NoneType::object());
  Object name(&scope, runtime_.newStrFromCStr("foo"));
  Object value(&scope, runtime_.newInt(1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ObjectBuiltins::dunderSetattr, object, name, value),
      LayoutId::kAttributeError, "'NoneType' object has no attribute 'foo'"));
}

TEST_F(ObjectBuiltinsTest,
       DunderSizeofWithNonHeapObjectReturnsSizeofRawObject) {
  HandleScope scope;
  Object small_int(&scope, SmallInt::fromWord(6));
  Object result(&scope, runBuiltin(ObjectBuiltins::dunderSizeof, small_int));
  EXPECT_TRUE(isIntEqualsWord(*result, kPointerSize));
}

TEST_F(ObjectBuiltinsTest, DunderSizeofWithLargeStrReturnsSizeofHeapObject) {
  HandleScope scope;
  HeapObject large_str(&scope, runtime_.heap()->createLargeStr(40));
  Object result(&scope, runBuiltin(ObjectBuiltins::dunderSizeof, large_str));
  EXPECT_TRUE(isIntEqualsWord(*result, large_str.size()));
}

TEST_F(
    ObjectBuiltinsTest,
    DunderNeWithSelfImplementingDunderEqReturningNotImplementedReturnsNotImplemented) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo():
  def __eq__(self, b): return NotImplemented

result = object.__ne__(Foo(), None)
)")
                   .isError());
  EXPECT_TRUE(moduleAt(&runtime_, "__main__", "result").isNotImplementedType());
}

TEST_F(ObjectBuiltinsTest,
       DunderNeWithSelfImplementingDunderEqReturningZeroReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo():
  def __eq__(self, b): return 0

result = object.__ne__(Foo(), None)
)")
                   .isError());
  // 0 is converted to False, and flipped again for __ne__ from __eq__.
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::trueObj());
}

TEST_F(ObjectBuiltinsTest,
       DunderNeWithSelfImplementingDunderEqReturningOneReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo():
  def __eq__(self, b): return 1

result = object.__ne__(Foo(), None)
)")
                   .isError());
  // 1 is converted to True, and flipped again for __ne__ from __eq__.
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(ObjectBuiltinsTest,
       DunderNeWithSelfImplementingDunderEqReturningFalseReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo():
  def __eq__(self, b): return False

result = object.__ne__(Foo(), None)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::trueObj());
}

TEST_F(ObjectBuiltinsTest,
       DunderNeWithSelfImplementingDunderEqReturningTrueReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo():
  def __eq__(self, b): return True

result = object.__ne__(Foo(), None)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(ObjectBuiltinsTest, DunderStrReturnsDunderRepr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  pass

f = Foo()
a = object.__str__(f)
b = object.__repr__(f)
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime_, "__main__", "b"));
  EXPECT_TRUE(isStrEquals(a, b));
}

TEST_F(ObjectBuiltinsTest, UserDefinedTypeInheritsDunderStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  pass

f = Foo()
a = object.__str__(f)
b = f.__str__()
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime_, "__main__", "b"));
  EXPECT_TRUE(isStrEquals(a, b));
}

TEST_F(ObjectBuiltinsTest,
       DunderInitDoesNotRaiseIfNewIsDifferentButInitIsSame) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
object.__init__(object)
)")
                   .isError());
  // It doesn't matter what the output is, just that it doesn't throw a
  // TypeError.
}

TEST_F(ObjectBuiltinsTest, DunderInitWithNoArgsRaisesTypeError) {
  // Passing no args to object.__init__ should throw a type error.
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, R"(
object.__init__()
)"),
      LayoutId::kTypeError,
      "TypeError: 'object.__init__' takes 1 positional arguments but 0 given"));
}

TEST_F(ObjectBuiltinsTest, DunderInitWithArgsRaisesTypeError) {
  // Passing extra args to object.__init__, without overwriting __new__,
  // should throw a type error.
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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
  Type type(&scope, runtime_.typeAt(LayoutId::kNoneType));
  EXPECT_TRUE(runBuiltin(NoneBuiltins::dunderNew, type).isNoneType());
}

TEST_F(NoneBuiltinsTest, NewWithExtraArgsRaisesTypeError) {
  EXPECT_TRUE(raised(
      runFromCStr(&runtime_, "NoneType.__new__(NoneType, 1, 2, 3, 4, 5)"),
      LayoutId::kTypeError));
}

TEST_F(NoneBuiltinsTest, DunderReprIsBoundMethod) {
  ASSERT_FALSE(runFromCStr(&runtime_, "a = None.__repr__").isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(a.isBoundMethod());
}

TEST_F(NoneBuiltinsTest, DunderReprReturnsNone) {
  ASSERT_FALSE(runFromCStr(&runtime_, "a = None.__repr__()").isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "None"));
}

TEST_F(NoneBuiltinsTest, BuiltinBaseIsNone) {
  HandleScope scope;
  Type none_type(&scope, runtime_.typeAt(LayoutId::kNoneType));
  EXPECT_EQ(none_type.builtinBase(), LayoutId::kNoneType);
}

TEST_F(ObjectBuiltinsTest, ObjectGetAttributeReturnsInstanceValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C: pass
c = C()
c.__hash__ = 42
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime_, "__main__", "c"));
  Object name(&scope, runtime_.newStrFromCStr("__hash__"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread_, c, name), 42));
}

TEST_F(ObjectBuiltinsTest, ObjectGetAttributeReturnsTypeValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  x = -11
c = C()
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime_, "__main__", "c"));
  Object name(&scope, runtime_.newStrFromCStr("x"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread_, c, name), -11));
}

TEST_F(ObjectBuiltinsTest, ObjectGetAttributeWithNonExistentNameReturnsError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C: pass
c = C()
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime_, "__main__", "c"));
  Object name(&scope, runtime_.newStrFromCStr("xxx"));
  EXPECT_TRUE(objectGetAttribute(thread_, c, name).isError());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(ObjectBuiltinsTest, ObjectGetAttributeCallsDunderGetOnDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): return 42
class A:
  foo = D()
a = A()
)")
                   .isError());
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread_, a, foo), 42));
}

TEST_F(ObjectBuiltinsTest,
       ObjectGetAttributeCallsDunderGetOnNonDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class D:
  def __get__(self, instance, owner): return 42
class A:
  foo = D()
a = A()
)")
                   .isError());
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread_, a, foo), 42));
}

TEST_F(ObjectBuiltinsTest,
       ObjectGetAttributePrefersDataDescriptorOverInstanceAttr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread_, a, foo), 42));
}

TEST_F(ObjectBuiltinsTest,
       ObjectGetAttributePrefersInstanceAttrOverNonDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class D:
  def __get__(self, instance, owner): return 42
class A:
  foo = D()
a = A()
a.foo = 12
)")
                   .isError());
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread_, a, foo), 12));
}

TEST_F(ObjectBuiltinsTest, ObjectGetAttributePropagatesDunderGetException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): raise UserWarning()
class A:
  foo = D()
a = A()
)")
                   .isError());
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(
      raised(objectGetAttribute(thread_, a, foo), LayoutId::kUserWarning));
}

TEST_F(ObjectBuiltinsTest,
       ObjectGetAttributeOnNoneNonDataDescriptorReturnsBoundMethod) {
  HandleScope scope(thread_);
  Object none(&scope, NoneType::object());
  Object attr_name(&scope, runtime_.newStrFromCStr("__repr__"));
  EXPECT_TRUE(objectGetAttribute(thread_, none, attr_name).isBoundMethod());
}

TEST_F(ObjectBuiltinsTest,
       ObjectGetAttributeSetLocationReturnsBoundMethodAndCachesFunction) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def foo():
    pass
foo = C.foo
i = C()
)")
                   .isError());
  Object foo(&scope, moduleAt(&runtime_, "__main__", "foo"));
  Object i(&scope, moduleAt(&runtime_, "__main__", "i"));

  Object name(&scope, runtime_.newStrFromCStr("foo"));
  Object to_cache(&scope, NoneType::object());
  Object result_obj(&scope,
                    objectGetAttributeSetLocation(thread_, i, name, &to_cache));
  ASSERT_TRUE(result_obj.isBoundMethod());
  BoundMethod result(&scope, *result_obj);
  EXPECT_EQ(result.function(), foo);
  EXPECT_EQ(result.self(), i);
  EXPECT_EQ(to_cache, foo);

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
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self):
    self.foo = 42
i = C()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime_, "__main__", "i"));

  Layout layout(&scope, runtime_.layoutAt(i.layoutId()));
  Object name(&scope, runtime_.newStrFromCStr("foo"));
  AttributeInfo info;
  ASSERT_TRUE(runtime_.layoutFindAttribute(thread_, layout, name, &info));
  ASSERT_TRUE(info.isInObject());

  Object to_cache(&scope, NoneType::object());
  EXPECT_TRUE(isIntEqualsWord(
      objectGetAttributeSetLocation(thread_, i, name, &to_cache), 42));
  EXPECT_TRUE(isIntEqualsWord(*to_cache, info.offset()));

  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrWithLocation(thread_, *i, *to_cache), 42));
}

TEST_F(
    ObjectBuiltinsTest,
    ObjectGetAttributeSetLocationReturnsInstanceVariableAndCachesNegativeOffset) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  pass
i = C()
i.foo = 17
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime_, "__main__", "i"));

  Layout layout(&scope, runtime_.layoutAt(i.layoutId()));
  Object name(&scope, runtime_.newStrFromCStr("foo"));
  AttributeInfo info;
  ASSERT_TRUE(runtime_.layoutFindAttribute(thread_, layout, name, &info));
  ASSERT_TRUE(info.isOverflow());

  Object to_cache(&scope, NoneType::object());
  EXPECT_TRUE(isIntEqualsWord(
      objectGetAttributeSetLocation(thread_, i, name, &to_cache), 17));
  EXPECT_TRUE(isIntEqualsWord(*to_cache, -info.offset() - 1));

  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrWithLocation(thread_, *i, *to_cache), 17));
}

TEST_F(ObjectBuiltinsTest,
       ObjectGetAttributeSetLocationRaisesAttributeErrorAndDoesNotSetLocation) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  pass
i = C()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime_, "__main__", "i"));

  Object name(&scope, runtime_.newStrFromCStr("xxx"));
  Object to_cache(&scope, NoneType::object());
  EXPECT_TRUE(
      objectGetAttributeSetLocation(thread_, i, name, &to_cache).isError());
  EXPECT_TRUE(to_cache.isNoneType());
}

TEST_F(ObjectBuiltinsTest, ObjectSetAttrSetsInstanceValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C: pass
i = C()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime_, "__main__", "i"));
  Object name(&scope, runtime_.internStrFromCStr(thread_, "foo"));
  Object value(&scope, runtime_.newInt(47));
  EXPECT_TRUE(objectSetAttr(thread_, i, name, value).isNoneType());
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread_, i, name), 47));
}

TEST_F(ObjectBuiltinsTest, ObjectSetAttrOnDataDescriptorCallsDunderSet) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object i(&scope, moduleAt(&runtime_, "__main__", "i"));
  Object foo_descr(&scope, moduleAt(&runtime_, "__main__", "foo_descr"));
  Object name(&scope, runtime_.internStrFromCStr(thread_, "foo"));
  Object value(&scope, runtime_.newInt(47));
  EXPECT_TRUE(objectSetAttr(thread_, i, name, value).isNoneType());
  Object set_args_obj(&scope, moduleAt(&runtime_, "__main__", "set_args"));
  ASSERT_TRUE(set_args_obj.isTuple());
  Tuple dunder_set_args(&scope, *set_args_obj);
  ASSERT_EQ(dunder_set_args.length(), 3);
  EXPECT_EQ(dunder_set_args.at(0), foo_descr);
  EXPECT_EQ(dunder_set_args.at(1), i);
  EXPECT_TRUE(isIntEqualsWord(dunder_set_args.at(2), 47));
}

TEST_F(ObjectBuiltinsTest, ObjectSetAttrPropagatesErrorsInDunderSet) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class D:
  def __set__(self, instance, value): raise UserWarning()
  def __get__(self, instance, owner): pass
class C:
  foo = D()
i = C()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime_, "__main__", "i"));
  Object name(&scope, runtime_.internStrFromCStr(thread_, "foo"));
  Object value(&scope, runtime_.newInt(1));
  EXPECT_TRUE(
      raised(objectSetAttr(thread_, i, name, value), LayoutId::kUserWarning));
}

TEST_F(ObjectBuiltinsTest, ObjectSetAttrOnNonHeapObjectRaisesAttributeError) {
  HandleScope scope(thread_);
  Object object(&scope, runtime_.newInt(42));
  Object name(&scope, runtime_.internStrFromCStr(thread_, "foo"));
  Object value(&scope, runtime_.newInt(1));
  EXPECT_TRUE(raisedWithStr(objectSetAttr(thread_, object, name, value),
                            LayoutId::kAttributeError,
                            "'int' object has no attribute 'foo'"));
}

TEST_F(ObjectBuiltinsTest, ObjectSetAttrSetLocationSetsValueCachesOffset) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self):
    self.foo = 0
i = C()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime_, "__main__", "i"));
  Object name(&scope, runtime_.internStrFromCStr(thread_, "foo"));

  AttributeInfo info;
  Layout layout(&scope, runtime_.layoutAt(i.layoutId()));
  ASSERT_TRUE(runtime_.layoutFindAttribute(thread_, layout, name, &info));
  ASSERT_TRUE(info.isInObject());

  Object value(&scope, runtime_.newInt(7));
  Object value2(&scope, runtime_.newInt(99));
  Object to_cache(&scope, NoneType::object());
  EXPECT_TRUE(objectSetAttrSetLocation(thread_, i, name, value, &to_cache)
                  .isNoneType());
  EXPECT_TRUE(isIntEqualsWord(*to_cache, info.offset()));
  ASSERT_TRUE(i.isHeapObject());
  HeapObject heap_object(&scope, *i);
  EXPECT_TRUE(
      isIntEqualsWord(heap_object.instanceVariableAt(info.offset()), 7));

  Interpreter::storeAttrWithLocation(thread_, *i, *to_cache, *value2);
  EXPECT_TRUE(
      isIntEqualsWord(heap_object.instanceVariableAt(info.offset()), 99));
}

TEST_F(ObjectBuiltinsTest,
       ObjectSetAttrSetLocationSetsOverflowValueCachesOffset) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C: pass
i = C()
i.foo = 0
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime_, "__main__", "i"));
  Object name(&scope, runtime_.internStrFromCStr(thread_, "foo"));

  AttributeInfo info;
  Layout layout(&scope, runtime_.layoutAt(i.layoutId()));
  ASSERT_TRUE(runtime_.layoutFindAttribute(thread_, layout, name, &info));
  ASSERT_TRUE(info.isOverflow());

  Object value(&scope, runtime_.newInt(-8));
  Object value2(&scope, runtime_.newInt(11));
  Object to_cache(&scope, NoneType::object());
  EXPECT_TRUE(objectSetAttrSetLocation(thread_, i, name, value, &to_cache)
                  .isNoneType());
  EXPECT_TRUE(isIntEqualsWord(*to_cache, -info.offset() - 1));
  ASSERT_TRUE(i.isHeapObject());
  HeapObject heap_object(&scope, *i);
  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread_, heap_object, name), -8));

  Interpreter::storeAttrWithLocation(thread_, *i, *to_cache, *value2);
  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread_, heap_object, name), 11));
}

}  // namespace python
