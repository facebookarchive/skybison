#include "gtest/gtest.h"

#include <memory>

#include "bytecode.h"
#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "marshal.h"
#include "runtime.h"
#include "test-utils.h"
#include "thread.h"

namespace python {
using namespace testing;

using BuildSlice = RuntimeFixture;
using BuildString = RuntimeFixture;
using FormatTest = RuntimeFixture;
using ListAppendTest = RuntimeFixture;
using ListInsertTest = RuntimeFixture;
using ListIterTest = RuntimeFixture;
using ThreadTest = RuntimeFixture;
using UnpackList = RuntimeFixture;
using UnpackSeq = RuntimeFixture;

TEST_F(ThreadTest, CheckMainThreadRuntime) {
  ASSERT_EQ(thread_->runtime(), &runtime_);
}

TEST_F(ThreadTest, RunEmptyFunction) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, NoneType::object());
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Thread thread2(1 * kKiB);
  thread2.setRuntime(&runtime_);
  Thread::setCurrentThread(&thread2);
  EXPECT_TRUE(runCode(code).isNoneType());

  // Avoid assertion in runtime destructor.
  Thread::setCurrentThread(thread_);
}

TEST_F(ThreadTest, RunHelloWorld) {
  const char* src = R"(
print('hello, world')
)";
  std::string result = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(result, "hello, world\n");
}

TEST_F(ThreadTest, ModuleBodyCallsHelloWorldFunction) {
  const char* src = R"(
def hello():
  print('hello, world')
hello()
)";
  // Execute the code and make sure we get back the result we expect
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "hello, world\n");
}

TEST_F(ThreadTest, DunderCallInstance) {
  HandleScope scope(thread_);

  const char* src = R"(
class C:
  def __init__(self, x, y):
    self.value = x + y
  def __call__(self, y):
    return self.value * y
c = C(10, 2)
g = c(3)
)";

  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Module main(&scope, findModule(&runtime_, "__main__"));
  Object global(&scope, moduleAt(&runtime_, main, "g"));
  EXPECT_TRUE(isIntEqualsWord(*global, 36));
}

TEST_F(ThreadTest, DunderCallInstanceWithDescriptor) {
  HandleScope scope(thread_);
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
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object result(&scope, moduleAt(&runtime_, main, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1111));
}

TEST_F(ThreadTest, DunderCallInstanceKw) {
  HandleScope scope(thread_);
  const char* src = R"(
class C:
  def __init__(self):
    self.value = None

  def __call__(self, y):
    return y

c = C()
result = c(y=3)
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Module main(&scope, findModule(&runtime_, "__main__"));
  Object result(&scope, moduleAt(&runtime_, main, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
}

TEST_F(ThreadTest, DunderCallInstanceSplatArgs) {
  HandleScope scope(thread_);
  const char* src = R"(
class C:
  def __call__(self, y):
    return y

c = C()
args = (3,)
result = c(*args)
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Module main(&scope, findModule(&runtime_, "__main__"));
  Object result(&scope, moduleAt(&runtime_, main, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
}

TEST_F(ThreadTest, DunderCallInstanceSplatKw) {
  HandleScope scope(thread_);
  const char* src = R"(
class C:
  def __call__(self, y):
    return y

c = C()
kwargs = {'y': 3}
result = c(**kwargs)
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Module main(&scope, findModule(&runtime_, "__main__"));
  Object result(&scope, moduleAt(&runtime_, main, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
}

TEST_F(ThreadTest, DunderCallInstanceSplatArgsAndKw) {
  HandleScope scope(thread_);
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
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Module main(&scope, findModule(&runtime_, "__main__"));
  Object result_x(&scope, moduleAt(&runtime_, main, "result_x"));
  EXPECT_TRUE(isIntEqualsWord(*result_x, 1));
  Object result_y(&scope, moduleAt(&runtime_, main, "result_y"));
  EXPECT_TRUE(isIntEqualsWord(*result_y, 3));
}

TEST_F(ThreadTest, OverlappingFrames) {
  HandleScope scope(thread_);

  // Push a frame for a code object with space for 3 items on the value stack
  Object name(&scope, Str::empty());
  Code caller_code(&scope, newEmptyCode());
  caller_code.setCode(Bytes::empty());
  caller_code.setStacksize(3);

  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime_.newDict());
  Function caller(
      &scope, Interpreter::makeFunction(thread_, name, caller_code, none, none,
                                        none, none, globals));

  thread_->currentFrame()->pushValue(*caller);
  Frame* caller_frame = thread_->pushCallFrame(*caller);

  // Push a frame for a code object that expects 3 arguments and needs space
  // for 3 local variables
  Code code(&scope, newEmptyCode());
  code.setCode(Bytes::empty());
  code.setArgcount(3);
  code.setNlocals(3);

  Function function(
      &scope, Interpreter::makeFunction(thread_, name, code, none, none, none,
                                        none, globals));

  // Push args on the stack in the sequence generated by CPython
  caller_frame->pushValue(*function);
  caller_frame->pushValue(runtime_.newInt(1111));
  caller_frame->pushValue(runtime_.newInt(2222));
  caller_frame->pushValue(runtime_.newInt(3333));
  Frame* frame = thread_->pushCallFrame(*function);

  // Make sure we can read the args from the frame
  EXPECT_TRUE(isIntEqualsWord(frame->local(0), 1111));
  EXPECT_TRUE(isIntEqualsWord(frame->local(1), 2222));
  EXPECT_TRUE(isIntEqualsWord(frame->local(2), 3333));
}

TEST_F(ThreadTest, EncodeTryBlock) {
  TryBlock block(TryBlock::kExcept, 200, 300);
  EXPECT_EQ(block.kind(), TryBlock::kExcept);
  EXPECT_EQ(block.handler(), 200);
  EXPECT_EQ(block.level(), 300);

  TryBlock decoded(block.asSmallInt());
  EXPECT_EQ(decoded.kind(), block.kind());
  EXPECT_EQ(decoded.handler(), block.handler());
  EXPECT_EQ(decoded.level(), block.level());
}

TEST_F(ThreadTest, PushPopFrame) {
  HandleScope scope(thread_);

  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  code.setCode(Bytes::empty());
  code.setNlocals(2);
  code.setStacksize(3);
  Dict globals(&scope, runtime_.newDict());
  Object none(&scope, NoneType::object());
  Function function(
      &scope, Interpreter::makeFunction(thread_, name, code, none, none, none,
                                        none, globals));

  thread_->currentFrame()->pushValue(*function);
  byte* prev_sp = thread_->stackPtr();
  Frame* frame = thread_->pushCallFrame(*function);

  // Verify frame invariants post-push
  EXPECT_EQ(frame->previousFrame(), thread_->initialFrame());
  EXPECT_EQ(frame->valueStackTop(), reinterpret_cast<RawObject*>(frame));
  EXPECT_EQ(frame->valueStackBase(), frame->valueStackTop());
  EXPECT_EQ(frame->numLocals(), 2);

  // Make sure we restore the thread's stack pointer back to its previous
  // location
  thread_->popFrame();
  EXPECT_EQ(thread_->stackPtr(), prev_sp);
}

TEST_F(ThreadTest, PushFrameWithNoCellVars) {
  HandleScope scope(thread_);

  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  code.setCode(Bytes::empty());
  code.setCellvars(NoneType::object());
  code.setFreevars(runtime_.newTuple(0));
  Dict globals(&scope, runtime_.newDict());
  Object none(&scope, NoneType::object());
  Function function(
      &scope, Interpreter::makeFunction(thread_, name, code, none, none, none,
                                        none, globals));
  thread_->currentFrame()->pushValue(*function);
  byte* prev_sp = thread_->stackPtr();
  thread_->pushCallFrame(*function);

  EXPECT_EQ(thread_->stackPtr(), prev_sp - Frame::kSize);
}

TEST_F(ThreadTest, PushFrameWithNoFreeVars) {
  HandleScope scope(thread_);

  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  code.setCode(Bytes::empty());
  code.setFreevars(NoneType::object());
  code.setCellvars(runtime_.newTuple(0));
  Dict globals(&scope, runtime_.newDict());
  Object none(&scope, NoneType::object());
  Function function(
      &scope, Interpreter::makeFunction(thread_, name, code, none, none, none,
                                        none, globals));
  thread_->currentFrame()->pushValue(*function);
  byte* prev_sp = thread_->stackPtr();
  thread_->pushCallFrame(*function);

  EXPECT_EQ(thread_->stackPtr(), prev_sp - Frame::kSize);
}

TEST_F(ThreadTest, OpenAndLinkFrameZerosBlockStack) {
  HandleScope scope(thread_);

  // Fill stack with nonsense data.
  Frame* frame = thread_->currentFrame();
  for (word i = 0; i < 100; i++) {
    frame->pushValue(SmallInt::fromWord(0xbadf00d));
  }
  frame->dropValues(100);

  Code code(&scope, newEmptyCode());
  code.setCode(Bytes::empty());
  Object qualname(&scope, Str::empty());
  Object empty_tuple(&scope, runtime_.emptyTuple());
  Dict empty_dict(&scope, runtime_.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread_, qualname, code, empty_tuple,
                                        empty_dict, empty_dict, empty_tuple,
                                        empty_dict));

  frame->pushValue(*function);
  Frame* new_frame = thread_->pushCallFrame(*function);
  // The block stack is a contiguous chunk of small integers.
  RawObject* block_stack =
      reinterpret_cast<RawObject*>(new_frame->blockStack());
  for (word i = 0; i < BlockStack::kSize / kPointerSize; i++) {
    EXPECT_EQ(block_stack[i], SmallInt::fromWord(0));
  }
}

TEST_F(ThreadTest, ManipulateValueStack) {
  Frame* frame = thread_->currentFrame();

  // Push 3 items on the value stack
  RawObject* sp = frame->valueStackTop();
  *--sp = SmallInt::fromWord(1111);
  *--sp = SmallInt::fromWord(2222);
  *--sp = SmallInt::fromWord(3333);
  frame->setValueStackTop(sp);
  ASSERT_EQ(frame->valueStackTop(), sp);

  // Verify the value stack is laid out as we expect
  EXPECT_TRUE(isIntEqualsWord(frame->peek(0), 3333));
  EXPECT_TRUE(isIntEqualsWord(frame->peek(1), 2222));
  EXPECT_TRUE(isIntEqualsWord(frame->peek(2), 1111));

  // Pop 2 items off the stack and check the stack is still as we expect
  frame->setValueStackTop(sp + 2);
  EXPECT_TRUE(isIntEqualsWord(frame->peek(0), 1111));
}

TEST_F(ThreadTest, ManipulateBlockStack) {
  Frame* frame = thread_->currentFrame();
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

TEST_F(ThreadTest, CallFunction) {
  HandleScope scope(thread_);

  // Build the code object for the following function
  //
  //     def noop(a, b):
  //         return 2222
  //
  Code callee_code(&scope, newEmptyCode());
  callee_code.setArgcount(2);
  callee_code.setNlocals(2);
  callee_code.setStacksize(1);
  callee_code.setConsts(runtime_.newTuple(1));
  Tuple::cast(callee_code.consts()).atPut(0, runtime_.newInt(2222));
  const byte callee_bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  callee_code.setCode(runtime_.newBytesWithAll(callee_bytecode));

  // Create the function object and bind it to the code object
  Object qualname(&scope, Str::empty());
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime_.newDict());
  Function callee(
      &scope, Interpreter::makeFunction(thread_, qualname, callee_code, none,
                                        none, none, none, globals));

  // Build a code object to call the function defined above
  Code caller_code(&scope, newEmptyCode());
  caller_code.setStacksize(3);
  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  consts.atPut(2, SmallInt::fromWord(2222));
  caller_code.setConsts(*consts);
  const byte caller_bytecode[] = {LOAD_CONST,   0, LOAD_CONST,    1,
                                  LOAD_CONST,   2, CALL_FUNCTION, 2,
                                  RETURN_VALUE, 0};
  caller_code.setCode(runtime_.newBytesWithAll(caller_bytecode));

  // Execute the caller and make sure we get back the expected result
  EXPECT_TRUE(isIntEqualsWord(runCode(caller_code), 2222));
}

TEST_F(ThreadTest, ExtendedArg) {
  HandleScope scope(thread_);

  const word num_consts = 258;
  const byte bytecode[] = {EXTENDED_ARG, 1, LOAD_CONST, 1, RETURN_VALUE, 0};

  Tuple constants(&scope, runtime_.newTuple(num_consts));

  auto zero = SmallInt::fromWord(0);
  auto non_zero = SmallInt::fromWord(0xDEADBEEF);
  for (word i = 0; i < num_consts - 2; i++) {
    constants.atPut(i, zero);
  }
  constants.atPut(num_consts - 1, non_zero);
  Code code(&scope, newEmptyCode());
  code.setConsts(*constants);
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(2);

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 0xDEADBEEF));
}

TEST_F(ThreadTest, CallBuiltinPrint) {
  std::string output = compileAndRunToString(
      &runtime_, "print(1111, 'testing 123', True, False)");
  EXPECT_EQ(output, "1111 testing 123 True False\n");
}

TEST_F(ThreadTest, CallBuiltinPrintKw) {
  std::string output =
      compileAndRunToString(&runtime_, "print('testing 123', end='abc')");
  EXPECT_STREQ(output.c_str(), "testing 123abc");
}

TEST_F(ThreadTest, ExecuteDupTop) {
  HandleScope scope(thread_);

  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(1111));
  Code code(&scope, newEmptyCode());
  code.setStacksize(2);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, DUP_TOP, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 1111));
}

TEST_F(ThreadTest, ExecuteDupTopTwo) {
  HandleScope scope(thread_);

  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, SmallInt::fromWord(2222));
  Code code(&scope, newEmptyCode());
  code.setStacksize(2);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,  0, LOAD_CONST,   1,
                           DUP_TOP_TWO, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 2222));
}

TEST_F(ThreadTest, ExecuteRotTwo) {
  HandleScope scope(thread_);

  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, SmallInt::fromWord(2222));
  Code code(&scope, newEmptyCode());
  code.setStacksize(2);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,   1,
                           ROT_TWO,    0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 1111));
}

TEST_F(ThreadTest, ExecuteRotThree) {
  HandleScope scope(thread_);

  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, SmallInt::fromWord(2222));
  consts.atPut(2, SmallInt::fromWord(3333));
  Code code(&scope, newEmptyCode());
  code.setStacksize(3);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,   1, LOAD_CONST, 2,
                           ROT_THREE,  0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 2222));
}

TEST_F(ThreadTest, ExecuteJumpAbsolute) {
  HandleScope scope(thread_);

  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, SmallInt::fromWord(2222));
  Code code(&scope, newEmptyCode());
  code.setStacksize(2);
  code.setConsts(*consts);
  const byte bytecode[] = {JUMP_ABSOLUTE, 4, LOAD_CONST,   0,
                           LOAD_CONST,    1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 2222));
}

TEST_F(ThreadTest, ExecuteJumpForward) {
  HandleScope scope(thread_);

  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, SmallInt::fromWord(2222));
  Code code(&scope, newEmptyCode());
  code.setStacksize(2);
  code.setConsts(*consts);
  const byte bytecode[] = {JUMP_FORWARD, 2, LOAD_CONST,   0,
                           LOAD_CONST,   1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 2222));
}

TEST_F(ThreadTest, ExecuteStoreLoadFast) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  code.setNlocals(2);
  const byte bytecode[] = {LOAD_CONST, 0, STORE_FAST,   1,
                           LOAD_FAST,  1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 1111));
}

TEST_F(ThreadTest, LoadGlobal) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple names(&scope, runtime_.newTuple(1));
  Object key(&scope, runtime_.newStrFromCStr("foo"));
  names.atPut(0, *key);
  code.setNames(*names);

  const byte bytecode[] = {LOAD_GLOBAL, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::NOFREE);

  Dict globals(&scope, runtime_.newDict());
  Object value(&scope, runtime_.newInt(1234));
  runtime_.moduleDictAtPut(thread_, globals, key, value);

  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, globals, globals), 1234));
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

TestData kGlobalVariableAccessTests[] = {
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
    {"LoadGlobalUnbound", "a is undefined\n",
     R"(
def f():
  try:
    print(a)
  except NameError:
    print("a is undefined")
f()
)",
     false},
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
    {"DeleteGlobalUnbound", "a is undefined\n",
     R"(
def f():
  global a
  try:
    del a
  except NameError:
    print("a is undefined")
f()
)",
     false},
    {"DeleteGlobalBuiltinUnbound", "isinstance is undefined\n",
     R"(
def f():
  global isinstance
  try:
    del isinstance
  except NameError:
    print("isinstance is undefined")
f()
)",
     false}};

class GlobalsTest : public ::testing::TestWithParam<TestData> {};

TEST_P(GlobalsTest, GlobalVariableAcess) {
  Runtime runtime;
  TestData data = GetParam();
  if (data.death) {
    EXPECT_DEATH(static_cast<void>(runFromCStr(&runtime, data.src)),
                 data.expected_output);
  } else {
    std::string output = compileAndRunToString(&runtime, data.src);
    EXPECT_EQ(output, data.expected_output);
  }
}

INSTANTIATE_TEST_CASE_P(GlobalVariableAcess, GlobalsTest,
                        ::testing::ValuesIn(kGlobalVariableAccessTests),
                        TestName);

TEST_F(ThreadTest, StoreGlobalCreateValueCell) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(42));
  code.setConsts(*consts);

  Tuple names(&scope, runtime_.newTuple(1));
  Object key(&scope, runtime_.newStrFromCStr("foo"));
  names.atPut(0, *key);
  code.setNames(*names);

  const byte bytecode[] = {LOAD_CONST, 0, STORE_GLOBAL, 0,
                           LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::NOFREE);

  Dict globals(&scope, runtime_.newDict());
  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, globals, globals), 42));
  EXPECT_TRUE(
      isIntEqualsWord(runtime_.moduleDictAt(thread_, globals, key), 42));
}

TEST_F(ThreadTest, StoreGlobalReuseValueCell) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(42));
  code.setConsts(*consts);

  Tuple names(&scope, runtime_.newTuple(1));
  Object key(&scope, runtime_.newStrFromCStr("foo"));
  names.atPut(0, *key);
  code.setNames(*names);

  const byte bytecode[] = {LOAD_CONST, 0, STORE_GLOBAL, 0,
                           LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::NOFREE);

  Dict globals(&scope, runtime_.newDict());
  Object value(&scope, runtime_.newInt(99));
  runtime_.moduleDictAtPut(thread_, globals, key, value);
  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, globals, globals), 42));
  EXPECT_TRUE(
      isIntEqualsWord(runtime_.moduleDictAt(thread_, globals, key), 42));
}

TEST_F(ThreadTest, StoreNameCreateValueCell) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(42));
  code.setConsts(*consts);

  Tuple names(&scope, runtime_.newTuple(1));
  Object key(&scope, runtime_.newStrFromCStr("foo"));
  names.atPut(0, *key);
  code.setNames(*names);

  const byte bytecode[] = {LOAD_CONST, 0, STORE_NAME,   0,
                           LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::NOFREE);

  Dict globals(&scope, runtime_.newDict());
  Dict locals(&scope, runtime_.newDict());
  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, globals, locals), 42));
  EXPECT_TRUE(isIntEqualsWord(runtime_.moduleDictAt(thread_, locals, key), 42));
}

TEST_F(ThreadTest, LoadNameInModuleBodyFromBuiltins) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());

  Tuple names(&scope, runtime_.newTuple(1));
  Object key(&scope, runtime_.newStrFromCStr("foo"));
  names.atPut(0, *key);
  code.setNames(*names);

  const byte bytecode[] = {LOAD_NAME, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::NOFREE);

  Dict globals(&scope, runtime_.newDict());
  Object module_name(&scope, runtime_.symbols()->Builtins());
  Module builtins(&scope, runtime_.newModule(module_name));
  Object dunder_builtins_name(&scope, runtime_.symbols()->DunderBuiltins());
  runtime_.moduleDictAtPut(thread_, globals, dunder_builtins_name, builtins);
  Object value(&scope, runtime_.newInt(123));
  runtime_.moduleAtPut(builtins, key, value);
  Dict locals(&scope, runtime_.newDict());
  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, globals, locals), 123));
}

TEST_F(ThreadTest, LoadNameFromGlobals) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());

  Tuple names(&scope, runtime_.newTuple(1));
  Object key(&scope, runtime_.newStrFromCStr("foo"));
  names.atPut(0, *key);
  code.setNames(*names);

  const byte bytecode[] = {LOAD_NAME, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::NOFREE);

  Dict globals(&scope, runtime_.newDict());
  Object value(&scope, runtime_.newInt(321));
  runtime_.typeDictAtPut(thread_, globals, key, value);
  Dict locals(&scope, runtime_.newDict());
  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, globals, locals), 321));
}

TEST_F(ThreadTest, LoadNameFromLocals) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());

  Tuple names(&scope, runtime_.newTuple(1));
  Object key(&scope, runtime_.newStrFromCStr("foo"));
  names.atPut(0, *key);
  code.setNames(*names);

  const byte bytecode[] = {LOAD_NAME, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::NOFREE);

  Dict globals(&scope, runtime_.newDict());
  Object globals_value(&scope, runtime_.newInt(456));
  runtime_.typeDictAtPut(thread_, globals, key, globals_value);
  Dict locals(&scope, runtime_.newDict());
  Object locals_value(&scope, runtime_.newInt(654));
  runtime_.typeDictAtPut(thread_, globals, key, locals_value);
  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, globals, locals), 654));
}

TEST_F(ThreadTest, MakeFunction) {
  HandleScope scope(thread_);

  Object name(&scope, runtime_.newStrFromCStr("hello"));
  Code func_code(&scope, newEmptyCode());
  func_code.setName(*name);
  func_code.setCode(runtime_.newBytes(0, 0));
  func_code.setFlags(Code::Flags::OPTIMIZED | Code::Flags::NEWLOCALS |
                     Code::Flags::NOFREE);

  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, *func_code);
  consts.atPut(1, runtime_.newStrFromCStr("hello_qualname"));
  consts.atPut(2, NoneType::object());
  code.setConsts(*consts);

  Tuple names(&scope, runtime_.newTuple(1));
  names.atPut(0, runtime_.newStrFromCStr("hello"));
  code.setNames(*names);

  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST, 1, MAKE_FUNCTION, 0,
                           STORE_NAME, 0, LOAD_CONST, 2, RETURN_VALUE,  0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setFlags(0);

  Dict globals(&scope, runtime_.newDict());
  Dict locals(&scope, runtime_.newDict());
  ASSERT_TRUE(thread_->exec(code, globals, locals).isNoneType());

  Object function_obj(&scope, runtime_.moduleDictAt(thread_, locals, name));
  ASSERT_TRUE(function_obj.isFunction());
  Function function(&scope, *function_obj);
  EXPECT_EQ(function.code(), func_code);
  EXPECT_TRUE(isStrEqualsCStr(function.name(), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(function.qualname(), "hello_qualname"));
  EXPECT_EQ(function.entry(), &interpreterTrampoline);
}

TEST_F(ThreadTest, BuildList) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, SmallInt::fromWord(111));
  consts.atPut(1, runtime_.newStrFromCStr("qqq"));
  consts.atPut(2, NoneType::object());
  code.setConsts(*consts);

  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,   1, LOAD_CONST, 2,
                           BUILD_LIST, 3, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result_obj(&scope, runCode(code));
  ASSERT_TRUE(result_obj.isList());
  List list(&scope, *result_obj);
  EXPECT_EQ(list.numItems(), 3);
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 111));
  EXPECT_TRUE(isStrEqualsCStr(list.at(1), "qqq"));
  EXPECT_TRUE(list.at(2).isNoneType());
}

TEST_F(ThreadTest, BuildSetEmpty) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  const byte bytecode[] = {BUILD_SET, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result_obj(&scope, runCode(code));
  ASSERT_TRUE(result_obj.isSet());
  Set result(&scope, *result_obj);
  EXPECT_EQ(result.numItems(), 0);
}

TEST_F(ThreadTest, BuildSetWithOneItem) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, runtime_.newInt(111));
  consts.atPut(1, runtime_.newInt(111));  // dup
  code.setConsts(*consts);

  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,   1,
                           BUILD_SET,  2, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result_obj(&scope, runCode(code));
  ASSERT_TRUE(result_obj.isSet());
  Set result(&scope, *result_obj);
  EXPECT_EQ(result.numItems(), 1);
  Object int_val(&scope, SmallInt::fromWord(111));
  EXPECT_TRUE(runtime_.setIncludes(thread_, result, int_val));
}

TEST_F(ThreadTest, BuildSet) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime_.newTuple(4));

  consts.atPut(0, runtime_.newInt(111));
  consts.atPut(1, runtime_.newInt(111));  // dup
  consts.atPut(2, runtime_.newStrFromCStr("qqq"));
  consts.atPut(3, NoneType::object());

  code.setConsts(*consts);

  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST, 1, LOAD_CONST,   2,
                           LOAD_CONST, 3, BUILD_SET,  4, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result_obj(&scope, runCode(code));
  ASSERT_TRUE(result_obj.isSet());
  Set result(&scope, *result_obj);
  EXPECT_EQ(result.numItems(), 3);
  Object int_val(&scope, runtime_.newInt(111));
  EXPECT_TRUE(runtime_.setIncludes(thread_, result, int_val));
  Object str(&scope, runtime_.newStrFromCStr("qqq"));
  EXPECT_TRUE(runtime_.setIncludes(thread_, result, str));
  Object none(&scope, NoneType::object());
  EXPECT_TRUE(runtime_.setIncludes(thread_, result, none));
}

static RawObject inspect_block(Thread*, Frame* frame, word) ALIGN_16;

// inspect_block is meant to be called raw, without any trampoline. When
// trampolines become mandatory in the future, this function should inspect
// frame->callerFrame instead of directly inspecting the frame.
static RawObject inspect_block(Thread*, Frame* frame, word) {
  // SETUP_LOOP should have pushed an entry onto the block stack.
  TryBlock block = frame->previousFrame()->blockStack()->peek();
  EXPECT_EQ(block.kind(), TryBlock::kLoop);
  EXPECT_EQ(block.handler(), 4 + 6);  // offset after SETUP_LOOP + loop size
  EXPECT_EQ(block.level(), 1);
  return NoneType::object();
}

TEST_F(ThreadTest, SetupLoopAndPopBlock) {
  HandleScope scope(thread_);

  Tuple consts(&scope, runtime_.newTuple(2));
  // inspect_block is meant to be called raw, without any trampoline.
  Str dummy(&scope, runtime_.symbols()->Dummy());
  Function inspect_block_func(
      &scope,
      runtime_.newBuiltinFunction(SymbolId::kDummy, dummy, inspect_block));
  Code::cast(inspect_block_func.code()).setArgcount(0);
  inspect_block_func.setArgcount(0);
  inspect_block_func.setTotalArgs(0);
  consts.atPut(0, *inspect_block_func);
  consts.atPut(1, runtime_.newInt(-55));
  const byte bytecode[] = {LOAD_CONST,    1, SETUP_LOOP, 6, LOAD_CONST, 0,
                           CALL_FUNCTION, 0, POP_TOP,    0, POP_BLOCK,  0,
                           RETURN_VALUE,  0};
  Code code(&scope, newEmptyCode());
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setConsts(*consts);
  code.setStacksize(4);

  EXPECT_TRUE(isIntEqualsWord(runCode(code), -55));
}

TEST_F(ThreadTest, PopJumpIfFalse) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, Bool::trueObj());
  consts.atPut(1, SmallInt::fromWord(1111));
  consts.atPut(2, SmallInt::fromWord(2222));
  code.setConsts(*consts);
  // Bytecode for the snippet:
  //   if x:
  //     return 1111
  //   return 2222
  const byte bytecode[] = {LOAD_CONST, 0, POP_JUMP_IF_FALSE, 8,
                           LOAD_CONST, 1, RETURN_VALUE,      0,
                           LOAD_CONST, 2, RETURN_VALUE,      0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  // Test when the condition evaluates to a truthy value
  EXPECT_TRUE(isIntEqualsWord(runCode(code), 1111));

  // Test when the condition evaluates to a falsey value
  consts.atPut(0, Bool::falseObj());
  EXPECT_TRUE(isIntEqualsWord(runCode(code), 2222));
}

TEST_F(ThreadTest, PopJumpIfTrue) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, Bool::falseObj());
  consts.atPut(1, SmallInt::fromWord(1111));
  consts.atPut(2, SmallInt::fromWord(2222));
  code.setConsts(*consts);
  // Bytecode for the snippet:
  //   if not x:
  //     return 1111
  //   return 2222
  const byte bytecode[] = {LOAD_CONST, 0, POP_JUMP_IF_TRUE, 8,
                           LOAD_CONST, 1, RETURN_VALUE,     0,
                           LOAD_CONST, 2, RETURN_VALUE,     0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  // Test when the condition evaluates to a falsey value
  EXPECT_TRUE(isIntEqualsWord(runCode(code), 1111));

  // Test when the condition evaluates to a truthy value
  consts.atPut(0, Bool::trueObj());
  EXPECT_TRUE(isIntEqualsWord(runCode(code), 2222));
}

TEST_F(ThreadTest, JumpIfFalseOrPop) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, Bool::falseObj());
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, JUMP_IF_FALSE_OR_POP, 6,
                           LOAD_CONST, 1, RETURN_VALUE,         0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  // If the condition is false, we should return the top of the stack, which is
  // the condition itself
  EXPECT_EQ(runCode(code), Bool::falseObj());

  // If the condition is true, we should pop the top of the stack (the
  // condition) and continue execution. In our case that loads a const and
  // returns it.
  consts.atPut(0, Bool::trueObj());
  EXPECT_TRUE(isIntEqualsWord(runCode(code), 1111));
}

TEST_F(ThreadTest, JumpIfTrueOrPop) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, Bool::trueObj());
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, JUMP_IF_TRUE_OR_POP, 6,
                           LOAD_CONST, 1, RETURN_VALUE,        0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  // If the condition is true, we should return the top of the stack, which is
  // the condition itself
  EXPECT_EQ(runCode(code), Bool::trueObj());

  // If the condition is false, we should pop the top of the stack (the
  // condition) and continue execution. In our case that loads a const and
  // returns it.
  consts.atPut(0, Bool::falseObj());
  EXPECT_TRUE(isIntEqualsWord(runCode(code), 1111));
}

TEST_F(ThreadTest, UnaryNot) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, Bool::trueObj());
  code.setConsts(*consts);
  // Bytecode for the snippet:
  //     return not x
  const byte bytecode[] = {LOAD_CONST, 0, UNARY_NOT, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  // If the condition is true, we should return false
  EXPECT_EQ(runCode(code), Bool::falseObj());

  // If the condition is false, we should return true
  consts.atPut(0, Bool::falseObj());
  EXPECT_EQ(runCode(code), Bool::trueObj());
}

static RawDict getMainModuleDict(Runtime* runtime) {
  HandleScope scope;
  Module mod(&scope, findModule(runtime, "__main__"));
  EXPECT_TRUE(mod.isModule());

  Dict dict(&scope, mod.dict());
  EXPECT_TRUE(dict.isDict());
  return *dict;
}

TEST_F(ThreadTest, LoadBuildTypeEmptyType) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  pass
)")
                   .isError());

  Dict dict(&scope, getMainModuleDict(&runtime_));

  Object key(&scope, runtime_.newStrFromCStr("C"));
  Object value(&scope, runtime_.dictAt(thread_, dict, key));
  ASSERT_TRUE(value.isValueCell());

  Type cls(&scope, ValueCell::cast(*value).value());
  ASSERT_TRUE(cls.name().isSmallStr());
  EXPECT_EQ(cls.name(), SmallStr::fromCStr("C"));

  Tuple mro(&scope, cls.mro());
  EXPECT_EQ(mro.length(), 2);
  EXPECT_EQ(mro.at(0), *cls);
  EXPECT_EQ(mro.at(1), runtime_.typeAt(LayoutId::kObject));
}

TEST_F(ThreadTest, LoadBuildTypeTypeWithInit) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self):
    pass
)")
                   .isError());

  Module mod(&scope, findModule(&runtime_, "__main__"));
  ASSERT_TRUE(mod.isModule());

  Dict mod_dict(&scope, mod.dict());
  ASSERT_TRUE(mod_dict.isDict());

  // Check for the class name in the module dict
  Object cls_name(&scope, runtime_.newStrFromCStr("C"));
  Object value(&scope, runtime_.dictAt(thread_, mod_dict, cls_name));
  ASSERT_TRUE(value.isValueCell());
  Type cls(&scope, ValueCell::cast(*value).value());

  // Check class MRO
  Tuple mro(&scope, cls.mro());
  EXPECT_EQ(mro.length(), 2);
  EXPECT_EQ(mro.at(0), *cls);
  EXPECT_EQ(mro.at(1), runtime_.typeAt(LayoutId::kObject));

  // Check class name
  ASSERT_TRUE(cls.name().isSmallStr());
  EXPECT_EQ(cls.name(), SmallStr::fromCStr("C"));

  Dict cls_dict(&scope, cls.dict());
  ASSERT_TRUE(cls_dict.isDict());

  // Check for the __init__ method name in the dict
  Object meth_name(&scope, runtime_.symbols()->DunderInit());
  ASSERT_TRUE(runtime_.dictIncludes(thread_, cls_dict, meth_name));
  value = runtime_.dictAt(thread_, cls_dict, meth_name);
  ASSERT_TRUE(value.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*value).value().isFunction());
}

static RawObject nativeExceptionTest(Thread* thread, Frame*, word) {
  HandleScope scope(thread);
  Str msg(&scope,
          Str::cast(thread->runtime()->newStrFromCStr("test exception")));
  return thread->raise(LayoutId::kRuntimeError, *msg);
}

TEST_F(ThreadTest, NativeExceptions) {
  HandleScope scope(thread_);

  Str name(&scope, runtime_.newStrFromCStr("fn"));
  Function fn(&scope, runtime_.newBuiltinFunction(SymbolId::kDummy, name,
                                                  nativeExceptionTest));
  Code::cast(fn.code()).setArgcount(0);
  fn.setArgcount(0);
  fn.setTotalArgs(0);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, *fn);
  code.setConsts(*consts);

  // Call the native function and check that a pending exception is left in the
  // Thread.
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(1);

  EXPECT_TRUE(raised(runCode(code), LayoutId::kRuntimeError));
  Object value(&scope, thread_->pendingExceptionValue());
  ASSERT_TRUE(value.isStr());
  Str str(&scope, *value);
  EXPECT_TRUE(str.equalsCStr("test exception"));
}

TEST_F(ThreadTest, PendingStopIterationValueInspectsTuple) {
  HandleScope scope(thread_);

  Tuple tuple(&scope, runtime_.newTuple(2));
  tuple.atPut(0, runtime_.newInt(123));
  tuple.atPut(1, runtime_.newInt(456));
  thread_->raise(LayoutId::kStopIteration, *tuple);

  ASSERT_TRUE(thread_->hasPendingStopIteration());
  EXPECT_TRUE(isIntEqualsWord(thread_->pendingStopIterationValue(), 123));
}

// MRO tests

static RawStr className(RawObject obj) {
  HandleScope scope;
  Type cls(&scope, obj);
  Str name(&scope, cls.name());
  return *name;
}

static RawObject getMro(Runtime* runtime, const char* src,
                        const char* desired_class) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  static_cast<void>(runFromCStr(runtime, src).isError());

  Dict mod_dict(&scope, getMainModuleDict(runtime));
  Object class_name(&scope, runtime->newStrFromCStr(desired_class));

  Object value(&scope, runtime->dictAt(thread, mod_dict, class_name));
  Type cls(&scope, ValueCell::cast(*value).value());

  return cls.mro();
}

TEST_F(ThreadTest, LoadBuildTypeVerifyMro) {
  HandleScope scope(thread_);

  const char* src = R"(
class A: pass
class B: pass
class C(A,B): pass
)";

  Tuple mro(&scope, getMro(&runtime_, src, "C"));
  EXPECT_EQ(mro.length(), 4);
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(0)), "C"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(1)), "A"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(2)), "B"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(3)), "object"));
}

TEST_F(ThreadTest, LoadBuildTypeVerifyMroInheritance) {
  HandleScope scope(thread_);

  const char* src = R"(
class A: pass
class B(A): pass
class C(B): pass
)";

  Tuple mro(&scope, getMro(&runtime_, src, "C"));
  EXPECT_EQ(mro.length(), 4);
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(0)), "C"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(1)), "B"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(2)), "A"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(3)), "object"));
}

TEST_F(ThreadTest, LoadBuildTypeVerifyMroMultiInheritance) {
  HandleScope scope(thread_);

  const char* src = R"(
class A: pass
class B(A): pass
class C: pass
class D(B,C): pass
)";

  Tuple mro(&scope, getMro(&runtime_, src, "D"));
  EXPECT_EQ(mro.length(), 5);
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(0)), "D"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(1)), "B"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(2)), "A"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(3)), "C"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(4)), "object"));
}

TEST_F(ThreadTest, LoadBuildTypeVerifyMroDiamond) {
  HandleScope scope(thread_);

  const char* src = R"(
class A: pass
class B(A): pass
class C(A): pass
class D(B,C): pass
)";

  Tuple mro(&scope, getMro(&runtime_, src, "D"));
  EXPECT_EQ(mro.length(), 5);
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(0)), "D"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(1)), "B"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(2)), "C"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(3)), "A"));
  EXPECT_TRUE(isStrEqualsCStr(className(mro.at(4)), "object"));
}

TEST_F(ThreadTest, LoadBuildTypeVerifyMroError) {
  const char* src = R"(
class A: pass
class B(A): pass
class C(A, B): pass
)";

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, src), LayoutId::kTypeError,
      "Cannot create a consistent method resolution order (MRO)"));
}

// iteration

TEST_F(ThreadTest, IteratePrint) {
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

  std::string output = compileAndRunToString(&runtime_, src);
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

TEST_F(ThreadTest, RaiseVarargs) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "raise 1"),
                            LayoutId::kTypeError,
                            "exceptions must derive from BaseException"));
}

TEST_F(ThreadTest, InheritFromObject) {
  HandleScope scope(thread_);
  const char* src = R"(
class Foo(object):
  pass
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  // Look up the class Foo
  Object main_obj(&scope, findModule(&runtime_, "__main__"));
  ASSERT_TRUE(main_obj.isModule());
  Module main(&scope, *main_obj);
  Object foo_obj(&scope, moduleAt(&runtime_, main, "Foo"));
  ASSERT_TRUE(foo_obj.isType());
  Type foo(&scope, *foo_obj);

  // Check that its MRO is itself and object
  ASSERT_TRUE(foo.mro().isTuple());
  Tuple mro(&scope, foo.mro());
  ASSERT_EQ(mro.length(), 2);
  EXPECT_EQ(mro.at(0), *foo);
  EXPECT_EQ(mro.at(1), runtime_.typeAt(LayoutId::kObject));
}

// imports

TEST_F(ThreadTest, ImportTest) {
  HandleScope scope(thread_);

  const char* module_src = R"(
def say_hello():
  print("hello");
)";

  const char* main_src = R"(
import hello
hello.say_hello()
)";

  // Pre-load the hello module so is cached.
  std::unique_ptr<char[]> module_buf(
      Runtime::compileFromCStr(module_src, "<test string>"));
  Object name(&scope, runtime_.newStrFromCStr("hello"));
  ASSERT_FALSE(
      runtime_.importModuleFromBuffer(module_buf.get(), name).isError());

  std::string output = compileAndRunToString(&runtime_, main_src);
  EXPECT_EQ(output, "hello\n");
}

TEST_F(ThreadTest, FailedImportTest) {
  const char* main_src = R"(
import hello
hello.say_hello()
)";

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, main_src),
                            LayoutId::kModuleNotFoundError,
                            "No module named 'hello'"));
}

TEST_F(ThreadTest, ImportMissingAttributeTest) {
  HandleScope scope(thread_);

  const char* module_src = R"(
def say_hello():
  print("hello");
)";

  const char* main_src = R"(
import hello
hello.foo()
)";

  // Pre-load the hello module so is cached.
  std::unique_ptr<char[]> module_buf(
      Runtime::compileFromCStr(module_src, "<test string>"));
  Object name(&scope, runtime_.newStrFromCStr("hello"));
  ASSERT_FALSE(
      runtime_.importModuleFromBuffer(module_buf.get(), name).isError());

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, main_src),
                            LayoutId::kAttributeError,
                            "module 'hello' has no attribute 'foo'"));
}

TEST_F(ThreadTest, ModuleSetAttrTest) {
  HandleScope scope(thread_);

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
  std::unique_ptr<char[]> module_buf(
      Runtime::compileFromCStr(module_src, "<test string>"));
  Object name(&scope, runtime_.newStrFromCStr("hello"));
  ASSERT_FALSE(
      runtime_.importModuleFromBuffer(module_buf.get(), name).isError());

  std::string output = compileAndRunToString(&runtime_, main_src);
  EXPECT_EQ(output, "goodbye\n");
}

TEST_F(ThreadTest, StoreFastStackEffect) {
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
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "1 2 3\n");
}

TEST_F(ThreadTest, SubscriptDict) {
  const char* src = R"(
a = {"1": 2, 2: 3}
print(a["1"])
# exceeds kInitialDictCapacity
b = { 0:0, 1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7, 8:8, 9:9, 10:10, 11:11 }
print(b[11])
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "2\n11\n");

  const char* src1 = R"(
a = {"1": 2, 2: 3}
print(a[1])
)";
  EXPECT_TRUE(raised(runFromCStr(&runtime_, src1), LayoutId::kKeyError));
}

TEST_F(ThreadTest, BuildDictNonLiteralKey) {
  const char* src = R"(
b = "foo"
a = { b: 3, 'c': 4 }
# we need one dict that exceeds kInitialDictCapacity
c = { b: 1, 1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7, 8:8, 9:9, 10:10, 11:11 }
print(a["foo"])
print(a["c"])
print(c[11])
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "3\n4\n11\n");
}

TEST_F(ThreadTest, Closure) {
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
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "1\n2\n3\n");
}

TEST_F(ThreadTest, UnpackSequence) {
  const char* src = R"(
a, b = (1, 2)
print(a, b)
a, b = [3, 4]
print(a, b)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "1 2\n3 4\n");
}

TEST_F(ThreadTest, BinaryTrueDivide) {
  const char* src = R"(
a = 6
b = 2
print(a / b)
a = 5
b = 2
print(a / b)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "3\n2.5\n");
}

TEST_F(FormatTest, NoConvEmpty) {
  const char* src = R"(
print(f'')
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "\n");
}

TEST_F(FormatTest, NoConvOneElement) {
  const char* src = R"(
a = "hello"
x = f'a={a}'
print(x)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "a=hello\n");
}

TEST_F(FormatTest, NoConvMultiElements) {
  const char* src = R"(
a = "hello"
b = "world"
c = "python"
x = f'{a} {b} {c}'
print(x)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "hello world python\n");
}

TEST_F(FormatTest, NoConvMultiElementsLarge) {
  const char* src = R"(
a = "Python"
b = "is"
c = "an interpreted high-level programming language for general-purpose programming.";
x = f'{a} {b} {c}'
print(x)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output,
            "Python is an interpreted high-level programming language for "
            "general-purpose programming.\n");
}

TEST_F(ThreadTest, BuildTupleUnpack) {
  HandleScope scope(thread_);
  const char* src = R"(
t = (*[0], *[1, 2], *[], *[3, 4, 5])
t1 = (*(0,), *(1, 2), *(), *(3, 4, 5))
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));

  Object t(&scope, moduleAt(&runtime_, main, "t"));
  EXPECT_TRUE(t.isTuple());
  Tuple tuple_t(&scope, *t);
  EXPECT_EQ(tuple_t.length(), 6);
  for (word i = 0; i < tuple_t.length(); i++) {
    EXPECT_TRUE(isIntEqualsWord(tuple_t.at(i), i));
  }

  Object t1(&scope, moduleAt(&runtime_, main, "t1"));
  EXPECT_TRUE(t1.isTuple());
  Tuple tuple_t1(&scope, *t1);
  EXPECT_EQ(tuple_t1.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(tuple_t1.at(0), 0));
  EXPECT_TRUE(isIntEqualsWord(tuple_t1.at(1), 1));
  EXPECT_TRUE(isIntEqualsWord(tuple_t1.at(2), 2));
  EXPECT_TRUE(isIntEqualsWord(tuple_t1.at(3), 3));
  EXPECT_TRUE(isIntEqualsWord(tuple_t1.at(4), 4));
  EXPECT_TRUE(isIntEqualsWord(tuple_t1.at(5), 5));
}

TEST_F(ThreadTest, BuildListUnpack) {
  HandleScope scope(thread_);
  const char* src = R"(
l = [*[0], *[1, 2], *[], *[3, 4, 5]]
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));

  Object l(&scope, moduleAt(&runtime_, main, "l"));
  EXPECT_TRUE(l.isList());
  List list_l(&scope, *l);
  EXPECT_EQ(list_l.numItems(), 6);
  EXPECT_TRUE(isIntEqualsWord(list_l.at(0), 0));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(1), 1));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(2), 2));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(3), 3));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(4), 4));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(5), 5));
}

TEST_F(ThreadTest, BuildSetUnpack) {
  HandleScope scope(thread_);
  const char* src = R"(
s = {*[0, 1], *{2, 3}, *(4, 5), *[]}
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));

  Object s(&scope, moduleAt(&runtime_, main, "s"));
  EXPECT_TRUE(s.isSet());
  Set set_s(&scope, *s);
  EXPECT_EQ(set_s.numItems(), 6);
  Object small_int(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(runtime_.setIncludes(thread_, set_s, small_int));
  small_int = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime_.setIncludes(thread_, set_s, small_int));
  small_int = SmallInt::fromWord(2);
  EXPECT_TRUE(runtime_.setIncludes(thread_, set_s, small_int));
  small_int = SmallInt::fromWord(3);
  EXPECT_TRUE(runtime_.setIncludes(thread_, set_s, small_int));
  small_int = SmallInt::fromWord(4);
  EXPECT_TRUE(runtime_.setIncludes(thread_, set_s, small_int));
  small_int = SmallInt::fromWord(5);
  EXPECT_TRUE(runtime_.setIncludes(thread_, set_s, small_int));
}

TEST_F(BuildString, buildStringEmpty) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());

  const byte bytecode[] = {BUILD_STRING, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(isStrEqualsCStr(runCode(code), ""));
}

TEST_F(BuildString, buildStringSingle) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime_.newTuple(1));
  Object str(&scope, SmallStr::fromCStr("foo"));
  consts.atPut(0, *str);
  code.setConsts(*consts);

  const byte bytecode[] = {LOAD_CONST, 0, BUILD_STRING, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(isStrEqualsCStr(runCode(code), "foo"));
}

TEST_F(BuildString, buildStringMultiSmall) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime_.newTuple(2));
  Object str(&scope, SmallStr::fromCStr("foo"));
  Object str1(&scope, SmallStr::fromCStr("bar"));
  consts.atPut(0, *str);
  consts.atPut(1, *str1);
  code.setConsts(*consts);

  const byte bytecode[] = {LOAD_CONST,   0, LOAD_CONST,   1,
                           BUILD_STRING, 2, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(isStrEqualsCStr(runCode(code), "foobar"));
}

TEST_F(BuildString, buildStringMultiLarge) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime_.newTuple(3));
  Object str(&scope, SmallStr::fromCStr("hello"));
  Object str1(&scope, SmallStr::fromCStr("world"));
  Object str2(&scope, SmallStr::fromCStr("python"));
  consts.atPut(0, *str);
  consts.atPut(1, *str1);
  consts.atPut(2, *str2);
  code.setConsts(*consts);

  const byte bytecode[] = {LOAD_CONST,   0, LOAD_CONST,   1, LOAD_CONST, 2,
                           BUILD_STRING, 3, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(isStrEqualsCStr(runCode(code), "helloworldpython"));
}

TEST_F(UnpackSeq, unpackRangePyStone) {
  const char* src = R"(
[Ident1, Ident2, Ident3, Ident4, Ident5] = range(1, 6)
print(Ident1, Ident2, Ident3, Ident4, Ident5)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "1 2 3 4 5\n");
}

TEST_F(UnpackSeq, unpackRange) {
  const char* src = R"(
[a ,b, c] = range(2, 5)
print(a, b, c)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "2 3 4\n");
}

// LIST_APPEND(listAdd) in list_comp, followed by unpack
// TODO(rkng): list support in BINARY_ADD
TEST_F(UnpackList, unpackListCompAppend) {
  const char* src = R"(
a = [1, 2, 3]
b = [x for x in a]
b1, b2, b3 = b
print(b1, b2, b3)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "1 2 3\n");
}

TEST_F(ThreadTest, SetAdd) {
  HandleScope scope(thread_);
  const char* src = R"(
a = [1, 2, 3]
b = {x for x in a}
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object b(&scope, moduleAt(&runtime_, main, "b"));
  ASSERT_TRUE(b.isSet());
  Set set_b(&scope, *b);
  EXPECT_EQ(set_b.numItems(), 3);
}

TEST_F(ThreadTest, MapAdd) {
  HandleScope scope(thread_);
  const char* src = R"(
a = ['a', 'b', 'c']
b = {x:x for x in a}
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object b(&scope, moduleAt(&runtime_, main, "b"));
  EXPECT_EQ(b.isDict(), true);
  Dict dict_b(&scope, *b);
  EXPECT_EQ(dict_b.numItems(), 3);
}

TEST_F(UnpackList, unpackNestedLists) {
  const char* src = R"(
b = [[1,2], [3,4,5]]
b1, b2 = b
b11, b12 = b1
b21, b22, b23 = b2
print(len(b), len(b1), len(b2), b11, b12, b21, b22, b23)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "2 2 3 1 2 3 4 5\n");
}

TEST_F(UnpackSeq, unpackRangeStep) {
  const char* src = R"(
[a ,b, c, d] = range(2, 10, 2)
print(a, b, c, d)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "2 4 6 8\n");
}

TEST_F(UnpackSeq, unpackRangeNeg) {
  const char* src = R"(
[a ,b, c, d, e] = range(-10, 0, 2)
print(a, b, c, d, e)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "-10 -8 -6 -4 -2\n");
}

TEST_F(ListIterTest, build) {
  const char* src = R"(
a = [1, 2, 3]
for x in a:
  print(x)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "1\n2\n3\n");
}

TEST_F(ListAppendTest, buildAndUnpack) {
  const char* src = R"(
a = [1, 2]
b = [x for x in [a] * 3]
b1, b2, b3 = b
b11, b12 = b1
print(len(b), len(b1), b11, b12)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "3 2 1 2\n");
}

TEST_F(ListInsertTest, InsertToList) {
  HandleScope scope(thread_);
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
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object a(&scope, moduleAt(&runtime_, main, "a"));
  Object b(&scope, moduleAt(&runtime_, main, "b"));
  Object l(&scope, moduleAt(&runtime_, main, "l"));
  Object s(&scope, moduleAt(&runtime_, main, "s"));

  EXPECT_FALSE(isIntEqualsWord(*a, 2));
  EXPECT_FALSE(isIntEqualsWord(*b, 12));

  List list_l(&scope, *l);
  ASSERT_EQ(list_l.numItems(), 16);
  EXPECT_TRUE(isIntEqualsWord(list_l.at(2), 2));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(12), 12));

  // sum(0..16) = 120
  EXPECT_TRUE(isIntEqualsWord(*s, 120));
}

TEST_F(ListInsertTest, InsertToListBounds) {
  HandleScope scope(thread_);
  const char* src = R"(
l = [x for x in range(1, 5)]
l.insert(100, 5)
l.insert(400, 6)
l.insert(-100, 0)
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object l(&scope, moduleAt(&runtime_, main, "l"));
  List list_l(&scope, *l);
  EXPECT_TRUE(isIntEqualsWord(list_l.at(0), 0));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(1), 1));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(2), 2));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(3), 3));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(4), 4));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(5), 5));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(6), 6));
}

TEST_F(ListInsertTest, InsertToNegativeIndex) {
  HandleScope scope(thread_);
  const char* src = R"(
l = [0, 2, 4]
l.insert(-2, 1)
l.insert(-1, 3)
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object l(&scope, moduleAt(&runtime_, main, "l"));
  List list_l(&scope, *l);
  ASSERT_EQ(list_l.numItems(), 5);
  EXPECT_TRUE(isIntEqualsWord(list_l.at(0), 0));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(1), 1));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(2), 2));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(3), 3));
  EXPECT_TRUE(isIntEqualsWord(list_l.at(4), 4));
}

TEST_F(ThreadTest, BaseTypeConflict) {
  const char* src = R"(
class Foo(list, dict): pass
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kTypeError,
                            "multiple bases have instance lay-out conflict"));
}

TEST_F(BuildSlice, noneSliceCopyList) {
  const char* src = R"(
a = [1, 2, 3]
b = a[:]
print(len(b), b[0], b[1], b[2])
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "3 1 2 3\n");
}

TEST_F(BuildSlice, sliceOperations) {
  const char* src = R"(
a = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
b = a[1:2:3]
print(len(b), b[0])
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "1 2\n");

  const char* src2 = R"(
a = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
b = a[1::3]
print(len(b), b[0], b[1], b[2])
)";
  std::string output2 = compileAndRunToString(&runtime_, src2);
  EXPECT_EQ(output2, "3 2 5 8\n");

  const char* src3 = R"(
a = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
b = a[8:2:-2]
print(len(b), b[0], b[1], b[2])
)";
  std::string output3 = compileAndRunToString(&runtime_, src3);
  ASSERT_EQ(output3, "3 9 7 5\n");

  const char* src4 = R"(
a = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
b = a[8:2:2]
print(len(b))
)";
  std::string output4 = compileAndRunToString(&runtime_, src4);
  EXPECT_EQ(output4, "0\n");
}

TEST_F(BuildSlice, noneSliceCopyListComp) {  // pystone
  const char* src = R"(
a = [1, 2, 3]
b = [x[:] for x in [a] * 2]
c = b is a
b1, b2 = b
b11, b12, b13 = b1
print(c, len(b), len(b1), b11, b12, b13)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "False 2 3 1 2 3\n");
}

TEST_F(ThreadTest, SliceNoneCopyListCompPrint) {  // based on pystone.py
  const char* src = R"(
Array1Glob = [0]*5
Array2Glob = [x[:] for x in [Array1Glob]*5]
print(len(Array1Glob), len(Array2Glob), len(Array2Glob[0]), Array1Glob, Array2Glob[0])
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "5 5 5 [0, 0, 0, 0, 0] [0, 0, 0, 0, 0]\n");
}

TEST_F(ThreadTest, BreakLoopWhileLoop) {
  const char* src = R"(
a = 0
while 1:
    a = a + 1
    print(a)
    if a == 3:
        break
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "1\n2\n3\n");
}

TEST_F(ThreadTest, BreakLoopWhileLoop1) {
  const char* src = R"(
a = 0
while 1:
    a = a + 1
    print(a)
    if a == 3:
        break
print("ok",a)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "1\n2\n3\nok 3\n");
}

TEST_F(ThreadTest, BreakLoopWhileLoopBytecode) {
  HandleScope scope(thread_);

  Tuple consts(&scope, runtime_.newTuple(4));
  Code code(&scope, newEmptyCode());
  consts.atPut(0, SmallInt::fromWord(0));
  consts.atPut(1, SmallInt::fromWord(1));
  consts.atPut(2, SmallInt::fromWord(3));
  consts.atPut(3, NoneType::object());
  code.setConsts(*consts);

  Tuple names(&scope, runtime_.newTuple(1));
  Object key(&scope, runtime_.newStrFromCStr("a"));
  names.atPut(0, *key);
  code.setNames(*names);

  // see python code in BreakLoop.whileLoop (sans print)
  const byte bytecode[] = {LOAD_CONST,        0,                  // 0
                           STORE_NAME,        0,                  // a
                           SETUP_LOOP,        22, LOAD_NAME,  0,  // a
                           LOAD_CONST,        1,                  // 1
                           BINARY_ADD,        0,  STORE_NAME, 0,  // a
                           LOAD_NAME,         0,                  // a
                           LOAD_CONST,        2,                  // 3
                           COMPARE_OP,        2,                  // ==
                           POP_JUMP_IF_FALSE, 6,  BREAK_LOOP, 0,
                           JUMP_ABSOLUTE,     6,  POP_BLOCK,  0,
                           LOAD_CONST,        3,  // None
                           RETURN_VALUE,      0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::NOFREE);

  Dict globals(&scope, runtime_.newDict());
  Dict locals(&scope, runtime_.newDict());
  ASSERT_TRUE(thread_->exec(code, globals, locals).isNoneType());
  EXPECT_TRUE(isIntEqualsWord(runtime_.moduleDictAt(thread_, locals, key), 3));
}

TEST_F(ThreadTest, BreakLoopRangeLoop) {
  const char* src = R"(
for x in range(1,6):
  if x == 3:
    break;
  print(x)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "1\n2\n");
}

TEST_F(ThreadTest, ContinueLoopRangeLoop) {
  HandleScope scope(thread_);
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
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object l(&scope, moduleAt(&runtime_, main, "l"));
  EXPECT_TRUE(l.isList());
  List list_l(&scope, *l);
  ASSERT_GE(list_l.numItems(), 3);
  EXPECT_EQ(list_l.at(0), SmallInt::fromWord(0));
  EXPECT_EQ(list_l.at(1), SmallInt::fromWord(1));
  EXPECT_EQ(list_l.at(2), SmallInt::fromWord(2));
}

TEST_F(ThreadTest, ContinueLoopRangeLoopByteCode) {
  HandleScope scope(thread_);

  Tuple consts(&scope, runtime_.newTuple(5));
  Code code(&scope, newEmptyCode());
  consts.atPut(0, SmallInt::fromWord(0));
  consts.atPut(1, SmallInt::fromWord(4));
  consts.atPut(2, SmallInt::fromWord(1));
  consts.atPut(3, SmallInt::fromWord(3));
  consts.atPut(4, NoneType::object());
  code.setConsts(*consts);
  code.setArgcount(0);
  code.setNlocals(2);

  Tuple names(&scope, runtime_.newTuple(2));
  Object key0(&scope, runtime_.newStrFromCStr("cnt"));
  Object key1(&scope, runtime_.newStrFromCStr("s"));
  names.atPut(0, *key0);
  names.atPut(1, *key1);
  code.setNames(*names);

  //  # python code:
  //  cnt = 0
  //  s = 0
  //  while cnt < 4:
  //      cnt += 1
  //      if cnt == 3:
  //          continue
  //      s += cnt
  //  return s
  const byte bytecode[] = {LOAD_CONST,        0,  // 0
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

  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 7));
}

TEST_F(ThreadTest, Func2TestPyStone) {  // mimic pystone.py Func2
  const char* src = R"(
def f1(x, y):
  return x + y
def f2():
  return f1(1, 2)
print(f2())
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "3\n");
}

TEST_F(ThreadTest, BinSubscrString) {  // pystone dependency
  const char* src = R"(
a = 'Hello'
print(a[0],a[1],a[2],a[3],a[4])
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "H e l l o\n");
}

TEST_F(ThreadTest, SetupExceptNoOp) {  // pystone dependency
  const char* src = R"(
def f(x):
  try: print(x)
  except ValueError:
    print("Invalid Argument")
f(100)
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "100\n");
}

TEST_F(ThreadTest, StrFormatEmpty) {
  const char* src = R"(
print("" % ("hi"))
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "\n");
}

TEST_F(ThreadTest, StrFormatNone) {
  const char* src = R"(
h = "hello"
p = "pyro pystone"
print("hello world" % (h, p))
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "hello world\n");
}

TEST_F(ThreadTest, StrFormatMod) {
  const char* src = R"(
print("%%%s" % (""))
)";
  const std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "%\n");
}

TEST_F(ThreadTest, StrFormatNeg1) {
  const char* src = R"(
h = "hi"
print("%" % (h, "world"))
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kValueError,
                            "Incomplete format"));
}

TEST_F(ThreadTest, StrFormatStr) {
  const char* src = R"(
h = "hello pyro"
print("%s" % (h))
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "hello pyro\n");
}

TEST_F(ThreadTest, StrFormatStr2) {
  const char* src = R"(
h = "hello"
p = "pyro"
print("%s%s" % (h, p))
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "hellopyro\n");
}

TEST_F(ThreadTest, StrFormatFloat) {
  const char* src = R"(
d = 67.89
print("There are %g pystones" % (d))
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "There are 67.89 pystones\n");
}

TEST_F(ThreadTest, StrFormatMixed) {
  const char* src = R"(
a = 123
d = 67.89
print("There are %d pystones %g" % (a, d))
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "There are 123 pystones 67.89\n");
}

TEST_F(ThreadTest, StrFormatMixed2) {
  const char* src = R"(
a = 123
d = 67.89
c = "now"
print("There are %d pystones %g %s" % (a, d, c))
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "There are 123 pystones 67.89 now\n");
}

TEST_F(ThreadTest, StrFormatMixed3) {
  const char* src = R"(
a = 123
d = 67.89
c = "now"
print("There are %d pystones %g %s what" % (a, d, c))
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "There are 123 pystones 67.89 now what\n");
}

TEST_F(ThreadTest, BuildTypeWithMetaclass) {
  HandleScope scope(thread_);
  const char* src = R"(
class Foo(metaclass=type):
  pass
a = Foo()
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object foo(&scope, moduleAt(&runtime_, main, "Foo"));
  EXPECT_TRUE(foo.isType());
  Object a(&scope, moduleAt(&runtime_, main, "a"));
  EXPECT_TRUE(runtime_.typeOf(*a) == *foo);
}

TEST_F(ThreadTest, BuildTypeWithMetaclass2) {
  HandleScope scope(thread_);
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
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object bar(&scope, moduleAt(&runtime_, main, "Bar"));
  EXPECT_TRUE(runtime_.isInstanceOfType(*bar));
  Object a(&scope, moduleAt(&runtime_, main, "a"));
  EXPECT_TRUE(runtime_.typeOf(*a) == *bar);
  Object b(&scope, moduleAt(&runtime_, main, "b"));
  EXPECT_TRUE(isIntEqualsWord(*b, 123));
  Object c(&scope, moduleAt(&runtime_, main, "c"));
  EXPECT_TRUE(isIntEqualsWord(*c, 456));
}

TEST_F(ThreadTest, NameLookupInTypeBodyFindsImplicitGlobal) {
  HandleScope scope(thread_);
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
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object a(&scope, moduleAt(&runtime_, main, "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 3));
  Object b(&scope, moduleAt(&runtime_, main, "b"));
  EXPECT_TRUE(isIntEqualsWord(*b, 6));
}

TEST_F(ThreadTest, NameLookupInTypeBodyFindsGlobal) {
  HandleScope scope(thread_);
  const char* src = R"(
var = 1
class C:
  global one
  global two
  one = var
  var = 2
  two = var
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object one(&scope, moduleAt(&runtime_, main, "one"));
  EXPECT_TRUE(isIntEqualsWord(*one, 1));
  Object two(&scope, moduleAt(&runtime_, main, "two"));
  EXPECT_TRUE(isIntEqualsWord(*two, 2));
}

TEST_F(ThreadTest, ExecuteDeleteName) {
  HandleScope scope(thread_);
  const char* src = R"(
var = 1
del var
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object var(&scope, moduleAt(&runtime_, main, "var"));
  EXPECT_TRUE(var.isError());
}

TEST_F(ThreadTest, SetupFinally) {
  HandleScope scope(thread_);
  const char* src = R"(
x = 1
try:
  pass
finally:
  x = 2
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object x(&scope, moduleAt(&runtime_, main, "x"));
  EXPECT_EQ(*x, SmallInt::fromWord(2));
}

TEST_F(ThreadTest, SetupAnnotationsAndStoreAnnotations) {
  HandleScope scope(thread_);
  const char* src = R"(
x: int = 1
class Foo:
  bar: int = 2
class_anno_dict = Foo.__annotations__
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Dict module_anno_dict(&scope, moduleAt(&runtime_, main, "__annotations__"));
  Object m_key(&scope, runtime_.newStrFromCStr("x"));
  Object m_value(&scope, runtime_.dictAt(thread_, module_anno_dict, m_key));
  EXPECT_EQ(*m_value, runtime_.typeAt(LayoutId::kInt));

  Dict class_anno_dict(&scope, moduleAt(&runtime_, main, "class_anno_dict"));
  Object c_key(&scope, runtime_.newStrFromCStr("bar"));
  Object c_value(&scope, runtime_.dictAt(thread_, class_anno_dict, c_key));
  EXPECT_EQ(*c_value, runtime_.typeAt(LayoutId::kInt));
}

TEST_F(ThreadTest, DeleteFastRaisesUnboundLocalError) {
  const char* src = R"(
def foo(a, b, c):
  del a
  return a
foo(1, 2, 3)
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src),
                            LayoutId::kUnboundLocalError,
                            "local variable 'a' referenced before assignment"));
}

TEST_F(ThreadTest, DeleteFast) {
  HandleScope scope(thread_);
  const char* src = R"(
def foo(a, b, c):
  del a
  return b
x = foo(1, 2, 3)
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object x(&scope, moduleAt(&runtime_, main, "x"));
  EXPECT_EQ(*x, SmallInt::fromWord(2));
}

TEST_F(ThreadTest, ConstructInstanceWithKwargs) {
  HandleScope scope(thread_);
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
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));

  Object result_a(&scope, moduleAt(&runtime_, main, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 1111));

  Object result_b(&scope, moduleAt(&runtime_, main, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 2222));

  Object result_c(&scope, moduleAt(&runtime_, main, "result_c"));
  EXPECT_TRUE(isIntEqualsWord(*result_c, 3333));
}

TEST_F(ThreadTest, LoadTypeDeref) {
  HandleScope scope(thread_);
  const char* src = R"(
def foo():
  a = 1
  class Foo:
    b = a
  return Foo.b
x = foo()
)";
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object x(&scope, moduleAt(&runtime_, main, "x"));
  EXPECT_EQ(*x, SmallInt::fromWord(1));
}

TEST_F(ThreadTest, LoadTypeDerefFromLocal) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(1111));
  Tuple freevars(&scope, runtime_.newTuple(1));
  freevars.atPut(0, SmallStr::fromCStr("lalala"));
  Tuple names(&scope, runtime_.newTuple(1));
  names.atPut(0, SmallStr::fromCStr("lalala"));
  code.setConsts(*consts);
  code.setNames(*names);
  code.setFreevars(*freevars);
  const byte bytecode[] = {LOAD_CONST,      0, STORE_NAME,   0,
                           LOAD_CLASSDEREF, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(2);
  code.setFlags(Code::Flags::NOFREE);

  Dict globals(&scope, runtime_.newDict());
  Dict locals(&scope, runtime_.newDict());
  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, globals, locals), 1111));
}

TEST_F(ThreadTest, PushCallFrameWithSameGlobalsPropagatesBuiltins) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  code.setCode(Bytes::empty());
  code.setNames(runtime_.newTuple(0));

  Object qualname(&scope, runtime_.newStrFromCStr("<anonymous>"));
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime_.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread_, qualname, code, none, none,
                                        none, none, globals));

  Frame* frame = thread_->currentFrame();
  frame->pushValue(*function);
  Frame* new_frame = thread_->pushCallFrame(*function);
  EXPECT_NE(new_frame, frame);
}

TEST_F(ThreadTest, ExecSetsMissingDunderBuiltins) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, NoneType::object());
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::NOFREE);
  Dict globals(&scope, runtime_.newDict());

  thread_->exec(code, globals, globals);

  Object builtins_module(&scope, runtime_.findModuleById(SymbolId::kBuiltins));
  Str dunder_builtins_name(&scope, runtime_.symbols()->DunderBuiltins());
  EXPECT_EQ(runtime_.moduleDictAt(thread_, globals, dunder_builtins_name),
            builtins_module);
}

TEST_F(ThreadTest, CallFunctionInDifferentModule) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def a():
  def inner_a():
    return object
  return inner_a
result0 = a()()
)")
                   .isError());
  // Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Object result0(&scope, moduleAt(&runtime_, "__main__", "result0"));
  Object object(&scope, moduleAt(&runtime_, "builtins", "object"));
  EXPECT_EQ(result0, object);
}

TEST_F(ThreadTest, RaiseWithFmtFormatsString) {
  EXPECT_TRUE(raisedWithStr(
      thread_->raiseWithFmt(LayoutId::kTypeError, "hello %Y", SymbolId::kDict),
      LayoutId::kTypeError, "hello dict"));
}

}  // namespace python
