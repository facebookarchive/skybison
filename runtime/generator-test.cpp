#include "gtest/gtest.h"

#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(GeneratorTest, Basic) {
  const char* src = R"(
def fib(n):
    a = 0
    b = 1
    for i in range(n):
        yield a
        a, b = a + b, a

result = [i for i in fib(7)]
)";

  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {0, 1, 1, 2, 3, 5, 8});
}

TEST(GeneratorTest, InitialSend) {
  const char* src = R"(
def gen():
  global value
  value = 3
  value += yield 0
  yield 'dummy'

g = gen()
g.send(None)
g.send(7)
)";

  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Object result(&scope, moduleAt(&runtime, "__main__", "value"));
  EXPECT_TRUE(isIntEqualsWord(*result, 10));
}

TEST(GeneratorTest, BadInitialSend) {
  const char* src = R"(
def gen():
  yield 0
gen().send(1)
)";
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                    "can't send non-None value to a just-started generator"));
}

TEST(GeneratorTest, YieldFrom) {
  const char* src = R"(
result = []
def log(obj):
  global result
  result.append(obj)

def str_maker(l):
  while True:
    val = yield l
    if val is None:
      break
    l += ' ' + val
  yield from range(5)
  return 'finished!'

def g1():
  start = yield 'ready'
  x = yield from str_maker(start)
  log(x)

g = g1()
log('priming')
log(g.__next__())
log('sending')
initial_str = 'initial string'
log(g.send(initial_str))
log(g.send('first'))
log(g.send('second'))
log(g.send(None))
for i in g:
  log(i)
)";

  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(
      result,
      {"priming", "ready", "sending", "initial string", "initial string first",
       "initial string first second", 0, 1, 2, 3, 4, "finished!"});

  // Manually check element 3 for object identity
  ASSERT_TRUE(result.isList());
  List list(&scope, *result);
  Object initial(&scope, moduleAt(&runtime, "__main__", "initial_str"));
  EXPECT_GE(list.numItems(), 3);
  EXPECT_EQ(list.at(3), *initial);
}

TEST(GeneratorTest, ReraiseAfterYield) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
def gen():
  try:
    raise RuntimeError("inside generator")
  except:
    yield
    raise

g = gen()
g.__next__()
try:
  raise RuntimeError("outside generator")
except:
  g.__next__()
)"),
                            LayoutId::kRuntimeError, "inside generator"));
}

TEST(GeneratorTest, ReturnFromTrySkipsExcept) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
result = 0

def gen():
  global result
  yield 0
  try:
    return 123
  except:
    result = -1
  yield 1

g = gen()
g.__next__()
try:
  g.__next__()
except StopIteration:
  result = 1
)");

  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*result).value(), 1);
}

TEST(GeneratorTest, NextAfterReturnRaisesStopIteration) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  EXPECT_EQ(runFromCStr(&runtime, R"(
def gen():
  yield 0
  return "hello there"

g = gen()
g.__next__()
)"),
            NoneType::object());
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "g.__next__()"),
                            LayoutId::kStopIteration, "hello there"));
  thread->clearPendingException();
  EXPECT_TRUE(
      raised(runFromCStr(&runtime, "g.__next__()"), LayoutId::kStopIteration));
  thread->clearPendingException();
  EXPECT_TRUE(
      raised(runFromCStr(&runtime, "g.__next__()"), LayoutId::kStopIteration));
}

TEST(GeneratorTest, NextAfterRaiseRaisesStopIteration) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  EXPECT_FALSE(runFromCStr(&runtime, R"(
def gen():
  yield 0
  raise RuntimeError("kaboom")
  yield 1

g = gen()
g.__next__()
)")
                   .isError());
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "g.__next__()"),
                            LayoutId::kRuntimeError, "kaboom"));
  thread->clearPendingException();
  EXPECT_TRUE(
      raised(runFromCStr(&runtime, "g.__next__()"), LayoutId::kStopIteration));
}

TEST(CoroutineTest, Basic) {
  const char* src = R"(
async def coro():
  return 24
c = coro()
)";

  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Object result(&scope, moduleAt(&runtime, "__main__", "c"));
  EXPECT_TRUE(result.isCoroutine());
}

TEST(CoroutineTest, BadInitialSend) {
  const char* src = R"(
async def coro():
  return 0
coro().send(1)
)";
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                    "can't send non-None value to a just-started coroutine"));
}

}  // namespace python
