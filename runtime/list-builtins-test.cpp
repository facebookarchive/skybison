#include "gtest/gtest.h"

#include "builtins-module.h"
#include "list-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ListBuiltinsTest, CopyWithNonListRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
result = list.copy(None)
)"),
                            LayoutId::kTypeError,
                            "expected 'list' instance but got NoneType"));
}

TEST(ListBuiltinsTest, CopyWithListReturnsNewInstance) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = [1, 2, 3]
result = list.copy(l)
)")
                   .isError());
  HandleScope scope;
  Object list(&scope, moduleAt(&runtime, "__main__", "l"));
  EXPECT_TRUE(list.isList());
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result_obj.isList());
  List result(&scope, *result_obj);
  EXPECT_NE(*list, *result);
  EXPECT_EQ(result.numItems(), 3);
}

TEST(ListBuiltinsTest, DunderEqReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = list.__eq__([1, 2, 3], [1, 2, 3])
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), RawBool::trueObj());
}

TEST(ListBuiltinsTest, DunderEqWithSameListReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
    def __eq__(self, other):
        return False
a = [1, 2, 3]
result = list.__eq__(a, a)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), RawBool::trueObj());
}

TEST(ListBuiltinsTest, DunderEqWithSameIdentityElementsReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
nan = float("nan")
result = list.__eq__([nan], [nan])
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), RawBool::trueObj());
}

TEST(ListBuiltinsTest, DunderEqWithEqualElementsReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), RawBool::trueObj());
}

TEST(ListBuiltinsTest, DunderEqWithDifferentSizeReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = list.__eq__([1, 2, 3], [1, 2])
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), RawBool::falseObj());
}

TEST(ListBuiltinsTest, DunderEqWithDifferentValuesReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = list.__eq__([1, 2, 3], [1, 2, 4])
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), RawBool::falseObj());
}

TEST(ListBuiltinsTest, DunderEqWithNonListRhsRaisesNotImplemented) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = list.__eq__([1, 2, 3], (1, 2, 3));
)")
                   .isError());
  EXPECT_TRUE(moduleAt(&runtime, "__main__", "result").isNotImplementedType());
}

TEST(ListBuiltinsTest, DunderEqWithNonListLhsReturnsTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "list.__eq__((1, 2, 3), [1, 2, 3])"),
      LayoutId::kTypeError, "'__eq__' requires 'list' but received a 'tuple'"));
}

TEST(ListBuiltinsTest, DunderInitFromList) {
  Runtime runtime;

  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = list([1, 2])
)")
                   .isError());
  HandleScope scope;
  List a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_EQ(a.numItems(), 2);
  EXPECT_TRUE(isIntEqualsWord(a.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(a.at(1), 2));
}

TEST(ListBuiltinsTest, NewListIsNotASingleton) {
  Runtime runtime;

  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = list() is not list()
b = list([1, 2]) is not list([1, 2])
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
}

TEST(ListBuiltinsTest, AddToNonEmptyList) {
  Runtime runtime;
  HandleScope scope;

  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = [1, 2]
b = [3, 4, 5]
c = a + b
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_TRUE(c.isList());
  List list(&scope, List::cast(*c));
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(list.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(list.at(2), 3));
  EXPECT_TRUE(isIntEqualsWord(list.at(3), 4));
  EXPECT_TRUE(isIntEqualsWord(list.at(4), 5));
}

TEST(ListBuiltinsTest, AddToEmptyList) {
  Runtime runtime;
  HandleScope scope;

  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = []
b = [1, 2, 3]
c = a + b
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_TRUE(c.isList());
  List list(&scope, List::cast(*c));
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(list.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(list.at(2), 3));
}

TEST(ListBuiltinsTest, AddWithNonListSelfRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "list.__add__(None, [])"), LayoutId::kTypeError,
      "'__add__' requires a 'list' object but got 'NoneType'"));
}

TEST(ListBuiltinsTest, AddListToTupleRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
a = [1, 2, 3]
b = (4, 5, 6)
c = a + b
)"),
                            LayoutId::kTypeError,
                            "can only concatenate list to list"));
}

TEST(ListBuiltinsTest, ListAppend) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
a = list()
b = list()
a.append(1)
a.append("2")
b.append(3)
a.append(b)
print(a[0], a[1], a[2][0])
)");
  EXPECT_EQ(output, "1 2 3\n");
}

TEST(ListBuiltinsTest, DunderContainsWithContainedElementReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Int value0(&scope, runtime.newInt(1));
  Bool value1(&scope, RawBool::falseObj());
  Str value2(&scope, runtime.newStrFromCStr("hello"));
  List list(&scope, runtime.newList());
  runtime.listAdd(list, value0);
  runtime.listAdd(list, value1);
  runtime.listAdd(list, value2);
  EXPECT_EQ(runBuiltin(ListBuiltins::dunderContains, list, value0),
            RawBool::trueObj());
  EXPECT_EQ(runBuiltin(ListBuiltins::dunderContains, list, value1),
            RawBool::trueObj());
  EXPECT_EQ(runBuiltin(ListBuiltins::dunderContains, list, value2),
            RawBool::trueObj());
}

TEST(ListBuiltinsTest, DunderContainsWithUncontainedElementReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Int value0(&scope, runtime.newInt(7));
  NoneType value1(&scope, RawNoneType::object());
  List list(&scope, runtime.newList());
  runtime.listAdd(list, value0);
  runtime.listAdd(list, value1);
  Int value2(&scope, runtime.newInt(42));
  Bool value3(&scope, RawBool::trueObj());
  EXPECT_EQ(runBuiltin(ListBuiltins::dunderContains, list, value2),
            RawBool::falseObj());
  EXPECT_EQ(runBuiltin(ListBuiltins::dunderContains, list, value3),
            RawBool::falseObj());
}

TEST(ListBuiltinsTest, DunderContainsWithIdenticalObjectReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __eq__(self, other):
    return False
value = Foo()
list = [value]
)")
                   .isError());
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  List list(&scope, moduleAt(&runtime, "__main__", "list"));
  EXPECT_EQ(runBuiltin(ListBuiltins::dunderContains, list, value),
            RawBool::trueObj());
}

TEST(ListBuiltinsTest,
     DunderContainsWithNonIdenticalEqualKeyObjectReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __eq__(self, other):
    return True
value = Foo()
list = [None]
)")
                   .isError());
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  List list(&scope, moduleAt(&runtime, "__main__", "list"));
  EXPECT_EQ(runBuiltin(ListBuiltins::dunderContains, list, value),
            RawBool::trueObj());
}

TEST(ListBuiltinsTest,
     DunderContainsWithNonIdenticalEqualListObjectReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  Object value1(&scope, moduleAt(&runtime, "__main__", "value1"));
  List list(&scope, moduleAt(&runtime, "__main__", "list"));
  EXPECT_EQ(runBuiltin(ListBuiltins::dunderContains, list, value1),
            RawBool::falseObj());
}

TEST(ListBuiltinsTest, DunderContainsWithRaisingEqPropagatesException) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __eq__(self, other):
    raise UserWarning("")
value = Foo()
list = [None]
)")
                   .isError());
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  List list(&scope, moduleAt(&runtime, "__main__", "list"));
  Object result(&scope, runBuiltin(ListBuiltins::dunderContains, list, value));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST(ListBuiltinsTest, DunderContainsWithRaisingDunderBoolPropagatesException) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  List list(&scope, moduleAt(&runtime, "__main__", "list"));
  EXPECT_TRUE(raised(runBuiltin(ListBuiltins::dunderContains, list, value),
                     LayoutId::kUserWarning));
}

TEST(ListBuiltinsTest, DunderContainsWithNonListSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Int i(&scope, SmallInt::fromWord(3));
  Object result(&scope, runBuiltin(ListBuiltins::dunderContains, i, i));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(ListBuiltinsTest, ListExtend) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
a = []
b = [1, 2, 3]
r = a.extend(b)
print(r is None, len(b) == 3)
)");
  EXPECT_EQ(output, "True True\n");
}

TEST(ListBuiltinsTest, ListInsertWithMissingArgumentsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "[1, 2].insert()"), LayoutId::kTypeError,
      "TypeError: 'list.insert' takes 3 positional arguments but 1 given"));
}

TEST(ListBuiltinsTest, ListInsertWithNonListRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "list.insert(None, 1, None)"), LayoutId::kTypeError,
      "'insert' requires a 'list' object but got 'NoneType'"));
}

TEST(ListBuiltinsTest, ListInsertWithNonIntIndexRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "[1, 2].insert({}, 3)"), LayoutId::kTypeError,
      "'dict' object cannot be interpreted as an integer"));
}

TEST(ListBuiltinsTest, ListInsertWithLargeIntIndexRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "[1, 2].insert(2 ** 63, 1)"),
                            LayoutId::kOverflowError,
                            "Python int too large to convert to C ssize_t"));
}

TEST(ListBuiltinsTest, ListInsertWithBoolIndexInsertsAtInt) {
  Runtime runtime;
  HandleScope scope;
  List self(&scope, runtime.newList());
  Object value(&scope, SmallInt::fromWord(3));
  runtime.listAdd(self, value);
  runtime.listAdd(self, value);
  Object fals(&scope, Bool::falseObj());
  Object tru(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(ListBuiltins::insert, self, tru, tru));
  EXPECT_EQ(result, NoneType::object());
  result = runBuiltin(ListBuiltins::insert, self, fals, fals);
  EXPECT_EQ(result, NoneType::object());
  ASSERT_EQ(self.numItems(), 4);
  EXPECT_EQ(self.at(0), fals);
  EXPECT_EQ(self.at(1), value);
  EXPECT_EQ(self.at(2), tru);
  EXPECT_EQ(self.at(3), value);
}

TEST(ListBuiltinsTest, ListInsertWithIntSubclassInsertsAtInt) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class N(int):
  pass
a = [0, 0, 0, 0, 0]
b = N(3)
)")
                   .isError());
  HandleScope scope;
  List self(&scope, moduleAt(&runtime, "__main__", "a"));
  Object index(&scope, moduleAt(&runtime, "__main__", "b"));
  Object value(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(ListBuiltins::insert, self, index, value));
  EXPECT_EQ(result, NoneType::object());
  EXPECT_EQ(self.numItems(), 6);
  EXPECT_EQ(self.at(3), value);
}

TEST(ListBuiltinsTest, ListPop) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
a = [1,2,3,4,5]
a.pop()
print(len(a))
a.pop(0)
a.pop(-1)
print(len(a), a[0], a[1])
)");
  EXPECT_EQ(output, "4\n2 2 3\n");

  std::string output2 = compileAndRunToString(&runtime, R"(
a = [1,2,3,4,5]
print(a.pop(), a.pop(0), a.pop(-2))
)");
  EXPECT_EQ(output2, "5 1 3\n");
}

TEST(ListBuiltinsTest, ListPopExcept) {
  Runtime runtime;
  Thread* thread = Thread::current();

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, R"(
a = [1, 2]
a.pop(1, 2, 3, 4)
)"),
      LayoutId::kTypeError,
      "TypeError: 'list.pop' takes max 2 positional arguments but 5 given"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "list.pop(None)"), LayoutId::kTypeError,
      "'pop' requires a 'list' object but got 'NoneType'"));
  thread->clearPendingException();

  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, R"(
a = [1, 2]
a.pop("i")
)"),
                    LayoutId::kTypeError,
                    "index object cannot be interpreted as an integer"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
a = [1]
a.pop()
a.pop()
)"),
                            LayoutId::kIndexError, "pop from empty list"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
a = [1]
a.pop(3)
)"),
                            LayoutId::kIndexError, "pop index out of range"));
}

TEST(ListBuiltinsTest, ListRemove) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
a = [5, 4, 3, 2, 1]
a.remove(2)
a.remove(5)
print(len(a), a[0], a[1], a[2])
)");
  EXPECT_EQ(output, "3 4 3 1\n");
}

TEST(ListBuiltinsTest, ListRemoveWithDuplicateItemsRemovesFirstMatchingItem) {
  Runtime runtime;
  HandleScope scope;
  Int value0(&scope, runtime.newInt(0));
  Int value1(&scope, runtime.newInt(1));
  Int value2(&scope, runtime.newInt(2));
  List list(&scope, runtime.newList());
  runtime.listAdd(list, value0);
  runtime.listAdd(list, value1);
  runtime.listAdd(list, value2);
  runtime.listAdd(list, value1);
  runtime.listAdd(list, value0);

  EXPECT_EQ(list.numItems(), 5);
  runBuiltin(ListBuiltins::remove, list, value1);
  ASSERT_EQ(list.numItems(), 4);
  EXPECT_EQ(list.at(0), value0);
  EXPECT_EQ(list.at(1), value2);
  EXPECT_EQ(list.at(2), value1);
  EXPECT_EQ(list.at(3), value0);
}

TEST(ListBuiltinsTest, ListRemoveWithIdenticalObjectGetsRemoved) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __eq__(self, other):
    return False
value = C()
list = [value]
)")
                   .isError());
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  List list(&scope, moduleAt(&runtime, "__main__", "list"));
  EXPECT_EQ(list.numItems(), 1);
  runBuiltin(ListBuiltins::remove, list, value);
  EXPECT_EQ(list.numItems(), 0);
}

TEST(ListBuiltinsTest, ListRemoveWithNonIdenticalEqualObjectInListGetsRemoved) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __eq__(self, other):
    return True
list = [C()]
)")
                   .isError());
  Object value(&scope, NoneType::object());
  List list(&scope, moduleAt(&runtime, "__main__", "list"));
  EXPECT_EQ(list.numItems(), 1);
  runBuiltin(ListBuiltins::remove, list, value);
  EXPECT_EQ(list.numItems(), 0);
}

TEST(ListBuiltinsTest,
     ListRemoveWithNonIdenticalEqualObjectAsKeyRaisesValueError) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  List list(&scope, moduleAt(&runtime, "__main__", "list"));
  Object result(&scope, runBuiltin(ListBuiltins::remove, list, value));
  EXPECT_TRUE(raised(*result, LayoutId::kValueError));
}

TEST(ListBuiltinsTest, ListRemoveWithRaisingDunderEqualPropagatesException) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __eq__(self, other):
    raise UserWarning('')
value = Foo()
list = [None]
)")
                   .isError());
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  List list(&scope, moduleAt(&runtime, "__main__", "list"));
  Object result(&scope, runBuiltin(ListBuiltins::remove, list, value));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST(ListBuiltinsTest, ListRemoveWithRaisingDunderBoolPropagatesException) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __bool__(self):
    raise UserWarning('')
class D:
  def __eq__(self, other):
    raise C()
value = D()
list = [None]
)")
                   .isError());
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  List list(&scope, moduleAt(&runtime, "__main__", "list"));
  Object result(&scope, runBuiltin(ListBuiltins::remove, list, value));
  EXPECT_TRUE(result.isError());
  // TODO(T39221304) check for kUserWarning here once isTrue() propagates
  // exceptions correctly.
}

TEST(ListBuiltinsTest, PrintList) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
a = [1, 0, True]
print(a)
a[0]=7
print(a)
)");
  EXPECT_EQ(output, "[1, 0, True]\n[7, 0, True]\n");
}

TEST(ListBuiltinsTest, ReplicateList) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
data = [1, 2, 3] * 3
for i in range(9):
  print(data[i])
)");
  EXPECT_EQ(output, "1\n2\n3\n1\n2\n3\n1\n2\n3\n");
}

TEST(ListBuiltinsTest, SubscriptList) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
l = [1, 2, 3, 4, 5, 6]
print(l[0], l[3], l[5])
l[0] = 6
l[5] = 1
print(l[0], l[3], l[5])
)");
  ASSERT_EQ(output, "1 4 6\n6 4 1\n");
}

TEST(ListBuiltinsTest, SlicePositiveStartIndex) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [2:]
  Slice slice(&scope, runtime.newSlice());
  slice.setStart(SmallInt::fromWord(2));
  List test(&scope, listSlice(thread, list1, slice));
  ASSERT_EQ(test.numItems(), 3);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(test.at(1), 4));
  EXPECT_TRUE(isIntEqualsWord(test.at(2), 5));
}

TEST(ListBuiltinsTest, SliceNegativeStartIndexIsRelativeToEnd) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [-2:]
  Slice slice(&scope, runtime.newSlice());
  slice.setStart(SmallInt::fromWord(-2));
  List test(&scope, listSlice(thread, list1, slice));
  ASSERT_EQ(test.numItems(), 2);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 4));
  EXPECT_TRUE(isIntEqualsWord(test.at(1), 5));
}

TEST(ListBuiltinsTest, SlicePositiveStopIndex) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [:2]
  Slice slice(&scope, runtime.newSlice());
  slice.setStop(SmallInt::fromWord(2));
  List test(&scope, listSlice(thread, list1, slice));
  ASSERT_EQ(test.numItems(), 2);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(test.at(1), 2));
}

TEST(ListBuiltinsTest, SliceNegativeStopIndexIsRelativeToEnd) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [:-2]
  Slice slice(&scope, runtime.newSlice());
  slice.setStop(SmallInt::fromWord(-2));
  List test(&scope, listSlice(thread, list1, slice));
  ASSERT_EQ(test.numItems(), 3);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(test.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(test.at(2), 3));
}

TEST(ListBuiltinsTest, SlicePositiveStep) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [::2]
  Slice slice(&scope, runtime.newSlice());
  slice.setStep(SmallInt::fromWord(2));
  List test(&scope, listSlice(thread, list1, slice));
  ASSERT_EQ(test.numItems(), 3);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(test.at(1), 3));
  EXPECT_TRUE(isIntEqualsWord(test.at(2), 5));
}

TEST(ListBuiltinsTest, SliceNegativeStepReversesOrder) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [::-2]
  Slice slice(&scope, runtime.newSlice());
  slice.setStep(SmallInt::fromWord(-2));
  List test(&scope, listSlice(thread, list1, slice));
  ASSERT_EQ(test.numItems(), 3);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 5));
  EXPECT_TRUE(isIntEqualsWord(test.at(1), 3));
  EXPECT_TRUE(isIntEqualsWord(test.at(2), 1));
}

TEST(ListBuiltinsTest, SliceStartOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [10::]
  Slice slice(&scope, runtime.newSlice());
  slice.setStart(SmallInt::fromWord(10));
  List test(&scope, listSlice(thread, list1, slice));
  ASSERT_EQ(test.numItems(), 0);
}

TEST(ListBuiltinsTest, SliceStopOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [:10]
  Slice slice(&scope, runtime.newSlice());
  slice.setStop(SmallInt::fromWord(10));
  List test(&scope, listSlice(thread, list1, slice));
  ASSERT_EQ(test.numItems(), 5);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(test.at(4), 5));
}

TEST(ListBuiltinsTest, SliceStepOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [::10]
  Slice slice(&scope, runtime.newSlice());
  slice.setStep(SmallInt::fromWord(10));
  List test(&scope, listSlice(thread, list1, slice));
  ASSERT_EQ(test.numItems(), 1);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 1));
}

TEST(ListBuiltinsTest, IdenticalSliceIsCopy) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test: t[::] is t
  Slice slice(&scope, runtime.newSlice());
  List test(&scope, listSlice(thread, list1, slice));
  ASSERT_EQ(test.numItems(), 5);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(test.at(4), 5));
  ASSERT_NE(*test, *list1);
}

TEST(ListBuiltinsTest, SetItem) {
  Runtime runtime;
  HandleScope scope;

  List list(&scope, listFromRange(1, 5));
  Object zero(&scope, SmallInt::fromWord(0));
  Object num(&scope, SmallInt::fromWord(2));

  EXPECT_TRUE(isIntEqualsWord(list.at(0), 1));
  Object result(&scope,
                runBuiltin(ListBuiltins::dunderSetItem, list, zero, num));
  EXPECT_TRUE(result.isNoneType());
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 2));

  // Negative index
  Object minus_one(&scope, SmallInt::fromWord(-1));
  EXPECT_TRUE(isIntEqualsWord(list.at(3), 4));
  Object result1(&scope,
                 runBuiltin(ListBuiltins::dunderSetItem, list, minus_one, num));
  EXPECT_TRUE(result1.isNoneType());
  EXPECT_TRUE(isIntEqualsWord(list.at(3), 2));
}

TEST(ListBuiltinsTest, GetItemWithNegativeIndex) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(1, 4));
  Object idx(&scope, SmallInt::fromWord(-3));
  Object result(&scope, runBuiltin(ListBuiltins::dunderGetItem, list, idx));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST(ListBuiltinsTest, DelItemWithNegativeIndex) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(1, 4));
  Object idx(&scope, SmallInt::fromWord(-3));
  ASSERT_TRUE(
      runBuiltin(BuiltinsModule::underListDelItem, list, idx).isNoneType());
  EXPECT_PYLIST_EQ(list, {2, 3});
}

TEST(ListBuiltinsTest, SetItemWithNegativeIndex) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(1, 4));
  Object idx(&scope, SmallInt::fromWord(-3));
  Object num(&scope, SmallInt::fromWord(0));
  Object result(&scope,
                runBuiltin(ListBuiltins::dunderSetItem, list, idx, num));
  ASSERT_TRUE(result.isNoneType());
  ASSERT_EQ(list.numItems(), 3);
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 0));
  EXPECT_TRUE(isIntEqualsWord(list.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(list.at(2), 3));
}

TEST(ListBuiltinsTest, GetItemWithInvalidNegativeIndexRaisesIndexError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
l = [1, 2, 3]
l[-4]
)"),
                            LayoutId::kIndexError, "list index out of range"));
}

TEST(ListBuiltinsTest, DelItemWithInvalidNegativeIndexRaisesIndexError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
l = [1, 2, 3]
del l[-4]
)"),
                            LayoutId::kIndexError,
                            "list assignment index out of range"));
}

TEST(ListBuiltinsTest, SetItemWithInvalidNegativeIndexRaisesIndexError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
l = [1, 2, 3]
l[-4] = 0
)"),
                            LayoutId::kIndexError,
                            "list assignment index out of range"));
}

TEST(ListBuiltinsTest, GetItemWithInvalidIndexRaisesIndexError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
l = [1, 2, 3]
l[5]
)"),
                            LayoutId::kIndexError, "list index out of range"));
}

TEST(ListBuiltinsTest, DelItemWithInvalidIndexRaisesIndexError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
l = [1, 2, 3]
del l[5]
)"),
                            LayoutId::kIndexError,
                            "list assignment index out of range"));
}

TEST(ListBuiltinsTest, SetItemWithInvalidIndexRaisesIndexError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
l = [1, 2, 3]
l[5] = 4
)"),
                            LayoutId::kIndexError,
                            "list assignment index out of range"));
}

TEST(ListBuiltinsTest, SetItemSliceBasic) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:5] = ['C', 'D', 'E']
result = letters
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"a", "b", "C", "D", "E", "f", "g"});
}

TEST(ListBuiltinsTest, SetItemSliceGrow) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:5] = ['C', 'D', 'E','X','Y','Z']
result = letters
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"a", "b", "C", "D", "E", "X", "Y", "Z", "f", "g"});
}

TEST(ListBuiltinsTest, SetItemSliceShrink) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:6] = ['C', 'D']
result = letters
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"a", "b", "C", "D", "g"});
}

TEST(ListBuiltinsTest, SetItemSliceIterable) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:6] = ('x', 'y', 12)
result = letters
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"a", "b", "x", "y", 12, "g"});
}

TEST(ListBuiltinsTest, SetItemSliceSelf) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:5] = letters
result = letters
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result,
                   {"a", "b", "a", "b", "c", "d", "e", "f", "g", "f", "g"});
}

// Reverse ordered bounds, but step still +1:
TEST(ListBuiltinsTest, SetItemSliceRevBounds) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = list(range(20))
a[5:2] = ['a','b','c','d','e']
result = a
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0, 1, 2,  3,  4,  "a", "b", "c", "d", "e", 5,  6, 7,
                            8, 9, 10, 11, 12, 13,  14,  15,  16,  17,  18, 19});
}

TEST(ListBuiltinsTest, SetItemSliceStep) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = list(range(20))
a[2:10:3] = ['a', 'b', 'c']
result = a
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0,  1,  "a", 3,  4,  "b", 6,  7,  "c", 9,
                            10, 11, 12,  13, 14, 15,  16, 17, 18,  19});
}

TEST(ListBuiltinsTest, SetItemSliceStepNeg) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = list(range(20))
a[10:2:-3] = ['a', 'b', 'c']
result = a
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0,   1,  2,  3,  "c", 5,  6,  "b", 8,  9,
                            "a", 11, 12, 13, 14,  15, 16, 17,  18, 19});
}

TEST(ListBuiltinsTest, SetItemSliceStepSizeErr) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, R"(
a = list(range(20))
a[2:10:3] = ['a', 'b', 'c', 'd']
)"),
      LayoutId::kValueError,
      "attempt to assign sequence of size 4 to extended slice of size 3"));
}

TEST(ListBuiltinsTest, SetItemSliceScalarErr) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:6] = 5
)"),
                            LayoutId::kTypeError, "object is not iterable"));
}

TEST(ListBuiltinsTest, SetItemSliceStepTuple) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = list(range(20))
a[2:10:3] = ('a', 'b', 'c')
result = a
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0,  1,  "a", 3,  4,  "b", 6,  7,  "c", 9,
                            10, 11, 12,  13, 14, 15,  16, 17, 18,  19});
}

TEST(ListBuiltinsTest, SetItemSliceShortValue) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, R"(
a = [1,2,3,4,5,6,7,8,9,10]
b = [0,0,0]
a[:8:2] = b
)"),
      LayoutId::kValueError,
      "attempt to assign sequence of size 3 to extended slice of size 4"));
}
TEST(ListBuiltinsTest, SetItemSliceShortStop) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = [1,2,3,4,5,6,7,8,9,10]
b = [0,0,0]
a[:1] = b
result = a
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0, 0, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10});
}

TEST(ListBuiltinsTest, SetItemSliceLongStop) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = [1,1,1]
b = [0,0,0,0,0]
a[:1] = b
result = a
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0, 0, 0, 0, 0, 1, 1});
}

TEST(ListBuiltinsTest, SetItemSliceShortStep) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = [1,2,3,4,5,6,7,8,9,10]
b = [0,0,0]
a[::1] = b
result = a
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0, 0, 0});
}

TEST(ListBuiltinsTest, GetItemWithTooFewArgumentsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
[].__getitem__()
)"),
                            LayoutId::kTypeError,
                            "TypeError: 'list.__getitem__' takes 2 positional "
                            "arguments but 1 given"));
}

TEST(ListBuiltinsTest, DelItemWithTooFewArgumentsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
[].__delitem__()
)"),
                            LayoutId::kTypeError,
                            "TypeError: 'list.__delitem__' takes 2 positional "
                            "arguments but 1 given"));
}

TEST(ListBuiltinsTest, SetItemWithTooFewArgumentsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
[].__setitem__(1)
)"),
                            LayoutId::kTypeError,
                            "TypeError: 'list.__setitem__' takes 3 positional "
                            "arguments but 2 given"));
}

TEST(ListBuiltinsTest, DelItemWithTooManyArgumentsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
[].__delitem__(1, 2)
)"),
                            LayoutId::kTypeError,
                            "TypeError: 'list.__delitem__' takes max 2 "
                            "positional arguments but 3 given"));
}

TEST(ListBuiltinsTest, GetItemWithTooManyArgumentsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
[].__getitem__(1, 2)
)"),
                            LayoutId::kTypeError,
                            "TypeError: 'list.__getitem__' takes max 2 "
                            "positional arguments but 3 given"));
}

TEST(ListBuiltinsTest, SetItemWithTooManyArgumentsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
[].__setitem__(1, 2, 3)
)"),
                            LayoutId::kTypeError,
                            "TypeError: 'list.__setitem__' takes max 3 "
                            "positional arguments but 4 given"));
}

TEST(ListBuiltinsTest, GetItemWithNonIntegralIndexRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
[].__getitem__("test")
)"),
                            LayoutId::kTypeError,
                            "list indices must be integers or slices"));
}

TEST(ListBuiltinsTest, DelItemWithNonIntegralIndexRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
[].__delitem__("test")
)"),
                            LayoutId::kTypeError,
                            "list indices must be integers or slices"));
}

TEST(ListBuiltinsTest, SetItemWithNonIntegralIndexRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
[].__setitem__("test", 1)
)"),
                            LayoutId::kTypeError,
                            "list indices must be integers or slices"));
}

TEST(ListBuiltinsTest, NonTypeInDunderNew) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
list.__new__(1)
)"),
                            LayoutId::kTypeError, "not a type object"));
}

TEST(ListBuiltinsTest, NonSubclassInDunderNew) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo: pass
list.__new__(Foo)
)"),
                            LayoutId::kTypeError, "not a subtype of list"));
}

TEST(ListBuiltinsTest, SubclassList) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  Object test1(&scope, moduleAt(&runtime, "__main__", "test1"));
  Object test2(&scope, moduleAt(&runtime, "__main__", "test2"));
  Object test3(&scope, moduleAt(&runtime, "__main__", "test3"));
  Object test4(&scope, moduleAt(&runtime, "__main__", "test4"));
  Object test5(&scope, moduleAt(&runtime, "__main__", "test5"));
  Object test6(&scope, moduleAt(&runtime, "__main__", "test6"));
  EXPECT_EQ(*test1, SmallInt::fromWord(1));
  EXPECT_EQ(*test2, SmallStr::fromCStr("a"));
  EXPECT_EQ(*test3, SmallInt::fromWord(2));
  EXPECT_EQ(*test4, SmallInt::fromWord(1));
  EXPECT_EQ(*test5, SmallInt::fromWord(2));
  EXPECT_EQ(*test6, SmallInt::fromWord(0));
}

TEST(ListBuiltinsTest, DelItem) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = [42,'foo', 'bar']
del a[2]
del a[0]
l = len(a)
e = a[0]
)")
                   .isError());
  Object len(&scope, moduleAt(&runtime, "__main__", "l"));
  Object element(&scope, moduleAt(&runtime, "__main__", "e"));
  EXPECT_EQ(*len, SmallInt::fromWord(1));
  EXPECT_EQ(*element, SmallStr::fromCStr("foo"));
}

TEST(ListBuiltinsTest, DelItemWithLastIndexRemovesLastItem) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(
      runBuiltin(BuiltinsModule::underListDelItem, list, idx).isNoneType());
  EXPECT_PYLIST_EQ(list, {0});
}

TEST(ListBuiltinsTest, DelItemWithFirstIndexRemovesFirstItem) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(
      runBuiltin(BuiltinsModule::underListDelItem, list, idx).isNoneType());
  EXPECT_PYLIST_EQ(list, {1});
}

TEST(ListBuiltinsTest, DelItemWithNegativeFirstIndexRemovesFirstItem) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(-2));
  ASSERT_TRUE(
      runBuiltin(BuiltinsModule::underListDelItem, list, idx).isNoneType());
  EXPECT_PYLIST_EQ(list, {1});
}

TEST(ListBuiltinsTest, DelItemWithNegativeLastIndexRemovesLastItem) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(-1));
  ASSERT_TRUE(
      runBuiltin(BuiltinsModule::underListDelItem, list, idx).isNoneType());
  EXPECT_PYLIST_EQ(list, {0});
}

TEST(ListBuiltinsTest, DelItemWithNumberGreaterThanSmallIntMaxDoesNotCrash) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(0, 2));
  Int big(&scope, runtime.newInt(SmallInt::kMaxValue + 100));
  EXPECT_TRUE(raised(runBuiltin(BuiltinsModule::underListDelItem, list, big),
                     LayoutId::kIndexError));
  EXPECT_PYLIST_EQ(list, {0, 1});
}

TEST(ListBuiltinsTest, DelSliceRemovesItems) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(1, 4));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(1));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(
      runBuiltin(BuiltinsModule::underListDelSlice, list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {2, 3});
}

TEST(ListBuiltinsTest, DelSliceRemovesFirstItem) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(0, 2));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(1));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(
      runBuiltin(BuiltinsModule::underListDelSlice, list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {1});
}

TEST(ListBuiltinsTest, DelSliceRemovesLastItem) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(0, 2));
  Int start(&scope, SmallInt::fromWord(1));
  Int stop(&scope, SmallInt::fromWord(2));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(
      runBuiltin(BuiltinsModule::underListDelSlice, list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {0});
}

TEST(ListBuiltinsTest, DelSliceWithStopEqualsLengthRemovesTrailingItems) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(1, 4));
  Int start(&scope, SmallInt::fromWord(1));
  Int stop(&scope, SmallInt::fromWord(3));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(
      runBuiltin(BuiltinsModule::underListDelSlice, list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {1});
}

TEST(ListBuiltinsTest, DelSliceWithStartEqualsZeroRemovesStartingItems) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(1, 4));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(1));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(
      runBuiltin(BuiltinsModule::underListDelSlice, list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {2, 3});
}

TEST(ListBuiltinsTest,
     DelSliceWithStartEqualsZeroAndStopEqualsLengthRemovesAllItems) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(1, 4));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(3));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(
      runBuiltin(BuiltinsModule::underListDelSlice, list, start, stop, step)
          .isNoneType());
  EXPECT_EQ(list.numItems(), 0);
}

TEST(ListBuiltinsTest, DelSliceWithStepEqualsTwoDeletesEveryEvenItem) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(0, 5));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(5));
  Int step(&scope, SmallInt::fromWord(2));
  ASSERT_TRUE(
      runBuiltin(BuiltinsModule::underListDelSlice, list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {1, 3});
}

TEST(ListBuiltinsTest, DelSliceWithStepEqualsTwoDeletesEveryOddItem) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(0, 5));
  Int start(&scope, SmallInt::fromWord(1));
  Int stop(&scope, SmallInt::fromWord(5));
  Int step(&scope, SmallInt::fromWord(2));
  ASSERT_TRUE(
      runBuiltin(BuiltinsModule::underListDelSlice, list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {0, 2, 4});
}

TEST(ListBuiltinsTest, DelSliceWithStepGreaterThanLengthDeletesOneItem) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(0, 5));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(5));
  Int step(&scope, SmallInt::fromWord(1000));
  ASSERT_TRUE(
      runBuiltin(BuiltinsModule::underListDelSlice, list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {1, 2, 3, 4});
}

TEST(ListBuiltinsTest, DunderIterReturnsListIter) {
  Runtime runtime;
  HandleScope scope;
  List empty_list(&scope, listFromRange(0, 0));
  Object iter(&scope, runBuiltin(ListBuiltins::dunderIter, empty_list));
  ASSERT_TRUE(iter.isListIterator());
}

TEST(ListBuiltinsTest, DunderRepr) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = [1, 2, 'hello'].__repr__()
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "[1, 2, 'hello']"));
}

TEST(ListIteratorBuiltinsTest, CallDunderNext) {
  Runtime runtime;
  HandleScope scope;
  List empty_list(&scope, listFromRange(0, 2));
  Object iter(&scope, runBuiltin(ListBuiltins::dunderIter, empty_list));
  ASSERT_TRUE(iter.isListIterator());

  Object item1(&scope, runBuiltin(ListIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isIntEqualsWord(*item1, 0));

  Object item2(&scope, runBuiltin(ListIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isIntEqualsWord(*item2, 1));
}

TEST(ListIteratorBuiltinsTest, DunderIterReturnsSelf) {
  Runtime runtime;
  HandleScope scope;
  List empty_list(&scope, listFromRange(0, 0));
  Object iter(&scope, runBuiltin(ListBuiltins::dunderIter, empty_list));
  ASSERT_TRUE(iter.isListIterator());

  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(ListIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(ListIteratorBuiltinsTest, DunderLengthHintOnEmptyListIterator) {
  Runtime runtime;
  HandleScope scope;
  List empty_list(&scope, listFromRange(0, 0));
  Object iter(&scope, runBuiltin(ListBuiltins::dunderIter, empty_list));
  ASSERT_TRUE(iter.isListIterator());

  Object length_hint(&scope,
                     runBuiltin(ListIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST(ListIteratorBuiltinsTest, DunderLengthHintOnConsumedListIterator) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(0, 1));
  Object iter(&scope, runBuiltin(ListBuiltins::dunderIter, list));
  ASSERT_TRUE(iter.isListIterator());

  Object length_hint1(&scope,
                      runBuiltin(ListIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint1, 1));

  // Consume the iterator
  Object item1(&scope, runBuiltin(ListIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isIntEqualsWord(*item1, 0));

  Object length_hint2(&scope,
                      runBuiltin(ListIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint2, 0));
}

TEST(ListBuiltinsTest, InsertToList) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());

  for (int i = 0; i < 9; i++) {
    if (i == 1 || i == 6) {
      continue;
    }
    Object value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(list, value);
  }
  EXPECT_FALSE(isIntEqualsWord(list.at(1), 1));
  EXPECT_FALSE(isIntEqualsWord(list.at(6), 6));

  Thread* thread = Thread::current();
  Object value2(&scope, SmallInt::fromWord(1));
  listInsert(thread, list, value2, 1);
  Object value12(&scope, SmallInt::fromWord(6));
  listInsert(thread, list, value12, 6);

  EXPECT_PYLIST_EQ(list, {0, 1, 2, 3, 4, 5, 6, 7, 8});
}

TEST(ListBuiltinsTest, InsertToListBounds) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  for (int i = 0; i < 10; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(list, value);
  }
  ASSERT_EQ(list.numItems(), 10);

  Thread* thread = Thread::current();
  Object value100(&scope, SmallInt::fromWord(100));
  listInsert(thread, list, value100, 100);
  ASSERT_EQ(list.numItems(), 11);
  EXPECT_TRUE(isIntEqualsWord(list.at(10), 100));

  Object value0(&scope, SmallInt::fromWord(400));
  listInsert(thread, list, value0, 0);
  ASSERT_EQ(list.numItems(), 12);
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 400));

  Object value_n(&scope, SmallInt::fromWord(-10));
  listInsert(thread, list, value_n, -10);
  ASSERT_EQ(list.numItems(), 13);
  EXPECT_TRUE(isIntEqualsWord(list.at(2), -10));
}

TEST(ListBuiltinsTest, PopList) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  for (int i = 0; i < 16; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(list, value);
  }
  ASSERT_EQ(list.numItems(), 16);

  // Pop from the end
  RawObject res1 = listPop(list, 15);
  ASSERT_EQ(list.numItems(), 15);
  EXPECT_TRUE(isIntEqualsWord(list.at(14), 14));
  EXPECT_TRUE(isIntEqualsWord(res1, 15));

  // Pop elements from 5 - 10
  for (int i = 0; i < 5; i++) {
    RawObject res5 = listPop(list, 5);
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
  RawObject res0 = listPop(list, 0);
  ASSERT_EQ(list.numItems(), 9);
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(res0, 0));
}

TEST(ListBuiltinsTest, ExtendList) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  List list1(&scope, runtime.newList());
  for (int i = 0; i < 4; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    Object value1(&scope, SmallInt::fromWord(i + 4));
    runtime.listAdd(list, value);
    runtime.listAdd(list1, value1);
  }
  EXPECT_EQ(list.numItems(), 4);
  Object list1_handle(&scope, *list1);
  listExtend(Thread::current(), list, list1_handle);
  EXPECT_PYLIST_EQ(list, {0, 1, 2, 3, 4, 5, 6, 7});
}

TEST(ListBuiltinsTest, ExtendListIterator) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  List list1(&scope, runtime.newList());
  for (int i = 0; i < 4; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    Object value1(&scope, SmallInt::fromWord(i + 4));
    runtime.listAdd(list, value);
    runtime.listAdd(list1, value1);
  }
  EXPECT_EQ(list.numItems(), 4);
  Object list1_handle(&scope, *list1);
  Object list1_iterator(&scope, runtime.newListIterator(list1_handle));
  listExtend(Thread::current(), list, list1_iterator);
  EXPECT_PYLIST_EQ(list, {0, 1, 2, 3, 4, 5, 6, 7});
}

TEST(ListBuiltinsTest, ExtendTuple) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  Object object_array0(&scope, runtime.newTuple(0));
  Tuple object_array1(&scope, runtime.newTuple(1));
  Tuple object_array16(&scope, runtime.newTuple(16));

  for (int i = 0; i < 4; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(list, value);
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

TEST(ListBuiltinsTest, ExtendSet) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list(&scope, runtime.newList());
  Set set(&scope, runtime.newSet());
  Object value(&scope, NoneType::object());
  word sum = 0;

  for (word i = 0; i < 16; i++) {
    value = SmallInt::fromWord(i);
    runtime.setAdd(thread, set, value);
    sum += i;
  }

  Object set_obj(&scope, *set);
  listExtend(Thread::current(), list, Object(&scope, *set_obj));
  EXPECT_EQ(list.numItems(), 16);

  for (word i = 0; i < 16; i++) {
    sum -= SmallInt::cast(list.at(i)).value();
  }
  ASSERT_EQ(sum, 0);
}

TEST(ListBuiltinsTest, ExtendDict) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list(&scope, runtime.newList());
  Dict dict(&scope, runtime.newDict());
  Object value(&scope, NoneType::object());
  word sum = 0;

  for (word i = 0; i < 16; i++) {
    value = SmallInt::fromWord(i);
    runtime.dictAtPut(thread, dict, value, value);
    sum += i;
  }

  Object dict_obj(&scope, *dict);
  listExtend(thread, list, Object(&scope, *dict_obj));
  EXPECT_EQ(list.numItems(), 16);

  for (word i = 0; i < 16; i++) {
    sum -= SmallInt::cast(list.at(i)).value();
  }
  ASSERT_EQ(sum, 0);
}

TEST(ListBuiltinsTest, ExtendIterator) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  Object iterable(&scope, runtime.newRange(1, 4, 1));
  listExtend(Thread::current(), list, iterable);

  EXPECT_PYLIST_EQ(list, {1, 2, 3});
}

TEST(ListBuiltinsTest, SortEmptyListSucceeds) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List empty(&scope, runtime.newList());
  ASSERT_EQ(listSort(thread, empty), NoneType::object());
}

TEST(ListBuiltinsTest, SortSingleElementListSucceeds) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list(&scope, runtime.newList());
  Object elt(&scope, SmallInt::fromWord(5));
  runtime.listAdd(list, elt);
  ASSERT_EQ(listSort(thread, list), NoneType::object());
  EXPECT_EQ(list.numItems(), 1);
  EXPECT_EQ(list.at(0), *elt);
}

TEST(ListBuiltinsTest, SortMultiElementListSucceeds) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list(&scope, runtime.newList());
  Object elt3(&scope, SmallInt::fromWord(3));
  runtime.listAdd(list, elt3);
  Object elt2(&scope, SmallInt::fromWord(2));
  runtime.listAdd(list, elt2);
  Object elt1(&scope, SmallInt::fromWord(1));
  runtime.listAdd(list, elt1);
  ASSERT_EQ(listSort(thread, list), NoneType::object());
  EXPECT_EQ(list.numItems(), 3);
  EXPECT_PYLIST_EQ(list, {1, 2, 3});
}

TEST(ListBuiltinsTest, SortMultiElementListSucceeds2) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list(&scope, runtime.newList());
  Object elt3(&scope, SmallInt::fromWord(1));
  runtime.listAdd(list, elt3);
  Object elt2(&scope, SmallInt::fromWord(3));
  runtime.listAdd(list, elt2);
  Object elt1(&scope, SmallInt::fromWord(2));
  runtime.listAdd(list, elt1);
  ASSERT_EQ(listSort(thread, list), NoneType::object());
  EXPECT_EQ(list.numItems(), 3);
  EXPECT_PYLIST_EQ(list, {1, 2, 3});
}

TEST(ListBuiltinsTest, SortIsStable) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list(&scope, runtime.newList());
  Object elt4(&scope, runtime.newStrFromCStr("q"));
  runtime.listAdd(list, elt4);
  Object elt3(&scope, runtime.newStrFromCStr("world"));
  runtime.listAdd(list, elt3);
  Object elt2(&scope, runtime.newStrFromCStr("hello"));
  runtime.listAdd(list, elt2);
  Object elt1(&scope, runtime.newStrFromCStr("hello"));
  runtime.listAdd(list, elt1);
  ASSERT_EQ(listSort(thread, list), NoneType::object());
  EXPECT_EQ(list.numItems(), 4);
  EXPECT_EQ(list.at(0), *elt2);
  EXPECT_EQ(list.at(1), *elt1);
  EXPECT_EQ(list.at(2), *elt4);
  EXPECT_EQ(list.at(3), *elt3);
}

TEST(ListBuiltinsTest, ListExtendSelfDuplicatesElements) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = [1, 2, 3]
a.extend(a)
)")
                   .isError());
  HandleScope scope;
  List a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_EQ(a.numItems(), 6);
  EXPECT_PYLIST_EQ(a, {1, 2, 3, 1, 2, 3});
}

TEST(ListBuiltinsTest, ListExtendListSubclassFallsBackToIter) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C(list):
  def __iter__(self):
    return [4, 5, 6].__iter__()
a = [1, 2, 3]
a.extend(C([1,2,3]))
)")
                   .isError());
  HandleScope scope;
  List a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_EQ(a.numItems(), 6);
  EXPECT_PYLIST_EQ(a, {1, 2, 3, 4, 5, 6});
}

TEST(ListBuiltinsTest, RecursiveListPrintsEllipsis) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
ls = []
ls.append(ls)
result = ls.__repr__()
)")
                   .isError());
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "[[...]]"));
}

TEST(ListBuiltinsTest, ReverseEmptyListDoesNothing) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = []
result.reverse()
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isList());
  EXPECT_EQ(List::cast(*result).numItems(), 0);
}

TEST(ListBuiltinsTest, ReverseOneElementListDoesNothing) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = [2]
result.reverse()
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isList());
  EXPECT_EQ(List::cast(*result).numItems(), 1);
  EXPECT_EQ(List::cast(*result).at(0), SmallInt::fromWord(2));
}

TEST(ListBuiltinsTest, ReverseOddManyElementListReversesList) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = [1, 2, 3]
result.reverse()
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {3, 2, 1});
}

TEST(ListBuiltinsTest, ReverseEvenManyElementListReversesList) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = [1, 2, 3, 4]
result.reverse()
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {4, 3, 2, 1});
}

TEST(ListBuiltinsTest, ReverseWithListSubclassDoesNotCallSubclassMethods) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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

TEST(ListBuiltinsTest, SortWithNonListRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
list.sort(None)
)"),
                            LayoutId::kTypeError,
                            "sort expected 'list' but got NoneType"));
  ;
}

TEST(ListBuiltinsTest, SortWithMultiElementListSortsElements) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
ls = [3, 2, 1]
ls.sort()
)")
                   .isError());
  HandleScope scope;
  Object ls(&scope, moduleAt(&runtime, "__main__", "ls"));
  EXPECT_PYLIST_EQ(ls, {1, 2, 3});
}

TEST(ListBuiltinsTest, SortWithNonCallableKeyRaisesException) {
  Runtime runtime;
  EXPECT_TRUE(raised(runFromCStr(&runtime, R"(
ls = [3, 2, 1]
ls.sort(key=5)
)"),
                     LayoutId::kTypeError));
  ;
}

TEST(ListBuiltinsTest, SortWithKeySortsAccordingToKey) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
ls = [2, 3, 1]
ls.sort(key=lambda x: -x)
)")
                   .isError());
  HandleScope scope;
  Object ls(&scope, moduleAt(&runtime, "__main__", "ls"));
  EXPECT_PYLIST_EQ(ls, {3, 2, 1});
}

TEST(ListBuiltinsTest, SortReverseReversesSortedList) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
ls = [2, 3, 1]
ls.sort(reverse=True)
)")
                   .isError());
  HandleScope scope;
  Object ls(&scope, moduleAt(&runtime, "__main__", "ls"));
  EXPECT_PYLIST_EQ(ls, {3, 2, 1});
}

TEST(ListBuiltinsTest, ClearWithNonListRaisesTypeError) {
  Runtime runtime;
  ASSERT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "list.clear(None)"), LayoutId::kTypeError,
      "'clear' requires a 'list' object but got 'NoneType'"));
}

TEST(ListBuiltinsTest, ClearRemovesElements) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
ls = [2, 3, 1]
list.clear(ls)
)")
                   .isError());
  HandleScope scope;
  Object ls(&scope, moduleAt(&runtime, "__main__", "ls"));
  EXPECT_PYLIST_EQ(ls, {});
}

TEST(ListBuiltinsTest, ClearRemovesAllElements) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  pass
l = [C()]
)")
                   .isError());

  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list(&scope, moduleAt(&runtime, "__main__", "l"));
  Object ref_obj(&scope, NoneType::object());
  {
    Object none(&scope, NoneType::object());
    Object c(&scope, list.at(0));
    ref_obj = runtime.newWeakRef(thread, c, none);
  }
  WeakRef ref(&scope, *ref_obj);
  EXPECT_NE(ref.referent(), NoneType::object());
  runBuiltin(ListBuiltins::clear, list);
  runtime.collectGarbage();
  EXPECT_EQ(ref.referent(), NoneType::object());
}

}  // namespace python
