#pragma once

#include "bytecode.h"
#include "globals.h"
#include "handles.h"
#include "symbols.h"

namespace python {

class Object;
class Frame;
class Thread;

class Interpreter {
 public:
  enum class BinaryOp {
    ADD,
    SUB,
    MUL,
    MATMUL,
    TRUEDIV,
    FLOORDIV,
    MOD,
    DIVMOD,
    POW,
    LSHIFT,
    RSHIFT,
    AND,
    XOR,
    OR
  };

  static Object* execute(Thread* thread, Frame* frame);

  static Object* call(Thread* thread, Frame* frame, word nargs);
  static Object* callKw(Thread* thread, Frame* frame, word nargs);
  static Object* callEx(Thread* thread, Frame* frame, word flags);

  // batch concat/join <num> string objects on the stack (no conversion)
  static Object* stringJoin(Thread* thread, Object** sp, word num);

  static Object* isTrue(Thread* thread, Frame* caller);

  static Object* callDescriptorGet(Thread* thread, Frame* caller,
                                   const Handle<Object>& descriptor,
                                   const Handle<Object>& receiver,
                                   const Handle<Object>& receiver_type);

  static Object* callDescriptorSet(Thread* thread, Frame* caller,
                                   const Handle<Object>& descriptor,
                                   const Handle<Object>& receiver,
                                   const Handle<Object>& value);

  static Object* lookupMethod(Thread* thread, Frame* caller,
                              const Handle<Object>& receiver,
                              SymbolId selector);

  static Object* callMethod1(Thread* thread, Frame* caller,
                             const Handle<Object>& method,
                             const Handle<Object>& self);

  static Object* callMethod2(Thread* thread, Frame* caller,
                             const Handle<Object>& method,
                             const Handle<Object>& self,
                             const Handle<Object>& other);

  static Object* callMethod3(Thread* thread, Frame* caller,
                             const Handle<Object>& method,
                             const Handle<Object>& self,
                             const Handle<Object>& arg1,
                             const Handle<Object>& arg2);

  static Object* callMethod4(Thread* thread, Frame* caller,
                             const Handle<Object>& method,
                             const Handle<Object>& self,
                             const Handle<Object>& arg1,
                             const Handle<Object>& arg2,
                             const Handle<Object>& arg3);

  static Object* unaryOperation(Thread* thread, Frame* caller,
                                const Handle<Object>& receiver,
                                SymbolId selector);

  static Object* binaryOperation(Thread* thread, Frame* caller, BinaryOp op,
                                 const Handle<Object>& left,
                                 const Handle<Object>& right);

  static Object* inplaceOperation(Thread* thread, Frame* caller, BinaryOp op,
                                  const Handle<Object>& left,
                                  const Handle<Object>& right);

  static Object* compareOperation(Thread* thread, Frame* caller, CompareOp op,
                                  const Handle<Object>& left,
                                  const Handle<Object>& right);

  static Object* sequenceContains(Thread* thread, Frame* caller,
                                  const Handle<Object>& value,
                                  const Handle<Object>& container);

  struct Context {
    Thread* thread;
    Frame* frame;
    word pc;
  };

  // Pseudo-opcodes
  static void doInvalidBytecode(Context* ctx, word arg);
  static void doNotImplemented(Context* ctx, word arg);

  // Opcodes
  static void doPopTop(Context* ctx, word arg);
  static void doRotTwo(Context* ctx, word arg);
  static void doRotThree(Context* ctx, word arg);
  static void doDupTop(Context* ctx, word arg);
  static void doDupTopTwo(Context* ctx, word arg);
  static void doNop(Context* ctx, word arg);
  static void doUnaryPositive(Context* ctx, word arg);
  static void doUnaryNegative(Context* ctx, word arg);
  static void doUnaryNot(Context* ctx, word arg);
  static void doUnaryInvert(Context* ctx, word arg);
  static void doBinaryMultiply(Context* ctx, word arg);
  static void doBinaryModulo(Context* ctx, word arg);
  static void doBinaryAdd(Context* ctx, word arg);
  static void doBinarySubtract(Context* ctx, word arg);
  static void doBinarySubscr(Context* ctx, word arg);
  static void doBinaryFloorDivide(Context* ctx, word arg);
  static void doBinaryTrueDivide(Context* ctx, word arg);
  static void doInplaceSubtract(Context* ctx, word arg);
  static void doStoreSubscr(Context* ctx, word arg);
  static void doBinaryAnd(Context* ctx, word arg);
  static void doBinaryXor(Context* ctx, word arg);
  static void doGetIter(Context* ctx, word arg);
  static void doLoadBuildClass(Context* ctx, word arg);
  static void doBreakLoop(Context* ctx, word arg);
  static void doWithCleanupStart(Context* ctx, word arg);
  static void doWithCleanupFinish(Context* ctx, word arg);
  static void doPopBlock(Context* ctx, word arg);
  static void doEndFinally(Context* ctx, word arg);
  static void doStoreName(Context* ctx, word arg);
  static void doUnpackSequence(Context* ctx, word arg);
  static void doForIter(Context* ctx, word arg);
  static void doStoreAttr(Context* ctx, word arg);
  static void doStoreGlobal(Context* ctx, word arg);
  static void doDeleteGlobal(Context* ctx, word arg);
  static void doLoadConst(Context* ctx, word arg);
  static void doLoadName(Context* ctx, word arg);
  static void doBuildTuple(Context* ctx, word arg);
  static void doBuildList(Context* ctx, word arg);
  static void doBuildSet(Context* ctx, word arg);
  static void doBuildMap(Context* ctx, word arg);
  static void doLoadAttr(Context* ctx, word arg);
  static void doCompareOp(Context* ctx, word arg);
  static void doImportName(Context* ctx, word arg);
  static void doImportFrom(Context* ctx, word arg);
  static void doJumpForward(Context* ctx, word arg);
  static void doJumpIfFalseOrPop(Context* ctx, word arg);
  static void doJumpIfTrueOrPop(Context* ctx, word arg);
  static void doJumpAbsolute(Context* ctx, word arg);
  static void doPopJumpIfFalse(Context* ctx, word arg);
  static void doPopJumpIfTrue(Context* ctx, word arg);
  static void doLoadGlobal(Context* ctx, word arg);
  static void doContinueLoop(Context* ctx, word arg);
  static void doSetupLoop(Context* ctx, word arg);
  static void doSetupExcept(Context* ctx, word arg);
  static void doLoadFast(Context* ctx, word arg);
  static void doStoreFast(Context* ctx, word arg);
  static void doCallFunction(Context* ctx, word arg);
  static void doMakeFunction(Context* ctx, word arg);
  static void doBuildSlice(Context* ctx, word arg);
  static void doLoadClosure(Context* ctx, word arg);
  static void doLoadDeref(Context* ctx, word arg);
  static void doStoreDeref(Context* ctx, word arg);
  static void doCallFunctionKw(Context* ctx, word arg);
  static void doCallFunctionEx(Context* ctx, word arg);
  static void doSetupWith(Context* ctx, word arg);
  static void doListAppend(Context* ctx, word arg);
  static void doSetAdd(Context* ctx, word arg);
  static void doMapAdd(Context* ctx, word arg);
  static void doBuildListUnpack(Context* ctx, word arg);
  static void doBuildTupleUnpack(Context* ctx, word arg);
  static void doBuildSetUnpack(Context* ctx, word arg);
  static void doFormatValue(Context* ctx, word arg);
  static void doBuildConstKeyMap(Context* ctx, word arg);
  static void doBuildString(Context* ctx, word arg);

 private:
  static Object* callBoundMethod(Thread* thread, Frame* frame, word nargs);

  static Object* callCallable(Thread* thread, Frame* frame, word nargs);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Interpreter);
};

}  // namespace python
