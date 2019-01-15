#include "gtest/gtest.h"

#include "list-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ListBuiltinsTest, DunderInitFromList) {
  Runtime runtime;

  runFromCStr(&runtime, R"(
a = list([1, 2])
)");
  HandleScope scope;
  List a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_EQ(a->numItems(), 2);
  EXPECT_EQ(RawInt::cast(a->at(0))->asWord(), 1);
  EXPECT_EQ(RawInt::cast(a->at(1))->asWord(), 2);
}

TEST(ListBuiltinsTest, NewListIsNotASingleton) {
  Runtime runtime;

  runFromCStr(&runtime, R"(
a = list() is not list()
b = list([1, 2]) is not list([1, 2])
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(a->value());
  EXPECT_TRUE(b->value());
}

TEST(ListBuiltinsTest, AddToNonEmptyList) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = [1, 2]
b = [3, 4, 5]
c = a + b
)");
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_TRUE(c->isList());
  List list(&scope, RawList::cast(*c));
  EXPECT_EQ(RawSmallInt::cast(list->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(list->at(1))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(list->at(2))->value(), 3);
  EXPECT_EQ(RawSmallInt::cast(list->at(3))->value(), 4);
  EXPECT_EQ(RawSmallInt::cast(list->at(4))->value(), 5);
}

TEST(ListBuiltinsTest, AddToEmptyList) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = []
b = [1, 2, 3]
c = a + b
)");
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_TRUE(c->isList());
  List list(&scope, RawList::cast(*c));
  EXPECT_EQ(RawSmallInt::cast(list->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(list->at(1))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(list->at(2))->value(), 3);
}

TEST(ListBuiltinsDeathTest, AddWithNonListSelfThrows) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, "list.__add__(None, [])"),
               "must be called with list instance as first argument");
}

TEST(ListBuiltinsDeathTest, AddListToTupleThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
a = [1, 2, 3]
b = (4, 5, 6)
c = a + b
)"),
               "can only concatenate list to list");
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

TEST(ListBuiltinsDeathTest, ListInsertExcept) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
a = [1, 2]
a.insert()
)"),
               "aborting due to pending exception: "
               "insert\\(\\) takes exactly two arguments");

  ASSERT_DEATH(runFromCStr(&runtime, R"(
list.insert(1, 2, 3)
)"),
               "aborting due to pending exception: "
               "descriptor 'insert' requires a 'list' object");

  ASSERT_DEATH(runFromCStr(&runtime, R"(
a = [1, 2]
a.insert("i", "val")
)"),
               "aborting due to pending exception: "
               "index object cannot be interpreted as an integer");
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
  EXPECT_EQ(output, "4\n2 2 4\n");

  std::string output2 = compileAndRunToString(&runtime, R"(
a = [1,2,3,4,5]
print(a.pop(), a.pop(0), a.pop(-2))
)");
  EXPECT_EQ(output2, "5 1 2\n");
}

TEST(ListBuiltinsDeathTest, ListPopExcept) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
a = [1, 2]
a.pop(1, 2, 3, 4)
)"),
               "aborting due to pending exception: "
               "pop\\(\\) takes at most 1 argument");

  ASSERT_DEATH(runFromCStr(&runtime, "list.pop(1)"),
               "aborting due to pending exception: "
               "descriptor 'pop' requires a 'list' object");

  ASSERT_DEATH(runFromCStr(&runtime, R"(
a = [1, 2]
a.pop("i")
)"),
               "aborting due to pending exception: "
               "index object cannot be interpreted as an integer");

  ASSERT_DEATH(runFromCStr(&runtime, R"(
a = [1]
a.pop()
a.pop()
)"),
               "unimplemented: "
               "Throw an RawIndexError for an out of range list");

  ASSERT_DEATH(runFromCStr(&runtime, R"(
a = [1]
a.pop(3)
)"),
               "unimplemented: "
               "Throw an RawIndexError for an out of range list");
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [2:]
  Object start(&scope, SmallInt::fromWord(2));
  Object stop(&scope, NoneType::object());
  Object step(&scope, NoneType::object());
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  List test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->numItems(), 3);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 3);
  EXPECT_EQ(RawSmallInt::cast(test->at(1))->value(), 4);
  EXPECT_EQ(RawSmallInt::cast(test->at(2))->value(), 5);
}

TEST(ListBuiltinsTest, SliceNegativeStartIndexIsRelativeToEnd) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [-2:]
  Object start(&scope, SmallInt::fromWord(-2));
  Object stop(&scope, NoneType::object());
  Object step(&scope, NoneType::object());
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  List test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->numItems(), 2);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 4);
  EXPECT_EQ(RawSmallInt::cast(test->at(1))->value(), 5);
}

TEST(ListBuiltinsTest, SlicePositiveStopIndex) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [:2]
  Object start(&scope, NoneType::object());
  Object stop(&scope, SmallInt::fromWord(2));
  Object step(&scope, NoneType::object());
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  List test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->numItems(), 2);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(test->at(1))->value(), 2);
}

TEST(ListBuiltinsTest, SliceNegativeStopIndexIsRelativeToEnd) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [:-2]
  Object start(&scope, NoneType::object());
  Object stop(&scope, SmallInt::fromWord(-2));
  Object step(&scope, NoneType::object());
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  List test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->numItems(), 3);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(test->at(1))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(test->at(2))->value(), 3);
}

TEST(ListBuiltinsTest, SlicePositiveStep) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [::2]
  Object start(&scope, NoneType::object());
  Object stop(&scope, NoneType::object());
  Object step(&scope, SmallInt::fromWord(2));
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  List test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->numItems(), 3);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(test->at(1))->value(), 3);
  EXPECT_EQ(RawSmallInt::cast(test->at(2))->value(), 5);
}

TEST(ListBuiltinsTest, SliceNegativeStepReversesOrder) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [::-2]
  Object start(&scope, NoneType::object());
  Object stop(&scope, NoneType::object());
  Object step(&scope, SmallInt::fromWord(-2));
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  List test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->numItems(), 3);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 5);
  EXPECT_EQ(RawSmallInt::cast(test->at(1))->value(), 3);
  EXPECT_EQ(RawSmallInt::cast(test->at(2))->value(), 1);
}

TEST(ListBuiltinsTest, SliceStartOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [10::]
  Object start(&scope, SmallInt::fromWord(10));
  Object stop(&scope, NoneType::object());
  Object step(&scope, NoneType::object());
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  List test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->numItems(), 0);
}

TEST(ListBuiltinsTest, SliceStopOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [:10]
  Object start(&scope, NoneType::object());
  Object stop(&scope, SmallInt::fromWord(10));
  Object step(&scope, NoneType::object());
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  List test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->numItems(), 5);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(test->at(4))->value(), 5);
}

TEST(ListBuiltinsTest, SliceStepOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test [::10]
  Object start(&scope, NoneType::object());
  Object stop(&scope, NoneType::object());
  Object step(&scope, SmallInt::fromWord(10));
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  List test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->numItems(), 1);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 1);
}

TEST(ListBuiltinsTest, IdenticalSliceIsCopy) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List list1(&scope, listFromRange(1, 6));

  // Test: t[::] is t
  Object start(&scope, NoneType::object());
  Object stop(&scope, NoneType::object());
  Object step(&scope, NoneType::object());
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  List test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->numItems(), 5);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(test->at(4))->value(), 5);
  ASSERT_NE(*test, *list1);
}

TEST(ListBuiltinsTest, SetItem) {
  Runtime runtime;
  HandleScope scope;

  List list(&scope, listFromRange(1, 5));
  Object zero(&scope, SmallInt::fromWord(0));
  Object num(&scope, SmallInt::fromWord(2));

  EXPECT_EQ(RawSmallInt::cast(list->at(0))->value(), 1);
  Object result(&scope,
                runBuiltin(ListBuiltins::dunderSetItem, list, zero, num));
  EXPECT_TRUE(result->isNoneType());
  EXPECT_EQ(RawSmallInt::cast(list->at(0))->value(), 2);

  // Negative index
  Object minus_one(&scope, SmallInt::fromWord(-1));
  EXPECT_EQ(RawSmallInt::cast(list->at(3))->value(), 4);
  Object result1(&scope,
                 runBuiltin(ListBuiltins::dunderSetItem, list, minus_one, num));
  EXPECT_TRUE(result1->isNoneType());
  EXPECT_EQ(RawSmallInt::cast(list->at(3))->value(), 2);
}

TEST(ListBuiltinsTest, GetItemWithNegativeIndex) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(1, 4));
  Object idx(&scope, SmallInt::fromWord(-3));
  Object result(&scope, runBuiltin(ListBuiltins::dunderGetItem, list, idx));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 1);
}

TEST(ListBuiltinsTest, DelItemWithNegativeIndex) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(1, 4));
  Object idx(&scope, SmallInt::fromWord(-3));
  Object result(&scope, runBuiltin(ListBuiltins::dunderDelItem, list, idx));
  ASSERT_TRUE(result->isNoneType());
  ASSERT_EQ(list->numItems(), 2);
  EXPECT_EQ(RawSmallInt::cast(list->at(0))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(list->at(1))->value(), 3);
}

TEST(ListBuiltinsTest, SetItemWithNegativeIndex) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(1, 4));
  Object idx(&scope, SmallInt::fromWord(-3));
  Object num(&scope, SmallInt::fromWord(0));
  Object result(&scope,
                runBuiltin(ListBuiltins::dunderSetItem, list, idx, num));
  ASSERT_TRUE(result->isNoneType());
  ASSERT_EQ(list->numItems(), 3);
  EXPECT_EQ(RawSmallInt::cast(list->at(0))->value(), 0);
  EXPECT_EQ(RawSmallInt::cast(list->at(1))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(list->at(2))->value(), 3);
}

TEST(ListBuiltinsDeathTest, GetItemWithInvalidNegativeIndexThrows) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
l = [1, 2, 3]
l[-4]
)"),
               "list index out of range");
}

TEST(ListBuiltinsDeathTest, DelItemWithInvalidNegativeIndexThrows) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
l = [1, 2, 3]
del l[-4]
)"),
               "list assignment index out of range");
}

TEST(ListBuiltinsDeathTest, SetItemWithInvalidNegativeIndexThrows) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
l = [1, 2, 3]
l[-4] = 0
)"),
               "list assignment index out of range");
}

TEST(ListBuiltinsDeathTest, GetItemWithInvalidIndexThrows) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
l = [1, 2, 3]
l[5]
)"),
               "list index out of range");
}

TEST(ListBuiltinsDeathTest, DelItemWithInvalidIndexThrows) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
l = [1, 2, 3]
del l[5]
)"),
               "list assignment index out of range");
}

TEST(ListBuiltinsDeathTest, SetItemWithInvalidIndexThrows) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
l = [1, 2, 3]
l[5] = 4
)"),
               "list assignment index out of range");
}

TEST(ListBuiltinsTest, SetItemSliceBasic) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:5] = ['C', 'D', 'E']
result = letters
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"a", "b", "C", "D", "E", "f", "g"});
}

TEST(ListBuiltinsTest, SetItemSliceGrow) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:5] = ['C', 'D', 'E','X','Y','Z']
result = letters
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"a", "b", "C", "D", "E", "X", "Y", "Z", "f", "g"});
}

TEST(ListBuiltinsTest, SetItemSliceShrink) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:6] = ['C', 'D']
result = letters
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"a", "b", "C", "D", "g"});
}

TEST(ListBuiltinsTest, SetItemSliceIterable) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:6] = ('x', 'y', 12)
result = letters
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"a", "b", "x", "y", 12, "g"});
}

TEST(ListBuiltinsTest, SetItemSliceSelf) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:5] = letters
result = letters
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result,
                   {"a", "b", "a", "b", "c", "d", "e", "f", "g", "f", "g"});
}

// Reverse ordered bounds, but step still +1:
TEST(ListBuiltinsTest, SetItemSliceRevBounds) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
a = list(range(20))
a[5:2] = ['a','b','c','d','e']
result = a
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0, 1, 2,  3,  4,  "a", "b", "c", "d", "e", 5,  6, 7,
                            8, 9, 10, 11, 12, 13,  14,  15,  16,  17,  18, 19});
}

TEST(ListBuiltinsTest, SetItemSliceStep) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
a = list(range(20))
a[2:10:3] = ['a', 'b', 'c']
result = a
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0,  1,  "a", 3,  4,  "b", 6,  7,  "c", 9,
                            10, 11, 12,  13, 14, 15,  16, 17, 18,  19});
}

TEST(ListBuiltinsTest, SetItemSliceStepNeg) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
a = list(range(20))
a[10:2:-3] = ['a', 'b', 'c']
result = a
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0,   1,  2,  3,  "c", 5,  6,  "b", 8,  9,
                            "a", 11, 12, 13, 14,  15, 16, 17,  18, 19});
}

TEST(ListBuiltinsTest, SetItemSliceStepSizeErr) {
  Runtime runtime;
  ASSERT_DEATH(
      runFromCStr(&runtime, R"(
a = list(range(20))
a[2:10:3] = ['a', 'b', 'c', 'd']
)"),
      "attempt to assign sequence of size 4 to extended slice of size 3");
}

TEST(ListBuiltinsTest, SetItemSliceScalarErr) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:6] = 5
)"),
               "can only assign an iterable");
}

TEST(ListBuiltinsTest, SetItemSliceStepTuple) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
a = list(range(20))
a[2:10:3] = ('a', 'b', 'c')
result = a
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0,  1,  "a", 3,  4,  "b", 6,  7,  "c", 9,
                            10, 11, 12,  13, 14, 15,  16, 17, 18,  19});
}

TEST(ListBuiltinsTest, SetItemSliceShortValue) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_DEATH(
      runFromCStr(&runtime, R"(
a = [1,2,3,4,5,6,7,8,9,10]
b = [0,0,0]
a[:8:2] = b
)"),
      "attempt to assign sequence of size 3 to extended slice of size 4");
}
TEST(ListBuiltinsTest, SetItemSliceShortStop) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
a = [1,2,3,4,5,6,7,8,9,10]
b = [0,0,0]
a[:1] = b
result = a
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0, 0, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10});
}

TEST(ListBuiltinsTest, SetItemSliceLongStop) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
a = [1,1,1]
b = [0,0,0,0,0]
a[:1] = b
result = a
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0, 0, 0, 0, 0, 1, 1});
}

TEST(ListBuiltinsTest, SetItemSliceShortStep) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
a = [1,2,3,4,5,6,7,8,9,10]
b = [0,0,0]
a[::1] = b
result = a
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0, 0, 0});
}

TEST(ListBuiltinsDeathTest, GetItemWithTooFewArgumentsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
[].__getitem__()
)"),
               R"(__getitem__\(\) takes exactly one argument \(0 given\))");
}

TEST(ListBuiltinsDeathTest, DelItemWithTooFewArgumentsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
[].__delitem__()
)"),
               "expected 1 arguments, got 0");
}

TEST(ListBuiltinsDeathTest, SetItemWithTooFewArgumentsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
[].__setitem__(1)
)"),
               "expected 2 arguments, got 1");
}

TEST(ListBuiltinsDeathTest, DelItemWithTooManyArgumentsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
[].__delitem__(1, 2)
)"),
               "expected 1 arguments, got 2");
}

TEST(ListBuiltinsDeathTest, GetItemWithTooManyArgumentsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
[].__getitem__(1, 2)
)"),
               R"(__getitem__\(\) takes exactly one argument \(2 given\))");
}

TEST(ListBuiltinsDeathTest, SetItemWithTooManyArgumentsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
[].__setitem__(1, 2, 3)
)"),
               "expected 2 arguments, got 3");
}

TEST(ListBuiltinsDeathTest, GetItemWithNonIntegralIndexThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
[].__getitem__("test")
)"),
               "list indices must be integers or slices");
}

TEST(ListBuiltinsDeathTest, DelItemWithNonIntegralIndexThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
[].__delitem__("test")
)"),
               "list indices must be integers or slices");
}

TEST(ListBuiltinsDeathTest, SetItemWithNonIntegralIndexThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
[].__setitem__("test", 1)
)"),
               "list indices must be integers or slices");
}

TEST(ListBuiltinsDeathTest, NonTypeInDunderNew) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
list.__new__(1)
)"),
               "not a type object");
}

TEST(ListBuiltinsDeathTest, NonSubclassInDunderNew) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
class Foo: pass
list.__new__(Foo)
)"),
               "not a subtype of list");
}

TEST(ListBuiltinsTest, SubclassList) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
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
)");
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
  runFromCStr(&runtime, R"(
a = [42,'foo', 'bar']
del a[2]
del a[0]
l = len(a)
e = a[0]
)");
  Object len(&scope, moduleAt(&runtime, "__main__", "l"));
  Object element(&scope, moduleAt(&runtime, "__main__", "e"));
  EXPECT_EQ(*len, SmallInt::fromWord(1));
  EXPECT_EQ(*element, SmallStr::fromCStr("foo"));
}

TEST(ListBuiltinsTest, DunderIterReturnsListIter) {
  Runtime runtime;
  HandleScope scope;
  List empty_list(&scope, listFromRange(0, 0));
  Object iter(&scope, runBuiltin(ListBuiltins::dunderIter, empty_list));
  ASSERT_TRUE(iter->isListIterator());
}

TEST(ListBuiltinsTest, DunderRepr) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
result = [1, 2, 'hello'].__repr__()
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "[1, 2, 'hello']");
}

TEST(ListIteratorBuiltinsTest, CallDunderNext) {
  Runtime runtime;
  HandleScope scope;
  List empty_list(&scope, listFromRange(0, 2));
  Object iter(&scope, runBuiltin(ListBuiltins::dunderIter, empty_list));
  ASSERT_TRUE(iter->isListIterator());

  Object item1(&scope, runBuiltin(ListIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*item1)->value(), 0);

  Object item2(&scope, runBuiltin(ListIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item2->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*item2)->value(), 1);
}

TEST(ListIteratorBuiltinsTest, DunderIterReturnsSelf) {
  Runtime runtime;
  HandleScope scope;
  List empty_list(&scope, listFromRange(0, 0));
  Object iter(&scope, runBuiltin(ListBuiltins::dunderIter, empty_list));
  ASSERT_TRUE(iter->isListIterator());

  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(ListIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(ListIteratorBuiltinsTest, DunderLengthHintOnEmptyListIterator) {
  Runtime runtime;
  HandleScope scope;
  List empty_list(&scope, listFromRange(0, 0));
  Object iter(&scope, runBuiltin(ListBuiltins::dunderIter, empty_list));
  ASSERT_TRUE(iter->isListIterator());

  Object length_hint(&scope,
                     runBuiltin(ListIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint)->value(), 0);
}

TEST(ListIteratorBuiltinsTest, DunderLengthHintOnConsumedListIterator) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, listFromRange(0, 1));
  Object iter(&scope, runBuiltin(ListBuiltins::dunderIter, list));
  ASSERT_TRUE(iter->isListIterator());

  Object length_hint1(&scope,
                      runBuiltin(ListIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint1)->value(), 1);

  // Consume the iterator
  Object item1(&scope, runBuiltin(ListIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*item1)->value(), 0);

  Object length_hint2(&scope,
                      runBuiltin(ListIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint2)->value(), 0);
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
  ASSERT_NE(RawSmallInt::cast(list->at(1))->value(), 1);
  ASSERT_NE(RawSmallInt::cast(list->at(6))->value(), 6);

  Thread* thread = Thread::currentThread();
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
  ASSERT_EQ(list->numItems(), 10);

  Thread* thread = Thread::currentThread();
  Object value100(&scope, SmallInt::fromWord(100));
  listInsert(thread, list, value100, 100);
  ASSERT_EQ(list->numItems(), 11);
  ASSERT_EQ(RawSmallInt::cast(list->at(10))->value(), 100);

  Object value0(&scope, SmallInt::fromWord(400));
  listInsert(thread, list, value0, 0);
  ASSERT_EQ(list->numItems(), 12);
  ASSERT_EQ(RawSmallInt::cast(list->at(0))->value(), 400);

  Object value_n(&scope, SmallInt::fromWord(-10));
  listInsert(thread, list, value_n, -10);
  ASSERT_EQ(list->numItems(), 13);
  ASSERT_EQ(RawSmallInt::cast(list->at(2))->value(), -10);
}

TEST(ListBuiltinsTest, PopList) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  for (int i = 0; i < 16; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(list, value);
  }
  ASSERT_EQ(list->numItems(), 16);

  // Pop from the end
  RawObject res1 = listPop(list, 15);
  ASSERT_EQ(list->numItems(), 15);
  ASSERT_EQ(RawSmallInt::cast(list->at(14))->value(), 14);
  ASSERT_EQ(RawSmallInt::cast(res1)->value(), 15);

  // Pop elements from 5 - 10
  for (int i = 0; i < 5; i++) {
    RawObject res5 = listPop(list, 5);
    ASSERT_EQ(RawSmallInt::cast(res5)->value(), i + 5);
  }
  ASSERT_EQ(list->numItems(), 10);
  for (int i = 0; i < 5; i++) {
    RawSmallInt elem = RawSmallInt::cast(list->at(i));
    ASSERT_EQ(elem->value(), i);
  }
  for (int i = 5; i < 10; i++) {
    RawSmallInt elem = RawSmallInt::cast(list->at(i));
    ASSERT_EQ(elem->value(), i + 5);
  }

  // Pop element 0
  RawObject res0 = listPop(list, 0);
  ASSERT_EQ(list->numItems(), 9);
  ASSERT_EQ(RawSmallInt::cast(list->at(0))->value(), 1);
  ASSERT_EQ(RawSmallInt::cast(res0)->value(), 0);
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
  EXPECT_EQ(list->numItems(), 4);
  Object list1_handle(&scope, *list1);
  listExtend(Thread::currentThread(), list, list1_handle);
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
  EXPECT_EQ(list->numItems(), 4);
  Object list1_handle(&scope, *list1);
  Object list1_iterator(&scope, runtime.newListIterator(list1_handle));
  listExtend(Thread::currentThread(), list, list1_iterator);
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
  listExtend(Thread::currentThread(), list, object_array0);
  EXPECT_EQ(list->numItems(), 4);

  Object object_array1_handle(&scope, *object_array1);
  object_array1->atPut(0, NoneType::object());
  listExtend(Thread::currentThread(), list, object_array1_handle);
  ASSERT_GE(list->numItems(), 5);
  ASSERT_TRUE(list->at(4)->isNoneType());

  for (word i = 0; i < 4; i++) {
    object_array16->atPut(i, SmallInt::fromWord(i));
  }

  Object object_array2_handle(&scope, *object_array16);
  listExtend(Thread::currentThread(), list, object_array2_handle);
  ASSERT_GE(list->numItems(), 4 + 1 + 4);
  EXPECT_EQ(list->at(5), SmallInt::fromWord(0));
  EXPECT_EQ(list->at(6), SmallInt::fromWord(1));
  EXPECT_EQ(list->at(7), SmallInt::fromWord(2));
  EXPECT_EQ(list->at(8), SmallInt::fromWord(3));
}

TEST(ListBuiltinsTest, ExtendSet) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  Set set(&scope, runtime.newSet());
  Object value(&scope, NoneType::object());
  word sum = 0;

  for (word i = 0; i < 16; i++) {
    value = SmallInt::fromWord(i);
    runtime.setAdd(set, value);
    sum += i;
  }

  Object set_obj(&scope, *set);
  listExtend(Thread::currentThread(), list, Object(&scope, *set_obj));
  EXPECT_EQ(list->numItems(), 16);

  for (word i = 0; i < 16; i++) {
    sum -= RawSmallInt::cast(list->at(i))->value();
  }
  ASSERT_EQ(sum, 0);
}

TEST(ListBuiltinsTest, ExtendDict) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  Dict dict(&scope, runtime.newDict());
  Object value(&scope, NoneType::object());
  word sum = 0;

  for (word i = 0; i < 16; i++) {
    value = SmallInt::fromWord(i);
    runtime.dictAtPut(dict, value, value);
    sum += i;
  }

  Object dict_obj(&scope, *dict);
  listExtend(Thread::currentThread(), list, Object(&scope, *dict_obj));
  EXPECT_EQ(list->numItems(), 16);

  for (word i = 0; i < 16; i++) {
    sum -= RawSmallInt::cast(list->at(i))->value();
  }
  ASSERT_EQ(sum, 0);
}

TEST(ListBuiltinsTest, ExtendIterator) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  Object iterable(&scope, runtime.newRange(1, 4, 1));
  listExtend(Thread::currentThread(), list, iterable);

  EXPECT_PYLIST_EQ(list, {1, 2, 3});
}

TEST(ListBuiltinsTest, SortEmptyListSucceeds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List empty(&scope, runtime.newList());
  ASSERT_EQ(listSort(thread, empty), NoneType::object());
}

TEST(ListBuiltinsTest, SortSingleElementListSucceeds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List list(&scope, runtime.newList());
  Object elt(&scope, SmallInt::fromWord(5));
  runtime.listAdd(list, elt);
  ASSERT_EQ(listSort(thread, list), NoneType::object());
  EXPECT_EQ(list->numItems(), 1);
  EXPECT_EQ(list->at(0), *elt);
}

TEST(ListBuiltinsTest, SortMultiElementListSucceeds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List list(&scope, runtime.newList());
  Object elt3(&scope, SmallInt::fromWord(3));
  runtime.listAdd(list, elt3);
  Object elt2(&scope, SmallInt::fromWord(2));
  runtime.listAdd(list, elt2);
  Object elt1(&scope, SmallInt::fromWord(1));
  runtime.listAdd(list, elt1);
  ASSERT_EQ(listSort(thread, list), NoneType::object());
  EXPECT_EQ(list->numItems(), 3);
  EXPECT_PYLIST_EQ(list, {1, 2, 3});
}

TEST(ListBuiltinsTest, SortIsStable) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
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
  EXPECT_EQ(list->numItems(), 4);
  EXPECT_EQ(list->at(0), *elt2);
  EXPECT_EQ(list->at(1), *elt1);
  EXPECT_EQ(list->at(2), *elt4);
  EXPECT_EQ(list->at(3), *elt3);
}

}  // namespace python
