#include "thread.h"

#include <memory>

#include "gtest/gtest.h"

#include "builtins-module.h"
#include "bytecode.h"
#include "dict-builtins.h"
#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "marshal.h"
#include "module-builtins.h"
#include "modules.h"
#include "runtime.h"
#include "test-utils.h"
#include "type-builtins.h"

namespace py {
namespace testing {

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
  ASSERT_EQ(thread_->runtime(), runtime_);
}

TEST_F(ThreadTest, RunEmptyFunction) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object none(&scope, NoneType::object());
  Tuple consts(&scope, runtime_->newTupleWith1(none));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Thread thread2(runtime_, 1 * kKiB);
  runtime_->interpreter()->setupThread(&thread2);
  Thread::setCurrentThread(&thread2);
  EXPECT_TRUE(runCode(code).isNoneType());

  // Avoid assertion in runtime destructor.
  Thread::setCurrentThread(thread_);
}

TEST_F(ThreadTest, ModuleBodyCallsHelloWorldFunction) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def hello():
  return 'hello, world'
result = hello()
)")
                   .isError());
  EXPECT_TRUE(
      isStrEqualsCStr(mainModuleAt(runtime_, "result"), "hello, world"));
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

  ASSERT_FALSE(runFromCStr(runtime_, src).isError());

  Object global(&scope, mainModuleAt(runtime_, "g"));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());

  Object result(&scope, mainModuleAt(runtime_, "result"));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());

  Object result(&scope, mainModuleAt(runtime_, "result"));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());

  Object result(&scope, mainModuleAt(runtime_, "result"));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());

  Object result_x(&scope, mainModuleAt(runtime_, "result_x"));
  EXPECT_TRUE(isIntEqualsWord(*result_x, 1));
  Object result_y(&scope, mainModuleAt(runtime_, "result_y"));
  EXPECT_TRUE(isIntEqualsWord(*result_y, 3));
}

TEST_F(ThreadTest, OverlappingFrames) {
  HandleScope scope(thread_);

  // Push a frame for a code object with space for 3 items on the value stack
  Object name(&scope, Str::empty());
  Code caller_code(&scope, newEmptyCode());
  caller_code.setCode(Bytes::empty());
  caller_code.setStacksize(3);

  Module module(&scope, findMainModule(runtime_));
  Function caller(&scope, runtime_->newFunctionWithCode(thread_, name,
                                                        caller_code, module));

  thread_->stackPush(*caller);
  thread_->pushCallFrame(*caller);

  // Push a frame for a code object that expects 3 arguments and needs space
  // for 3 local variables
  Code code(&scope, newEmptyCode());
  code.setCode(Bytes::empty());
  code.setArgcount(3);
  code.setNlocals(3);

  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));

  // Push args on the stack in the sequence generated by CPython
  thread_->stackPush(*function);
  thread_->stackPush(runtime_->newInt(1111));
  thread_->stackPush(runtime_->newInt(2222));
  thread_->stackPush(runtime_->newInt(3333));
  Frame* frame = thread_->pushCallFrame(*function);

  // Make sure we can read the args from the frame
  EXPECT_TRUE(isIntEqualsWord(frame->local(0), 1111));
  EXPECT_TRUE(isIntEqualsWord(frame->local(1), 2222));
  EXPECT_TRUE(isIntEqualsWord(frame->local(2), 3333));
}

TEST_F(ThreadTest, EncodeTryBlock) {
  TryBlock block(TryBlock::kExceptHandler, 200, 300);
  EXPECT_EQ(block.kind(), TryBlock::kExceptHandler);
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
  Module module(&scope, findMainModule(runtime_));
  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));

  RawObject* prev_sp = thread_->stackPointer();
  thread_->stackPush(*function);
  Frame* frame = thread_->pushCallFrame(*function);

  // Verify frame invariants post-push
  EXPECT_TRUE(frame->previousFrame()->isSentinel());
  EXPECT_EQ(thread_->stackPointer(), reinterpret_cast<RawObject*>(frame));
  EXPECT_EQ(thread_->valueStackBase(), thread_->stackPointer());

  // Make sure we restore the thread's stack pointer back to its previous
  // location
  thread_->popFrame();
  EXPECT_EQ(thread_->stackPointer(), prev_sp);
}

TEST_F(ThreadTest, PushFrameWithNoCellVars) {
  HandleScope scope(thread_);

  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  code.setCode(Bytes::empty());
  code.setCellvars(NoneType::object());
  code.setFreevars(runtime_->emptyTuple());
  Module module(&scope, findMainModule(runtime_));
  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));
  thread_->stackPush(*function);
  byte* prev_sp = reinterpret_cast<byte*>(thread_->stackPointer());
  thread_->pushCallFrame(*function);

  EXPECT_EQ(reinterpret_cast<byte*>(thread_->stackPointer()),
            prev_sp - Frame::kSize);
}

TEST_F(ThreadTest, PushFrameWithNoFreeVars) {
  HandleScope scope(thread_);

  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  code.setCode(Bytes::empty());
  code.setFreevars(NoneType::object());
  code.setCellvars(runtime_->emptyTuple());
  Module module(&scope, findMainModule(runtime_));
  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));
  thread_->stackPush(*function);
  byte* prev_sp = reinterpret_cast<byte*>(thread_->stackPointer());
  thread_->pushCallFrame(*function);

  EXPECT_EQ(reinterpret_cast<byte*>(thread_->stackPointer()),
            prev_sp - Frame::kSize);
}

TEST_F(ThreadTest, ManipulateValueStack) {
  // Push 3 items on the value stack
  RawObject* sp = thread_->stackPointer();
  *--sp = SmallInt::fromWord(1111);
  *--sp = SmallInt::fromWord(2222);
  *--sp = SmallInt::fromWord(3333);
  thread_->setStackPointer(sp);
  ASSERT_EQ(thread_->stackPointer(), sp);

  // Verify the value stack is laid out as we expect
  EXPECT_TRUE(isIntEqualsWord(thread_->stackPeek(0), 3333));
  EXPECT_TRUE(isIntEqualsWord(thread_->stackPeek(1), 2222));
  EXPECT_TRUE(isIntEqualsWord(thread_->stackPeek(2), 1111));

  // Pop 2 items off the stack and check the stack is still as we expect
  thread_->setStackPointer(sp + 2);
  EXPECT_TRUE(isIntEqualsWord(thread_->stackPeek(0), 1111));
}

TEST_F(ThreadTest, ManipulateBlockStack) {
  Frame* frame = thread_->currentFrame();

  TryBlock pushed1(TryBlock::kFinally, 100, 10);
  frame->blockStackPush(pushed1);

  TryBlock pushed2(TryBlock::kExceptHandler, 200, 20);
  frame->blockStackPush(pushed2);

  TryBlock popped2 = frame->blockStackPop();
  EXPECT_EQ(popped2.kind(), pushed2.kind());
  EXPECT_EQ(popped2.handler(), pushed2.handler());
  EXPECT_EQ(popped2.level(), pushed2.level());

  TryBlock popped1 = frame->blockStackPop();
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
  Object obj(&scope, runtime_->newInt(2222));
  callee_code.setConsts(runtime_->newTupleWith1(obj));
  const byte callee_bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  callee_code.setCode(runtime_->newBytesWithAll(callee_bytecode));

  // Create the function object and bind it to the code object
  Object qualname(&scope, Str::empty());
  Module module(&scope, findMainModule(runtime_));
  Function callee(&scope, runtime_->newFunctionWithCode(thread_, qualname,
                                                        callee_code, module));

  // Build a code object to call the function defined above
  Code caller_code(&scope, newEmptyCode());
  caller_code.setStacksize(3);
  Object obj1(&scope, SmallInt::fromWord(1111));
  Object obj2(&scope, SmallInt::fromWord(2222));
  caller_code.setConsts(runtime_->newTupleWith3(callee, obj1, obj2));
  const byte caller_bytecode[] = {LOAD_CONST,   0, LOAD_CONST,    1,
                                  LOAD_CONST,   2, CALL_FUNCTION, 2,
                                  RETURN_VALUE, 0};
  caller_code.setCode(runtime_->newBytesWithAll(caller_bytecode));

  // Execute the caller and make sure we get back the expected result
  EXPECT_TRUE(isIntEqualsWord(runCode(caller_code), 2222));
}

TEST_F(ThreadTest, ExtendedArg) {
  HandleScope scope(thread_);

  const word num_consts = 258;
  const byte bytecode[] = {EXTENDED_ARG, 1, LOAD_CONST, 1, RETURN_VALUE, 0};

  MutableTuple constants(&scope, runtime_->newMutableTuple(num_consts));

  auto zero = SmallInt::fromWord(0);
  auto non_zero = SmallInt::fromWord(0xDEADBEEF);
  for (word i = 0; i < num_consts - 2; i++) {
    constants.atPut(i, zero);
  }
  constants.atPut(num_consts - 1, non_zero);
  Code code(&scope, newEmptyCode());
  code.setConsts(constants.becomeImmutable());
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(2);

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 0xDEADBEEF));
}

TEST_F(ThreadTest, ExecuteDupTop) {
  HandleScope scope(thread_);

  Object obj(&scope, SmallInt::fromWord(1111));
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  Code code(&scope, newEmptyCode());
  code.setStacksize(2);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, DUP_TOP, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 1111));
}

TEST_F(ThreadTest, ExecuteDupTopTwo) {
  HandleScope scope(thread_);

  Object obj1(&scope, SmallInt::fromWord(1111));
  Object obj2(&scope, SmallInt::fromWord(2222));
  Tuple consts(&scope, runtime_->newTupleWith2(obj1, obj2));
  Code code(&scope, newEmptyCode());
  code.setStacksize(2);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,  0, LOAD_CONST,   1,
                           DUP_TOP_TWO, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 2222));
}

TEST_F(ThreadTest, ExecuteRotTwo) {
  HandleScope scope(thread_);

  Object obj1(&scope, SmallInt::fromWord(1111));
  Object obj2(&scope, SmallInt::fromWord(2222));
  Tuple consts(&scope, runtime_->newTupleWith2(obj1, obj2));
  Code code(&scope, newEmptyCode());
  code.setStacksize(2);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,   1,
                           ROT_TWO,    0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 1111));
}

TEST_F(ThreadTest, ExecuteRotThree) {
  HandleScope scope(thread_);

  Object obj1(&scope, SmallInt::fromWord(1111));
  Object obj2(&scope, SmallInt::fromWord(2222));
  Object obj3(&scope, SmallInt::fromWord(3333));
  Tuple consts(&scope, runtime_->newTupleWith3(obj1, obj2, obj3));
  Code code(&scope, newEmptyCode());
  code.setStacksize(3);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,   1, LOAD_CONST, 2,
                           ROT_THREE,  0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 2222));
}

TEST_F(ThreadTest, ExecuteRotFour) {
  HandleScope scope(thread_);

  Object obj1(&scope, SmallInt::fromWord(1111));
  Object obj2(&scope, SmallInt::fromWord(2222));
  Object obj3(&scope, SmallInt::fromWord(3333));
  Object obj4(&scope, SmallInt::fromWord(4444));
  Tuple consts(&scope, runtime_->newTupleWith4(obj1, obj2, obj3, obj4));
  Code code(&scope, newEmptyCode());
  code.setStacksize(4);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST, 1, LOAD_CONST,   2,
                           LOAD_CONST, 3, ROT_FOUR,   0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 3333));
}

TEST_F(ThreadTest, ExecuteJumpAbsolute) {
  HandleScope scope(thread_);

  Object obj1(&scope, SmallInt::fromWord(1111));
  Object obj2(&scope, SmallInt::fromWord(2222));
  Tuple consts(&scope, runtime_->newTupleWith2(obj1, obj2));
  Code code(&scope, newEmptyCode());
  code.setStacksize(2);
  code.setConsts(*consts);
  const byte bytecode[] = {JUMP_ABSOLUTE, 4, LOAD_CONST,   0,
                           LOAD_CONST,    1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 2222));
}

TEST_F(ThreadTest, ExecuteJumpForward) {
  HandleScope scope(thread_);

  Object obj1(&scope, SmallInt::fromWord(1111));
  Object obj2(&scope, SmallInt::fromWord(2222));
  Tuple consts(&scope, runtime_->newTupleWith2(obj1, obj2));
  Code code(&scope, newEmptyCode());
  code.setStacksize(2);
  code.setConsts(*consts);
  const byte bytecode[] = {JUMP_FORWARD, 2, LOAD_CONST,   0,
                           LOAD_CONST,   1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 2222));
}

TEST_F(ThreadTest, ExecuteStoreLoadFast) {
  HandleScope scope(thread_);

  Object obj(&scope, SmallInt::fromWord(1111));
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  Code code(&scope, newEmptyCode());
  code.setConsts(*consts);
  code.setNlocals(2);
  const byte bytecode[] = {LOAD_CONST, 0, STORE_FAST,   1,
                           LOAD_FAST,  1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 1111));
}

TEST_F(ThreadTest, LoadGlobal) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Tuple names(&scope, runtime_->newTupleWith1(name));
  code.setNames(*names);

  const byte bytecode[] = {LOAD_GLOBAL, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::kNofree);

  Module module(&scope, findMainModule(runtime_));
  Object value(&scope, runtime_->newInt(1234));
  moduleAtPut(thread_, module, name, value);

  Object none(&scope, NoneType::object());
  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, module, none), 1234));
}

TEST_F(ThreadTest, StoreGlobalCreateValueCell) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());

  Object obj(&scope, SmallInt::fromWord(42));
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  code.setConsts(*consts);

  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Tuple names(&scope, runtime_->newTupleWith1(name));
  code.setNames(*names);

  const byte bytecode[] = {LOAD_CONST, 0, STORE_GLOBAL, 0,
                           LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::kNofree);

  Module module(&scope, findMainModule(runtime_));
  Object none(&scope, NoneType::object());
  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, module, none), 42));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(module, name), 42));
}

TEST_F(ThreadTest, StoreGlobalReuseValueCell) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());

  Object obj(&scope, SmallInt::fromWord(42));
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  code.setConsts(*consts);

  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Tuple names(&scope, runtime_->newTupleWith1(name));
  code.setNames(*names);

  const byte bytecode[] = {LOAD_CONST, 0, STORE_GLOBAL, 0,
                           LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::kNofree);

  Module module(&scope, findMainModule(runtime_));
  Object value(&scope, runtime_->newInt(99));
  moduleAtPut(thread_, module, name, value);
  Object none(&scope, NoneType::object());
  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, module, none), 42));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(module, name), 42));
}

TEST_F(ThreadTest, LoadNameInModuleBodyFromBuiltins) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());

  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Tuple names(&scope, runtime_->newTupleWith1(name));
  code.setNames(*names);

  const byte bytecode[] = {LOAD_NAME, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::kNofree);

  Object builtins_name(&scope, runtime_->symbols()->at(ID(builtins)));
  Module builtins(&scope, runtime_->newModule(builtins_name));
  Module module(&scope, findMainModule(runtime_));
  moduleAtPutById(thread_, module, ID(__builtins__), builtins);
  Object value(&scope, runtime_->newInt(123));
  moduleAtPut(thread_, builtins, name, value);
  Dict locals(&scope, runtime_->newDict());
  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, module, locals), 123));
}

TEST_F(ThreadTest, LoadNameFromGlobals) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());

  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Tuple names(&scope, runtime_->newTupleWith1(name));
  code.setNames(*names);

  const byte bytecode[] = {LOAD_NAME, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::kNofree);

  Module module(&scope, findMainModule(runtime_));
  Object value(&scope, runtime_->newInt(321));
  moduleAtPut(thread_, module, name, value);
  Dict locals(&scope, runtime_->newDict());

  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, module, locals), 321));
}

TEST_F(ThreadTest, LoadNameFromLocals) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());

  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Tuple names(&scope, runtime_->newTupleWith1(name));
  code.setNames(*names);

  const byte bytecode[] = {LOAD_NAME, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::kNofree);

  Module module(&scope, findMainModule(runtime_));
  Object globals_value(&scope, runtime_->newInt(456));
  moduleAtPut(thread_, module, name, globals_value);
  Dict locals(&scope, runtime_->newDict());
  Object locals_value(&scope, runtime_->newInt(654));
  dictAtPutByStr(thread_, locals, name, locals_value);
  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, module, locals), 654));
}

TEST_F(ThreadTest, MakeFunction) {
  HandleScope scope(thread_);

  Code func_code(&scope, newEmptyCode());
  func_code.setName(runtime_->newStrFromCStr("hello"));
  func_code.setCode(runtime_->newBytes(0, 0));
  func_code.setFlags(Code::Flags::kOptimized | Code::Flags::kNewlocals |
                     Code::Flags::kNofree);

  Code code(&scope, newEmptyCode());

  Object obj1(&scope, runtime_->newStrFromCStr("hello_qualname"));
  Object obj2(&scope, NoneType::object());
  Tuple consts(&scope, runtime_->newTupleWith3(func_code, obj1, obj2));
  code.setConsts(*consts);

  Object name(&scope, runtime_->newStrFromCStr("hello"));
  Tuple names(&scope, runtime_->newTupleWith1(name));
  code.setNames(*names);

  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST, 1, MAKE_FUNCTION, 0,
                           STORE_NAME, 0, LOAD_CONST, 2, RETURN_VALUE,  0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setFlags(0);

  Module module(&scope, findMainModule(runtime_));
  Dict locals(&scope, runtime_->newDict());
  ASSERT_TRUE(thread_->exec(code, module, locals).isNoneType());

  Object function_obj(&scope, dictAtByStr(thread_, locals, name));
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

  Object obj1(&scope, SmallInt::fromWord(111));
  Object obj2(&scope, runtime_->newStrFromCStr("qqq"));
  Object obj3(&scope, NoneType::object());
  Tuple consts(&scope, runtime_->newTupleWith3(obj1, obj2, obj3));
  code.setConsts(*consts);

  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,   1, LOAD_CONST, 2,
                           BUILD_LIST, 3, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

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
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object result_obj(&scope, runCode(code));
  ASSERT_TRUE(result_obj.isSet());
  Set result(&scope, *result_obj);
  EXPECT_EQ(result.numItems(), 0);
}

TEST_F(ThreadTest, BuildSetWithOneItem) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());

  Object obj(&scope, runtime_->newInt(111));
  Tuple consts(&scope, runtime_->newTupleWith2(obj, obj));  // dup
  code.setConsts(*consts);

  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,   1,
                           BUILD_SET,  2, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object result_obj(&scope, runCode(code));
  ASSERT_TRUE(result_obj.isSet());
  Set result(&scope, *result_obj);
  EXPECT_EQ(result.numItems(), 1);
  Object int_val(&scope, SmallInt::fromWord(111));
  EXPECT_TRUE(setIncludes(thread_, result, int_val));
}

TEST_F(ThreadTest, BuildSet) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());

  Object obj1(&scope, runtime_->newInt(111));
  Object obj2(&scope, runtime_->newStrFromCStr("qqq"));
  Object obj3(&scope, NoneType::object());
  Tuple consts(&scope, runtime_->newTupleWith4(obj1, obj1, obj2, obj3));  // dup

  code.setConsts(*consts);

  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST, 1, LOAD_CONST,   2,
                           LOAD_CONST, 3, BUILD_SET,  4, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object result_obj(&scope, runCode(code));
  ASSERT_TRUE(result_obj.isSet());
  Set result(&scope, *result_obj);
  EXPECT_EQ(result.numItems(), 3);
  Object int_val(&scope, runtime_->newInt(111));
  EXPECT_TRUE(setIncludes(thread_, result, int_val));
  Object str(&scope, runtime_->newStrFromCStr("qqq"));
  EXPECT_TRUE(setIncludes(thread_, result, str));
  Object none(&scope, NoneType::object());
  EXPECT_TRUE(setIncludes(thread_, result, none));
}

TEST_F(ThreadTest, PopJumpIfFalseWithTruthy) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj1(&scope, Bool::trueObj());
  Object obj2(&scope, SmallInt::fromWord(1111));
  Object obj3(&scope, SmallInt::fromWord(2222));
  Tuple consts(&scope, runtime_->newTupleWith3(obj1, obj2, obj3));
  code.setConsts(*consts);
  // Bytecode for the snippet:
  //   if x:
  //     return 1111
  //   return 2222
  const byte bytecode[] = {LOAD_CONST, 0, POP_JUMP_IF_FALSE, 8,
                           LOAD_CONST, 1, RETURN_VALUE,      0,
                           LOAD_CONST, 2, RETURN_VALUE,      0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 1111));
}

TEST_F(ThreadTest, PopJumpIfFalseWithFalsey) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj1(&scope, Bool::falseObj());
  Object obj2(&scope, SmallInt::fromWord(1111));
  Object obj3(&scope, SmallInt::fromWord(2222));
  Tuple consts(&scope, runtime_->newTupleWith3(obj1, obj2, obj3));
  code.setConsts(*consts);
  // Bytecode for the snippet:
  //   if x:
  //     return 1111
  //   return 2222
  const byte bytecode[] = {LOAD_CONST, 0, POP_JUMP_IF_FALSE, 8,
                           LOAD_CONST, 1, RETURN_VALUE,      0,
                           LOAD_CONST, 2, RETURN_VALUE,      0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 2222));
}

TEST_F(ThreadTest, PopJumpIfTrueWithFalsey) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj1(&scope, Bool::falseObj());
  Object obj2(&scope, SmallInt::fromWord(1111));
  Object obj3(&scope, SmallInt::fromWord(2222));
  Tuple consts(&scope, runtime_->newTupleWith3(obj1, obj2, obj3));
  code.setConsts(*consts);
  // Bytecode for the snippet:
  //   if not x:
  //     return 1111
  //   return 2222
  const byte bytecode[] = {LOAD_CONST, 0, POP_JUMP_IF_TRUE, 8,
                           LOAD_CONST, 1, RETURN_VALUE,     0,
                           LOAD_CONST, 2, RETURN_VALUE,     0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 1111));
}

TEST_F(ThreadTest, PopJumpIfTrueWithTruthy) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj1(&scope, Bool::trueObj());
  Object obj2(&scope, SmallInt::fromWord(1111));
  Object obj3(&scope, SmallInt::fromWord(2222));
  Tuple consts(&scope, runtime_->newTupleWith3(obj1, obj2, obj3));
  code.setConsts(*consts);
  // Bytecode for the snippet:
  //   if not x:
  //     return 1111
  //   return 2222
  const byte bytecode[] = {LOAD_CONST, 0, POP_JUMP_IF_TRUE, 8,
                           LOAD_CONST, 1, RETURN_VALUE,     0,
                           LOAD_CONST, 2, RETURN_VALUE,     0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCode(code), 2222));
}

TEST_F(ThreadTest, JumpIfFalseOrPopWithFalsey) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj1(&scope, Bool::falseObj());
  Object obj2(&scope, SmallInt::fromWord(1111));
  Tuple consts(&scope, runtime_->newTupleWith2(obj1, obj2));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, JUMP_IF_FALSE_OR_POP, 6,
                           LOAD_CONST, 1, RETURN_VALUE,         0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  // If the condition is false, we should return the top of the stack, which is
  // the condition itself
  EXPECT_EQ(runCode(code), Bool::falseObj());
}

TEST_F(ThreadTest, JumpIfFalseOrPopWithTruthy) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj1(&scope, Bool::trueObj());
  Object obj2(&scope, SmallInt::fromWord(1111));
  Tuple consts(&scope, runtime_->newTupleWith2(obj1, obj2));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, JUMP_IF_FALSE_OR_POP, 6,
                           LOAD_CONST, 1, RETURN_VALUE,         0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  // If the condition is true, we should pop the top of the stack (the
  // condition) and continue execution. In our case that loads a const and
  // returns it.
  EXPECT_TRUE(isIntEqualsWord(runCode(code), 1111));
}

TEST_F(ThreadTest, JumpIfTrueOrPopWithTruthy) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj1(&scope, Bool::trueObj());
  Object obj2(&scope, SmallInt::fromWord(1111));
  Tuple consts(&scope, runtime_->newTupleWith2(obj1, obj2));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, JUMP_IF_TRUE_OR_POP, 6,
                           LOAD_CONST, 1, RETURN_VALUE,        0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  // If the condition is true, we should return the top of the stack, which is
  // the condition itself
  EXPECT_EQ(runCode(code), Bool::trueObj());
}

TEST_F(ThreadTest, JumpIfTrueOrPopWithFalsey) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj1(&scope, Bool::falseObj());
  Object obj2(&scope, SmallInt::fromWord(1111));
  Tuple consts(&scope, runtime_->newTupleWith2(obj1, obj2));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, JUMP_IF_TRUE_OR_POP, 6,
                           LOAD_CONST, 1, RETURN_VALUE,        0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  // If the condition is false, we should pop the top of the stack (the
  // condition) and continue execution. In our case that loads a const and
  // returns it.
  EXPECT_TRUE(isIntEqualsWord(runCode(code), 1111));
}

TEST_F(ThreadTest, UnaryNotWithTruthy) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj(&scope, Bool::trueObj());
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  code.setConsts(*consts);
  // Bytecode for the snippet:
  //     return not x
  const byte bytecode[] = {LOAD_CONST, 0, UNARY_NOT, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  // If the condition is true, we should return false
  EXPECT_EQ(runCode(code), Bool::falseObj());
}

TEST_F(ThreadTest, UnaryNotWithFalsey) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj(&scope, Bool::falseObj());
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  code.setConsts(*consts);
  // Bytecode for the snippet:
  //     return not x
  const byte bytecode[] = {LOAD_CONST, 0, UNARY_NOT, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  // If the condition is false, we should return true
  EXPECT_EQ(runCode(code), Bool::trueObj());
}

TEST_F(ThreadTest, LoadBuildTypeEmptyType) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  pass
)")
                   .isError());

  Type cls(&scope, mainModuleAt(runtime_, "C"));
  ASSERT_TRUE(cls.name().isSmallStr());
  EXPECT_EQ(cls.name(), SmallStr::fromCStr("C"));

  Tuple mro(&scope, cls.mro());
  EXPECT_EQ(mro.length(), 2);
  EXPECT_EQ(mro.at(0), *cls);
  EXPECT_EQ(mro.at(1), runtime_->typeAt(LayoutId::kObject));
}

TEST_F(ThreadTest, LoadBuildTypeTypeWithInit) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self):
    pass
)")
                   .isError());

  Module mod(&scope, findMainModule(runtime_));
  ASSERT_TRUE(mod.isModule());

  Str cls_name(&scope, Runtime::internStrFromCStr(thread_, "C"));
  Object value(&scope, moduleAt(mod, cls_name));
  ASSERT_TRUE(value.isType());
  Type cls(&scope, *value);

  // Check class MRO
  Tuple mro(&scope, cls.mro());
  EXPECT_EQ(mro.length(), 2);
  EXPECT_EQ(mro.at(0), *cls);
  EXPECT_EQ(mro.at(1), runtime_->typeAt(LayoutId::kObject));

  // Check class name
  ASSERT_TRUE(cls.name().isSmallStr());
  EXPECT_EQ(cls.name(), SmallStr::fromCStr("C"));

  // Check for the __init__ method name in the dict
  value = typeAtById(thread_, cls, ID(__init__));
  ASSERT_FALSE(value.isError());
  EXPECT_TRUE(value.isFunction());
}

static ALIGN_16 RawObject nativeExceptionTest(Thread* thread, Arguments) {
  HandleScope scope(thread);
  Str msg(&scope,
          Str::cast(thread->runtime()->newStrFromCStr("test exception")));
  return thread->raise(LayoutId::kRuntimeError, *msg);
}

TEST_F(ThreadTest, NativeExceptions) {
  HandleScope scope(thread_);

  Object name(&scope, runtime_->newStrFromCStr("fn"));
  Object empty_tuple(&scope, runtime_->emptyTuple());
  Code fn_code(&scope, runtime_->newBuiltinCode(
                           /*argcount=*/0, /*posonlyargcount=*/0,
                           /*kwonlyargcount=*/0, /*flags=*/0,
                           nativeExceptionTest, empty_tuple, name));
  Module module(&scope, findMainModule(runtime_));
  Function fn(&scope,
              runtime_->newFunctionWithCode(thread_, name, fn_code, module));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTupleWith1(fn));
  code.setConsts(*consts);

  // Call the native fn and check that a pending exception is left in the
  // Thread.
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(1);

  EXPECT_TRUE(raised(runCode(code), LayoutId::kRuntimeError));
  Object value(&scope, thread_->pendingExceptionValue());
  ASSERT_TRUE(value.isStr());
  Str str(&scope, *value);
  EXPECT_TRUE(str.equalsCStr("test exception"));
}

TEST_F(ThreadTest, PendingStopIterationValueInspectsTuple) {
  HandleScope scope(thread_);

  Object obj1(&scope, runtime_->newInt(123));
  Object obj2(&scope, runtime_->newInt(456));
  Tuple tuple(&scope, runtime_->newTupleWith2(obj1, obj2));
  thread_->raise(LayoutId::kStopIteration, *tuple);

  ASSERT_TRUE(thread_->hasPendingStopIteration());
  EXPECT_TRUE(isIntEqualsWord(thread_->pendingStopIterationValue(), 123));
}

TEST_F(ThreadTest,
       RaiseStopIterationWithTupleReturnedByPendingStopIterationValue) {
  HandleScope scope(thread_);

  Object obj1(&scope, runtime_->newInt(123));
  Object obj2(&scope, runtime_->newInt(456));
  Tuple tuple(&scope, runtime_->newTupleWith2(obj1, obj2));
  thread_->raiseStopIterationWithValue(tuple);

  ASSERT_TRUE(thread_->hasPendingStopIteration());
  EXPECT_EQ(thread_->pendingStopIterationValue(), tuple);
}

TEST_F(ThreadTest,
       RaiseStopIterationWithStopIterationReturnedByPendingStopIterationValue) {
  HandleScope scope(thread_);

  Object obj1(&scope, runtime_->newInt(123));
  Type stop_iteration_type(&scope, runtime_->typeAt(LayoutId::kStopIteration));
  Object stop_iteration(&scope,
                        Interpreter::call1(thread_, stop_iteration_type, obj1));
  thread_->raiseStopIterationWithValue(stop_iteration);

  ASSERT_TRUE(thread_->hasPendingStopIteration());
  EXPECT_EQ(thread_->pendingStopIterationValue(), stop_iteration);
}

TEST_F(ThreadTest,
       RaiseStopIterationWithIntReturnedByPendingStopIterationValue) {
  HandleScope scope(thread_);

  Object obj1(&scope, runtime_->newInt(123));
  thread_->raiseStopIterationWithValue(obj1);

  ASSERT_TRUE(thread_->hasPendingStopIteration());
  EXPECT_EQ(thread_->pendingStopIterationValue(), obj1);
}

TEST_F(ThreadTest, RaiseWithTypePreservesTraceback) {
  HandleScope scope(thread_);
  Layout layout(&scope, runtime_->layoutAt(LayoutId::kBaseException));
  BaseException exc(&scope, runtime_->newInstance(layout));
  Object tb(&scope, runtime_->newTraceback());
  exc.setTraceback(*tb);
  thread_->raiseWithType(runtime_->typeOf(*exc), *exc);
  Object pending_exc_tb(&scope, thread_->pendingExceptionTraceback());
  EXPECT_EQ(tb, pending_exc_tb);
}

// MRO tests

TEST_F(ThreadTest, LoadBuildTypeVerifyMro) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A: pass
class B: pass
class C(A,B): pass
)")
                   .isError());

  Object a(&scope, mainModuleAt(runtime_, "A"));
  Object b(&scope, mainModuleAt(runtime_, "B"));
  Type c(&scope, mainModuleAt(runtime_, "C"));
  Object object(&scope, moduleAtByCStr(runtime_, "builtins", "object"));
  Tuple mro(&scope, c.mro());
  ASSERT_EQ(mro.length(), 4);
  EXPECT_EQ(mro.at(0), c);
  EXPECT_EQ(mro.at(1), a);
  EXPECT_EQ(mro.at(2), b);
  EXPECT_EQ(mro.at(3), object);
}

TEST_F(ThreadTest, LoadBuildTypeVerifyMroInheritance) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A: pass
class B(A): pass
class C(B): pass
)")
                   .isError());

  Object a(&scope, mainModuleAt(runtime_, "A"));
  Object b(&scope, mainModuleAt(runtime_, "B"));
  Type c(&scope, mainModuleAt(runtime_, "C"));
  Object object(&scope, moduleAtByCStr(runtime_, "builtins", "object"));
  Tuple mro(&scope, c.mro());
  ASSERT_EQ(mro.length(), 4);
  EXPECT_EQ(mro.at(0), c);
  EXPECT_EQ(mro.at(1), b);
  EXPECT_EQ(mro.at(2), a);
  EXPECT_EQ(mro.at(3), object);
}

TEST_F(ThreadTest, LoadBuildTypeVerifyMroMultiInheritance) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A: pass
class B(A): pass
class C: pass
class D(B,C): pass
)")
                   .isError());

  Object a(&scope, mainModuleAt(runtime_, "A"));
  Object b(&scope, mainModuleAt(runtime_, "B"));
  Object c(&scope, mainModuleAt(runtime_, "C"));
  Type d(&scope, mainModuleAt(runtime_, "D"));
  Object object(&scope, moduleAtByCStr(runtime_, "builtins", "object"));
  Tuple mro(&scope, d.mro());
  ASSERT_EQ(mro.length(), 5);
  EXPECT_EQ(mro.at(0), d);
  EXPECT_EQ(mro.at(1), b);
  EXPECT_EQ(mro.at(2), a);
  EXPECT_EQ(mro.at(3), c);
  EXPECT_EQ(mro.at(4), object);
}

TEST_F(ThreadTest, LoadBuildTypeVerifyMroDiamond) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A: pass
class B(A): pass
class C(A): pass
class D(B,C): pass
)")
                   .isError());

  Object a(&scope, mainModuleAt(runtime_, "A"));
  Object b(&scope, mainModuleAt(runtime_, "B"));
  Object c(&scope, mainModuleAt(runtime_, "C"));
  Type d(&scope, mainModuleAt(runtime_, "D"));
  Object object(&scope, moduleAtByCStr(runtime_, "builtins", "object"));
  Tuple mro(&scope, d.mro());
  ASSERT_EQ(mro.length(), 5);
  EXPECT_EQ(mro.at(0), d);
  EXPECT_EQ(mro.at(1), b);
  EXPECT_EQ(mro.at(2), c);
  EXPECT_EQ(mro.at(3), a);
  EXPECT_EQ(mro.at(4), object);
}

TEST_F(ThreadTest, LoadBuildTypeVerifyMroError) {
  const char* src = R"(
class A: pass
class B(A): pass
class C(A, B): pass
)";

  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, src), LayoutId::kTypeError,
                            "Cannot create a consistent method resolution "
                            "order (MRO) for bases A, B"));
}

// iteration

TEST_F(ThreadTest, RaiseVarargs) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "raise 1"),
                            LayoutId::kTypeError,
                            "exceptions must derive from BaseException"));
}

TEST_F(ThreadTest, InheritFromObject) {
  HandleScope scope(thread_);
  const char* src = R"(
class Foo(object):
  pass
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());

  // Look up the class Foo
  Object main_obj(&scope, findMainModule(runtime_));
  ASSERT_TRUE(main_obj.isModule());
  Object foo_obj(&scope, mainModuleAt(runtime_, "Foo"));
  ASSERT_TRUE(foo_obj.isType());
  Type foo(&scope, *foo_obj);

  // Check that its MRO is itself and object
  ASSERT_TRUE(foo.mro().isTuple());
  Tuple mro(&scope, foo.mro());
  ASSERT_EQ(mro.length(), 2);
  EXPECT_EQ(mro.at(0), *foo);
  EXPECT_EQ(mro.at(1), runtime_->typeAt(LayoutId::kObject));
}

// imports

TEST_F(ThreadTest, ImportTest) {
  HandleScope scope(thread_);
  Object module_src(&scope, runtime_->newStrFromCStr(R"(
def say_hello():
  return "hello"
)"));
  Object filename(&scope, runtime_->newStrFromCStr("<test string>"));
  const char* main_src = R"(
import hello
result = hello.say_hello()
)";

  // Pre-load the hello module so is cached.
  Code code(&scope, compile(thread_, module_src, filename, ID(exec),
                            /*flags=*/0, /*optimize=*/0));
  Object name(&scope, runtime_->newStrFromCStr("hello"));
  ASSERT_FALSE(executeModuleFromCode(thread_, code, name).isError());
  ASSERT_FALSE(runFromCStr(runtime_, main_src).isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "hello"));
}

TEST_F(ThreadTest, FailedImportTest) {
  const char* main_src = R"(
import hello
hello.say_hello()
)";

  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, main_src),
                            LayoutId::kModuleNotFoundError,
                            "No module named 'hello'"));
}

TEST_F(ThreadTest, ImportMissingAttributeTest) {
  HandleScope scope(thread_);
  Object module_src(&scope, runtime_->newStrFromCStr(R"(
def say_hello():
  print("hello");
)"));
  Object filename(&scope, runtime_->newStrFromCStr("<test string>"));
  const char* main_src = R"(
import hello
hello.foo()
)";

  // Pre-load the hello module so is cached.
  Code code(&scope, compile(thread_, module_src, filename, ID(exec),
                            /*flags=*/0, /*optimize=*/0));
  Object name(&scope, runtime_->newStrFromCStr("hello"));
  ASSERT_FALSE(executeModuleFromCode(thread_, code, name).isError());

  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, main_src),
                            LayoutId::kAttributeError,
                            "module 'hello' has no attribute 'foo'"));
}

TEST_F(ThreadTest, ModuleSetAttrTest) {
  HandleScope scope(thread_);
  Object module_src(&scope, runtime_->newStrFromCStr(R"(
def say_hello():
  return "hello"
)"));
  Object filename(&scope, runtime_->newStrFromCStr("<test string>"));
  const char* main_src = R"(
import hello
def goodbye():
  return "goodbye"
hello.say_hello = goodbye
result = hello.say_hello()
)";

  // Pre-load the hello module so is cached.
  Code code(&scope, compile(thread_, module_src, filename, ID(exec),
                            /*flags=*/0, /*optimize=*/0));
  Object name(&scope, runtime_->newStrFromCStr("hello"));
  ASSERT_FALSE(executeModuleFromCode(thread_, code, name).isError());
  ASSERT_FALSE(runFromCStr(runtime_, main_src).isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "goodbye"));
}

TEST_F(ThreadTest, StoreFastStackEffect) {
  const char* src = R"(
def printit(x, y, z):
  return (x, y, z)

def test():
  x = 1
  y = 2
  z = 3
  return printit(x, y, z)

result = test()
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isTuple());
  Tuple tuple(&scope, *result);
  ASSERT_EQ(tuple.length(), 3);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(2), 3));
}

TEST_F(ThreadTest, Closure) {
  const char* src = R"(
result = []
def f():
  a = 1
  def g():
    b = 2
    def h():
      result.append(b)
    result.append(a)
    h()
    b = 3
    h()
  g()
f()
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(ThreadTest, StackOverflowRaisesRecursionError) {
  // TODO(T59724033): Not using binop doesn't raise RecursionError, which
  // signifies that our recursion depth checking is problematic.
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
def foo(x):
  return foo(x + 1)

foo(1.0)
)"),
                            LayoutId::kRecursionError,
                            "maximum recursion depth exceeded"));
}

TEST_F(FormatTest, NoConvEmpty) {
  const char* src = R"(
result = f''
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), ""));
}

TEST_F(FormatTest, NoConvOneElement) {
  const char* src = R"(
a = "hello"
result = f'a={a}'
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "a=hello"));
}

TEST_F(FormatTest, NoConvMultiElements) {
  const char* src = R"(
a = "hello"
b = "world"
c = "python"
result = f'{a} {b} {c}'
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  EXPECT_TRUE(
      isStrEqualsCStr(mainModuleAt(runtime_, "result"), "hello world python"));
}

TEST_F(FormatTest, NoConvMultiElementsLarge) {
  const char* src = R"(
a = "Python"
b = "is"
c = "an interpreted high-level programming language for general-purpose programming."
result = f'{a} {b} {c}'
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  EXPECT_TRUE(isStrEqualsCStr(
      mainModuleAt(runtime_, "result"),
      "Python is an interpreted high-level programming language for "
      "general-purpose programming."));
}

TEST_F(ThreadTest, BuildTupleUnpack) {
  HandleScope scope(thread_);
  const char* src = R"(
t = (*[0], *[1, 2], *[], *[3, 4, 5])
t1 = (*(0,), *(1, 2), *(), *(3, 4, 5))
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());

  Object t(&scope, mainModuleAt(runtime_, "t"));
  EXPECT_TRUE(t.isTuple());
  Tuple tuple_t(&scope, *t);
  EXPECT_EQ(tuple_t.length(), 6);
  for (word i = 0; i < tuple_t.length(); i++) {
    EXPECT_TRUE(isIntEqualsWord(tuple_t.at(i), i));
  }

  Object t1(&scope, mainModuleAt(runtime_, "t1"));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());

  Object l(&scope, mainModuleAt(runtime_, "l"));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());

  Object s(&scope, mainModuleAt(runtime_, "s"));
  EXPECT_TRUE(s.isSet());
  Set set_s(&scope, *s);
  EXPECT_EQ(set_s.numItems(), 6);
  Object small_int(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(setIncludes(thread_, set_s, small_int));
  small_int = SmallInt::fromWord(1);
  EXPECT_TRUE(setIncludes(thread_, set_s, small_int));
  small_int = SmallInt::fromWord(2);
  EXPECT_TRUE(setIncludes(thread_, set_s, small_int));
  small_int = SmallInt::fromWord(3);
  EXPECT_TRUE(setIncludes(thread_, set_s, small_int));
  small_int = SmallInt::fromWord(4);
  EXPECT_TRUE(setIncludes(thread_, set_s, small_int));
  small_int = SmallInt::fromWord(5);
  EXPECT_TRUE(setIncludes(thread_, set_s, small_int));
}

TEST_F(BuildString, BuildStringEmpty) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());

  const byte bytecode[] = {BUILD_STRING, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isStrEqualsCStr(runCode(code), ""));
}

TEST_F(BuildString, BuildStringSingle) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());

  Object str(&scope, SmallStr::fromCStr("foo"));
  Tuple consts(&scope, runtime_->newTupleWith1(str));
  code.setConsts(*consts);

  const byte bytecode[] = {LOAD_CONST, 0, BUILD_STRING, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isStrEqualsCStr(runCode(code), "foo"));
}

TEST_F(BuildString, BuildStringMultiSmall) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());

  Object str(&scope, SmallStr::fromCStr("foo"));
  Object str1(&scope, SmallStr::fromCStr("bar"));
  Tuple consts(&scope, runtime_->newTupleWith2(str, str1));
  code.setConsts(*consts);

  const byte bytecode[] = {LOAD_CONST,   0, LOAD_CONST,   1,
                           BUILD_STRING, 2, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isStrEqualsCStr(runCode(code), "foobar"));
}

TEST_F(BuildString, BuildStringMultiLarge) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());

  Object str(&scope, SmallStr::fromCStr("hello"));
  Object str1(&scope, SmallStr::fromCStr("world"));
  Object str2(&scope, SmallStr::fromCStr("python"));
  Tuple consts(&scope, runtime_->newTupleWith3(str, str1, str2));
  code.setConsts(*consts);

  const byte bytecode[] = {LOAD_CONST,   0, LOAD_CONST,   1, LOAD_CONST, 2,
                           BUILD_STRING, 3, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isStrEqualsCStr(runCode(code), "helloworldpython"));
}

TEST_F(UnpackSeq, UnpackRange) {
  const char* src = R"(
[a ,b, c] = range(2, 5)
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "a"), 2));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "b"), 3));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "c"), 4));
}

// LIST_APPEND(listAdd) in list_comp, followed by unpack
// TODO(rkng): list support in BINARY_ADD
TEST_F(UnpackList, UnpackListCompAppend) {
  const char* src = R"(
a = [1, 2, 3]
b = [x for x in a]
b1, b2, b3 = b
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "b1"), 1));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "b2"), 2));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "b3"), 3));
}

TEST_F(ThreadTest, SetAdd) {
  HandleScope scope(thread_);
  const char* src = R"(
a = [1, 2, 3]
b = {x for x in a}
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object b(&scope, mainModuleAt(runtime_, "b"));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object b(&scope, mainModuleAt(runtime_, "b"));
  EXPECT_EQ(b.isDict(), true);
  Dict dict_b(&scope, *b);
  EXPECT_EQ(dict_b.numItems(), 3);
}

TEST_F(UnpackList, UnpackNestedLists) {
  const char* src = R"(
b = [[1,2], [3,4,5]]
b1, b2 = b
b11, b12 = b1
b21, b22, b23 = b2
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  HandleScope scope(thread_);
  List b(&scope, mainModuleAt(runtime_, "b"));
  EXPECT_EQ(b.numItems(), 2);
  List b1(&scope, mainModuleAt(runtime_, "b1"));
  EXPECT_EQ(b1.numItems(), 2);
  List b2(&scope, mainModuleAt(runtime_, "b2"));
  EXPECT_EQ(b2.numItems(), 3);
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "b11"), 1));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "b12"), 2));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "b21"), 3));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "b22"), 4));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "b23"), 5));
}

TEST_F(UnpackSeq, UnpackRangeStep) {
  const char* src = R"(
[a ,b, c, d] = range(2, 10, 2)
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "a"), 2));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "b"), 4));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "c"), 6));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "d"), 8));
}

TEST_F(UnpackSeq, UnpackRangeNeg) {
  const char* src = R"(
[a ,b, c, d, e] = range(-10, 0, 2)
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "a"), -10));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "b"), -8));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "c"), -6));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "d"), -4));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "e"), -2));
}

TEST_F(ListIterTest, Build) {
  const char* src = R"(
a = [1, 2, 3]
result = []
for x in a:
  result.append(x)
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(ListAppendTest, BuildAndUnpack) {
  const char* src = R"(
a = [1, 2]
b = [x for x in [a] * 3]
b1, b2, b3 = b
b11, b12 = b1
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  HandleScope scope(thread_);
  List b(&scope, mainModuleAt(runtime_, "b"));
  EXPECT_EQ(b.numItems(), 3);
  List b1(&scope, mainModuleAt(runtime_, "b1"));
  EXPECT_EQ(b1.numItems(), 2);
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "b11"), 1));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "b12"), 2));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Object l(&scope, mainModuleAt(runtime_, "l"));
  Object s(&scope, mainModuleAt(runtime_, "s"));

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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object l(&scope, mainModuleAt(runtime_, "l"));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object l(&scope, mainModuleAt(runtime_, "l"));
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, src), LayoutId::kTypeError,
                            "multiple bases have instance lay-out conflict"));
}

TEST_F(ThreadTest, BreakLoopWhileLoop) {
  const char* src = R"(
a = 0
result = []
while 1:
    a = a + 1
    result.append(a)
    if a == 3:
        break
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(ThreadTest, BreakLoopWhileLoop1) {
  const char* src = R"(
a = 0
result = []
while 1:
    a = a + 1
    result.append(a)
    if a == 3:
        break
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "a"), 3));
}

TEST_F(ThreadTest, BreakLoopRangeLoop) {
  const char* src = R"(
result = []
for x in range(1,6):
  if x == 3:
    break;
  result.append(x)
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2});
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object l(&scope, mainModuleAt(runtime_, "l"));
  EXPECT_TRUE(l.isList());
  List list_l(&scope, *l);
  ASSERT_GE(list_l.numItems(), 3);
  EXPECT_EQ(list_l.at(0), SmallInt::fromWord(0));
  EXPECT_EQ(list_l.at(1), SmallInt::fromWord(1));
  EXPECT_EQ(list_l.at(2), SmallInt::fromWord(2));
}

TEST_F(ThreadTest, Func2TestPyStone) {  // mimic pystone.py Func2
  const char* src = R"(
def f1(x, y):
  return x + y
def f2():
  return f1(1, 2)
result = f2()
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 3));
}

TEST_F(ThreadTest, BinSubscrString) {  // pystone dependency
  const char* src = R"(
a = 'Hello'
r0 = a[0]
r1 = a[1]
r2 = a[2]
r3 = a[3]
r4 = a[4]
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "r0"), "H"));
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "r1"), "e"));
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "r2"), "l"));
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "r3"), "l"));
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "r4"), "o"));
}

TEST_F(ThreadTest, SetupExceptNoOp) {  // pystone dependency
  const char* src = R"(
def f(x):
  try:
    return x
  except ValueError:
    return "Invalid Argument"
result = f(100)
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 100));
}

TEST_F(ThreadTest, BuildTypeWithMetaclass) {
  HandleScope scope(thread_);
  const char* src = R"(
class Foo(metaclass=type):
  pass
a = Foo()
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object foo(&scope, mainModuleAt(runtime_, "Foo"));
  EXPECT_TRUE(foo.isType());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_TRUE(runtime_->typeOf(*a) == *foo);
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object bar(&scope, mainModuleAt(runtime_, "Bar"));
  EXPECT_TRUE(runtime_->isInstanceOfType(*bar));
  Object a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_TRUE(runtime_->typeOf(*a) == *bar);
  Object b(&scope, mainModuleAt(runtime_, "b"));
  EXPECT_TRUE(isIntEqualsWord(*b, 123));
  Object c(&scope, mainModuleAt(runtime_, "c"));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 3));
  Object b(&scope, mainModuleAt(runtime_, "b"));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object one(&scope, mainModuleAt(runtime_, "one"));
  EXPECT_TRUE(isIntEqualsWord(*one, 1));
  Object two(&scope, mainModuleAt(runtime_, "two"));
  EXPECT_TRUE(isIntEqualsWord(*two, 2));
}

TEST_F(ThreadTest, ExecuteDeleteName) {
  HandleScope scope(thread_);
  const char* src = R"(
var = 1
del var
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object var(&scope, mainModuleAt(runtime_, "var"));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object x(&scope, mainModuleAt(runtime_, "x"));
  EXPECT_EQ(*x, SmallInt::fromWord(2));
}

TEST_F(ThreadTest, SetupAnnotationsAndStoreAnnotations) {
  HandleScope scope(thread_);
  const char* src = R"(
global_with_annotation: int = 1
class ClassWithAnnotation:
  attribute_with_annotation: int = 2
class_anno_dict = ClassWithAnnotation.__annotations__
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Dict module_anno_dict(&scope, mainModuleAt(runtime_, "__annotations__"));
  Str m_key(&scope, runtime_->newStrFromCStr("global_with_annotation"));
  Object m_value(&scope, dictAtByStr(thread_, module_anno_dict, m_key));
  EXPECT_EQ(*m_value, runtime_->typeAt(LayoutId::kInt));

  Dict class_anno_dict(&scope, mainModuleAt(runtime_, "class_anno_dict"));
  Str c_key(&scope, runtime_->newStrFromCStr("attribute_with_annotation"));
  Object c_value(&scope, dictAtByStr(thread_, class_anno_dict, c_key));
  EXPECT_EQ(*c_value, runtime_->typeAt(LayoutId::kInt));
}

TEST_F(ThreadTest, DeleteFastRaisesUnboundLocalError) {
  const char* src = R"(
def foo(a, b, c):
  del a
  return a
foo(1, 2, 3)
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, src),
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object x(&scope, mainModuleAt(runtime_, "x"));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());

  Object result_a(&scope, mainModuleAt(runtime_, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 1111));

  Object result_b(&scope, mainModuleAt(runtime_, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 2222));

  Object result_c(&scope, mainModuleAt(runtime_, "result_c"));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object x(&scope, mainModuleAt(runtime_, "x"));
  EXPECT_EQ(*x, SmallInt::fromWord(1));
}

TEST_F(ThreadTest, LoadTypeDerefFromLocal) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  Object obj1(&scope, SmallInt::fromWord(1111));
  Tuple consts(&scope, runtime_->newTupleWith1(obj1));
  Object obj2(&scope, SmallStr::fromCStr("lalala"));
  Tuple freevars(&scope, runtime_->newTupleWith1(obj2));
  Object obj3(&scope, SmallStr::fromCStr("lalala"));
  Tuple names(&scope, runtime_->newTupleWith1(obj3));
  code.setConsts(*consts);
  code.setNames(*names);
  code.setFreevars(*freevars);
  const byte bytecode[] = {LOAD_CONST,      0, STORE_NAME,   0,
                           LOAD_CLASSDEREF, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(2);
  code.setFlags(Code::Flags::kNofree);

  Module module(&scope, findMainModule(runtime_));
  Dict locals(&scope, runtime_->newDict());
  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, module, locals), 1111));
}

TEST_F(ThreadTest, PushCallFrameWithSameGlobalsPropagatesBuiltins) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  code.setCode(Bytes::empty());
  code.setNames(runtime_->emptyTuple());

  Object qualname(&scope, runtime_->newStrFromCStr("<anonymous>"));
  Module module(&scope, findMainModule(runtime_));
  Function function(
      &scope, runtime_->newFunctionWithCode(thread_, qualname, code, module));

  Frame* frame = thread_->currentFrame();
  thread_->stackPush(*function);
  Frame* new_frame = thread_->pushCallFrame(*function);
  EXPECT_NE(new_frame, frame);
}

TEST_F(ThreadTest, ExecSetsMissingDunderBuiltins) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  Object obj(&scope, NoneType::object());
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::kNofree);
  Object name(&scope, runtime_->newStrFromCStr("<test module>"));
  Module module(&scope, runtime_->newModule(name));
  Object none(&scope, NoneType::object());

  ASSERT_TRUE(thread_->exec(code, module, none).isNoneType());

  Module builtins_module(&scope, runtime_->findModuleById(ID(builtins)));
  Object proxy(&scope, builtins_module.moduleProxy());
  EXPECT_EQ(moduleAtById(thread_, module, ID(__builtins__)), proxy);
}

TEST_F(ThreadTest, CallFunctionInDifferentModule) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def a():
  def inner_a():
    return object
  return inner_a
result0 = a()()
)")
                   .isError());
  // Object a(&scope, mainModuleAt(runtime_, "a"));
  Object result0(&scope, mainModuleAt(runtime_, "result0"));
  Object object(&scope, moduleAtByCStr(runtime_, "builtins", "object"));
  EXPECT_EQ(result0, object);
}

TEST_F(ThreadTest, RaiseWithFmtFormatsString) {
  EXPECT_TRUE(raisedWithStr(
      thread_->raiseWithFmt(LayoutId::kTypeError, "hello %Y", ID(dict)),
      LayoutId::kTypeError, "hello dict"));
}

TEST_F(ThreadTest, RaiseOSErrorFromErrnoWithInvalidErrnoReturnsOSError) {
  EXPECT_TRUE(raised(thread_->raiseOSErrorFromErrno(-5), LayoutId::kOSError));
}

TEST_F(ThreadTest, RaiseOSErrorFromErrnoPicksOSErrorSubclass) {
  EXPECT_TRUE(raised(thread_->raiseOSErrorFromErrno(EACCES),
                     LayoutId::kPermissionError));
}

TEST_F(ThreadTest, StrOffsetWithNegativeReturnsNegativeOne) {
  HandleScope scope(thread_);
  Str str(&scope, SmallStr::fromCStr("foo"));
  EXPECT_EQ(thread_->strOffset(str, -10), -1);
}

TEST_F(ThreadTest, StrOffsetWithLargeNumberReturnsLength) {
  HandleScope scope(thread_);
  Str str(&scope, SmallStr::fromCStr("foo"));
  EXPECT_EQ(thread_->strOffset(str, 5), 3);
}

TEST_F(ThreadTest, StrOffsetWithIndexReturnsOffset) {
  HandleScope scope(thread_);
  Str str(&scope, SmallStr::fromCStr("\u00e9tude"));
  EXPECT_EQ(thread_->strOffset(str, 2), 3);
}

TEST_F(ThreadTest, StrOffsetWithCacheHitReturnsCorrectOffsets) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("\u00e9l\u00e9gie"));
  EXPECT_EQ(thread_->strOffset(str, 2), 3);
  EXPECT_EQ(thread_->strOffset(str, 4), 6);
  EXPECT_EQ(thread_->strOffset(str, 1), 2);
}

TEST_F(ThreadTest, StrOffsetWithCacheMissReturnsCorrectOffsets) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("\u00e9l\u00e9gie"));
  EXPECT_EQ(thread_->strOffset(str, 2), 3);

  // out of bounds
  EXPECT_EQ(thread_->strOffset(str, 10), 8);
  EXPECT_EQ(thread_->strOffset(str, 1), 2);

  // new string
  str = runtime_->newStrFromCStr("foo");
  EXPECT_EQ(thread_->strOffset(str, 2), 2);
}

}  // namespace testing
}  // namespace py
