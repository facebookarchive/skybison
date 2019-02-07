#include "gtest/gtest.h"

#include <iostream>
#include <memory>

#include "bytecode.h"
#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "marshal.h"
#include "runtime.h"
#include "test-utils.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {
using namespace testing;

TEST(ThreadTest, CheckMainThreadRuntime) {
  Runtime runtime;
  auto thread = Thread::currentThread();
  ASSERT_EQ(thread->runtime(), &runtime);
}

TEST(ThreadTest, RunEmptyFunction) {
  Runtime runtime;
  HandleScope scope;
  const char* buffer =
      "\x33\x0D\x0D\x0A\x3B\x5B\xB8\x59\x05\x00\x00\x00\xE3\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x40\x00\x00\x00\x73\x04\x00"
      "\x00\x00\x64\x00\x53\x00\x29\x01\x4E\xA9\x00\x72\x01\x00\x00\x00\x72\x01"
      "\x00\x00\x00\x72\x01\x00\x00\x00\xFA\x07\x70\x61\x73\x73\x2E\x70\x79\xDA"
      "\x08\x3C\x6D\x6F\x64\x75\x6C\x65\x3E\x01\x00\x00\x00\x73\x00\x00\x00"
      "\x00";
  Marshal::Reader reader(&scope, &runtime, buffer);

  int32 magic = reader.readLong();
  EXPECT_EQ(magic, 0x0A0D0D33);
  int32 mtime = reader.readLong();
  EXPECT_EQ(mtime, 0x59B85B3B);
  int32 size = reader.readLong();
  EXPECT_EQ(size, 5);

  Object code_obj(&scope, reader.readObject());
  ASSERT_TRUE(code_obj->isCode());
  Code code(&scope, *code_obj);
  EXPECT_EQ(code->argcount(), 0);

  Thread thread(1 * kKiB);
  RawObject result = thread.run(code);
  ASSERT_EQ(result, NoneType::object());  // returns None
}

TEST(ThreadTest, RunHelloWorld) {
  Runtime runtime;
  const char* src = R"(
print('hello, world')
)";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, "hello, world\n");
}

TEST(ThreadTest, ModuleBodyCallsHelloWorldFunction) {
  Runtime runtime;
  const char* src = R"(
def hello():
  print('hello, world')
hello()
)";
  // Execute the code and make sure we get back the result we expect
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello, world\n");
}

TEST(ThreadTest, DunderCallInstance) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class C:
  def __init__(self, x, y):
    self.value = x + y
  def __call__(self, y):
    return self.value * y
c = C(10, 2)
g = c(3)
)";

  runFromCStr(&runtime, src);

  Module main(&scope, findModule(&runtime, "__main__"));
  Object global(&scope, moduleAt(&runtime, main, "g"));
  ASSERT_FALSE(global->isError());
  ASSERT_TRUE(global->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*global)->value(), 36);
}

TEST(ThreadTest, DunderCallInstanceWithDescriptor) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
result = None

def stage2(x):
    global result
    result = x

class Stage1:
  def __get__(self, instance, owner):
    return stage2

class Stage0:
  __call__ = Stage1()

c = Stage0()
c(1111)
)";
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object result(&scope, moduleAt(&runtime, main, "result"));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*result)->value(), 1111);
}

TEST(ThreadTest, DunderCallInstanceKw) {
  const char* src = R"(
class C:
  def __init__(self):
    self.value = None

  def __call__(self, y):
    return y

c = C()
result = c(y=3)
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);

  Module main(&scope, findModule(&runtime, "__main__"));
  Object result(&scope, moduleAt(&runtime, main, "result"));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*result)->value(), 3);
}

TEST(ThreadTest, DunderCallInstanceSplatArgs) {
  const char* src = R"(
class C:
  def __call__(self, y):
    return y

c = C()
args = (3,)
result = c(*args)
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);

  Module main(&scope, findModule(&runtime, "__main__"));
  Object result(&scope, moduleAt(&runtime, main, "result"));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*result)->value(), 3);
}

TEST(ThreadTest, DunderCallInstanceSplatKw) {
  const char* src = R"(
class C:
  def __call__(self, y):
    return y

c = C()
kwargs = {'y': 3}
result = c(**kwargs)
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);

  Module main(&scope, findModule(&runtime, "__main__"));
  Object result(&scope, moduleAt(&runtime, main, "result"));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*result)->value(), 3);
}

TEST(ThreadTest, DunderCallInstanceSplatArgsAndKw) {
  const char* src = R"(
result_x = None
result_y = None
class C:
  def __call__(self, x, y):
    global result_x
    global result_y
    result_x = x
    result_y = y

c = C()
args = (1,)
kwargs = {'y': 3}
c(*args, **kwargs)
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);

  Module main(&scope, findModule(&runtime, "__main__"));
  Object result_x(&scope, moduleAt(&runtime, main, "result_x"));
  ASSERT_TRUE(result_x->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*result_x)->value(), 1);
  Object result_y(&scope, moduleAt(&runtime, main, "result_y"));
  ASSERT_TRUE(result_y->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*result_y)->value(), 3);
}

TEST(ThreadTest, OverlappingFrames) {
  Runtime runtime;
  HandleScope scope;

  // Push a frame for a code object with space for 3 items on the value stack
  Code caller_code(&scope, testing::newEmptyCode(&runtime));
  caller_code->setStacksize(3);
  auto thread = Thread::currentThread();
  auto caller_frame = thread->pushFrame(caller_code);
  RawObject* sp = caller_frame->valueStackTop();
  // Push args on the stack in the sequence generated by CPython
  auto arg1 = SmallInt::fromWord(1111);
  *--sp = arg1;
  auto arg2 = SmallInt::fromWord(2222);
  *--sp = arg2;
  auto arg3 = SmallInt::fromWord(3333);
  *--sp = arg3;
  caller_frame->setValueStackTop(sp);

  // Push a frame for a code object that expects 3 arguments and needs space
  // for 3 local variables
  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setArgcount(3);
  code->setNlocals(3);
  auto frame = thread->pushFrame(code);

  // Make sure we can read the args from the frame
  RawObject local = frame->getLocal(0);
  ASSERT_TRUE(local->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(local)->value(), arg1->value());

  local = frame->getLocal(1);
  ASSERT_TRUE(local->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(local)->value(), arg2->value());

  local = frame->getLocal(2);
  ASSERT_TRUE(local->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(local)->value(), arg3->value());
}

TEST(ThreadTest, EncodeTryBlock) {
  TryBlock block(TryBlock::kExcept, 200, 300);
  EXPECT_EQ(block.kind(), TryBlock::kExcept);
  EXPECT_EQ(block.handler(), 200);
  EXPECT_EQ(block.level(), 300);

  TryBlock decoded(block.asSmallInt());
  EXPECT_EQ(decoded.kind(), block.kind());
  EXPECT_EQ(decoded.handler(), block.handler());
  EXPECT_EQ(decoded.level(), block.level());
}

TEST(ThreadTest, PushPopFrame) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setNlocals(2);
  code->setStacksize(3);

  auto thread = Thread::currentThread();
  byte* prev_sp = thread->stackPtr();
  auto frame = thread->pushFrame(code);

  // Verify frame invariants post-push
  EXPECT_EQ(frame->previousFrame(), thread->initialFrame());
  EXPECT_EQ(frame->code(), *code);
  EXPECT_EQ(frame->valueStackTop(), reinterpret_cast<RawObject*>(frame));
  EXPECT_EQ(frame->valueStackBase(), frame->valueStackTop());
  EXPECT_EQ(frame->numLocals(), 2);

  // Make sure we restore the thread's stack pointer back to its previous
  // location
  thread->popFrame();
  EXPECT_EQ(thread->stackPtr(), prev_sp);
}

TEST(ThreadTest, PushFrameWithNoCellVars) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setCellvars(NoneType::object());
  code->setFreevars(runtime.newTuple(0));
  auto thread = Thread::currentThread();
  byte* prev_sp = thread->stackPtr();
  thread->pushFrame(code);

  EXPECT_EQ(thread->stackPtr(), prev_sp - Frame::kSize);
}

TEST(ThreadTest, PushFrameWithNoFreeVars) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setFreevars(NoneType::object());
  code->setCellvars(runtime.newTuple(0));
  auto thread = Thread::currentThread();
  byte* prev_sp = thread->stackPtr();
  thread->pushFrame(code);

  EXPECT_EQ(thread->stackPtr(), prev_sp - Frame::kSize);
}

TEST(ThreadTest, ZeroInitializeBlockStack) {
  Runtime runtime;
  auto thread = Thread::currentThread();
  Frame* frame1 = thread->openAndLinkFrame(0, 0, 10);
  RawObject* sp = frame1->valueStackTop();
  for (word i = 0; i < 10; i++) {
    *sp-- = SmallInt::fromWord(1111);
  }
  Frame* frame2 = thread->openAndLinkFrame(0, 0, 10);
  // The block stack is a contiguous chunk of small integers.
  RawObject* bs = reinterpret_cast<RawObject*>(frame2->blockStack());
  for (word i = 0; i < BlockStack::kSize / kPointerSize; i++) {
    EXPECT_EQ(bs[i], SmallInt::fromWord(0));
  }
}

TEST(ThreadTest, ManipulateValueStack) {
  Runtime runtime;
  HandleScope scope;
  auto thread = Thread::currentThread();
  auto frame = thread->openAndLinkFrame(0, 0, 3);

  // Push 3 items on the value stack
  RawObject* sp = frame->valueStackTop();
  *--sp = SmallInt::fromWord(1111);
  *--sp = SmallInt::fromWord(2222);
  *--sp = SmallInt::fromWord(3333);
  frame->setValueStackTop(sp);
  ASSERT_EQ(frame->valueStackTop(), sp);

  // Verify the value stack is laid out as we expect
  word values[] = {3333, 2222, 1111};
  for (int i = 0; i < 3; i++) {
    RawObject object = frame->peek(i);
    ASSERT_TRUE(object->isSmallInt())
        << "Value at stack depth " << i << " is not an integer";
    EXPECT_EQ(RawSmallInt::cast(object)->value(), values[i])
        << "Incorrect value at stack depth " << i;
  }

  // Pop 2 items off the stack and check the stack is still as we expect
  frame->setValueStackTop(sp + 2);
  RawObject top = frame->peek(0);
  ASSERT_TRUE(top->isSmallInt()) << "Stack top isn't an integer";
  EXPECT_EQ(RawSmallInt::cast(top)->value(), 1111)
      << "Incorrect value for stack top";
}

TEST(ThreadTest, ManipulateBlockStack) {
  Runtime runtime;
  HandleScope scope;
  auto thread = Thread::currentThread();
  auto frame = thread->openAndLinkFrame(0, 0, 0);
  BlockStack* block_stack = frame->blockStack();

  TryBlock pushed1(TryBlock::kLoop, 100, 10);
  block_stack->push(pushed1);

  TryBlock pushed2(TryBlock::kExcept, 200, 20);
  block_stack->push(pushed2);

  TryBlock popped2 = block_stack->pop();
  EXPECT_EQ(popped2.kind(), pushed2.kind());
  EXPECT_EQ(popped2.handler(), pushed2.handler());
  EXPECT_EQ(popped2.level(), pushed2.level());

  TryBlock popped1 = block_stack->pop();
  EXPECT_EQ(popped1.kind(), pushed1.kind());
  EXPECT_EQ(popped1.handler(), pushed1.handler());
  EXPECT_EQ(popped1.level(), pushed1.level());
}

TEST(ThreadTest, CallFunction) {
  Runtime runtime;
  HandleScope scope;

  // Build the code object for the following function
  //
  //     def noop(a, b):
  //         return 2222
  //
  auto expected_result = SmallInt::fromWord(2222);
  Code callee_code(&scope, testing::newEmptyCode(&runtime));
  callee_code->setArgcount(2);
  callee_code->setStacksize(1);
  callee_code->setConsts(runtime.newTuple(1));
  RawTuple::cast(callee_code->consts())->atPut(0, expected_result);
  const byte callee_bc[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  callee_code->setCode(runtime.newBytesWithAll(callee_bc));

  // Create the function object and bind it to the code object
  Function callee(&scope, runtime.newFunction());
  callee->setCode(*callee_code);
  callee->setEntry(interpreterTrampoline);

  // Build a code object to call the function defined above
  Code caller_code(&scope, testing::newEmptyCode(&runtime));
  caller_code->setStacksize(3);
  Tuple consts(&scope, runtime.newTuple(3));
  consts->atPut(0, *callee);
  consts->atPut(1, SmallInt::fromWord(1111));
  consts->atPut(2, SmallInt::fromWord(2222));
  caller_code->setConsts(*consts);
  const byte caller_bc[] = {LOAD_CONST,    0, LOAD_CONST,   1, LOAD_CONST, 2,
                            CALL_FUNCTION, 2, RETURN_VALUE, 0};
  caller_code->setCode(runtime.newBytesWithAll(caller_bc));

  // Execute the caller and make sure we get back the expected result
  RawObject result = Thread::currentThread()->run(caller_code);
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), expected_result->value());
}

TEST(ThreadTest, ExtendedArg) {
  const word num_consts = 258;
  const byte bytecode[] = {EXTENDED_ARG, 1, LOAD_CONST, 1, RETURN_VALUE, 0};

  Runtime runtime;
  HandleScope scope;
  Tuple constants(&scope, runtime.newTuple(num_consts));

  auto zero = SmallInt::fromWord(0);
  auto non_zero = SmallInt::fromWord(0xDEADBEEF);
  for (word i = 0; i < num_consts - 2; i++) {
    constants->atPut(i, zero);
  }
  constants->atPut(num_consts - 1, non_zero);
  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setConsts(*constants);
  code->setCode(runtime.newBytesWithAll(bytecode));
  code->setStacksize(2);

  RawObject result = Thread::currentThread()->run(code);

  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 0xDEADBEEF);
}

TEST(ThreadTest, CallBuiltinPrint) {
  Runtime runtime;
  std::string output = compileAndRunToString(
      &runtime, "print(1111, 'testing 123', True, False)");
  EXPECT_EQ(output, "1111 testing 123 True False\n");
}

TEST(ThreadTest, CallBuiltinPrintKw) {
  Runtime runtime;
  std::string output =
      compileAndRunToString(&runtime, "print('testing 123', end='abc')");
  EXPECT_STREQ(output.c_str(), "testing 123abc");
}

TEST(ThreadTest, ExecuteDupTop) {
  Runtime runtime;
  HandleScope scope;

  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, SmallInt::fromWord(1111));
  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setStacksize(2);
  code->setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, DUP_TOP, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 1111);
}

TEST(ThreadTest, ExecuteDupTopTwo) {
  Runtime runtime;
  HandleScope scope;

  Tuple consts(&scope, runtime.newTuple(2));
  consts->atPut(0, SmallInt::fromWord(1111));
  consts->atPut(1, SmallInt::fromWord(2222));
  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setStacksize(2);
  code->setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,  0, LOAD_CONST,   1,
                           DUP_TOP_TWO, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 2222);
}

TEST(ThreadTest, ExecuteRotTwo) {
  Runtime runtime;
  HandleScope scope;

  Tuple consts(&scope, runtime.newTuple(2));
  consts->atPut(0, SmallInt::fromWord(1111));
  consts->atPut(1, SmallInt::fromWord(2222));
  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setStacksize(2);
  code->setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,   1,
                           ROT_TWO,    0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 1111);
}

TEST(ThreadTest, ExecuteRotThree) {
  Runtime runtime;
  HandleScope scope;

  Tuple consts(&scope, runtime.newTuple(3));
  consts->atPut(0, SmallInt::fromWord(1111));
  consts->atPut(1, SmallInt::fromWord(2222));
  consts->atPut(2, SmallInt::fromWord(3333));
  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setStacksize(3);
  code->setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,   1, LOAD_CONST, 2,
                           ROT_THREE,  0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 2222);
}

TEST(ThreadTest, ExecuteJumpAbsolute) {
  Runtime runtime;
  HandleScope scope;

  Tuple consts(&scope, runtime.newTuple(2));
  consts->atPut(0, SmallInt::fromWord(1111));
  consts->atPut(1, SmallInt::fromWord(2222));
  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setStacksize(2);
  code->setConsts(*consts);
  const byte bytecode[] = {JUMP_ABSOLUTE, 4, LOAD_CONST,   0,
                           LOAD_CONST,    1, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 2222);
}

TEST(ThreadTest, ExecuteJumpForward) {
  Runtime runtime;
  HandleScope scope;

  Tuple consts(&scope, runtime.newTuple(2));
  consts->atPut(0, SmallInt::fromWord(1111));
  consts->atPut(1, SmallInt::fromWord(2222));
  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setStacksize(2);
  code->setConsts(*consts);
  const byte bytecode[] = {JUMP_FORWARD, 2, LOAD_CONST,   0,
                           LOAD_CONST,   1, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 2222);
}

TEST(ThreadTest, ExecuteStoreLoadFast) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, SmallInt::fromWord(1111));
  code->setConsts(*consts);
  code->setNlocals(2);
  const byte bytecode[] = {LOAD_CONST, 0, STORE_FAST,   1,
                           LOAD_FAST,  1, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 1111);
}

TEST(ThreadTest, LoadGlobal) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple names(&scope, runtime.newTuple(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  names->atPut(0, *key);
  code->setNames(*names);

  const byte bytecode[] = {LOAD_GLOBAL, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(code);

  Dict globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());
  ValueCell value_cell(&scope, runtime.newValueCell());
  value_cell->setValue(SmallInt::fromWord(1234));
  Object value(&scope, *value_cell);
  runtime.dictAtPut(globals, key, value);
  frame->setGlobals(*globals);
  frame->setFastGlobals(runtime.computeFastGlobals(code, globals, builtins));

  Object result(&scope, Interpreter::execute(thread, frame));
  EXPECT_EQ(*result, value_cell->value());
}

struct TestData {
  const char* name;
  const char* expected_output;
  const char* src;
  const bool death;
};

static std::string TestName(::testing::TestParamInfo<TestData> info) {
  return info.param.name;
}

TestData kFastGlobalTests[] = {
    {"LoadGlobal", "1\n",
     R"(
a = 1
def f():
  print(a)
f()
)",
     false},

    {"LoadGlobalFromBuiltin", "True\n",
     R"(
class A(): pass
a = A()
def f():
  print(isinstance(a, A))
f()
)",
     false},

    {"LoadGlobalUnbound", ".*Unbound global 'a'",
     R"(
def f():
  print(a)
f()
)",
     true},

    {"StoreGlobal", "2\n2\n",
     R"(
def f():
  global a
  a = 2
  print(a)
f()
print(a)
)",
     false},

    {"StoreGlobalShadowBuiltin", "2\n",
     R"(
def f():
  global isinstance
  isinstance = 2
f()
print(isinstance)
)",
     false},

    {"DeleteGlobal", "True\nTrue\n",
     R"(
class A(): pass
a = A()
def f():
  global isinstance
  isinstance = 1
  del isinstance
  print(isinstance(a, A))  # fallback to builtin
f()
print(isinstance(a, A))
)",
     false},

    {"DeleteGlobalUnbound", ".*Unbound Globals.*",
     R"(
def f():
  global a
  del a
f()
)",
     true},

    {"DeleteGlobalBuiltinUnbound", ".*Unbound Globals.*",
     R"(
def f():
  global isinstance
  del isinstance
f()
)",
     true}

};

class GlobalsTest : public ::testing::TestWithParam<TestData> {};

TEST_P(GlobalsTest, FastGlobal) {
  Runtime runtime;
  TestData data = GetParam();
  if (data.death) {
    EXPECT_DEATH(runFromCStr(&runtime, data.src), data.expected_output);
  } else {
    std::string output = compileAndRunToString(&runtime, data.src);
    EXPECT_EQ(output, data.expected_output);
  }
}

INSTANTIATE_TEST_CASE_P(FastGlobal, GlobalsTest,
                        ::testing::ValuesIn(kFastGlobalTests), TestName);

TEST(ThreadTest, StoreGlobalCreateValueCell) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, SmallInt::fromWord(42));
  code->setConsts(*consts);

  Tuple names(&scope, runtime.newTuple(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  names->atPut(0, *key);
  code->setNames(*names);

  const byte bytecode[] = {LOAD_CONST,  0, STORE_GLOBAL, 0,
                           LOAD_GLOBAL, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(code);

  Dict globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());
  frame->setGlobals(*globals);
  frame->setFastGlobals(runtime.computeFastGlobals(code, globals, builtins));

  Object result(&scope, Interpreter::execute(thread, frame));

  Object value(&scope, runtime.dictAt(globals, key));
  ASSERT_TRUE(value->isValueCell());
  EXPECT_EQ(*result, RawValueCell::cast(*value)->value());
}

TEST(ThreadTest, StoreGlobalReuseValueCell) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, SmallInt::fromWord(42));
  code->setConsts(*consts);

  Tuple names(&scope, runtime.newTuple(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  names->atPut(0, *key);
  code->setNames(*names);

  const byte bytecode[] = {LOAD_CONST,  0, STORE_GLOBAL, 0,
                           LOAD_GLOBAL, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(code);

  ValueCell value_cell1(&scope, runtime.newValueCell());
  value_cell1->setValue(SmallInt::fromWord(99));

  Dict globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());
  Object value(&scope, *value_cell1);
  runtime.dictAtPut(globals, key, value);
  frame->setGlobals(*globals);
  frame->setFastGlobals(runtime.computeFastGlobals(code, globals, builtins));

  Object result(&scope, Interpreter::execute(thread, frame));

  Object value_cell2(&scope, runtime.dictAt(globals, key));
  ASSERT_TRUE(value_cell2->isValueCell());
  EXPECT_EQ(*value_cell2, *value_cell1);
  EXPECT_EQ(SmallInt::fromWord(42), value_cell1->value());
}

TEST(ThreadTest, StoreNameCreateValueCell) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, SmallInt::fromWord(42));
  code->setConsts(*consts);

  Tuple names(&scope, runtime.newTuple(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  names->atPut(0, *key);
  code->setNames(*names);

  const byte bytecode[] = {LOAD_CONST, 0, STORE_NAME,   0,
                           LOAD_NAME,  0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(code);

  Dict implicit_globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());
  frame->setImplicitGlobals(*implicit_globals);
  frame->setFastGlobals(
      runtime.computeFastGlobals(code, implicit_globals, builtins));

  Object result(&scope, Interpreter::execute(thread, frame));

  Object value(&scope, runtime.dictAt(implicit_globals, key));
  ASSERT_TRUE(value->isValueCell());
  EXPECT_EQ(*result, RawValueCell::cast(*value)->value());
}

TEST(ThreadTest, LoadNameInModuleBodyFromBuiltins) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple names(&scope, runtime.newTuple(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  names->atPut(0, *key);
  code->setNames(*names);

  const byte bytecode[] = {LOAD_NAME, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Dict globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());
  Object builtins_value(&scope, runtime.newInt(123));
  runtime.dictAtPutInValueCell(builtins, key, builtins_value);

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(code);
  frame->setBuiltins(*builtins);
  frame->setGlobals(*globals);
  // This should be a no-op because there are no loads or stores to globals.
  frame->setFastGlobals(runtime.computeFastGlobals(code, globals, builtins));
  frame->setImplicitGlobals(*globals);  // simulate module body

  Object result(&scope, Interpreter::execute(thread, frame));

  Object value_cell(&scope, runtime.dictAt(builtins, key));
  ASSERT_TRUE(value_cell->isValueCell());
  EXPECT_EQ(*builtins_value, RawValueCell::cast(*value_cell)->value());
}

TEST(ThreadTest, LoadNameInModuleBodyFromGlobals) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, SmallInt::fromWord(42));
  code->setConsts(*consts);

  Tuple names(&scope, runtime.newTuple(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  names->atPut(0, *key);
  code->setNames(*names);

  const byte bytecode[] = {LOAD_CONST, 0, STORE_GLOBAL, 0,
                           LOAD_NAME,  0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Dict globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(code);
  frame->setBuiltins(*builtins);
  frame->setGlobals(*globals);
  frame->setFastGlobals(runtime.computeFastGlobals(code, globals, builtins));
  frame->setImplicitGlobals(*globals);  // simulate module body

  Object result(&scope, Interpreter::execute(thread, frame));

  Object val0(&scope, runtime.dictAt(globals, key));  // 2-level indirection
  ASSERT_TRUE(val0->isValueCell());
  Object val1(&scope, RawValueCell::cast(*val0));
  ASSERT_TRUE(val1->isValueCell());
  EXPECT_EQ(*result, RawValueCell::cast(*val1)->value());
}

TEST(ThreadTest, LoadNameInTypeBodyFromGlobal) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, SmallInt::fromWord(42));
  code->setConsts(*consts);

  Tuple names(&scope, runtime.newTuple(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  names->atPut(0, *key);
  code->setNames(*names);

  const byte bytecode[] = {LOAD_CONST, 0, STORE_GLOBAL, 0,
                           LOAD_NAME,  0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Dict globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(code);
  frame->setBuiltins(*builtins);
  frame->setGlobals(*globals);
  frame->setFastGlobals(runtime.computeFastGlobals(code, globals, builtins));

  Dict implicit_globals(&scope, runtime.newDict());
  frame->setImplicitGlobals(*implicit_globals);  // simulate cls body

  Object result(&scope, Interpreter::execute(thread, frame));

  Object val0(&scope, runtime.dictAt(globals, key));  // 2-level indirection
  ASSERT_TRUE(val0->isValueCell());
  Object val1(&scope, RawValueCell::cast(*val0));
  ASSERT_TRUE(val1->isValueCell());
  EXPECT_EQ(*result, RawValueCell::cast(*val1)->value());
}

TEST(ThreadTest, LoadNameInTypeBodyFromImplicitGlobals) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, SmallInt::fromWord(42));
  code->setConsts(*consts);

  Tuple names(&scope, runtime.newTuple(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  names->atPut(0, *key);
  code->setNames(*names);

  const byte bytecode[] = {LOAD_CONST, 0, STORE_NAME,   0,
                           LOAD_NAME,  0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Dict globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(code);
  frame->setBuiltins(*builtins);
  frame->setGlobals(*globals);
  frame->setFastGlobals(runtime.computeFastGlobals(code, globals, builtins));

  Dict implicit_globals(&scope, runtime.newDict());
  frame->setImplicitGlobals(*implicit_globals);  // simulate cls body

  Object result(&scope, Interpreter::execute(thread, frame));

  Object val(&scope, runtime.dictAt(implicit_globals, key));
  ASSERT_TRUE(val->isValueCell());  // 1-level indirection
  EXPECT_EQ(*result, RawValueCell::cast(*val)->value());
}

TEST(ThreadTest, MakeFunction) {
  Runtime runtime;
  HandleScope scope;

  Code module(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(3));
  Code code(&scope, testing::newEmptyCode(&runtime));
  consts->atPut(0, *code);
  Object key(&scope, runtime.newStrFromCStr("hello"));
  consts->atPut(1, *key);
  consts->atPut(2, NoneType::object());
  module->setConsts(*consts);

  Tuple names(&scope, runtime.newTuple(1));
  names->atPut(0, runtime.newStrFromCStr("hello"));
  module->setNames(*names);

  const byte bc[] = {LOAD_CONST, 0, LOAD_CONST, 1, MAKE_FUNCTION, 0,
                     STORE_NAME, 0, LOAD_CONST, 2, RETURN_VALUE,  0};
  module->setCode(runtime.newBytesWithAll(bc));
  code->setCode(runtime.newBytesWithAll(bc));
  code->setFlags(Code::Flags::NOFREE);
  code->setNames(*names);

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(module);

  Dict implicit_globals(&scope, runtime.newDict());
  Dict globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());
  frame->setGlobals(*globals);
  frame->setBuiltins(*builtins);
  frame->setImplicitGlobals(*implicit_globals);

  Object result(&scope, Interpreter::execute(thread, frame));

  Object value(&scope, runtime.dictAt(implicit_globals, key));
  ASSERT_TRUE(value->isValueCell());
  ASSERT_TRUE(RawValueCell::cast(*value)->value()->isFunction());

  Function function(&scope, RawValueCell::cast(*value)->value());
  EXPECT_EQ(function->code(), consts->at(0));
  EXPECT_EQ(function->qualname(), consts->at(1));
  EXPECT_EQ(function->entry(), &interpreterTrampoline);
}

TEST(ThreadTest, BuildList) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(3));
  consts->atPut(0, SmallInt::fromWord(111));
  consts->atPut(1, runtime.newStrFromCStr("qqq"));
  consts->atPut(2, NoneType::object());
  code->setConsts(*consts);

  const byte bc[] = {LOAD_CONST, 0, LOAD_CONST,   1, LOAD_CONST, 2,
                     BUILD_LIST, 3, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc));

  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isList());

  RawList list = RawList::cast(result);
  EXPECT_EQ(list->capacity(), 3);

  ASSERT_TRUE(list->at(0)->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(list->at(0))->value(), 111);

  ASSERT_TRUE(list->at(1)->isSmallStr());
  EXPECT_EQ(list->at(1), SmallStr::fromCStr("qqq"));
  EXPECT_EQ(list->at(2), NoneType::object());
}

TEST(ThreadTest, BuildSetEmpty) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));
  const byte bc[] = {BUILD_SET, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc));

  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSet());

  Set set(&scope, result);
  EXPECT_EQ(set->numItems(), 0);
}

TEST(ThreadTest, BuildSetWithOneItem) {
  Runtime runtime;
  HandleScope scope;
  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(2));
  Object smi(&scope, SmallInt::fromWord(111));
  consts->atPut(0, *smi);
  consts->atPut(1, *smi);  // dup
  code->setConsts(*consts);

  const byte bc[] = {LOAD_CONST, 0, LOAD_CONST,   1,
                     BUILD_SET,  2, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc));

  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSet());

  Set set(&scope, result);
  EXPECT_EQ(set->numItems(), 1);

  EXPECT_TRUE(runtime.setIncludes(set, smi));
}

TEST(ThreadTest, BuildSet) {
  Runtime runtime;
  HandleScope scope;
  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(4));

  Object smi(&scope, SmallInt::fromWord(111));
  consts->atPut(0, *smi);
  consts->atPut(1, *smi);  // dup

  Object str(&scope, runtime.newStrFromCStr("qqq"));
  consts->atPut(2, *str);

  Object none(&scope, NoneType::object());
  consts->atPut(3, *none);

  code->setConsts(*consts);

  const byte bc[] = {LOAD_CONST, 0, LOAD_CONST, 1, LOAD_CONST,   2,
                     LOAD_CONST, 3, BUILD_SET,  4, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc));

  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSet());

  Set set(&scope, result);
  EXPECT_EQ(set->numItems(), 3);

  EXPECT_TRUE(runtime.setIncludes(set, smi));
  EXPECT_TRUE(runtime.setIncludes(set, str));
  EXPECT_TRUE(runtime.setIncludes(set, none));
}

TEST(ThreadTest, SetupLoop) {
  Runtime runtime;
  HandleScope scope;

  auto inspect_block = [](Thread*, Frame* frame, word) -> RawObject {
    // SETUP_LOOP should have pushed an entry onto the block stack with a
    // stack depth of 3
    TryBlock block = frame->blockStack()->pop();
    EXPECT_EQ(block.kind(), TryBlock::kLoop);
    EXPECT_EQ(block.handler(), 102);
    EXPECT_EQ(block.level(), 3);
    return NoneType::object();
  };
  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, runtime.newBuiltinFunction(SymbolId::kDummy, inspect_block,
                                              unimplementedTrampoline,
                                              unimplementedTrampoline));
  const byte bc[] = {SETUP_LOOP, 100, LOAD_CONST,   0, CALL_FUNCTION, 0,
                     POP_TOP,    0,   RETURN_VALUE, 0};
  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setCode(runtime.newBytesWithAll(bc));
  code->setConsts(*consts);
  code->setStacksize(4);

  // Create a frame with three items on the stack
  auto thread = Thread::currentThread();
  auto frame = thread->pushFrame(code);
  RawObject* sp = frame->valueStackTop();
  *--sp = SmallInt::fromWord(1111);
  *--sp = SmallInt::fromWord(2222);
  *--sp = SmallInt::fromWord(3333);
  frame->setValueStackTop(sp);

  EXPECT_NO_FATAL_FAILURE(Interpreter::execute(thread, frame));
}

TEST(ThreadTest, PopBlock) {
  Runtime runtime;
  HandleScope scope;

  const byte bc[] = {POP_BLOCK, 0, RETURN_VALUE, 0};
  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setCode(runtime.newBytesWithAll(bc));
  code->setStacksize(3);

  // Create a frame with three items on the stack
  auto thread = Thread::currentThread();
  auto frame = thread->pushFrame(code);
  RawObject* sp = frame->valueStackTop();
  *--sp = SmallInt::fromWord(1111);
  *--sp = SmallInt::fromWord(2222);
  *--sp = SmallInt::fromWord(3333);
  frame->setValueStackTop(sp);

  // Push an entry onto the block stack. When popped, this should set the stack
  // pointer to point to the bottom most element on the stack.
  frame->blockStack()->push(TryBlock(TryBlock::kLoop, 0, 1));

  RawObject result = Interpreter::execute(thread, frame);

  // The RETURN_VALUE instruction should return bottom most item from the stack,
  // assuming that POP_BLOCK worked correctly.
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 1111);
}

TEST(ThreadTest, PopJumpIfFalse) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(3));
  consts->atPut(0, Bool::trueObj());
  consts->atPut(1, SmallInt::fromWord(1111));
  consts->atPut(2, SmallInt::fromWord(2222));
  code->setConsts(*consts);
  // Bytecode for the snippet:
  //   if x:
  //     return 1111
  //   return 2222
  const byte bc[] = {LOAD_CONST,   0, POP_JUMP_IF_FALSE, 8, LOAD_CONST,   1,
                     RETURN_VALUE, 0, LOAD_CONST,        2, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc));

  // Test when the condition evaluates to a truthy value
  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 1111);

  // Test when the condition evaluates to a falsey value
  consts->atPut(0, Bool::falseObj());
  result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 2222);
}

TEST(ThreadTest, PopJumpIfTrue) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(3));
  consts->atPut(0, Bool::falseObj());
  consts->atPut(1, SmallInt::fromWord(1111));
  consts->atPut(2, SmallInt::fromWord(2222));
  code->setConsts(*consts);
  // Bytecode for the snippet:
  //   if not x:
  //     return 1111
  //   return 2222
  const byte bc[] = {LOAD_CONST,   0, POP_JUMP_IF_TRUE, 8, LOAD_CONST,   1,
                     RETURN_VALUE, 0, LOAD_CONST,       2, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc));

  // Test when the condition evaluates to a falsey value
  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 1111);

  // Test when the condition evaluates to a truthy value
  consts->atPut(0, Bool::trueObj());
  result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 2222);
}

TEST(ThreadTest, JumpIfFalseOrPop) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(2));
  consts->atPut(0, Bool::falseObj());
  consts->atPut(1, SmallInt::fromWord(1111));
  code->setConsts(*consts);
  const byte bc[] = {LOAD_CONST, 0, JUMP_IF_FALSE_OR_POP, 6,
                     LOAD_CONST, 1, RETURN_VALUE,         0};
  code->setCode(runtime.newBytesWithAll(bc));

  // If the condition is false, we should return the top of the stack, which is
  // the condition itself
  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isBool());
  EXPECT_FALSE(RawBool::cast(result)->value());

  // If the condition is true, we should pop the top of the stack (the
  // condition) and continue execution. In our case that loads a const and
  // returns it.
  consts->atPut(0, Bool::trueObj());
  result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 1111);
}

TEST(ThreadTest, JumpIfTrueOrPop) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(2));
  consts->atPut(0, Bool::trueObj());
  consts->atPut(1, SmallInt::fromWord(1111));
  code->setConsts(*consts);
  const byte bc[] = {LOAD_CONST, 0, JUMP_IF_TRUE_OR_POP, 6,
                     LOAD_CONST, 1, RETURN_VALUE,        0};
  code->setCode(runtime.newBytesWithAll(bc));

  // If the condition is true, we should return the top of the stack, which is
  // the condition itself
  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isBool());
  EXPECT_TRUE(RawBool::cast(result)->value());

  // If the condition is false, we should pop the top of the stack (the
  // condition) and continue execution. In our case that loads a const and
  // returns it.
  consts->atPut(0, Bool::falseObj());
  result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 1111);
}

TEST(ThreadTest, UnaryNot) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, Bool::trueObj());
  code->setConsts(*consts);
  // Bytecode for the snippet:
  //     return not x
  const byte bc[] = {LOAD_CONST, 0, UNARY_NOT, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc));

  // If the condition is true, we should return false
  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isBool());
  EXPECT_FALSE(RawBool::cast(result)->value());

  // If the condition is false, we should return true
  consts->atPut(0, Bool::falseObj());
  result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isBool());
  EXPECT_TRUE(RawBool::cast(result)->value());
}

static RawDict getMainModuleDict(Runtime* runtime) {
  HandleScope scope;
  Module mod(&scope, findModule(runtime, "__main__"));
  EXPECT_TRUE(mod->isModule());

  Dict dict(&scope, mod->dict());
  EXPECT_TRUE(dict->isDict());
  return *dict;
}

TEST(ThreadTest, LoadBuildTypeEmptyType) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class C:
  pass
)";

  RawObject result = runFromCStr(&runtime, src);
  ASSERT_EQ(result, NoneType::object());  // returns None

  Dict dict(&scope, getMainModuleDict(&runtime));

  Object key(&scope, runtime.newStrFromCStr("C"));
  Object value(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(value->isValueCell());

  Type cls(&scope, RawValueCell::cast(*value)->value());
  ASSERT_TRUE(cls->name()->isSmallStr());
  EXPECT_EQ(cls->name(), SmallStr::fromCStr("C"));

  Tuple mro(&scope, cls->mro());
  EXPECT_EQ(mro->length(), 2);
  EXPECT_EQ(mro->at(0), *cls);
  EXPECT_EQ(mro->at(1), runtime.typeAt(LayoutId::kObject));
}

TEST(ThreadTest, LoadBuildTypeTypeWithInit) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class C:
  def __init__(self):
    pass
)";

  RawObject result = runFromCStr(&runtime, src);
  ASSERT_EQ(result, NoneType::object());  // returns None

  Module mod(&scope, findModule(&runtime, "__main__"));
  ASSERT_TRUE(mod->isModule());

  Dict mod_dict(&scope, mod->dict());
  ASSERT_TRUE(mod_dict->isDict());

  // Check for the class name in the module dict
  Object cls_name(&scope, runtime.newStrFromCStr("C"));
  Object value(&scope, runtime.dictAt(mod_dict, cls_name));
  ASSERT_TRUE(value->isValueCell());
  Type cls(&scope, RawValueCell::cast(*value)->value());

  // Check class MRO
  Tuple mro(&scope, cls->mro());
  EXPECT_EQ(mro->length(), 2);
  EXPECT_EQ(mro->at(0), *cls);
  EXPECT_EQ(mro->at(1), runtime.typeAt(LayoutId::kObject));

  // Check class name
  ASSERT_TRUE(cls->name()->isSmallStr());
  EXPECT_EQ(cls->name(), SmallStr::fromCStr("C"));

  Dict cls_dict(&scope, cls->dict());
  ASSERT_TRUE(cls_dict->isDict());

  // Check for the __init__ method name in the dict
  Object meth_name(&scope, runtime.symbols()->DunderInit());
  ASSERT_TRUE(runtime.dictIncludes(cls_dict, meth_name));
  value = runtime.dictAt(cls_dict, meth_name);
  ASSERT_TRUE(value->isValueCell());
  ASSERT_TRUE(RawValueCell::cast(*value)->value()->isFunction());
}

static RawObject nativeExceptionTest(Thread* thread, Frame*, word) {
  HandleScope scope;
  Str msg(&scope,
          RawStr::cast(thread->runtime()->newStrFromCStr("test exception")));
  return thread->raiseRuntimeError(*msg);
}

TEST(ThreadTest, NativeExceptions) {
  Runtime runtime;
  HandleScope scope;

  Function fn(&scope, runtime.newFunction());
  fn->setEntry(nativeTrampoline<nativeExceptionTest>);

  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, *fn);
  code->setConsts(*consts);

  // Call the native function and check that a pending exception is left in the
  // Thread.
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));
  code->setStacksize(1);

  Thread* thread = Thread::currentThread();
  EXPECT_TRUE(raised(thread->run(code), LayoutId::kRuntimeError));
  Object value(&scope, thread->pendingExceptionValue());
  ASSERT_TRUE(value->isStr());
  Str str(&scope, *value);
  EXPECT_TRUE(str->equalsCStr("test exception"));
}

// MRO tests

static RawStr className(RawObject obj) {
  HandleScope scope;
  Type cls(&scope, obj);
  Str name(&scope, cls->name());
  return *name;
}

static RawObject getMro(Runtime* runtime, const char* src,
                        const char* desired_class) {
  HandleScope scope;

  Object result(&scope, runFromCStr(runtime, src));

  Dict mod_dict(&scope, getMainModuleDict(runtime));
  Object class_name(&scope, runtime->newStrFromCStr(desired_class));

  Object value(&scope, runtime->dictAt(mod_dict, class_name));
  Type cls(&scope, RawValueCell::cast(*value)->value());

  return cls->mro();
}

TEST(ThreadTest, LoadBuildTypeVerifyMro) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class A: pass
class B: pass
class C(A,B): pass
)";

  Tuple mro(&scope, getMro(&runtime, src, "C"));
  EXPECT_EQ(mro->length(), 4);
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(0)), "C"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(1)), "A"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(2)), "B"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(3)), "object"));
}

TEST(ThreadTest, LoadBuildTypeVerifyMroInheritance) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class A: pass
class B(A): pass
class C(B): pass
)";

  Tuple mro(&scope, getMro(&runtime, src, "C"));
  EXPECT_EQ(mro->length(), 4);
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(0)), "C"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(1)), "B"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(2)), "A"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(3)), "object"));
}

TEST(ThreadTest, LoadBuildTypeVerifyMroMultiInheritance) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class A: pass
class B(A): pass
class C: pass
class D(B,C): pass
)";

  Tuple mro(&scope, getMro(&runtime, src, "D"));
  EXPECT_EQ(mro->length(), 5);
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(0)), "D"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(1)), "B"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(2)), "A"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(3)), "C"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(4)), "object"));
}

TEST(ThreadTest, LoadBuildTypeVerifyMroDiamond) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class A: pass
class B(A): pass
class C(A): pass
class D(B,C): pass
)";

  Tuple mro(&scope, getMro(&runtime, src, "D"));
  EXPECT_EQ(mro->length(), 5);
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(0)), "D"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(1)), "B"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(2)), "C"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(3)), "A"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro->at(4)), "object"));
}

TEST(ThreadTest, LoadBuildTypeVerifyMroError) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class A: pass
class B(A): pass
class C(A, B): pass
)";

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, src), LayoutId::kTypeError,
      "Cannot create a consistent method resolution order (MRO)"));
}

// iteration

TEST(ThreadTest, IteratePrint) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
for i in range(3):
  print(i)
for i in range(3,6):
  print(i)
for i in range(6,12,2):
  print(i)
for i in range(6,3,-1):
  print(i)
for i in range(42,0,1):
  print(i)
for i in range(42,100,-1):
  print(i)
)";

  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "0\n1\n2\n3\n4\n5\n6\n8\n10\n6\n5\n4\n");
}

TestData kManipulateLocalsTests[] = {
    // Load an argument when no local variables are present
    {"LoadSingleArg", "1\n",
     R"(
def test(x):
  print(x)
test(1)
)",
     false},

    // Load and store an argument when no local variables are present
    {"LoadStoreSingleArg", "1\n2\n",
     R"(
def test(x):
  print(x)
  x = 2
  print(x)
test(1)
)",
     false},

    // Load multiple arguments when no local variables are present
    {"LoadManyArgs", "1 2 3\n",
     R"(
def test(x, y, z):
  print(x, y, z)
test(1, 2, 3)
)",
     false},

    // Load/store multiple arguments when no local variables are present
    {"LoadStoreManyArgs", "1 2 3\n3 2 1\n",
     R"(
def test(x, y, z):
  print(x, y, z)
  x = 3
  z = 1
  print(x, y, z)
test(1, 2, 3)
)",
     false},

    // Load a single local variable when no arguments are present
    {"LoadSingleLocalVar", "1\n",
     R"(
def test():
  x = 1
  print(x)
test()
)",
     false},

    // Load multiple local variables when no arguments are present
    {"LoadManyLocalVars", "1 2 3\n",
     R"(
def test():
  x = 1
  y = 2
  z = 3
  print(x, y, z)
test()
)",
     false},

    // Mixed local var and arg usage
    {"MixedLocals", "1 2 3\n3 2 1\n",
     R"(
def test(x, y):
  z = 3
  print(x, y, z)
  x = z
  z = 1
  print(x, y, z)
test(1, 2)
)",
     false},
};

class LocalsTest : public ::testing::TestWithParam<TestData> {};

TEST_P(LocalsTest, ManipulateLocals) {
  Runtime runtime;
  TestData data = GetParam();
  std::string output = compileAndRunToString(&runtime, data.src);
  EXPECT_EQ(output, data.expected_output);
}

INSTANTIATE_TEST_CASE_P(ManipulateLocals, LocalsTest,
                        ::testing::ValuesIn(kManipulateLocalsTests), TestName);

TEST(ThreadTest, RaiseVarargs) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "raise 1"),
                            LayoutId::kTypeError,
                            "exceptions must derive from BaseException"));
}

TEST(ThreadTest, InheritFromObject) {
  const char* src = R"(
class Foo(object):
  pass
)";
  Runtime runtime;
  runFromCStr(&runtime, src);

  // Look up the class Foo
  HandleScope scope;
  RawObject object = findModule(&runtime, "__main__");
  ASSERT_TRUE(object->isModule());
  Module main(&scope, object);
  object = moduleAt(&runtime, main, "Foo");
  ASSERT_TRUE(object->isType());
  Type type(&scope, object);

  // Check that its MRO is itself and object
  ASSERT_TRUE(type->mro()->isTuple());
  Tuple mro(&scope, type->mro());
  ASSERT_EQ(mro->length(), 2);
  EXPECT_EQ(mro->at(0), *type);
  EXPECT_EQ(mro->at(1), runtime.typeAt(LayoutId::kObject));
}

// imports

TEST(ThreadTest, ImportTest) {
  Runtime runtime;
  HandleScope scope;

  const char* module_src = R"(
def say_hello():
  print("hello");
)";

  const char* main_src = R"(
import hello
hello.say_hello()
)";

  // Pre-load the hello module so is cached.
  std::unique_ptr<char[]> module_buf(Runtime::compile(module_src));
  Object name(&scope, runtime.newStrFromCStr("hello"));
  runtime.importModuleFromBuffer(module_buf.get(), name);

  std::string output = compileAndRunToString(&runtime, main_src);
  EXPECT_EQ(output, "hello\n");
}

TEST(ThreadTest, FailedImportTest) {
  Runtime runtime;
  HandleScope scope;

  const char* main_src = R"(
import hello
hello.say_hello()
)";

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, main_src),
                            LayoutId::kRuntimeError,
                            "importModule is unimplemented!"));
}

TEST(ThreadTest, ImportMissingAttributeTest) {
  Runtime runtime;
  HandleScope scope;

  const char* module_src = R"(
def say_hello():
  print("hello");
)";

  const char* main_src = R"(
import hello
hello.foo()
)";

  // Pre-load the hello module so is cached.
  std::unique_ptr<char[]> module_buf(Runtime::compile(module_src));
  Object name(&scope, runtime.newStrFromCStr("hello"));
  runtime.importModuleFromBuffer(module_buf.get(), name);

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, main_src),
                            LayoutId::kAttributeError, "missing attribute"));
}

TEST(ThreadTest, ModuleSetAttrTest) {
  Runtime runtime;
  HandleScope scope;

  const char* module_src = R"(
def say_hello():
  print("hello");
)";

  const char* main_src = R"(
import hello
def goodbye():
  print("goodbye")
hello.say_hello = goodbye
hello.say_hello()
)";

  // Pre-load the hello module so is cached.
  std::unique_ptr<char[]> module_buf(Runtime::compile(module_src));
  Object name(&scope, runtime.newStrFromCStr("hello"));
  runtime.importModuleFromBuffer(module_buf.get(), name);

  std::string output = compileAndRunToString(&runtime, main_src);
  EXPECT_EQ(output, "goodbye\n");
}

TEST(ThreadTest, StoreFastStackEffect) {
  const char* src = R"(
def printit(x, y, z):
  print(x, y, z)

def test():
  x = 1
  y = 2
  z = 3
  printit(x, y, z)

test()
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2 3\n");
}

TEST(ThreadTest, SubscriptDict) {
  const char* src = R"(
a = {"1": 2, 2: 3}
print(a["1"])
# exceeds kInitialDictCapacity
b = { 0:0, 1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7, 8:8, 9:9, 10:10, 11:11 }
print(b[11])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "2\n11\n");

  const char* src1 = R"(
a = {"1": 2, 2: 3}
print(a[1])
)";
  EXPECT_TRUE(raised(runFromCStr(&runtime, src1), LayoutId::kKeyError));
}

TEST(ThreadTest, BuildDictNonLiteralKey) {
  const char* src = R"(
b = "foo"
a = { b: 3, 'c': 4 }
# we need one dict that exceeds kInitialDictCapacity
c = { b: 1, 1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7, 8:8, 9:9, 10:10, 11:11 }
print(a["foo"])
print(a["c"])
print(c[11])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "3\n4\n11\n");
}

TEST(ThreadTest, Closure) {
  const char* src = R"(
def f():
  a = 1
  def g():
    b = 2
    def h():
      print(b)
    print(a)
    h()
    b = 3
    h()
  g()
f()
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n3\n");
}

TEST(ThreadTest, UnpackSequence) {
  const char* src = R"(
a, b = (1, 2)
print(a, b)
a, b = [3, 4]
print(a, b)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2\n3 4\n");
}

TEST(TheadTest, BinaryTrueDivide) {
  const char* src = R"(
a = 6
b = 2
print(a / b)
a = 5
b = 2
print(a / b)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "3\n2.5\n");
}

TEST(FormatTest, NoConvEmpty) {
  const char* src = R"(
print(f'')
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "\n");
}

TEST(FormatTest, NoConvOneElement) {
  const char* src = R"(
a = "hello"
x = f'a={a}'
print(x)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "a=hello\n");
}

TEST(FormatTest, NoConvMultiElements) {
  const char* src = R"(
a = "hello"
b = "world"
c = "python"
x = f'{a} {b} {c}'
print(x)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello world python\n");
}

TEST(FormatTest, NoConvMultiElementsLarge) {
  const char* src = R"(
a = "Python"
b = "is"
c = "an interpreted high-level programming language for general-purpose programming.";
x = f'{a} {b} {c}'
print(x)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output,
            "Python is an interpreted high-level programming language for "
            "general-purpose programming.\n");
}

TEST(TestThread, BuildTupleUnpack) {
  const char* src = R"(
t = (*[0], *[1, 2], *[], *[3, 4, 5])
t1 = (*(0,), *(1, 2), *(), *(3, 4, 5))
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));

  Object t(&scope, moduleAt(&runtime, main, "t"));
  EXPECT_TRUE(t->isTuple());
  Tuple tuple_t(&scope, *t);
  EXPECT_EQ(tuple_t->length(), 6);
  for (word i = 0; i < tuple_t->length(); i++) {
    EXPECT_EQ(RawSmallInt::cast(tuple_t->at(i))->value(), i);
  }

  Object t1(&scope, moduleAt(&runtime, main, "t1"));
  EXPECT_TRUE(t1->isTuple());
  Tuple tuple_t1(&scope, *t1);
  EXPECT_EQ(tuple_t1->length(), 6);
  EXPECT_EQ(RawSmallInt::cast(tuple_t1->at(0))->value(), 0);
  EXPECT_EQ(RawSmallInt::cast(tuple_t1->at(1))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(tuple_t1->at(2))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(tuple_t1->at(3))->value(), 3);
  EXPECT_EQ(RawSmallInt::cast(tuple_t1->at(4))->value(), 4);
  EXPECT_EQ(RawSmallInt::cast(tuple_t1->at(5))->value(), 5);
}

TEST(TestThread, BuildListUnpack) {
  const char* src = R"(
l = [*[0], *[1, 2], *[], *[3, 4, 5]]
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));

  Object l(&scope, moduleAt(&runtime, main, "l"));
  EXPECT_TRUE(l->isList());
  List list_l(&scope, *l);
  EXPECT_EQ(list_l->numItems(), 6);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(0))->value(), 0);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(1))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(2))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(3))->value(), 3);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(4))->value(), 4);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(5))->value(), 5);
}

TEST(TestThread, BuildSetUnpack) {
  const char* src = R"(
s = {*[0, 1], *{2, 3}, *(4, 5), *[]}
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));

  Object s(&scope, moduleAt(&runtime, main, "s"));
  EXPECT_TRUE(s->isSet());
  Set set_s(&scope, *s);
  EXPECT_EQ(set_s->numItems(), 6);
  Object small_int(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(runtime.setIncludes(set_s, small_int));
  small_int = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(set_s, small_int));
  small_int = SmallInt::fromWord(2);
  EXPECT_TRUE(runtime.setIncludes(set_s, small_int));
  small_int = SmallInt::fromWord(3);
  EXPECT_TRUE(runtime.setIncludes(set_s, small_int));
  small_int = SmallInt::fromWord(4);
  EXPECT_TRUE(runtime.setIncludes(set_s, small_int));
  small_int = SmallInt::fromWord(5);
  EXPECT_TRUE(runtime.setIncludes(set_s, small_int));
}

TEST(BuildString, buildStringEmpty) {
  Runtime runtime;
  HandleScope scope;
  Code code(&scope, testing::newEmptyCode(&runtime));

  const byte bc[] = {BUILD_STRING, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc));

  RawObject obj = Thread::currentThread()->run(code);
  EXPECT_TRUE(obj->isStr());
  EXPECT_TRUE(obj->isSmallStr());

  Str result(&scope, obj);
  EXPECT_TRUE(result->equalsCStr(""));
}

TEST(BuildString, buildStringSingle) {
  Runtime runtime;
  HandleScope scope;
  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(1));
  const char* expected = "foo";
  Object str(&scope, SmallStr::fromCStr(expected));
  consts->atPut(0, *str);
  code->setConsts(*consts);

  const byte bc[] = {LOAD_CONST, 0, BUILD_STRING, 1, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc));

  RawObject obj = Thread::currentThread()->run(code);
  EXPECT_TRUE(obj->isStr());
  EXPECT_TRUE(obj->isSmallStr());

  Str result(&scope, obj);
  EXPECT_TRUE(result->equalsCStr(expected));
}

TEST(BuildString, buildStringMultiSmall) {
  Runtime runtime;
  HandleScope scope;
  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(2));
  Object str(&scope, SmallStr::fromCStr("foo"));
  Object str1(&scope, SmallStr::fromCStr("bar"));
  consts->atPut(0, *str);
  consts->atPut(1, *str1);
  code->setConsts(*consts);

  const byte bc[] = {LOAD_CONST,   0, LOAD_CONST,   1,
                     BUILD_STRING, 2, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc));

  RawObject obj = Thread::currentThread()->run(code);
  EXPECT_TRUE(obj->isStr());
  EXPECT_TRUE(obj->isSmallStr());

  Str result(&scope, obj);
  EXPECT_TRUE(result->equalsCStr("foobar"));
}

TEST(BuildString, buildStringMultiLarge) {
  Runtime runtime;
  HandleScope scope;
  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(3));
  Object str(&scope, SmallStr::fromCStr("hello"));
  Object str1(&scope, SmallStr::fromCStr("world"));
  Object str2(&scope, SmallStr::fromCStr("python"));
  consts->atPut(0, *str);
  consts->atPut(1, *str1);
  consts->atPut(2, *str2);
  code->setConsts(*consts);

  const byte bc[] = {LOAD_CONST,   0, LOAD_CONST,   1, LOAD_CONST, 2,
                     BUILD_STRING, 3, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc));

  RawObject obj = Thread::currentThread()->run(code);
  EXPECT_TRUE(obj->isStr());
  EXPECT_TRUE(obj->isLargeStr());

  Str result(&scope, obj);
  EXPECT_TRUE(result->equalsCStr("helloworldpython"));
}

TEST(UnpackSeq, unpackRangePyStone) {
  const char* src = R"(
[Ident1, Ident2, Ident3, Ident4, Ident5] = range(1, 6)
print(Ident1, Ident2, Ident3, Ident4, Ident5)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2 3 4 5\n");
}

TEST(UnpackSeq, unpackRange) {
  const char* src = R"(
[a ,b, c] = range(2, 5)
print(a, b, c)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "2 3 4\n");
}

// LIST_APPEND(listAdd) in list_comp, followed by unpack
// TODO(rkng): list support in BINARY_ADD
TEST(UnpackList, unpackListCompAppend) {
  const char* src = R"(
a = [1, 2, 3]
b = [x for x in a]
b1, b2, b3 = b
print(b1, b2, b3)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2 3\n");
}

TEST(ThreadTest, SetAdd) {
  const char* src = R"(
a = [1, 2, 3]
b = {x for x in a}
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(b->isSet());
  Set set_b(&scope, *b);
  EXPECT_EQ(set_b->numItems(), 3);
}

TEST(TestThread, MapAdd) {
  const char* src = R"(
a = ['a', 'b', 'c']
b = {x:x for x in a}
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(b->isDict(), true);
  Dict dict_b(&scope, *b);
  EXPECT_EQ(dict_b->numItems(), 3);
}

TEST(UnpackList, unpackNestedLists) {
  const char* src = R"(
b = [[1,2], [3,4,5]]
b1, b2 = b
b11, b12 = b1
b21, b22, b23 = b2
print(len(b), len(b1), len(b2), b11, b12, b21, b22, b23)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "2 2 3 1 2 3 4 5\n");
}

TEST(UnpackSeq, unpackRangeStep) {
  const char* src = R"(
[a ,b, c, d] = range(2, 10, 2)
print(a, b, c, d)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "2 4 6 8\n");
}

TEST(UnpackSeq, unpackRangeNeg) {
  const char* src = R"(
[a ,b, c, d, e] = range(-10, 0, 2)
print(a, b, c, d, e)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "-10 -8 -6 -4 -2\n");
}

TEST(ListIterTest, build) {
  const char* src = R"(
a = [1, 2, 3]
for x in a:
  print(x)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n3\n");
}

TEST(ListAppendTest, buildAndUnpack) {
  const char* src = R"(
a = [1, 2]
b = [x for x in [a] * 3]
b1, b2, b3 = b
b11, b12 = b1
print(len(b), len(b1), b11, b12)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "3 2 1 2\n");
}

TEST(ListInsertTest, InsertToList) {
  const char* src = R"(
l = []
for i in range(16):
  if i == 2 or i == 12:
    continue
  l.append(i)

a, b = l[2], l[12]

l.insert(2, 2)
l.insert(12, 12)

s = 0
for el in l:
    s += el
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  Object l(&scope, moduleAt(&runtime, main, "l"));
  Object s(&scope, moduleAt(&runtime, main, "s"));

  ASSERT_NE(RawSmallInt::cast(*a)->value(), 2);
  ASSERT_NE(RawSmallInt::cast(*b)->value(), 12);

  List list_l(&scope, *l);
  ASSERT_EQ(list_l->numItems(), 16);
  ASSERT_EQ(RawSmallInt::cast(list_l->at(2))->value(), 2);
  ASSERT_EQ(RawSmallInt::cast(list_l->at(12))->value(), 12);

  // sum(0..16) = 120
  ASSERT_EQ(RawSmallInt::cast(*s)->value(), 120);
}

TEST(ListInsertTest, InsertToListBounds) {
  const char* src = R"(
l = [x for x in range(1, 5)]
l.insert(100, 5)
l.insert(400, 6)
l.insert(-100, 0)
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object l(&scope, moduleAt(&runtime, main, "l"));
  List list_l(&scope, *l);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(0))->value(), 0);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(1))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(2))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(3))->value(), 3);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(4))->value(), 4);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(5))->value(), 5);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(6))->value(), 6);
}

TEST(ListInsertTest, InsertToNegativeIndex) {
  const char* src = R"(
l = [0, 2, 4]
l.insert(-2, 1)
l.insert(-1, 3)
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object l(&scope, moduleAt(&runtime, main, "l"));
  List list_l(&scope, *l);
  ASSERT_EQ(list_l->numItems(), 5);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(0))->value(), 0);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(1))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(2))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(3))->value(), 3);
  EXPECT_EQ(RawSmallInt::cast(list_l->at(4))->value(), 4);
}

TEST(ThreadTest, BaseTypeConflict) {
  const char* src = R"(
class Foo(list, dict): pass
)";
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                            "multiple bases have instance lay-out conflict"));
}

TEST(BuildSlice, noneSliceCopyList) {
  const char* src = R"(
a = [1, 2, 3]
b = a[:]
print(len(b), b[0], b[1], b[2])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "3 1 2 3\n");
}

TEST(BuildSlice, sliceOperations) {
  const char* src = R"(
a = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
b = a[1:2:3]
print(len(b), b[0])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2\n");

  const char* src2 = R"(
a = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
b = a[1::3]
print(len(b), b[0], b[1], b[2])
)";
  std::string output2 = compileAndRunToString(&runtime, src2);
  EXPECT_EQ(output2, "3 2 5 8\n");

  const char* src3 = R"(
a = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
b = a[8:2:-2]
print(len(b), b[0], b[1], b[2])
)";
  std::string output3 = compileAndRunToString(&runtime, src3);
  ASSERT_EQ(output3, "3 9 7 5\n");

  const char* src4 = R"(
a = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
b = a[8:2:2]
print(len(b))
)";
  std::string output4 = compileAndRunToString(&runtime, src4);
  EXPECT_EQ(output4, "0\n");
}

TEST(BuildSlice, noneSliceCopyListComp) {  // pystone
  const char* src = R"(
a = [1, 2, 3]
b = [x[:] for x in [a] * 2]
c = b is a
b1, b2 = b
b11, b12, b13 = b1
print(c, len(b), len(b1), b11, b12, b13)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "False 2 3 1 2 3\n");
}

TEST(ThreadTest, SliceNoneCopyListCompPrint) {  // based on pystone.py
  const char* src = R"(
Array1Glob = [0]*5
Array2Glob = [x[:] for x in [Array1Glob]*5]
print(len(Array1Glob), len(Array2Glob), len(Array2Glob[0]), Array1Glob, Array2Glob[0])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "5 5 5 [0, 0, 0, 0, 0] [0, 0, 0, 0, 0]\n");
}

TEST(ThreadTest, BreakLoopWhileLoop) {
  const char* src = R"(
a = 0
while 1:
    a = a + 1
    print(a)
    if a == 3:
        break
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n3\n");
}

TEST(ThreadTest, BreakLoopWhileLoop1) {
  const char* src = R"(
a = 0
while 1:
    a = a + 1
    print(a)
    if a == 3:
        break
print("ok",a)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n3\nok 3\n");
}

TEST(ThreadTest, BreakLoopWhileLoopBytecode) {
  Runtime runtime;
  HandleScope scope;

  Tuple consts(&scope, runtime.newTuple(4));
  Code code(&scope, testing::newEmptyCode(&runtime));
  consts->atPut(0, SmallInt::fromWord(0));
  consts->atPut(1, SmallInt::fromWord(1));
  consts->atPut(2, SmallInt::fromWord(3));
  consts->atPut(3, NoneType::object());
  code->setConsts(*consts);

  Tuple names(&scope, runtime.newTuple(1));
  Object key(&scope, runtime.newStrFromCStr("a"));
  names->atPut(0, *key);
  code->setNames(*names);

  // see python code in BreakLoop.whileLoop (sans print)
  const byte bc[] = {LOAD_CONST,        0,                  // 0
                     STORE_NAME,        0,                  // a
                     SETUP_LOOP,        22, LOAD_NAME,  0,  // a
                     LOAD_CONST,        1,                  // 1
                     BINARY_ADD,        0,  STORE_NAME, 0,  // a
                     LOAD_NAME,         0,                  // a
                     LOAD_CONST,        2,                  // 3
                     COMPARE_OP,        2,                  // ==
                     POP_JUMP_IF_FALSE, 6,  BREAK_LOOP, 0, JUMP_ABSOLUTE, 6,
                     POP_BLOCK,         0,  LOAD_CONST, 3,  // None
                     RETURN_VALUE,      0};
  code->setCode(runtime.newBytesWithAll(bc));

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(code);

  Dict implicit_globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());

  frame->setImplicitGlobals(*implicit_globals);
  frame->setFastGlobals(
      runtime.computeFastGlobals(code, implicit_globals, builtins));

  Object result(&scope, Interpreter::execute(thread, frame));
  Object value(&scope, runtime.dictAt(implicit_globals, key));
  ASSERT_TRUE(value->isValueCell());
  RawObject value_obj = RawValueCell::cast(*value)->value();
  EXPECT_TRUE(value_obj->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(value_obj)->value(), 3);
}

TEST(ThreadTest, BreakLoopRangeLoop) {
  const char* src = R"(
for x in range(1,6):
  if x == 3:
    break;
  print(x)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n");
}

TEST(ThreadTest, ContinueLoopRangeLoop) {
  const char* src = R"(
l = []

for x in range(4):
    if x == 3:
        try:
            continue
        except:
            # This is to prevent the peephole optimizer
            # from turning the CONTINUE_LOOP op
            # into a JUMP_ABSOLUTE op
            pass
    l.append(x)
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object l(&scope, moduleAt(&runtime, main, "l"));
  EXPECT_TRUE(l->isList());
  List list_l(&scope, *l);
  ASSERT_GE(list_l->numItems(), 3);
  EXPECT_EQ(list_l->at(0), SmallInt::fromWord(0));
  EXPECT_EQ(list_l->at(1), SmallInt::fromWord(1));
  EXPECT_EQ(list_l->at(2), SmallInt::fromWord(2));
}

TEST(ThreadTest, ContinueLoopRangeLoopByteCode) {
  Runtime runtime;
  HandleScope scope;

  Tuple consts(&scope, runtime.newTuple(5));
  Code code(&scope, testing::newEmptyCode(&runtime));
  consts->atPut(0, SmallInt::fromWord(0));
  consts->atPut(1, SmallInt::fromWord(4));
  consts->atPut(2, SmallInt::fromWord(1));
  consts->atPut(3, SmallInt::fromWord(3));
  consts->atPut(4, NoneType::object());
  code->setConsts(*consts);
  code->setArgcount(0);
  code->setNlocals(2);

  Tuple names(&scope, runtime.newTuple(2));
  Object key0(&scope, runtime.newStrFromCStr("cnt"));
  Object key1(&scope, runtime.newStrFromCStr("s"));
  names->atPut(0, *key0);
  names->atPut(1, *key0);
  code->setNames(*names);

  //  # python code:
  //  cnt = 0
  //  s = 0
  //  while cnt < 4:
  //      cnt += 1
  //      if cnt == 3:
  //          continue
  //      s += cnt
  //  return s
  const byte bc[] = {LOAD_CONST,        0,  // 0
                     STORE_FAST,        0,  // (cnt)

                     LOAD_CONST,        0,  // 0
                     STORE_FAST,        1,  // s

                     SETUP_LOOP,        38,  // (to 48)
                     LOAD_FAST,         0,   // (cnt)
                     LOAD_CONST,        1,   // (4)
                     COMPARE_OP,        0,   // (<)
                     POP_JUMP_IF_FALSE, 46,

                     LOAD_FAST,         0,                    // (cnt)
                     LOAD_CONST,        2,                    // (1)
                     INPLACE_ADD,       0,  STORE_FAST,   0,  // (cnt)

                     LOAD_FAST,         0,  // (cnt)
                     LOAD_CONST,        3,  // (3)
                     COMPARE_OP,        2,  // (==)
                     POP_JUMP_IF_FALSE, 36,

                     CONTINUE_LOOP,     10,

                     LOAD_FAST,         1,                    // (s)
                     LOAD_FAST,         0,                    // (cnt)
                     INPLACE_ADD,       0,  STORE_FAST,   1,  // (s)
                     JUMP_ABSOLUTE,     10, POP_BLOCK,    0,

                     LOAD_FAST,         1,  RETURN_VALUE, 0};

  code->setCode(runtime.newBytesWithAll(bc));

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(code);

  Dict implicit_globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());

  frame->setImplicitGlobals(*implicit_globals);
  frame->setFastGlobals(
      runtime.computeFastGlobals(code, implicit_globals, builtins));

  Object result(&scope, Interpreter::execute(thread, frame));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*result)->value(), 7);
}

TEST(ThreadTest, Func2TestPyStone) {  // mimic pystone.py Func2
  const char* src = R"(
def f1(x, y):
  return x + y
def f2():
  return f1(1, 2)
print(f2())
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "3\n");
}

TEST(ThreadTest, BinSubscrString) {  // pystone dependency
  const char* src = R"(
a = 'Hello'
print(a[0],a[1],a[2],a[3],a[4])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "H e l l o\n");
}

TEST(ThreadTest, SetupExceptNoOp) {  // pystone dependency
  const char* src = R"(
def f(x):
  try: print(x)
  except ValueError:
    print("Invalid Argument")
f(100)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "100\n");
}

TEST(ThreadTest, ImportFromNeg) {
  const char* src = R"(
from time import foobarbaz
)";

  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kImportError,
                            "cannot import name"));
}

TEST(ThreadTest, StrFormatEmpty) {
  const char* src = R"(
print("" % ("hi"))
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "\n");
}

TEST(ThreadTest, StrFormatNone) {
  const char* src = R"(
h = "hello"
p = "pyro pystone"
print("hello world" % (h, p))
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello world\n");
}

TEST(ThreadTest, StrFormatMod) {
  const char* src = R"(
print("%%%s" % (""))
)";

  Runtime runtime;
  const std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "%\n");
}

TEST(ThreadTest, StrFormatNeg1) {
  const char* src = R"(
h = "hi"
print("%" % (h, "world"))
)";

  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kValueError,
                            "Incomplete format"));
}

TEST(ThreadTest, StrFormatStr) {
  const char* src = R"(
h = "hello pyro"
print("%s" % (h))
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello pyro\n");
}

TEST(ThreadTest, StrFormatStr2) {
  const char* src = R"(
h = "hello"
p = "pyro"
print("%s%s" % (h, p))
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hellopyro\n");
}

TEST(ThreadTest, StrFormatFloat) {
  const char* src = R"(
d = 67.89
print("There are %g pystones" % (d))
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "There are 67.89 pystones\n");
}

TEST(ThreadTest, StrFormatMixed) {
  const char* src = R"(
a = 123
d = 67.89
print("There are %d pystones %g" % (a, d))
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "There are 123 pystones 67.89\n");
}

TEST(ThreadTest, StrFormatMixed2) {
  const char* src = R"(
a = 123
d = 67.89
c = "now"
print("There are %d pystones %g %s" % (a, d, c))
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "There are 123 pystones 67.89 now\n");
}

TEST(ThreadTest, StrFormatMixed3) {
  const char* src = R"(
a = 123
d = 67.89
c = "now"
print("There are %d pystones %g %s what" % (a, d, c))
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "There are 123 pystones 67.89 now what\n");
}

TEST(ThreadTest, BuildTypeWithMetaclass) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
class Foo(metaclass=type):
  pass
a = Foo()
)";
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_TRUE(foo->isType());
  Object a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(runtime.typeOf(*a) == *foo);
}

TEST(ThreadTest, BuildTypeWithMetaclass2) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
class Foo(type):
  def __new__(mcls, name, bases, dict):
    cls = super(Foo, mcls).__new__(mcls, name, bases, dict)
    cls.lalala = 123
    return cls
class Bar(metaclass=Foo):
  def __init__(self):
    self.hahaha = 456
b = Bar.lalala
a = Bar()
c = a.hahaha
)";
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object bar(&scope, moduleAt(&runtime, main, "Bar"));
  EXPECT_TRUE(runtime.isInstanceOfType(*bar));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(runtime.typeOf(*a) == *bar);
  Object b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 123);
  Object c(&scope, moduleAt(&runtime, main, "c"));
  EXPECT_EQ(RawSmallInt::cast(*c)->value(), 456);
}

TEST(ThreadTest, NameLookupInTypeBodyFindsImplicitGlobal) {
  const char* src = R"(
a = 0
b = 0
class C:
    global a
    global b
    PI = 3
    a = PI
    PIPI = PI * 2
    b = PIPI
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 3);
  Object b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 6);
}

TEST(ThreadTest, NameLookupInTypeBodyFindsGlobal) {
  const char* src = R"(
var = 1
class C:
  global one
  global two
  one = var
  var = 2
  two = var
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object one(&scope, moduleAt(&runtime, main, "one"));
  EXPECT_EQ(RawSmallInt::cast(*one)->value(), 1);
  Object two(&scope, moduleAt(&runtime, main, "two"));
  EXPECT_EQ(RawSmallInt::cast(*two)->value(), 2);
}

TEST(ThreadTest, ExecuteDeleteName) {
  const char* src = R"(
var = 1
del var
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object var(&scope, moduleAt(&runtime, main, "var"));
  EXPECT_TRUE(var->isError());
}

TEST(ThreadTest, SetupFinally) {
  const char* src = R"(
x = 1
try:
  pass
finally:
  x = 2
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object x(&scope, moduleAt(&runtime, main, "x"));
  EXPECT_EQ(*x, SmallInt::fromWord(2));
}

TEST(ThreadTest, SetupAnnotationsAndStoreAnnotations) {
  const char* src = R"(
x: int = 1
class Foo:
  bar: int = 2
class_anno_dict = Foo.__annotations__
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Dict module_anno_dict(&scope, moduleAt(&runtime, main, "__annotations__"));
  Object m_key(&scope, runtime.newStrFromCStr("x"));
  Object m_value(&scope, runtime.dictAt(module_anno_dict, m_key));
  EXPECT_EQ(*m_value, runtime.typeAt(LayoutId::kInt));

  Dict class_anno_dict(&scope, moduleAt(&runtime, main, "class_anno_dict"));
  Object c_key(&scope, runtime.newStrFromCStr("bar"));
  Object c_value(&scope, runtime.dictAt(class_anno_dict, c_key));
  EXPECT_EQ(*c_value, runtime.typeAt(LayoutId::kInt));
}

TEST(ThreadTest, DeleteFastRaisesUnboundLocalError) {
  const char* src = R"(
def foo(a, b, c):
  del a
  return a
foo(1, 2, 3)
)";
  Runtime runtime;
  HandleScope scope;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src),
                            LayoutId::kUnboundLocalError,
                            "local variable 'a' referenced before assignment"));
}

TEST(ThreadTest, DeleteFast) {
  const char* src = R"(
def foo(a, b, c):
  del a
  return b
x = foo(1, 2, 3)
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object x(&scope, moduleAt(&runtime, main, "x"));
  EXPECT_EQ(*x, SmallInt::fromWord(2));
}

TEST(ThreadTest, ConstructInstanceWithKwargs) {
  const char* src = R"(
result_a = None
result_b = None
result_c = None

class Foo:
  def __init__(self, a, b=None, c=None):
    global result_a, result_b, result_c
    result_a = a
    result_b = b
    result_c = c

foo = Foo(1111, b=2222, c=3333)
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));

  Object result_a(&scope, moduleAt(&runtime, main, "result_a"));
  ASSERT_TRUE(result_a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*result_a)->value(), 1111);

  Object result_b(&scope, moduleAt(&runtime, main, "result_b"));
  ASSERT_TRUE(result_b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*result_b)->value(), 2222);

  Object result_c(&scope, moduleAt(&runtime, main, "result_c"));
  ASSERT_TRUE(result_c->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*result_c)->value(), 3333);
}

TEST(ThreadTest, LoadTypeDeref) {
  const char* src = R"(
def foo():
  a = 1
  class Foo:
    b = a
  return Foo.b
x = foo()
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object x(&scope, moduleAt(&runtime, main, "x"));
  EXPECT_EQ(*x, SmallInt::fromWord(1));
}

TEST(ThreadTest, LoadTypeDerefFromLocal) {
  Runtime runtime;
  HandleScope scope;
  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, SmallInt::fromWord(1111));
  Tuple freevars(&scope, runtime.newTuple(1));
  freevars->atPut(0, SmallStr::fromCStr("lalala"));
  Tuple names(&scope, runtime.newTuple(1));
  names->atPut(0, SmallStr::fromCStr("lalala"));
  code->setConsts(*consts);
  code->setNames(*names);
  code->setFreevars(*freevars);
  const byte bytecode[] = {LOAD_CONST,      0, STORE_NAME,   0,
                           LOAD_CLASSDEREF, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));
  code->setStacksize(2);
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(code);
  frame->setCode(*code);
  Dict implicit_global(&scope, runtime.newDict());
  frame->setImplicitGlobals(*implicit_global);
  RawObject result = Interpreter::execute(thread, frame);
  ASSERT_TRUE(result->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(result)->value(), 1111);
}

}  // namespace python
