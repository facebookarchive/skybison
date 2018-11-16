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
  runtime.runFromCStr(src);
  Handle<Object> result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result->isList()) << typeName(&runtime, *result);
  Handle<List> list(result);
  EXPECT_EQ(SmallInt::cast(list->at(0))->value(), 0);
  EXPECT_EQ(SmallInt::cast(list->at(1))->value(), 1);
  EXPECT_EQ(SmallInt::cast(list->at(2))->value(), 1);
  EXPECT_EQ(SmallInt::cast(list->at(3))->value(), 2);
  EXPECT_EQ(SmallInt::cast(list->at(4))->value(), 3);
  EXPECT_EQ(SmallInt::cast(list->at(5))->value(), 5);
  EXPECT_EQ(SmallInt::cast(list->at(6))->value(), 8);
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
  runtime.runFromCStr(src);
  Handle<Object> result(&scope, moduleAt(&runtime, "__main__", "value"));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_TRUE(SmallInt::cast(*result)->value() == 10);
}

TEST(GeneratorTest, BadInitialSend) {
  const char* src = R"(
def gen():
  yield 0
gen().send(1)
)";
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(src),
               "can't send non-None value to a just-started generator");
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
  runtime.runFromCStr(src);
  Handle<Object> result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result->isList());
  Handle<List> list(result);
  ASSERT_EQ(list->allocated(), 12);
  EXPECT_PYSTRING_EQ(Str::cast(list->at(0)), "priming");
  EXPECT_PYSTRING_EQ(Str::cast(list->at(1)), "ready");
  EXPECT_PYSTRING_EQ(Str::cast(list->at(2)), "sending");
  Handle<Object> initial(&scope, moduleAt(&runtime, "__main__", "initial_str"));
  ASSERT_TRUE(initial->isStr());
  EXPECT_PYSTRING_EQ(Str::cast(*initial), "initial string");
  EXPECT_EQ(list->at(3), *initial);
  EXPECT_PYSTRING_EQ(Str::cast(list->at(4)), "initial string first");
  EXPECT_PYSTRING_EQ(Str::cast(list->at(5)), "initial string first second");
  EXPECT_EQ(SmallInt::cast(list->at(6))->value(), 0);
  EXPECT_EQ(SmallInt::cast(list->at(7))->value(), 1);
  EXPECT_EQ(SmallInt::cast(list->at(8))->value(), 2);
  EXPECT_EQ(SmallInt::cast(list->at(9))->value(), 3);
  EXPECT_EQ(SmallInt::cast(list->at(10))->value(), 4);
  EXPECT_PYSTRING_EQ(Str::cast(list->at(11)), "finished!");
}

TEST(CoroutineTest, Basic) {
  const char* src = R"(
async def coro():
  return 24
c = coro()
)";

  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Handle<Object> result(&scope, moduleAt(&runtime, "__main__", "c"));
  EXPECT_TRUE(result->isCoroutine());
}

TEST(CoroutineTest, BadInitialSend) {
  const char* src = R"(
async def coro():
  return 0
coro().send(1)
)";
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(src),
               "can't send non-None value to a just-started coroutine");
}

}  // namespace python
