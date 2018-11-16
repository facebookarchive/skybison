#include "gtest/gtest.h"

#include "list-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ListBuiltinsTest, DunderInitFromList) {
  Runtime runtime;

  runtime.runFromCStr(R"(
a = list([1, 2])
)");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  List a(&scope, moduleAt(&runtime, main, "a"));
  ASSERT_EQ(a->numItems(), 2);
  EXPECT_EQ(RawInt::cast(a->at(0))->asWord(), 1);
  EXPECT_EQ(RawInt::cast(a->at(1))->asWord(), 2);
}

TEST(ListBuiltinsTest, NewListIsNotASingleton) {
  Runtime runtime;

  runtime.runFromCStr(R"(
a = list() is not list()
b = list([1, 2]) is not list([1, 2])
)");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  Bool b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_TRUE(a->value());
  EXPECT_TRUE(b->value());
}

TEST(ListBuiltinsTest, AddToNonEmptyList) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = [1, 2]
b = [3, 4, 5]
c = a + b
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
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

  runtime.runFromCStr(R"(
a = []
b = [1, 2, 3]
c = a + b
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(c->isList());
  List list(&scope, RawList::cast(*c));
  EXPECT_EQ(RawSmallInt::cast(list->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(list->at(1))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(list->at(2))->value(), 3);
}

TEST(ListBuiltinsDeathTest, AddWithNonListSelfThrows) {
  const char* src = R"(
list.__add__(None, [])
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "must be called with list instance as first argument");
}

TEST(ListBuiltinsDeathTest, AddListToTupleThrowsTypeError) {
  const char* src = R"(
a = [1, 2, 3]
b = (4, 5, 6)
c = a + b
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "can only concatenate list to list");
}

TEST(ListBuiltinsTest, ListAppend) {
  const char* src = R"(
a = list()
b = list()
a.append(1)
a.append("2")
b.append(3)
a.append(b)
print(a[0], a[1], a[2][0])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2 3\n");
}

TEST(ListBuiltinsTest, ListExtend) {
  const char* src = R"(
a = []
b = [1, 2, 3]
r = a.extend(b)
print(r is None, len(b) == 3)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "True True\n");
}

TEST(ListBuiltinsDeathTest, ListInsertExcept) {
  Runtime runtime;
  const char* src1 = R"(
a = [1, 2]
a.insert()
)";
  ASSERT_DEATH(runtime.runFromCStr(src1),
               "aborting due to pending exception: "
               "insert\\(\\) takes exactly two arguments");

  const char* src2 = R"(
list.insert(1, 2, 3)
)";
  ASSERT_DEATH(runtime.runFromCStr(src2),
               "aborting due to pending exception: "
               "descriptor 'insert' requires a 'list' object");

  const char* src3 = R"(
a = [1, 2]
a.insert("i", "val")
)";
  ASSERT_DEATH(runtime.runFromCStr(src3),
               "aborting due to pending exception: "
               "index object cannot be interpreted as an integer");
}

TEST(ListBuiltinsTest, ListPop) {
  const char* src = R"(
a = [1,2,3,4,5]
a.pop()
print(len(a))
a.pop(0)
a.pop(-1)
print(len(a), a[0], a[1])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "4\n2 2 4\n");

  const char* src2 = R"(
a = [1,2,3,4,5]
print(a.pop(), a.pop(0), a.pop(-2))
)";
  std::string output2 = compileAndRunToString(&runtime, src2);
  EXPECT_EQ(output2, "5 1 2\n");
}

TEST(ListBuiltinsDeathTest, ListPopExcept) {
  Runtime runtime;
  const char* src1 = R"(
a = [1, 2]
a.pop(1, 2, 3, 4)
)";
  ASSERT_DEATH(runtime.runFromCStr(src1),
               "aborting due to pending exception: "
               "pop\\(\\) takes at most 1 argument");

  const char* src2 = R"(
list.pop(1)
)";
  ASSERT_DEATH(runtime.runFromCStr(src2),
               "aborting due to pending exception: "
               "descriptor 'pop' requires a 'list' object");

  const char* src3 = R"(
a = [1, 2]
a.pop("i")
)";
  ASSERT_DEATH(runtime.runFromCStr(src3),
               "aborting due to pending exception: "
               "index object cannot be interpreted as an integer");

  const char* src4 = R"(
a = [1]
a.pop()
a.pop()
)";
  ASSERT_DEATH(runtime.runFromCStr(src4),
               "unimplemented: "
               "Throw an RawIndexError for an out of range list");

  const char* src5 = R"(
a = [1]
a.pop(3)
)";
  ASSERT_DEATH(runtime.runFromCStr(src5),
               "unimplemented: "
               "Throw an RawIndexError for an out of range list");
}

TEST(ListBuiltinsTest, ListRemove) {
  const char* src = R"(
a = [5, 4, 3, 2, 1]
a.remove(2)
a.remove(5)
print(len(a), a[0], a[1], a[2])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "3 4 3 1\n");
}

TEST(ListBuiltinsTest, PrintList) {
  const char* src = R"(
a = [1, 0, True]
print(a)
a[0]=7
print(a)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "[1, 0, True]\n[7, 0, True]\n");
}

TEST(ListBuiltinsTest, ReplicateList) {
  const char* src = R"(
data = [1, 2, 3] * 3
for i in range(9):
  print(data[i])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n3\n1\n2\n3\n1\n2\n3\n");
}

TEST(ListBuiltinsTest, SubscriptList) {
  Runtime runtime;
  const char* src = R"(
l = [1, 2, 3, 4, 5, 6]
print(l[0], l[3], l[5])
l[0] = 6
l[5] = 1
print(l[0], l[3], l[5])
)";
  std::string output = compileAndRunToString(&runtime, src);
  ASSERT_EQ(output, "1 4 6\n6 4 1\n");
}

// Equivalent to evaluating "list(range(start, stop))" in Python
static RawObject listFromRange(word start, word stop) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List result(&scope, thread->runtime()->newList());
  Object value(&scope, NoneType::object());
  for (word i = start; i < stop; i++) {
    value = SmallInt::fromWord(i);
    thread->runtime()->listAdd(result, value);
  }
  return *result;
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
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 3, 0);
  List list(&scope, listFromRange(1, 5));

  frame->setLocal(0, RawObject::cast(*list));
  frame->setLocal(1, SmallInt::fromWord(0));
  frame->setLocal(2, SmallInt::fromWord(2));

  EXPECT_EQ(RawSmallInt::cast(list->at(0))->value(), 1);
  Object result(&scope, ListBuiltins::dunderSetItem(thread, frame, 3));
  EXPECT_TRUE(result->isNoneType());
  EXPECT_EQ(RawSmallInt::cast(list->at(0))->value(), 2);

  // Negative index
  frame->setLocal(1, SmallInt::fromWord(-1));
  EXPECT_EQ(RawSmallInt::cast(list->at(3))->value(), 4);
  Object result1(&scope, ListBuiltins::dunderSetItem(thread, frame, 3));
  EXPECT_TRUE(result1->isNoneType());
  EXPECT_EQ(RawSmallInt::cast(list->at(3))->value(), 2);
}

TEST(ListBuiltinsTest, GetItemWithNegativeIndex) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  List list(&scope, listFromRange(1, 4));
  frame->setLocal(0, RawObject::cast(*list));
  frame->setLocal(1, SmallInt::fromWord(-3));

  Object result(&scope, ListBuiltins::dunderGetItem(thread, frame, 2));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 1);
  thread->popFrame();
}

TEST(ListBuiltinsTest, DelItemWithNegativeIndex) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  List list(&scope, listFromRange(1, 4));
  frame->setLocal(0, RawObject::cast(*list));
  frame->setLocal(1, SmallInt::fromWord(-3));

  Object result(&scope, ListBuiltins::dunderDelItem(thread, frame, 2));
  ASSERT_TRUE(result->isNoneType());
  ASSERT_EQ(list->numItems(), 2);
  EXPECT_EQ(RawSmallInt::cast(list->at(0))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(list->at(1))->value(), 3);
  thread->popFrame();
}

TEST(ListBuiltinsTest, SetItemWithNegativeIndex) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 3, 0);
  List list(&scope, listFromRange(1, 4));
  frame->setLocal(0, RawObject::cast(*list));
  frame->setLocal(1, SmallInt::fromWord(-3));
  frame->setLocal(2, SmallInt::fromWord(0));

  Object result(&scope, ListBuiltins::dunderSetItem(thread, frame, 3));
  ASSERT_TRUE(result->isNoneType());
  ASSERT_EQ(list->numItems(), 3);
  EXPECT_EQ(RawSmallInt::cast(list->at(0))->value(), 0);
  EXPECT_EQ(RawSmallInt::cast(list->at(1))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(list->at(2))->value(), 3);
  thread->popFrame();
}

TEST(ListBuiltinsDeathTest, GetItemWithInvalidNegativeIndexThrows) {
  const char* src = R"(
l = [1, 2, 3]
l[-4]
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "list index out of range");
}

TEST(ListBuiltinsDeathTest, DelItemWithInvalidNegativeIndexThrows) {
  const char* src = R"(
l = [1, 2, 3]
del l[-4]
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "list assignment index out of range");
}

TEST(ListBuiltinsDeathTest, SetItemWithInvalidNegativeIndexThrows) {
  const char* src = R"(
l = [1, 2, 3]
l[-4] = 0
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "list assignment index out of range");
}

TEST(ListBuiltinsDeathTest, GetItemWithInvalidIndexThrows) {
  const char* src = R"(
l = [1, 2, 3]
l[5]
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "list index out of range");
}

TEST(ListBuiltinsDeathTest, DelItemWithInvalidIndexThrows) {
  const char* src = R"(
l = [1, 2, 3]
del l[5]
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "list assignment index out of range");
}

TEST(ListBuiltinsDeathTest, SetItemWithInvalidIndexThrows) {
  const char* src = R"(
l = [1, 2, 3]
l[5] = 4
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "list assignment index out of range");
}

TEST(ListBuiltinsTest, SetItemSliceBasic) {
  const char* src = R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:5] = ['C', 'D', 'E']
result = letters
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"a", "b", "C", "D", "E", "f", "g"});
}

TEST(ListBuiltinsTest, SetItemSliceGrow) {
  const char* src = R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:5] = ['C', 'D', 'E','X','Y','Z']
result = letters
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"a", "b", "C", "D", "E", "X", "Y", "Z", "f", "g"});
}

TEST(ListBuiltinsTest, SetItemSliceShrink) {
  const char* src = R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:6] = ['C', 'D']
result = letters
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"a", "b", "C", "D", "g"});
}

TEST(ListBuiltinsTest, SetItemSliceIterable) {
  const char* src = R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:6] = ('x', 'y', 12)
result = letters
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"a", "b", "x", "y", 12, "g"});
}

TEST(ListBuiltinsTest, SetItemSliceSelf) {
  const char* src = R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:5] = letters
result = letters
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result,
                   {"a", "b", "a", "b", "c", "d", "e", "f", "g", "f", "g"});
}

// Reverse ordered bounds, but step still +1:
TEST(ListBuiltinsTest, SetItemSliceRevBounds) {
  const char* src = R"(
a = list(range(20))
a[5:2] = ['a','b','c','d','e']
result = a
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0, 1, 2,  3,  4,  "a", "b", "c", "d", "e", 5,  6, 7,
                            8, 9, 10, 11, 12, 13,  14,  15,  16,  17,  18, 19});
}

TEST(ListBuiltinsTest, SetItemSliceStep) {
  const char* src = R"(
a = list(range(20))
a[2:10:3] = ['a', 'b', 'c']
result = a
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0,  1,  "a", 3,  4,  "b", 6,  7,  "c", 9,
                            10, 11, 12,  13, 14, 15,  16, 17, 18,  19});
}

TEST(ListBuiltinsTest, SetItemSliceStepNeg) {
  const char* src = R"(
a = list(range(20))
a[10:2:-3] = ['a', 'b', 'c']
result = a
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0,   1,  2,  3,  "c", 5,  6,  "b", 8,  9,
                            "a", 11, 12, 13, 14,  15, 16, 17,  18, 19});
}

TEST(ListBuiltinsTest, SetItemSliceStepSizeErr) {
  const char* src = R"(
a = list(range(20))
a[2:10:3] = ['a', 'b', 'c', 'd']
)";
  Runtime runtime;
  ASSERT_DEATH(
      runtime.runFromCStr(src),
      "attempt to assign sequence of size 4 to extended slice of size 3");
}

TEST(ListBuiltinsTest, SetItemSliceScalarErr) {
  const char* src = R"(
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
letters[2:6] = 5
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "can only assign an iterable");
}

TEST(ListBuiltinsTest, SetItemSliceStepTuple) {
  const char* src = R"(
a = list(range(20))
a[2:10:3] = ('a', 'b', 'c')
result = a
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0,  1,  "a", 3,  4,  "b", 6,  7,  "c", 9,
                            10, 11, 12,  13, 14, 15,  16, 17, 18,  19});
}

TEST(ListBuiltinsDeathTest, GetItemWithTooFewArgumentsThrowsTypeError) {
  const char* src = R"(
[].__getitem__()
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               R"(__getitem__\(\) takes exactly one argument \(0 given\))");
}

TEST(ListBuiltinsDeathTest, DelItemWithTooFewArgumentsThrowsTypeError) {
  const char* src = R"(
[].__delitem__()
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "expected 1 arguments, got 0");
}

TEST(ListBuiltinsDeathTest, SetItemWithTooFewArgumentsThrowsTypeError) {
  const char* src = R"(
[].__setitem__(1)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "expected 2 arguments, got 1");
}

TEST(ListBuiltinsDeathTest, DelItemWithTooManyArgumentsThrowsTypeError) {
  const char* src = R"(
[].__delitem__(1, 2)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "expected 1 arguments, got 2");
}

TEST(ListBuiltinsDeathTest, GetItemWithTooManyArgumentsThrowsTypeError) {
  const char* src = R"(
[].__getitem__(1, 2)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               R"(__getitem__\(\) takes exactly one argument \(2 given\))");
}

TEST(ListBuiltinsDeathTest, SetItemWithTooManyArgumentsThrowsTypeError) {
  const char* src = R"(
[].__setitem__(1, 2, 3)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "expected 2 arguments, got 3");
}

TEST(ListBuiltinsDeathTest, GetItemWithNonIntegralIndexThrowsTypeError) {
  const char* src = R"(
[].__getitem__("test")
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "list indices must be integers or slices");
}

TEST(ListBuiltinsDeathTest, DelItemWithNonIntegralIndexThrowsTypeError) {
  const char* src = R"(
[].__delitem__("test")
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "list indices must be integers or slices");
}

TEST(ListBuiltinsDeathTest, SetItemWithNonIntegralIndexThrowsTypeError) {
  const char* src = R"(
[].__setitem__("test", 1)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "list indices must be integers or slices");
}

TEST(ListBuiltinsDeathTest, NonTypeInDunderNew) {
  const char* src = R"(
list.__new__(1)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "not a type object");
}

TEST(ListBuiltinsDeathTest, NonSubclassInDunderNew) {
  const char* src = R"(
class Foo: pass
list.__new__(Foo)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "not a subtype of list");
}

TEST(ListBuiltinsTest, SubclassList) {
  const char* src = R"(
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
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object test1(&scope, moduleAt(&runtime, main, "test1"));
  Object test2(&scope, moduleAt(&runtime, main, "test2"));
  Object test3(&scope, moduleAt(&runtime, main, "test3"));
  Object test4(&scope, moduleAt(&runtime, main, "test4"));
  Object test5(&scope, moduleAt(&runtime, main, "test5"));
  Object test6(&scope, moduleAt(&runtime, main, "test6"));
  EXPECT_EQ(*test1, SmallInt::fromWord(1));
  EXPECT_EQ(*test2, SmallStr::fromCStr("a"));
  EXPECT_EQ(*test3, SmallInt::fromWord(2));
  EXPECT_EQ(*test4, SmallInt::fromWord(1));
  EXPECT_EQ(*test5, SmallInt::fromWord(2));
  EXPECT_EQ(*test6, SmallInt::fromWord(0));
}

TEST(ListBuiltinsTest, DelItem) {
  const char* src = R"(
a = [42,'foo', 'bar']
del a[2]
del a[0]
l = len(a)
e = a[0]
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object len(&scope, moduleAt(&runtime, main, "l"));
  Object element(&scope, moduleAt(&runtime, main, "e"));
  EXPECT_EQ(*len, SmallInt::fromWord(1));
  EXPECT_EQ(*element, SmallStr::fromCStr("foo"));
}

TEST(ListBuiltinsTest, DunderIterReturnsListIter) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  List empty_list(&scope, listFromRange(0, 0));

  frame->setLocal(0, *empty_list);
  Object iter(&scope, ListBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isListIterator());
}

TEST(ListBuiltinsTest, DunderRepr) {
  const char* src = R"(
result = [1, 2, 'hello'].__repr__()
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "[1, 2, 'hello']");
}

TEST(ListIteratorBuiltinsTest, CallDunderNext) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  List empty_list(&scope, listFromRange(0, 2));

  frame->setLocal(0, *empty_list);
  Object iter(&scope, ListBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isListIterator());

  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderNext));
  ASSERT_FALSE(next_method->isError());

  Object item1(&scope,
               Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*item1)->value(), 0);

  Object item2(&scope,
               Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item2->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*item2)->value(), 1);
}

TEST(ListIteratorBuiltinsTest, DunderIterReturnsSelf) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  List empty_list(&scope, listFromRange(0, 0));

  frame->setLocal(0, *empty_list);
  Object iter(&scope, ListBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isListIterator());

  // Now call __iter__ on the iterator object
  Object iter_iter(&scope, Interpreter::lookupMethod(thread, frame, iter,
                                                     SymbolId::kDunderIter));
  ASSERT_FALSE(iter_iter->isError());
  Object result(&scope,
                Interpreter::callMethod1(thread, frame, iter_iter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(ListIteratorBuiltinsTest, DunderLengthHintOnEmptyListIterator) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  List empty_list(&scope, listFromRange(0, 0));

  frame->setLocal(0, *empty_list);
  Object iter(&scope, ListBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isListIterator());

  Object length_hint_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderLengthHint));
  ASSERT_FALSE(length_hint_method->isError());

  Object length_hint(&scope, Interpreter::callMethod1(
                                 thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint)->value(), 0);
}

TEST(ListIteratorBuiltinsTest, DunderLengthHintOnConsumedListIterator) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  List empty_list(&scope, listFromRange(0, 1));

  frame->setLocal(0, *empty_list);
  Object iter(&scope, ListBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isListIterator());

  Object length_hint_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderLengthHint));
  ASSERT_FALSE(length_hint_method->isError());

  Object length_hint1(&scope, Interpreter::callMethod1(
                                  thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint1)->value(), 1);

  // Consume the iterator
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderNext));
  ASSERT_FALSE(next_method->isError());
  Object item1(&scope,
               Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*item1)->value(), 0);

  Object length_hint2(&scope, Interpreter::callMethod1(
                                  thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint2)->value(), 0);
}

}  // namespace python
