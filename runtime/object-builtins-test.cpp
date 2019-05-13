#include "gtest/gtest.h"

#include "frame.h"
#include "ic.h"
#include "object-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ObjectBuiltinsTest, DunderReprReturnsTypeNameAndPointer) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  pass

a = object.__repr__(Foo())
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
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

TEST(ObjectBuiltinsTest, DunderEqWithIdenticalObjectsReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = object.__eq__(None, None)
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(*result, Bool::trueObj());
}

TEST(ObjectBuiltinsTest, DunderEqWithNonIdenticalObjectsReturnsNotImplemented) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = object.__eq__(object(), object())
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST(ObjectBuiltinsTest, DunderGetattributeReturnsAttribute) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C: pass
i = C()
i.foo = 79
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));
  Object name(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(ObjectBuiltins::dunderGetattribute, i, name), 79));
}

TEST(ObjectBuiltinsTest, DunderGetattributeWithNonStringNameRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object object(&scope, NoneType::object());
  Object name(&scope, runtime.newInt(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ObjectBuiltins::dunderGetattribute, object, name),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST(ObjectBuiltinsTest,
     DunderGetattributeWithMissingAttributeRaisesAttributeError) {
  Runtime runtime;
  HandleScope scope;
  Object object(&scope, NoneType::object());
  Object name(&scope, runtime.newStrFromCStr("xxx"));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ObjectBuiltins::dunderGetattribute, object, name),
      LayoutId::kAttributeError, "'NoneType' object has no attribute 'xxx'"));
}

TEST(ObjectBuiltinsTest, DunderSetattrSetsValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C: pass
i = C()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));
  Object name(&scope, runtime.newStrFromCStr("foo"));
  Object value(&scope, runtime.newInt(42));
  EXPECT_TRUE(
      runBuiltin(ObjectBuiltins::dunderSetattr, i, name, value).isNoneType());
  ASSERT_TRUE(i.isHeapObject());
  HeapObject i_heap_object(&scope, *i);
  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread, i_heap_object, name), 42));
}

TEST(ObjectBuiltinsTest, DunderSetattrWithNonStringNameRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object object(&scope, NoneType::object());
  Object name(&scope, runtime.newInt(0));
  Object value(&scope, runtime.newInt(1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ObjectBuiltins::dunderSetattr, object, name, value),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST(ObjectBuiltinsTest, DunderSetattrOnBuiltinTypeRaisesAttributeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, NoneType::object());
  Object name(&scope, runtime.newStrFromCStr("foo"));
  Object value(&scope, runtime.newInt(1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ObjectBuiltins::dunderSetattr, object, name, value),
      LayoutId::kAttributeError, "'NoneType' object has no attribute 'foo'"));
}

TEST(ObjectBuiltinsTest, DunderSizeofWithNonHeapObjectReturnsSizeofRawObject) {
  Runtime runtime;
  HandleScope scope;
  Object small_int(&scope, SmallInt::fromWord(6));
  Object result(&scope, runBuiltin(ObjectBuiltins::dunderSizeof, small_int));
  EXPECT_TRUE(isIntEqualsWord(*result, kPointerSize));
}

TEST(ObjectBuiltinsTest, DunderSizeofWithLargeStrReturnsSizeofHeapObject) {
  Runtime runtime;
  HandleScope scope;
  HeapObject large_str(&scope, runtime.heap()->createLargeStr(40));
  Object result(&scope, runBuiltin(ObjectBuiltins::dunderSizeof, large_str));
  EXPECT_TRUE(isIntEqualsWord(*result, large_str.size()));
}

TEST(
    ObjectBuiltinsTest,
    DunderNeWithSelfImplementingDunderEqReturningNotImplementedReturnsNotImplemented) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo():
  def __eq__(self, b): return NotImplemented

result = object.__ne__(Foo(), None)
)")
                   .isError());
  EXPECT_TRUE(moduleAt(&runtime, "__main__", "result").isNotImplementedType());
}

TEST(ObjectBuiltinsTest,
     DunderNeWithSelfImplementingDunderEqReturningZeroReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo():
  def __eq__(self, b): return 0

result = object.__ne__(Foo(), None)
)")
                   .isError());
  // 0 is converted to False, and flipped again for __ne__ from __eq__.
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(ObjectBuiltinsTest,
     DunderNeWithSelfImplementingDunderEqReturningOneReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo():
  def __eq__(self, b): return 1

result = object.__ne__(Foo(), None)
)")
                   .isError());
  // 1 is converted to True, and flipped again for __ne__ from __eq__.
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(ObjectBuiltinsTest,
     DunderNeWithSelfImplementingDunderEqReturningFalseReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo():
  def __eq__(self, b): return False

result = object.__ne__(Foo(), None)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(ObjectBuiltinsTest,
     DunderNeWithSelfImplementingDunderEqReturningTrueReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo():
  def __eq__(self, b): return True

result = object.__ne__(Foo(), None)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(ObjectBuiltinsTest, DunderStrReturnsDunderRepr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  pass

f = Foo()
a = object.__str__(f)
b = object.__repr__(f)
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(isStrEquals(a, b));
}

TEST(ObjectBuiltinsTest, UserDefinedTypeInheritsDunderStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  pass

f = Foo()
a = object.__str__(f)
b = f.__str__()
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(isStrEquals(a, b));
}

TEST(ObjectBuiltinsTest, DunderInitDoesNotRaiseIfNewIsDifferentButInitIsSame) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __new__(cls):
    return object.__new__(cls)

Foo.__init__(Foo(), 1)
)")
                   .isError());
  // It doesn't matter what the output is, just that it doesn't throw a
  // TypeError.
}

TEST(ObjectBuiltinsTest, DunderInitWithNonInstanceIsOk) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
object.__init__(object)
)")
                   .isError());
  // It doesn't matter what the output is, just that it doesn't throw a
  // TypeError.
}

TEST(ObjectBuiltinsTest, DunderInitWithNoArgsRaisesTypeError) {
  Runtime runtime;
  // Passing no args to object.__init__ should throw a type error.
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, R"(
object.__init__()
)"),
      LayoutId::kTypeError,
      "TypeError: 'object.__init__' takes 1 positional arguments but 0 given"));
}

TEST(ObjectBuiltinsTest, DunderInitWithArgsRaisesTypeError) {
  Runtime runtime;
  // Passing extra args to object.__init__, without overwriting __new__,
  // should throw a type error.
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
  pass

Foo.__init__(Foo(), 1)
)"),
                            LayoutId::kTypeError,
                            "object.__init__() takes no parameters"));
}

TEST(ObjectBuiltinsTest, DunderInitWithNewAndInitRaisesTypeError) {
  Runtime runtime;
  // Passing extra args to object.__init__, and overwriting only __init__,
  // should throw a type error.
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
  def __init__(self):
    object.__init__(self, 1)

Foo()
)"),
                            LayoutId::kTypeError,
                            "object.__init__() takes no parameters"));
}

TEST(NoneBuiltinsTest, NewReturnsNone) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kNoneType));
  EXPECT_TRUE(runBuiltin(NoneBuiltins::dunderNew, type).isNoneType());
}

TEST(NoneBuiltinsTest, NewWithExtraArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raised(runFromCStr(&runtime, "NoneType.__new__(NoneType, 1, 2, 3, 4, 5)"),
             LayoutId::kTypeError));
}

TEST(NoneBuiltinsTest, DunderReprIsBoundMethod) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "a = None.__repr__").isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(a.isBoundMethod());
}

TEST(NoneBuiltinsTest, DunderReprReturnsNone) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "a = None.__repr__()").isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "None"));
}

TEST(NoneBuiltinsTest, BuiltinBaseIsNone) {
  Runtime runtime;
  HandleScope scope;
  Type none_type(&scope, runtime.typeAt(LayoutId::kNoneType));
  EXPECT_EQ(none_type.builtinBase(), LayoutId::kNoneType);
}

TEST(ObjectBuiltinsTest, ObjectGetAttributeReturnsInstanceValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C: pass
c = C()
c.__hash__ = 42
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object name(&scope, runtime.newStrFromCStr("__hash__"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread, c, name), 42));
}

TEST(ObjectBuiltinsTest, ObjectGetAttributeReturnsTypeValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  x = -11
c = C()
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object name(&scope, runtime.newStrFromCStr("x"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread, c, name), -11));
}

TEST(ObjectBuiltinsTest, ObjectGetAttributeWithNonExistentNameReturnsError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C: pass
c = C()
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object name(&scope, runtime.newStrFromCStr("xxx"));
  EXPECT_TRUE(objectGetAttribute(thread, c, name).isError());
  EXPECT_FALSE(thread->hasPendingException());
}

TEST(ObjectBuiltinsTest, ObjectGetAttributeCallsDunderGetOnDataDescriptor) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): return 42
class A:
  foo = D()
a = A()
)")
                   .isError());
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread, a, foo), 42));
}

TEST(ObjectBuiltinsTest, ObjectGetAttributeCallsDunderGetOnNonDataDescriptor) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class D:
  def __get__(self, instance, owner): return 42
class A:
  foo = D()
a = A()
)")
                   .isError());
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread, a, foo), 42));
}

TEST(ObjectBuiltinsTest,
     ObjectGetAttributePrefersDataDescriptorOverInstanceAttr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread, a, foo), 42));
}

TEST(ObjectBuiltinsTest,
     ObjectGetAttributePrefersInstanceAttrOverNonDataDescriptor) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class D:
  def __get__(self, instance, owner): return 42
class A:
  foo = D()
a = A()
a.foo = 12
)")
                   .isError());
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread, a, foo), 12));
}

TEST(ObjectBuiltinsTest, ObjectGetAttributePropagatesDunderGetException) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): raise UserWarning()
class A:
  foo = D()
a = A()
)")
                   .isError());
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(
      raised(objectGetAttribute(thread, a, foo), LayoutId::kUserWarning));
}

TEST(ObjectBuiltinsTest,
     ObjectGetAttributeOnNoneNonDataDescriptorReturnsBoundMethod) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object none(&scope, NoneType::object());
  Object attr_name(&scope, runtime.newStrFromCStr("__repr__"));
  EXPECT_TRUE(objectGetAttribute(thread, none, attr_name).isBoundMethod());
}

TEST(ObjectBuiltinsTest,
     ObjectGetAttributeSetLocationReturnsBoundMethodAndCachesFunction) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def foo():
    pass
foo = C.foo
i = C()
)")
                   .isError());
  Object foo(&scope, moduleAt(&runtime, "__main__", "foo"));
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));

  Object name(&scope, runtime.newStrFromCStr("foo"));
  Object to_cache(&scope, NoneType::object());
  Object result_obj(&scope,
                    objectGetAttributeSetLocation(thread, i, name, &to_cache));
  ASSERT_TRUE(result_obj.isBoundMethod());
  BoundMethod result(&scope, *result_obj);
  EXPECT_EQ(result.function(), foo);
  EXPECT_EQ(result.self(), i);
  EXPECT_EQ(to_cache, foo);

  Object load_cached_result_obj(
      &scope, Interpreter::loadAttrWithLocation(thread, *i, *to_cache));
  ASSERT_TRUE(load_cached_result_obj.isBoundMethod());
  BoundMethod load_cached_result(&scope, *load_cached_result_obj);
  EXPECT_EQ(load_cached_result.function(), foo);
  EXPECT_EQ(load_cached_result.self(), i);
}

TEST(ObjectBuiltinsTest,
     ObjectGetAttributeSetLocationReturnsInstanceVariableAndCachesOffset) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __init__(self):
    self.foo = 42
i = C()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));

  Layout layout(&scope, runtime.layoutAt(i.layoutId()));
  Object name(&scope, runtime.newStrFromCStr("foo"));
  AttributeInfo info;
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout, name, &info));
  ASSERT_TRUE(info.isInObject());

  Object to_cache(&scope, NoneType::object());
  EXPECT_TRUE(isIntEqualsWord(
      objectGetAttributeSetLocation(thread, i, name, &to_cache), 42));
  EXPECT_TRUE(isIntEqualsWord(*to_cache, info.offset()));

  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrWithLocation(thread, *i, *to_cache), 42));
}

TEST(
    ObjectBuiltinsTest,
    ObjectGetAttributeSetLocationReturnsInstanceVariableAndCachesNegativeOffset) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  pass
i = C()
i.foo = 17
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));

  Layout layout(&scope, runtime.layoutAt(i.layoutId()));
  Object name(&scope, runtime.newStrFromCStr("foo"));
  AttributeInfo info;
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout, name, &info));
  ASSERT_TRUE(info.isOverflow());

  Object to_cache(&scope, NoneType::object());
  EXPECT_TRUE(isIntEqualsWord(
      objectGetAttributeSetLocation(thread, i, name, &to_cache), 17));
  EXPECT_TRUE(isIntEqualsWord(*to_cache, -info.offset() - 1));

  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrWithLocation(thread, *i, *to_cache), 17));
}

TEST(ObjectBuiltinsTest,
     ObjectGetAttributeSetLocationRaisesAttributeErrorAndDoesNotSetLocation) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  pass
i = C()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));

  Object name(&scope, runtime.newStrFromCStr("xxx"));
  Object to_cache(&scope, NoneType::object());
  EXPECT_TRUE(
      objectGetAttributeSetLocation(thread, i, name, &to_cache).isError());
  EXPECT_TRUE(to_cache.isNoneType());
}

TEST(ObjectBuiltinsTest, ObjectSetAttrSetsInstanceValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C: pass
i = C()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));
  Object name(&scope, runtime.internStrFromCStr(thread, "foo"));
  Object value(&scope, runtime.newInt(47));
  EXPECT_TRUE(objectSetAttr(thread, i, name, value).isNoneType());
  EXPECT_TRUE(isIntEqualsWord(objectGetAttribute(thread, i, name), 47));
}

TEST(ObjectBuiltinsTest, ObjectSetAttrOnDataDescriptorCallsDunderSet) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));
  Object foo_descr(&scope, moduleAt(&runtime, "__main__", "foo_descr"));
  Object name(&scope, runtime.internStrFromCStr(thread, "foo"));
  Object value(&scope, runtime.newInt(47));
  EXPECT_TRUE(objectSetAttr(thread, i, name, value).isNoneType());
  Object set_args_obj(&scope, moduleAt(&runtime, "__main__", "set_args"));
  ASSERT_TRUE(set_args_obj.isTuple());
  Tuple dunder_set_args(&scope, *set_args_obj);
  ASSERT_EQ(dunder_set_args.length(), 3);
  EXPECT_EQ(dunder_set_args.at(0), foo_descr);
  EXPECT_EQ(dunder_set_args.at(1), i);
  EXPECT_TRUE(isIntEqualsWord(dunder_set_args.at(2), 47));
}

TEST(ObjectBuiltinsTest, ObjectSetAttrPropagatesErrorsInDunderSet) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class D:
  def __set__(self, instance, value): raise UserWarning()
  def __get__(self, instance, owner): pass
class C:
  foo = D()
i = C()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));
  Object name(&scope, runtime.internStrFromCStr(thread, "foo"));
  Object value(&scope, runtime.newInt(1));
  EXPECT_TRUE(
      raised(objectSetAttr(thread, i, name, value), LayoutId::kUserWarning));
}

TEST(ObjectBuiltinsTest, ObjectSetAttrOnNonHeapObjectRaisesAttributeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, runtime.newInt(42));
  Object name(&scope, runtime.internStrFromCStr(thread, "foo"));
  Object value(&scope, runtime.newInt(1));
  EXPECT_TRUE(raisedWithStr(objectSetAttr(thread, object, name, value),
                            LayoutId::kAttributeError,
                            "'int' object has no attribute 'foo'"));
}

TEST(ObjectBuiltinsTest, ObjectSetAttrSetLocationSetsValueCachesOffset) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __init__(self):
    self.foo = 0
i = C()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));
  Object name(&scope, runtime.internStrFromCStr(thread, "foo"));

  AttributeInfo info;
  Layout layout(&scope, runtime.layoutAt(i.layoutId()));
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout, name, &info));
  ASSERT_TRUE(info.isInObject());

  Object value(&scope, runtime.newInt(7));
  Object value2(&scope, runtime.newInt(99));
  Object to_cache(&scope, NoneType::object());
  EXPECT_TRUE(
      objectSetAttrSetLocation(thread, i, name, value, &to_cache).isNoneType());
  EXPECT_TRUE(isIntEqualsWord(*to_cache, info.offset()));
  ASSERT_TRUE(i.isHeapObject());
  HeapObject heap_object(&scope, *i);
  EXPECT_TRUE(
      isIntEqualsWord(heap_object.instanceVariableAt(info.offset()), 7));

  Interpreter::storeAttrWithLocation(thread, *i, *to_cache, *value2);
  EXPECT_TRUE(
      isIntEqualsWord(heap_object.instanceVariableAt(info.offset()), 99));
}

TEST(ObjectBuiltinsTest,
     ObjectSetAttrSetLocationSetsOverflowValueCachesOffset) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C: pass
i = C()
i.foo = 0
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));
  Object name(&scope, runtime.internStrFromCStr(thread, "foo"));

  AttributeInfo info;
  Layout layout(&scope, runtime.layoutAt(i.layoutId()));
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout, name, &info));
  ASSERT_TRUE(info.isOverflow());

  Object value(&scope, runtime.newInt(-8));
  Object value2(&scope, runtime.newInt(11));
  Object to_cache(&scope, NoneType::object());
  EXPECT_TRUE(
      objectSetAttrSetLocation(thread, i, name, value, &to_cache).isNoneType());
  EXPECT_TRUE(isIntEqualsWord(*to_cache, -info.offset() - 1));
  ASSERT_TRUE(i.isHeapObject());
  HeapObject heap_object(&scope, *i);
  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread, heap_object, name), -8));

  Interpreter::storeAttrWithLocation(thread, *i, *to_cache, *value2);
  EXPECT_TRUE(
      isIntEqualsWord(instanceGetAttribute(thread, heap_object, name), 11));
}

}  // namespace python
