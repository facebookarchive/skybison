#include "gtest/gtest.h"

#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {

using namespace testing;

using CoroutineTest = RuntimeFixture;
using GeneratorTest = RuntimeFixture;
using AsyncGeneratorTest = RuntimeFixture;

TEST_F(GeneratorTest, Basic) {
  const char* src = R"(
def fib(n):
    a = 0
    b = 1
    for i in range(n):
        yield a
        a, b = a + b, a

result = [i for i in fib(7)]
)";

  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {0, 1, 1, 2, 3, 5, 8});
}

TEST_F(GeneratorTest, InitialSend) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def gen():
  global value
  value = 3
  value += yield 0
  yield 'dummy'

g = gen()
g.send(None)
g.send(7)
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "value"));
  EXPECT_TRUE(isIntEqualsWord(*result, 10));
}

TEST_F(GeneratorTest, BadInitialSend) {
  const char* src = R"(
def gen():
  yield 0
gen().send(1)
)";
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kTypeError,
                    "can't send non-None value to a just-started generator"));
}

TEST_F(GeneratorTest, YieldFrom) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(
      result,
      {"priming", "ready", "sending", "initial string", "initial string first",
       "initial string first second", 0, 1, 2, 3, 4, "finished!"});

  // Manually check element 3 for object identity
  ASSERT_TRUE(result.isList());
  List list(&scope, *result);
  Object initial(&scope, mainModuleAt(&runtime_, "initial_str"));
  EXPECT_GE(list.numItems(), 3);
  EXPECT_EQ(list.at(3), *initial);
}

TEST_F(GeneratorTest, ReraiseAfterYield) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(GeneratorTest, ReturnFromTrySkipsExcept) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
)")
                   .isError());

  Object result(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result.isSmallInt());
  EXPECT_EQ(SmallInt::cast(*result).value(), 1);
}

TEST_F(GeneratorTest, NextAfterReturnRaisesStopIteration) {
  EXPECT_EQ(runFromCStr(&runtime_, R"(
def gen():
  yield 0
  return "hello there"

g = gen()
g.__next__()
)"),
            NoneType::object());
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "g.__next__()"),
                            LayoutId::kStopIteration, "hello there"));
  thread_->clearPendingException();
  EXPECT_TRUE(
      raised(runFromCStr(&runtime_, "g.__next__()"), LayoutId::kStopIteration));
  thread_->clearPendingException();
  EXPECT_TRUE(
      raised(runFromCStr(&runtime_, "g.__next__()"), LayoutId::kStopIteration));
}

TEST_F(GeneratorTest, NextAfterRaiseRaisesStopIteration) {
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
def gen():
  yield 0
  raise RuntimeError("kaboom")
  yield 1

g = gen()
g.__next__()
)")
                   .isError());
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "g.__next__()"),
                            LayoutId::kRuntimeError, "kaboom"));
  thread_->clearPendingException();
  EXPECT_TRUE(
      raised(runFromCStr(&runtime_, "g.__next__()"), LayoutId::kStopIteration));
}

TEST_F(CoroutineTest, Basic) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
async def coro():
  return 24
c = coro()
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "c"));
  EXPECT_TRUE(result.isCoroutine());
}

TEST_F(CoroutineTest, BadInitialSend) {
  const char* src = R"(
async def coro():
  return 0
coro().send(1)
)";
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kTypeError,
                    "can't send non-None value to a just-started coroutine"));
}

TEST_F(AsyncGeneratorTest, Create) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
async def async_gen():
  yield 1234
ag = async_gen()
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "ag"));
  EXPECT_TRUE(result.isAsyncGenerator());
}

}  // namespace py
