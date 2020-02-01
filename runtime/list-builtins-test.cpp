#include "list-builtins.h"

#include "gtest/gtest.h"

#include "builtins-module.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {

using namespace testing;

using ListBuiltinsTest = RuntimeFixture;
using ListIteratorBuiltinsTest = RuntimeFixture;

TEST_F(ListBuiltinsTest, CopyWithListReturnsNewInstance) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = [1, 2, 3]
result = list.copy(l)
)")
                   .isError());
  HandleScope scope(thread_);
  Object list(&scope, mainModuleAt(runtime_, "l"));
  EXPECT_TRUE(list.isList());
  Object result_obj(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(result_obj.isList());
  List result(&scope, *result_obj);
  EXPECT_NE(*list, *result);
  EXPECT_EQ(result.numItems(), 3);
}

TEST_F(ListBuiltinsTest, DunderEqReturnsTrue) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = list.__eq__([1, 2, 3], [1, 2, 3])
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(runtime_, "result"), RawBool::trueObj());
}

TEST_F(ListBuiltinsTest, DunderEqWithSameListReturnsTrue) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
    def __eq__(self, other):
        return False
a = [1, 2, 3]
result = list.__eq__(a, a)
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(runtime_, "result"), RawBool::trueObj());
}

TEST_F(ListBuiltinsTest, DunderEqWithSameIdentityElementsReturnsTrue) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
nan = float("nan")
result = list.__eq__([nan], [nan])
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(runtime_, "result"), RawBool::trueObj());
}

TEST_F(ListBuiltinsTest, DunderEqWithEqualElementsReturnsTrue) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
    def __init__(self, value):
        self.value = value
    def __eq__(self, other):
        return type(self.value).__eq__(self.value, other.value)
a = Foo(1)
b = Foo(1)
result = list.__eq__([a], [b])
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(runtime_, "result"), RawBool::trueObj());
}

TEST_F(ListBuiltinsTest, DunderEqWithDifferentSizeReturnsFalse) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = list.__eq__([1, 2, 3], [1, 2])
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(runtime_, "result"), RawBool::falseObj());
}

TEST_F(ListBuiltinsTest, DunderEqWithDifferentValuesReturnsFalse) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = list.__eq__([1, 2, 3], [1, 2, 4])
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(runtime_, "result"), RawBool::falseObj());
}

TEST_F(ListBuiltinsTest, DunderEqWithNonListRhsReturnsNotImplemented) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = list.__eq__([1, 2, 3], (1, 2, 3));
)")
                   .isError());
  EXPECT_TRUE(mainModuleAt(runtime_, "result").isNotImplementedType());
}

TEST_F(ListBuiltinsTest, DunderInitFromList) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = list([1, 2])
)")
                   .isError());
  HandleScope scope(thread_);
  List a(&scope, mainModuleAt(runtime_, "a"));
  ASSERT_EQ(a.numItems(), 2);
  EXPECT_TRUE(isIntEqualsWord(a.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(a.at(1), 2));
}

TEST_F(ListBuiltinsTest, NewListIsNotASingleton) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = list() is not list()
b = list([1, 2]) is not list([1, 2])
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, mainModuleAt(runtime_, "a"));
  Bool b(&scope, mainModuleAt(runtime_, "b"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
}

TEST_F(ListBuiltinsTest, AddToNonEmptyList) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = [1, 2]
b = [3, 4, 5]
c = a + b
)")
                   .isError());
  Object c(&scope, mainModuleAt(runtime_, "c"));
  ASSERT_TRUE(c.isList());
  List list(&scope, List::cast(*c));
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(list.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(list.at(2), 3));
  EXPECT_TRUE(isIntEqualsWord(list.at(3), 4));
  EXPECT_TRUE(isIntEqualsWord(list.at(4), 5));
}

TEST_F(ListBuiltinsTest, AddToEmptyList) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = []
b = [1, 2, 3]
c = a + b
)")
                   .isError());
  Object c(&scope, mainModuleAt(runtime_, "c"));
  ASSERT_TRUE(c.isList());
  List list(&scope, List::cast(*c));
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(list.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(list.at(2), 3));
}

TEST_F(ListBuiltinsTest, AddWithListSubclassReturnsList) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(list):
  pass
a = []
b = C([1, 2, 3])
c = a + b
)")
                   .isError());
  Object c(&scope, mainModuleAt(runtime_, "c"));
  EXPECT_PYLIST_EQ(c, {1, 2, 3});
}

TEST_F(ListBuiltinsTest, AddWithNonListSelfRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "list.__add__(None, [])"), LayoutId::kTypeError,
      "'__add__' requires a 'list' object but got 'NoneType'"));
}

TEST_F(ListBuiltinsTest, AddListToTupleRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
a = [1, 2, 3]
b = (4, 5, 6)
c = a + b
)"),
                            LayoutId::kTypeError,
                            "can only concatenate list to list"));
}

TEST_F(ListBuiltinsTest, ListAppend) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = list()
b = list()
a.append(1)
a.append("2")
b.append(3)
a.append(b)
)")
                   .isError());
  HandleScope scope(thread_);
  List a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_TRUE(isIntEqualsWord(a.at(0), 1));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "2"));
  List b(&scope, a.at(2));
  EXPECT_TRUE(isIntEqualsWord(b.at(0), 3));
}

TEST_F(ListBuiltinsTest, DunderContainsWithContainedElementReturnsTrue) {
  HandleScope scope(thread_);
  Int value0(&scope, runtime_->newInt(1));
  Bool value1(&scope, RawBool::falseObj());
  Str value2(&scope, runtime_->newStrFromCStr("hello"));
  List list(&scope, runtime_->newList());
  runtime_->listAdd(thread_, list, value0);
  runtime_->listAdd(thread_, list, value1);
  runtime_->listAdd(thread_, list, value2);
  EXPECT_EQ(runBuiltin(METH(list, __contains__), list, value0),
            RawBool::trueObj());
  EXPECT_EQ(runBuiltin(METH(list, __contains__), list, value1),
            RawBool::trueObj());
  EXPECT_EQ(runBuiltin(METH(list, __contains__), list, value2),
            RawBool::trueObj());
}

TEST_F(ListBuiltinsTest, DunderContainsWithUncontainedElementReturnsFalse) {
  HandleScope scope(thread_);
  Int value0(&scope, runtime_->newInt(7));
  NoneType value1(&scope, RawNoneType::object());
  List list(&scope, runtime_->newList());
  runtime_->listAdd(thread_, list, value0);
  runtime_->listAdd(thread_, list, value1);
  Int value2(&scope, runtime_->newInt(42));
  Bool value3(&scope, RawBool::trueObj());
  EXPECT_EQ(runBuiltin(METH(list, __contains__), list, value2),
            RawBool::falseObj());
  EXPECT_EQ(runBuiltin(METH(list, __contains__), list, value3),
            RawBool::falseObj());
}

TEST_F(ListBuiltinsTest, DunderContainsWithIdenticalObjectReturnsTrue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  def __eq__(self, other):
    return False
value = Foo()
list = [value]
)")
                   .isError());
  Object value(&scope, mainModuleAt(runtime_, "value"));
  List list(&scope, mainModuleAt(runtime_, "list"));
  EXPECT_EQ(runBuiltin(METH(list, __contains__), list, value),
            RawBool::trueObj());
}

TEST_F(ListBuiltinsTest,
       DunderContainsWithNonIdenticalEqualKeyObjectReturnsTrue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  def __eq__(self, other):
    return True
value = Foo()
list = [None]
)")
                   .isError());
  Object value(&scope, mainModuleAt(runtime_, "value"));
  List list(&scope, mainModuleAt(runtime_, "list"));
  EXPECT_EQ(runBuiltin(METH(list, __contains__), list, value),
            RawBool::trueObj());
}

TEST_F(ListBuiltinsTest,
       DunderContainsWithNonIdenticalEqualListObjectReturnsFalse) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  def __eq__(self, other):
    return True
class Bar:
  def __eq__(self, other):
    return False
value0 = Foo()
value1 = Bar()
list = [value0]
)")
                   .isError());
  Object value1(&scope, mainModuleAt(runtime_, "value1"));
  List list(&scope, mainModuleAt(runtime_, "list"));
  EXPECT_EQ(runBuiltin(METH(list, __contains__), list, value1),
            RawBool::falseObj());
}

TEST_F(ListBuiltinsTest, DunderContainsWithRaisingEqPropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  def __eq__(self, other):
    raise UserWarning("")
value = Foo()
list = [None]
)")
                   .isError());
  Object value(&scope, mainModuleAt(runtime_, "value"));
  List list(&scope, mainModuleAt(runtime_, "list"));
  Object result(&scope, runBuiltin(METH(list, __contains__), list, value));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST_F(ListBuiltinsTest,
       DunderContainsWithRaisingDunderBoolPropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  def __bool__(self):
    raise UserWarning("")
class Bar:
  def __eq__(self, other):
    return Foo()
value = Bar()
list = [None]
)")
                   .isError());
  Object value(&scope, mainModuleAt(runtime_, "value"));
  List list(&scope, mainModuleAt(runtime_, "list"));
  EXPECT_TRUE(raised(runBuiltin(METH(list, __contains__), list, value),
                     LayoutId::kUserWarning));
}

TEST_F(ListBuiltinsTest, DunderContainsWithNonListSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Int i(&scope, SmallInt::fromWord(3));
  Object result(&scope, runBuiltin(METH(list, __contains__), i, i));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(ListBuiltinsTest, ListInsertWithMissingArgumentsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "[1, 2].insert()"), LayoutId::kTypeError,
      "'list.insert' takes min 3 positional arguments but 1 given"));
}

TEST_F(ListBuiltinsTest, ListInsertWithNonListRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "list.insert(None, 1, None)"), LayoutId::kTypeError,
      "'insert' requires a 'list' object but got 'NoneType'"));
}

TEST_F(ListBuiltinsTest, ListInsertWithNonIntIndexRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "[1, 2].insert({}, 3)"), LayoutId::kTypeError,
      "'dict' object cannot be interpreted as an integer"));
}

TEST_F(ListBuiltinsTest, ListInsertWithLargeIntIndexRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "[1, 2].insert(2 ** 63, 1)"),
                            LayoutId::kOverflowError,
                            "Python int too large to convert to C ssize_t"));
}

TEST_F(ListBuiltinsTest, ListInsertWithBoolIndexInsertsAtInt) {
  HandleScope scope(thread_);
  List self(&scope, runtime_->newList());
  Object value(&scope, SmallInt::fromWord(3));
  runtime_->listAdd(thread_, self, value);
  runtime_->listAdd(thread_, self, value);
  Object fals(&scope, Bool::falseObj());
  Object tru(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(METH(list, insert), self, tru, tru));
  EXPECT_EQ(result, NoneType::object());
  result = runBuiltin(METH(list, insert), self, fals, fals);
  EXPECT_EQ(result, NoneType::object());
  ASSERT_EQ(self.numItems(), 4);
  EXPECT_EQ(self.at(0), fals);
  EXPECT_EQ(self.at(1), value);
  EXPECT_EQ(self.at(2), tru);
  EXPECT_EQ(self.at(3), value);
}

TEST_F(ListBuiltinsTest, ListInsertWithIntSubclassInsertsAtInt) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class N(int):
  pass
a = [0, 0, 0, 0, 0]
b = N(3)
)")
                   .isError());
  HandleScope scope(thread_);
  List self(&scope, mainModuleAt(runtime_, "a"));
  Object index(&scope, mainModuleAt(runtime_, "b"));
  Object value(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(METH(list, insert), self, index, value));
  EXPECT_EQ(result, NoneType::object());
  EXPECT_EQ(self.numItems(), 6);
  EXPECT_EQ(self.at(3), value);
}

TEST_F(ListBuiltinsTest, ListRemove) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = [5, 4, 3, 2, 1]
a.remove(2)
a.remove(5)
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_PYLIST_EQ(a, {4, 3, 1});
}

TEST_F(ListBuiltinsTest, ListRemoveWithDuplicateItemsRemovesFirstMatchingItem) {
  HandleScope scope(thread_);
  Int value0(&scope, runtime_->newInt(0));
  Int value1(&scope, runtime_->newInt(1));
  Int value2(&scope, runtime_->newInt(2));
  List list(&scope, runtime_->newList());
  runtime_->listAdd(thread_, list, value0);
  runtime_->listAdd(thread_, list, value1);
  runtime_->listAdd(thread_, list, value2);
  runtime_->listAdd(thread_, list, value1);
  runtime_->listAdd(thread_, list, value0);

  EXPECT_EQ(list.numItems(), 5);
  runBuiltin(METH(list, remove), list, value1);
  ASSERT_EQ(list.numItems(), 4);
  EXPECT_EQ(list.at(0), value0);
  EXPECT_EQ(list.at(1), value2);
  EXPECT_EQ(list.at(2), value1);
  EXPECT_EQ(list.at(3), value0);
}

TEST_F(ListBuiltinsTest, ListRemoveWithIdenticalObjectGetsRemoved) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __eq__(self, other):
    return False
value = C()
list = [value]
)")
                   .isError());
  Object value(&scope, mainModuleAt(runtime_, "value"));
  List list(&scope, mainModuleAt(runtime_, "list"));
  EXPECT_EQ(list.numItems(), 1);
  runBuiltin(METH(list, remove), list, value);
  EXPECT_EQ(list.numItems(), 0);
}

TEST_F(ListBuiltinsTest,
       ListRemoveWithNonIdenticalEqualObjectInListGetsRemoved) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __eq__(self, other):
    return True
list = [C()]
)")
                   .isError());
  Object value(&scope, NoneType::object());
  List list(&scope, mainModuleAt(runtime_, "list"));
  EXPECT_EQ(list.numItems(), 1);
  runBuiltin(METH(list, remove), list, value);
  EXPECT_EQ(list.numItems(), 0);
}

TEST_F(ListBuiltinsTest,
       ListRemoveWithNonIdenticalEqualObjectAsKeyRaisesValueError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __eq__(self, other):
    return True
class D:
  def __eq__(self, other):
    return False
value = C()
list = [D()]
)")
                   .isError());
  Object value(&scope, mainModuleAt(runtime_, "value"));
  List list(&scope, mainModuleAt(runtime_, "list"));
  Object result(&scope, runBuiltin(METH(list, remove), list, value));
  EXPECT_TRUE(raised(*result, LayoutId::kValueError));
}

TEST_F(ListBuiltinsTest, ListRemoveWithRaisingDunderEqualPropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  def __eq__(self, other):
    raise UserWarning('')
value = Foo()
list = [None]
)")
                   .isError());
  Object value(&scope, mainModuleAt(runtime_, "value"));
  List list(&scope, mainModuleAt(runtime_, "list"));
  Object result(&scope, runBuiltin(METH(list, remove), list, value));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST_F(ListBuiltinsTest, ListRemoveWithRaisingDunderBoolPropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __bool__(self):
    raise UserWarning('foo')
class D:
  def __eq__(self, other):
    return C()
value = D()
list = [None]
)")
                   .isError());
  Object value(&scope, mainModuleAt(runtime_, "value"));
  List list(&scope, mainModuleAt(runtime_, "list"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(list, remove), list, value),
                            LayoutId::kUserWarning, "foo"));
}

TEST_F(ListBuiltinsTest, ReplicateList) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = [1, 2, 3] * 3
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3, 1, 2, 3, 1, 2, 3});
}

TEST_F(ListBuiltinsTest, ReplicateListWithNegativeRhsReturnsEmptyList) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = [1, 2, 3] * -3
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {});
}

TEST_F(ListBuiltinsTest, SliceWithPositiveStepReturnsForwardsList) {
  HandleScope scope(thread_);
  List list1(&scope, listFromRange(1, 6));

  // Test [2:]
  List test(&scope, listSlice(thread_, list1, 2, 5, 1));
  ASSERT_EQ(test.numItems(), 3);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(test.at(1), 4));
  EXPECT_TRUE(isIntEqualsWord(test.at(2), 5));
}

TEST_F(ListBuiltinsTest, SliceWithNegativeStepReturnsBackwardsList) {
  HandleScope scope(thread_);
  List list1(&scope, listFromRange(1, 6));

  // Test [::-2]
  List test(&scope, listSlice(thread_, list1, 4, -1, -2));
  ASSERT_EQ(test.numItems(), 3);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 5));
  EXPECT_TRUE(isIntEqualsWord(test.at(1), 3));
  EXPECT_TRUE(isIntEqualsWord(test.at(2), 1));
}

TEST_F(ListBuiltinsTest, IdenticalSliceIsCopy) {
  HandleScope scope(thread_);
  List list1(&scope, listFromRange(1, 6));

  // Test: t[::] is t
  List test(&scope, listSlice(thread_, list1, 0, 5, 1));
  ASSERT_EQ(test.numItems(), 5);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(test.at(4), 5));
  ASSERT_NE(*test, *list1);
}

TEST_F(ListBuiltinsTest, DelitemWithInvalidNegativeIndexRaisesIndexError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
l = [1, 2, 3]
del l[-4]
)"),
                            LayoutId::kIndexError,
                            "list assignment index out of range"));
}

TEST_F(ListBuiltinsTest, DelitemWithInvalidIndexRaisesIndexError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
l = [1, 2, 3]
del l[5]
)"),
                            LayoutId::kIndexError,
                            "list assignment index out of range"));
}

TEST_F(ListBuiltinsTest, DelitemWithTooFewArgumentsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, R"(
[].__delitem__()
)"),
      LayoutId::kTypeError,
      "'list.__delitem__' takes min 2 positional arguments but 1 given"));
}

TEST_F(ListBuiltinsTest, DelitemWithTooManyArgumentsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, R"(
[].__delitem__(1, 2)
)"),
      LayoutId::kTypeError,
      "'list.__delitem__' takes max 2 positional arguments but 3 given"));
}

TEST_F(ListBuiltinsTest, DelitemWithNonIntegralIndexRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
[].__delitem__("test")
)"),
                            LayoutId::kTypeError,
                            "list indices must be integers or slices"));
}

TEST_F(ListBuiltinsTest, NonTypeInDunderNew) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
list.__new__(1)
)"),
                            LayoutId::kTypeError, "not a type object"));
}

TEST_F(ListBuiltinsTest, NonSubclassInDunderNew) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class Foo: pass
list.__new__(Foo)
)"),
                            LayoutId::kTypeError, "not a subtype of list"));
}

TEST_F(ListBuiltinsTest, SubclassList) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo():
  def __init__(self):
    self.a = "a"
class Bar(Foo, list): pass
a = Bar()
a.append(1)
test1, test2 = a[0], a.a
a.insert(0, 2)
test3, test4 = a[0], a[1]
a.pop()
test5 = a[0]
a.remove(2)
test6 = len(a)
)")
                   .isError());
  Object test1(&scope, mainModuleAt(runtime_, "test1"));
  Object test2(&scope, mainModuleAt(runtime_, "test2"));
  Object test3(&scope, mainModuleAt(runtime_, "test3"));
  Object test4(&scope, mainModuleAt(runtime_, "test4"));
  Object test5(&scope, mainModuleAt(runtime_, "test5"));
  Object test6(&scope, mainModuleAt(runtime_, "test6"));
  EXPECT_EQ(*test1, SmallInt::fromWord(1));
  EXPECT_EQ(*test2, SmallStr::fromCStr("a"));
  EXPECT_EQ(*test3, SmallInt::fromWord(2));
  EXPECT_EQ(*test4, SmallInt::fromWord(1));
  EXPECT_EQ(*test5, SmallInt::fromWord(2));
  EXPECT_EQ(*test6, SmallInt::fromWord(0));
}

TEST_F(ListBuiltinsTest, Delitem) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = [42,'foo', 'bar']
del a[2]
del a[0]
l = len(a)
e = a[0]
)")
                   .isError());
  Object len(&scope, mainModuleAt(runtime_, "l"));
  Object element(&scope, mainModuleAt(runtime_, "e"));
  EXPECT_EQ(*len, SmallInt::fromWord(1));
  EXPECT_EQ(*element, SmallStr::fromCStr("foo"));
}

TEST_F(ListBuiltinsTest, DunderIterReturnsListIter) {
  HandleScope scope(thread_);
  List empty_list(&scope, listFromRange(0, 0));
  Object iter(&scope, runBuiltin(METH(list, __iter__), empty_list));
  ASSERT_TRUE(iter.isListIterator());
}

TEST_F(ListIteratorBuiltinsTest, CallDunderNext) {
  HandleScope scope(thread_);
  List empty_list(&scope, listFromRange(0, 2));
  Object iter(&scope, runBuiltin(METH(list, __iter__), empty_list));
  ASSERT_TRUE(iter.isListIterator());

  Object item1(&scope, runBuiltin(METH(list_iterator, __next__), iter));
  EXPECT_TRUE(isIntEqualsWord(*item1, 0));

  Object item2(&scope, runBuiltin(METH(list_iterator, __next__), iter));
  EXPECT_TRUE(isIntEqualsWord(*item2, 1));
}

TEST_F(ListIteratorBuiltinsTest, DunderIterReturnsSelf) {
  HandleScope scope(thread_);
  List empty_list(&scope, listFromRange(0, 0));
  Object iter(&scope, runBuiltin(METH(list, __iter__), empty_list));
  ASSERT_TRUE(iter.isListIterator());

  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(METH(list_iterator, __iter__), iter));
  ASSERT_EQ(*result, *iter);
}

TEST_F(ListIteratorBuiltinsTest, DunderLengthHintOnEmptyListIterator) {
  HandleScope scope(thread_);
  List empty_list(&scope, listFromRange(0, 0));
  Object iter(&scope, runBuiltin(METH(list, __iter__), empty_list));
  ASSERT_TRUE(iter.isListIterator());

  Object length_hint(&scope,
                     runBuiltin(METH(list_iterator, __length_hint__), iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(ListIteratorBuiltinsTest, DunderLengthHintOnConsumedListIterator) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 1));
  Object iter(&scope, runBuiltin(METH(list, __iter__), list));
  ASSERT_TRUE(iter.isListIterator());

  Object length_hint1(&scope,
                      runBuiltin(METH(list_iterator, __length_hint__), iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint1, 1));

  // Consume the iterator
  Object item1(&scope, runBuiltin(METH(list_iterator, __next__), iter));
  EXPECT_TRUE(isIntEqualsWord(*item1, 0));

  Object length_hint2(&scope,
                      runBuiltin(METH(list_iterator, __length_hint__), iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint2, 0));
}

TEST_F(ListBuiltinsTest, InsertToList) {
  HandleScope scope(thread_);
  List list(&scope, runtime_->newList());

  for (int i = 0; i < 9; i++) {
    if (i == 1 || i == 6) {
      continue;
    }
    Object value(&scope, SmallInt::fromWord(i));
    runtime_->listAdd(thread_, list, value);
  }
  EXPECT_FALSE(isIntEqualsWord(list.at(1), 1));
  EXPECT_FALSE(isIntEqualsWord(list.at(6), 6));

  Object value2(&scope, SmallInt::fromWord(1));
  listInsert(thread_, list, value2, 1);
  Object value12(&scope, SmallInt::fromWord(6));
  listInsert(thread_, list, value12, 6);

  EXPECT_PYLIST_EQ(list, {0, 1, 2, 3, 4, 5, 6, 7, 8});
}

TEST_F(ListBuiltinsTest, InsertToListBounds) {
  HandleScope scope(thread_);
  List list(&scope, runtime_->newList());
  for (int i = 0; i < 10; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime_->listAdd(thread_, list, value);
  }
  ASSERT_EQ(list.numItems(), 10);

  Object value100(&scope, SmallInt::fromWord(100));
  listInsert(thread_, list, value100, 100);
  ASSERT_EQ(list.numItems(), 11);
  EXPECT_TRUE(isIntEqualsWord(list.at(10), 100));

  Object value0(&scope, SmallInt::fromWord(400));
  listInsert(thread_, list, value0, 0);
  ASSERT_EQ(list.numItems(), 12);
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 400));

  Object value_n(&scope, SmallInt::fromWord(-10));
  listInsert(thread_, list, value_n, -10);
  ASSERT_EQ(list.numItems(), 13);
  EXPECT_TRUE(isIntEqualsWord(list.at(2), -10));
}

TEST_F(ListBuiltinsTest, PopList) {
  HandleScope scope(thread_);
  List list(&scope, runtime_->newList());
  for (int i = 0; i < 16; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime_->listAdd(thread_, list, value);
  }
  ASSERT_EQ(list.numItems(), 16);

  // Pop from the end
  RawObject res1 = listPop(thread_, list, 15);
  ASSERT_EQ(list.numItems(), 15);
  EXPECT_TRUE(isIntEqualsWord(list.at(14), 14));
  EXPECT_TRUE(isIntEqualsWord(res1, 15));

  // Pop elements from 5 - 10
  for (int i = 0; i < 5; i++) {
    RawObject res5 = listPop(thread_, list, 5);
    EXPECT_TRUE(isIntEqualsWord(res5, i + 5));
  }
  ASSERT_EQ(list.numItems(), 10);
  for (int i = 0; i < 5; i++) {
    EXPECT_TRUE(isIntEqualsWord(list.at(i), i));
  }
  for (int i = 5; i < 10; i++) {
    EXPECT_TRUE(isIntEqualsWord(list.at(i), i + 5));
  }

  // Pop element 0
  RawObject res0 = listPop(thread_, list, 0);
  ASSERT_EQ(list.numItems(), 9);
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(res0, 0));
}

TEST_F(ListBuiltinsTest, ExtendList) {
  HandleScope scope(thread_);
  List list(&scope, runtime_->newList());
  List list1(&scope, runtime_->newList());
  for (int i = 0; i < 4; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    Object value1(&scope, SmallInt::fromWord(i + 4));
    runtime_->listAdd(thread_, list, value);
    runtime_->listAdd(thread_, list1, value1);
  }
  EXPECT_EQ(list.numItems(), 4);
  Object list1_handle(&scope, *list1);
  listExtend(Thread::current(), list, list1_handle);
  EXPECT_PYLIST_EQ(list, {0, 1, 2, 3, 4, 5, 6, 7});
}

TEST_F(ListBuiltinsTest, ExtendTuple) {
  HandleScope scope(thread_);
  List list(&scope, runtime_->newList());
  Object object_array0(&scope, runtime_->emptyTuple());
  Tuple object_array1(&scope, runtime_->newTuple(1));
  Tuple object_array16(&scope, runtime_->newTuple(16));

  for (int i = 0; i < 4; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime_->listAdd(thread_, list, value);
  }
  listExtend(Thread::current(), list, object_array0);
  EXPECT_EQ(list.numItems(), 4);

  Object object_array1_handle(&scope, *object_array1);
  object_array1.atPut(0, NoneType::object());
  listExtend(Thread::current(), list, object_array1_handle);
  ASSERT_GE(list.numItems(), 5);
  ASSERT_TRUE(list.at(4).isNoneType());

  for (word i = 0; i < 4; i++) {
    object_array16.atPut(i, SmallInt::fromWord(i));
  }

  Object object_array2_handle(&scope, *object_array16);
  listExtend(Thread::current(), list, object_array2_handle);
  ASSERT_GE(list.numItems(), 4 + 1 + 4);
  EXPECT_EQ(list.at(5), SmallInt::fromWord(0));
  EXPECT_EQ(list.at(6), SmallInt::fromWord(1));
  EXPECT_EQ(list.at(7), SmallInt::fromWord(2));
  EXPECT_EQ(list.at(8), SmallInt::fromWord(3));
}

TEST_F(ListBuiltinsTest, SortEmptyListSucceeds) {
  HandleScope scope(thread_);
  List empty(&scope, runtime_->newList());
  ASSERT_EQ(listSort(thread_, empty), NoneType::object());
}

TEST_F(ListBuiltinsTest, SortSingleElementListSucceeds) {
  HandleScope scope(thread_);
  List list(&scope, runtime_->newList());
  Object elt(&scope, SmallInt::fromWord(5));
  runtime_->listAdd(thread_, list, elt);
  ASSERT_EQ(listSort(thread_, list), NoneType::object());
  EXPECT_EQ(list.numItems(), 1);
  EXPECT_EQ(list.at(0), *elt);
}

TEST_F(ListBuiltinsTest, SortMultiElementListSucceeds) {
  HandleScope scope(thread_);
  List list(&scope, runtime_->newList());
  Object elt3(&scope, SmallInt::fromWord(3));
  runtime_->listAdd(thread_, list, elt3);
  Object elt2(&scope, SmallInt::fromWord(2));
  runtime_->listAdd(thread_, list, elt2);
  Object elt1(&scope, SmallInt::fromWord(1));
  runtime_->listAdd(thread_, list, elt1);
  ASSERT_EQ(listSort(thread_, list), NoneType::object());
  EXPECT_EQ(list.numItems(), 3);
  EXPECT_PYLIST_EQ(list, {1, 2, 3});
}

TEST_F(ListBuiltinsTest, SortMultiElementListSucceeds2) {
  HandleScope scope(thread_);
  List list(&scope, runtime_->newList());
  Object elt3(&scope, SmallInt::fromWord(1));
  runtime_->listAdd(thread_, list, elt3);
  Object elt2(&scope, SmallInt::fromWord(3));
  runtime_->listAdd(thread_, list, elt2);
  Object elt1(&scope, SmallInt::fromWord(2));
  runtime_->listAdd(thread_, list, elt1);
  ASSERT_EQ(listSort(thread_, list), NoneType::object());
  EXPECT_EQ(list.numItems(), 3);
  EXPECT_PYLIST_EQ(list, {1, 2, 3});
}

TEST_F(ListBuiltinsTest, SortIsStable) {
  HandleScope scope(thread_);
  List list(&scope, runtime_->newList());
  Object elt4(&scope, runtime_->newStrFromCStr("q"));
  runtime_->listAdd(thread_, list, elt4);
  Object elt3(&scope, runtime_->newStrFromCStr("world"));
  runtime_->listAdd(thread_, list, elt3);
  Object elt2(&scope, runtime_->newStrFromCStr("hello"));
  runtime_->listAdd(thread_, list, elt2);
  Object elt1(&scope, runtime_->newStrFromCStr("hello"));
  runtime_->listAdd(thread_, list, elt1);
  ASSERT_EQ(listSort(thread_, list), NoneType::object());
  EXPECT_EQ(list.numItems(), 4);
  EXPECT_EQ(list.at(0), *elt2);
  EXPECT_EQ(list.at(1), *elt1);
  EXPECT_EQ(list.at(2), *elt4);
  EXPECT_EQ(list.at(3), *elt3);
}

TEST_F(ListBuiltinsTest, ListExtendSelfDuplicatesElements) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = [1, 2, 3]
a.extend(a)
)")
                   .isError());
  HandleScope scope(thread_);
  List a(&scope, mainModuleAt(runtime_, "a"));
  ASSERT_EQ(a.numItems(), 6);
  EXPECT_PYLIST_EQ(a, {1, 2, 3, 1, 2, 3});
}

TEST_F(ListBuiltinsTest, ListExtendListSubclassFallsBackToIter) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(list):
  def __iter__(self):
    return [4, 5, 6].__iter__()
a = [1, 2, 3]
a.extend(C([1,2,3]))
)")
                   .isError());
  HandleScope scope(thread_);
  List a(&scope, mainModuleAt(runtime_, "a"));
  ASSERT_EQ(a.numItems(), 6);
  EXPECT_PYLIST_EQ(a, {1, 2, 3, 4, 5, 6});
}

TEST_F(ListBuiltinsTest, ReverseEmptyListDoesNothing) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = []
result.reverse()
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isList());
  EXPECT_EQ(List::cast(*result).numItems(), 0);
}

TEST_F(ListBuiltinsTest, ReverseOneElementListDoesNothing) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = [2]
result.reverse()
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isList());
  EXPECT_EQ(List::cast(*result).numItems(), 1);
  EXPECT_EQ(List::cast(*result).at(0), SmallInt::fromWord(2));
}

TEST_F(ListBuiltinsTest, ReverseOddManyElementListReversesList) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = [1, 2, 3]
result.reverse()
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {3, 2, 1});
}

TEST_F(ListBuiltinsTest, ReverseEvenManyElementListReversesList) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = [1, 2, 3, 4]
result.reverse()
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {4, 3, 2, 1});
}

TEST_F(ListBuiltinsTest, ReverseWithListSubclassDoesNotCallSubclassMethods) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(list):
    def __getitem__(self, key):
        raise Exception("hi")
    def __setitem__(self, key, val):
        raise Exception("hi")
result = C([1, 2, 3, 4])
result.reverse()
)")
                   .isError());
  EXPECT_FALSE(Thread::current()->hasPendingException());
}

TEST_F(ListBuiltinsTest, SortWithMultiElementListSortsElements) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
ls = [3, 2, 1]
ls.sort()
)")
                   .isError());
  HandleScope scope(thread_);
  Object ls(&scope, mainModuleAt(runtime_, "ls"));
  EXPECT_PYLIST_EQ(ls, {1, 2, 3});
}

TEST_F(ListBuiltinsTest, SortWithNonCallableKeyRaisesException) {
  EXPECT_TRUE(raised(runFromCStr(runtime_, R"(
ls = [3, 2, 1]
ls.sort(key=5)
)"),
                     LayoutId::kTypeError));
  ;
}

TEST_F(ListBuiltinsTest, SortWithKeySortsAccordingToKey) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
ls = [2, 3, 1]
ls.sort(key=lambda x: -x)
)")
                   .isError());
  HandleScope scope(thread_);
  Object ls(&scope, mainModuleAt(runtime_, "ls"));
  EXPECT_PYLIST_EQ(ls, {3, 2, 1});
}

TEST_F(ListBuiltinsTest, SortReverseReversesSortedList) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
ls = [2, 3, 1]
ls.sort(reverse=True)
)")
                   .isError());
  HandleScope scope(thread_);
  Object ls(&scope, mainModuleAt(runtime_, "ls"));
  EXPECT_PYLIST_EQ(ls, {3, 2, 1});
}

TEST_F(ListBuiltinsTest, ClearWithNonListRaisesTypeError) {
  ASSERT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "list.clear(None)"), LayoutId::kTypeError,
      "'clear' requires a 'list' object but got 'NoneType'"));
}

TEST_F(ListBuiltinsTest, ClearRemovesElements) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
ls = [2, 3, 1]
list.clear(ls)
)")
                   .isError());
  HandleScope scope(thread_);
  Object ls(&scope, mainModuleAt(runtime_, "ls"));
  EXPECT_PYLIST_EQ(ls, {});
}

TEST_F(ListBuiltinsTest, ClearRemovesAllElements) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  pass
l = [C()]
)")
                   .isError());

  HandleScope scope(thread_);
  List list(&scope, mainModuleAt(runtime_, "l"));
  Object ref_obj(&scope, NoneType::object());
  {
    Object none(&scope, NoneType::object());
    Object c(&scope, list.at(0));
    ref_obj = runtime_->newWeakRef(thread_, c, none);
  }
  WeakRef ref(&scope, *ref_obj);
  EXPECT_NE(ref.referent(), NoneType::object());
  runBuiltin(METH(list, clear), list);
  runtime_->collectGarbage();
  EXPECT_EQ(ref.referent(), NoneType::object());
}

}  // namespace py
