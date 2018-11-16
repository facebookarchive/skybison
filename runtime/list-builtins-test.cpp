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
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<List> a(&scope, moduleAt(&runtime, main, "a"));
  ASSERT_EQ(a->allocated(), 2);
  EXPECT_EQ(Int::cast(a->at(0))->asWord(), 1);
  EXPECT_EQ(Int::cast(a->at(1))->asWord(), 2);
}

TEST(ListBuiltinsTest, NewListIsNotASingleton) {
  Runtime runtime;

  runtime.runFromCStr(R"(
a = list() is not list()
b = list([1, 2]) is not list([1, 2])
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Bool> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Bool> b(&scope, moduleAt(&runtime, main, "b"));
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
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(c->isList());
  Handle<List> list(&scope, List::cast(*c));
  EXPECT_EQ(SmallInt::cast(list->at(0))->value(), 1);
  EXPECT_EQ(SmallInt::cast(list->at(1))->value(), 2);
  EXPECT_EQ(SmallInt::cast(list->at(2))->value(), 3);
  EXPECT_EQ(SmallInt::cast(list->at(3))->value(), 4);
  EXPECT_EQ(SmallInt::cast(list->at(4))->value(), 5);
}

TEST(ListBuiltinsTest, AddToEmptyList) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = []
b = [1, 2, 3]
c = a + b
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(c->isList());
  Handle<List> list(&scope, List::cast(*c));
  EXPECT_EQ(SmallInt::cast(list->at(0))->value(), 1);
  EXPECT_EQ(SmallInt::cast(list->at(1))->value(), 2);
  EXPECT_EQ(SmallInt::cast(list->at(2))->value(), 3);
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
               "Throw an IndexError for an out of range list");

  const char* src5 = R"(
a = [1]
a.pop(3)
)";
  ASSERT_DEATH(runtime.runFromCStr(src5),
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
    Handle<Object> value(&scope, SmallInt::fromWord(i));
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
  Handle<Object> start(&scope, SmallInt::fromWord(2));
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 3);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 3);
  EXPECT_EQ(SmallInt::cast(test->at(1))->value(), 4);
  EXPECT_EQ(SmallInt::cast(test->at(2))->value(), 5);
}

TEST(ListBuiltinsTest, SliceNegativeStartIndexIsRelativeToEnd) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [-2:]
  Handle<Object> start(&scope, SmallInt::fromWord(-2));
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 2);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 4);
  EXPECT_EQ(SmallInt::cast(test->at(1))->value(), 5);
}

TEST(ListBuiltinsTest, SlicePositiveStopIndex) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [:2]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, SmallInt::fromWord(2));
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 2);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(SmallInt::cast(test->at(1))->value(), 2);
}

TEST(ListBuiltinsTest, SliceNegativeStopIndexIsRelativeToEnd) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [:-2]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, SmallInt::fromWord(-2));
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 3);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(SmallInt::cast(test->at(1))->value(), 2);
  EXPECT_EQ(SmallInt::cast(test->at(2))->value(), 3);
}

TEST(ListBuiltinsTest, SlicePositiveStep) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [::2]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, SmallInt::fromWord(2));
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 3);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(SmallInt::cast(test->at(1))->value(), 3);
  EXPECT_EQ(SmallInt::cast(test->at(2))->value(), 5);
}

TEST(ListBuiltinsTest, SliceNegativeStepReversesOrder) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [::-2]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, SmallInt::fromWord(-2));
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 3);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 5);
  EXPECT_EQ(SmallInt::cast(test->at(1))->value(), 3);
  EXPECT_EQ(SmallInt::cast(test->at(2))->value(), 1);
}

TEST(ListBuiltinsTest, SliceStartOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [10::]
  Handle<Object> start(&scope, SmallInt::fromWord(10));
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
  Handle<Object> stop(&scope, SmallInt::fromWord(10));
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 5);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(SmallInt::cast(test->at(4))->value(), 5);
}

TEST(ListBuiltinsTest, SliceStepOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<List> list1(&scope, listFromRange(1, 6));

  // Test [::10]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, SmallInt::fromWord(10));
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<List> test(&scope, ListBuiltins::slice(thread, *list1, *slice));
  ASSERT_EQ(test->allocated(), 1);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 1);
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
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(SmallInt::cast(test->at(4))->value(), 5);
  ASSERT_NE(*test, *list1);
}

TEST(ListBuiltinsTest, SetItem) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 3, 0);
  Handle<List> list(&scope, listFromRange(1, 5));

  frame->setLocal(0, Object::cast(*list));
  frame->setLocal(1, SmallInt::fromWord(0));
  frame->setLocal(2, SmallInt::fromWord(2));

  EXPECT_EQ(SmallInt::cast(list->at(0))->value(), 1);
  Handle<Object> result(&scope, ListBuiltins::dunderSetItem(thread, frame, 3));
  EXPECT_TRUE(result->isNone());
  EXPECT_EQ(SmallInt::cast(list->at(0))->value(), 2);

  // Negative index
  frame->setLocal(1, SmallInt::fromWord(-1));
  EXPECT_EQ(SmallInt::cast(list->at(3))->value(), 4);
  Handle<Object> result1(&scope, ListBuiltins::dunderSetItem(thread, frame, 3));
  EXPECT_TRUE(result1->isNone());
  EXPECT_EQ(SmallInt::cast(list->at(3))->value(), 2);
}

TEST(ListBuiltinsDeathTest, SetItemWithFewerArgumentsThrows) {
  const char* src = R"(
[].__setitem__(1)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "expected 3 arguments");
}

TEST(ListBuiltinsDeathTest, SetItemWithInvalidIndexThrows) {
  const char* src = R"(
[].__setitem__(1, "test")
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "list assignment index out of range");
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
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> test1(&scope, moduleAt(&runtime, main, "test1"));
  Handle<Object> test2(&scope, moduleAt(&runtime, main, "test2"));
  Handle<Object> test3(&scope, moduleAt(&runtime, main, "test3"));
  Handle<Object> test4(&scope, moduleAt(&runtime, main, "test4"));
  Handle<Object> test5(&scope, moduleAt(&runtime, main, "test5"));
  Handle<Object> test6(&scope, moduleAt(&runtime, main, "test6"));
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
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> len(&scope, moduleAt(&runtime, main, "l"));
  Handle<Object> element(&scope, moduleAt(&runtime, main, "e"));
  EXPECT_EQ(*len, SmallInt::fromWord(1));
  EXPECT_EQ(*element, SmallStr::fromCStr("foo"));
}

TEST(ListBuiltinsTest, DunderIterReturnsListIter) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<List> empty_list(&scope, listFromRange(0, 0));

  frame->setLocal(0, *empty_list);
  Handle<Object> iter(&scope, ListBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isListIterator());
}

TEST(ListIteratorBuiltinsTest, CallDunderNext) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<List> empty_list(&scope, listFromRange(0, 2));

  frame->setLocal(0, *empty_list);
  Handle<Object> iter(&scope, ListBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isListIterator());

  Handle<Object> next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderNext));
  ASSERT_FALSE(next_method->isError());

  Handle<Object> item1(
      &scope, Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item1->isSmallInt());
  ASSERT_EQ(SmallInt::cast(*item1)->value(), 0);

  Handle<Object> item2(
      &scope, Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item2->isSmallInt());
  ASSERT_EQ(SmallInt::cast(*item2)->value(), 1);
}

TEST(ListIteratorBuiltinsTest, DunderIterReturnsSelf) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<List> empty_list(&scope, listFromRange(0, 0));

  frame->setLocal(0, *empty_list);
  Handle<Object> iter(&scope, ListBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isListIterator());

  // Now call __iter__ on the iterator object
  Handle<Object> iter_iter(
      &scope,
      Interpreter::lookupMethod(thread, frame, iter, SymbolId::kDunderIter));
  ASSERT_FALSE(iter_iter->isError());
  Handle<Object> result(
      &scope, Interpreter::callMethod1(thread, frame, iter_iter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(ListIteratorBuiltinsTest, DunderLengthHintOnEmptyListIterator) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<List> empty_list(&scope, listFromRange(0, 0));

  frame->setLocal(0, *empty_list);
  Handle<Object> iter(&scope, ListBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isListIterator());

  Handle<Object> length_hint_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderLengthHint));
  ASSERT_FALSE(length_hint_method->isError());

  Handle<Object> length_hint(
      &scope,
      Interpreter::callMethod1(thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint->isSmallInt());
  ASSERT_EQ(SmallInt::cast(*length_hint)->value(), 0);
}

TEST(ListIteratorBuiltinsTest, DunderLengthHintOnConsumedListIterator) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<List> empty_list(&scope, listFromRange(0, 1));

  frame->setLocal(0, *empty_list);
  Handle<Object> iter(&scope, ListBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isListIterator());

  Handle<Object> length_hint_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderLengthHint));
  ASSERT_FALSE(length_hint_method->isError());

  Handle<Object> length_hint1(
      &scope,
      Interpreter::callMethod1(thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(SmallInt::cast(*length_hint1)->value(), 1);

  // Consume the iterator
  Handle<Object> next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderNext));
  ASSERT_FALSE(next_method->isError());
  Handle<Object> item1(
      &scope, Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item1->isSmallInt());
  ASSERT_EQ(SmallInt::cast(*item1)->value(), 0);

  Handle<Object> length_hint2(
      &scope,
      Interpreter::callMethod1(thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(SmallInt::cast(*length_hint2)->value(), 0);
}

}  // namespace python
