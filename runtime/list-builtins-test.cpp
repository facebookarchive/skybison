#include "gtest/gtest.h"

#include "list-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ListBuiltinsTest, AddToNonEmptyList) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = [1, 2]
b = [3, 4, 5]
c = a + b
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(c->isList());
  Handle<List> list(&scope, List::cast(*c));
  EXPECT_EQ(SmallInteger::cast(list->at(0))->value(), 1);
  EXPECT_EQ(SmallInteger::cast(list->at(1))->value(), 2);
  EXPECT_EQ(SmallInteger::cast(list->at(2))->value(), 3);
  EXPECT_EQ(SmallInteger::cast(list->at(3))->value(), 4);
  EXPECT_EQ(SmallInteger::cast(list->at(4))->value(), 5);
}

TEST(ListBuiltinsTest, AddToEmptyList) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = []
b = [1, 2, 3]
c = a + b
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(c->isList());
  Handle<List> list(&scope, List::cast(*c));
  EXPECT_EQ(SmallInteger::cast(list->at(0))->value(), 1);
  EXPECT_EQ(SmallInteger::cast(list->at(1))->value(), 2);
  EXPECT_EQ(SmallInteger::cast(list->at(2))->value(), 3);
}

TEST(ListBuiltinsDeathTest, AddWithNonListSelfThrows) {
  const char* src = R"(
list.__add__(None, [])
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCString(src),
               "must be called with list instance as first argument");
}

TEST(ListBuiltinsDeathTest, AddListToTupleThrowsTypeError) {
  const char* src = R"(
a = [1, 2, 3]
b = (4, 5, 6)
c = a + b
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCString(src),
               "can only concatenate list to list");
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
  ASSERT_DEATH(runtime.runFromCString(src1),
               "aborting due to pending exception: "
               "insert\\(\\) takes exactly two arguments");

  const char* src2 = R"(
list.insert(1, 2, 3)
)";
  ASSERT_DEATH(runtime.runFromCString(src2),
               "aborting due to pending exception: "
               "descriptor 'insert' requires a 'list' object");

  const char* src3 = R"(
a = [1, 2]
a.insert("i", "val")
)";
  ASSERT_DEATH(runtime.runFromCString(src3),
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
  ASSERT_DEATH(runtime.runFromCString(src1),
               "aborting due to pending exception: "
               "pop\\(\\) takes at most 1 argument");

  const char* src2 = R"(
list.pop(1)
)";
  ASSERT_DEATH(runtime.runFromCString(src2),
               "aborting due to pending exception: "
               "descriptor 'pop' requires a 'list' object");

  const char* src3 = R"(
a = [1, 2]
a.pop("i")
)";
  ASSERT_DEATH(runtime.runFromCString(src3),
               "aborting due to pending exception: "
               "index object cannot be interpreted as an integer");

  const char* src4 = R"(
a = [1]
a.pop()
a.pop()
)";
  ASSERT_DEATH(runtime.runFromCString(src4),
               "unimplemented: "
               "Throw an IndexError for an out of range list");

  const char* src5 = R"(
a = [1]
a.pop(3)
)";
  ASSERT_DEATH(runtime.runFromCString(src5),
               "unimplemented: "
               "Throw an IndexError for an out of range list");
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
static Object* listFromRange(word start, word stop) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> result(&scope, thread->runtime()->newList());
  for (word i = start; i < stop; i++) {
    Handle<Object> value(&scope, SmallInteger::fromWord(i));
    thread->runtime()->listAdd(result, value);
  }
  return *result;
}

TEST(ListBuiltinsTest, SlicePositiveStartIndex) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [2:]
  Handle<Object> start(&scope, SmallInteger::fromWord(2));
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 3);
  EXPECT_EQ(SmallInteger::cast(test->at(0))->value(), 3);
  EXPECT_EQ(SmallInteger::cast(test->at(1))->value(), 4);
  EXPECT_EQ(SmallInteger::cast(test->at(2))->value(), 5);
}

TEST(ListBuiltinsTest, SliceNegativeStartIndexIsRelativeToEnd) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [-2:]
  Handle<Object> start(&scope, SmallInteger::fromWord(-2));
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 2);
  EXPECT_EQ(SmallInteger::cast(test->at(0))->value(), 4);
  EXPECT_EQ(SmallInteger::cast(test->at(1))->value(), 5);
}

TEST(ListBuiltinsTest, SlicePositiveStopIndex) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [:2]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, SmallInteger::fromWord(2));
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 2);
  EXPECT_EQ(SmallInteger::cast(test->at(0))->value(), 1);
  EXPECT_EQ(SmallInteger::cast(test->at(1))->value(), 2);
}

TEST(ListBuiltinsTest, SliceNegativeStopIndexIsRelativeToEnd) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [:-2]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, SmallInteger::fromWord(-2));
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 3);
  EXPECT_EQ(SmallInteger::cast(test->at(0))->value(), 1);
  EXPECT_EQ(SmallInteger::cast(test->at(1))->value(), 2);
  EXPECT_EQ(SmallInteger::cast(test->at(2))->value(), 3);
}

TEST(ListBuiltinsTest, SlicePositiveStep) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [::2]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, SmallInteger::fromWord(2));
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 3);
  EXPECT_EQ(SmallInteger::cast(test->at(0))->value(), 1);
  EXPECT_EQ(SmallInteger::cast(test->at(1))->value(), 3);
  EXPECT_EQ(SmallInteger::cast(test->at(2))->value(), 5);
}

TEST(ListBuiltinsTest, SliceNegativeStepReversesOrder) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [::-2]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, SmallInteger::fromWord(-2));
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 3);
  EXPECT_EQ(SmallInteger::cast(test->at(0))->value(), 5);
  EXPECT_EQ(SmallInteger::cast(test->at(1))->value(), 3);
  EXPECT_EQ(SmallInteger::cast(test->at(2))->value(), 1);
}

TEST(ListBuiltinsTest, SliceStartOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [10::]
  Handle<Object> start(&scope, SmallInteger::fromWord(10));
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 0);
}

TEST(ListBuiltinsTest, SliceStopOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [:10]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, SmallInteger::fromWord(10));
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 5);
  EXPECT_EQ(SmallInteger::cast(test->at(0))->value(), 1);
  EXPECT_EQ(SmallInteger::cast(test->at(4))->value(), 5);
}

TEST(ListBuiltinsTest, SliceStepOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [::10]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, SmallInteger::fromWord(10));
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 1);
  EXPECT_EQ(SmallInteger::cast(test->at(0))->value(), 1);
}

TEST(ListBuiltinsTest, IdenticalSliceIsCopy) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test: t[::] is t
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 5);
  EXPECT_EQ(SmallInteger::cast(test->at(0))->value(), 1);
  EXPECT_EQ(SmallInteger::cast(test->at(4))->value(), 5);
  ASSERT_NE(*test, *list1);
}

}  // namespace python
