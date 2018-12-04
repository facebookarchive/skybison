#pragma once

#include "bytecode.h"
#include "globals.h"
#include "handles.h"
#include "symbols.h"

namespace python {

class RawObject;
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

  static RawObject execute(Thread* thread, Frame* frame);

  static RawObject call(Thread* thread, Frame* frame, word nargs);
  static RawObject callKw(Thread* thread, Frame* frame, word nargs);
  static RawObject callEx(Thread* thread, Frame* frame, word flags);

  // batch concat/join <num> string objects on the stack (no conversion)
  static RawObject stringJoin(Thread* thread, RawObject* sp, word num);

  static RawObject isTrue(Thread* thread, Frame* caller);

  static RawObject callDescriptorGet(Thread* thread, Frame* caller,
                                     const Object& descriptor,
                                     const Object& receiver,
                                     const Object& receiver_type);

  static RawObject callDescriptorSet(Thread* thread, Frame* caller,
                                     const Object& descriptor,
                                     const Object& receiver,
                                     const Object& value);

  static RawObject callDescriptorDelete(Thread* thread, Frame* caller,
                                        const Object& descriptor,
                                        const Object& receiver);

  static RawObject lookupMethod(Thread* thread, Frame* caller,
                                const Object& receiver, SymbolId selector);

  static RawObject callFunction0(Thread* thread, Frame* caller,
                                 const Object& func);
  static RawObject callFunction1(Thread* thread, Frame* caller,
                                 const Object& func, const Object& arg1);
  static RawObject callFunction(Thread* thread, Frame* caller,
                                const Object& func, const Tuple& args);

  static RawObject callMethod1(Thread* thread, Frame* caller,
                               const Object& method, const Object& self);

  static RawObject callMethod2(Thread* thread, Frame* caller,
                               const Object& method, const Object& self,
                               const Object& other);

  static RawObject callMethod3(Thread* thread, Frame* caller,
                               const Object& method, const Object& self,
                               const Object& arg1, const Object& arg2);

  static RawObject callMethod4(Thread* thread, Frame* caller,
                               const Object& method, const Object& self,
                               const Object& arg1, const Object& arg2,
                               const Object& arg3);

  static RawObject unaryOperation(Thread* thread, Frame* caller,
                                  const Object& receiver, SymbolId selector);

  static RawObject binaryOperation(Thread* thread, Frame* caller, BinaryOp op,
                                   const Object& left, const Object& right);

  static RawObject inplaceOperation(Thread* thread, Frame* caller, BinaryOp op,
                                    const Object& left, const Object& right);

  static RawObject compareOperation(Thread* thread, Frame* caller, CompareOp op,
                                    const Object& left, const Object& right);

  static RawObject sequenceContains(Thread* thread, Frame* caller,
                                    const Object& value,
                                    const Object& container);

  // Perform the meat of YIELD_FROM. Returns Error if the subiterator is
  // finished and execution should continue. Otherwise, returns the value from
  // the subiterator to return to the caller.
  static RawObject yieldFrom(Thread* thread, Frame* frame);

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
  static void doBinaryMatrixMultiply(Context* ctx, word arg);
  static void doInplaceMatrixMultiply(Context* ctx, word arg);
  static void doBinaryPower(Context* ctx, word arg);
  static void doBinaryMultiply(Context* ctx, word arg);
  static void doBinaryModulo(Context* ctx, word arg);
  static void doBinaryAdd(Context* ctx, word arg);
  static void doBinarySubtract(Context* ctx, word arg);
  static void doBinarySubscr(Context* ctx, word arg);
  static void doBinaryFloorDivide(Context* ctx, word arg);
  static void doBinaryTrueDivide(Context* ctx, word arg);
  static void doInplaceFloorDivide(Context* ctx, word arg);
  static void doInplaceTrueDivide(Context* ctx, word arg);
  static void doGetAiter(Context* ctx, word arg);
  static void doGetAnext(Context* ctx, word arg);
  static void doBeforeAsyncWith(Context* ctx, word arg);
  static void doInplaceAdd(Context* ctx, word arg);
  static void doInplaceSubtract(Context* ctx, word arg);
  static void doInplaceMultiply(Context* ctx, word arg);
  static void doInplaceModulo(Context* ctx, word arg);
  static void doStoreSubscr(Context* ctx, word arg);
  static void doDeleteSubscr(Context* ctx, word arg);
  static void doBinaryLshift(Context* ctx, word arg);
  static void doBinaryRshift(Context* ctx, word arg);
  static void doBinaryAnd(Context* ctx, word arg);
  static void doBinaryXor(Context* ctx, word arg);
  static void doBinaryOr(Context* ctx, word arg);
  static void doInplacePower(Context* ctx, word arg);
  static void doGetIter(Context* ctx, word arg);
  static void doGetYieldFromIter(Context* ctx, word arg);
  static void doPrintExpr(Context* ctx, word);
  static void doLoadBuildClass(Context* ctx, word arg);
  static void doGetAwaitable(Context* ctx, word arg);
  static void doInplaceLshift(Context* ctx, word arg);
  static void doInplaceRshift(Context* ctx, word arg);
  static void doInplaceAnd(Context* ctx, word arg);
  static void doInplaceXor(Context* ctx, word arg);
  static void doInplaceOr(Context* ctx, word arg);
  static void doBreakLoop(Context* ctx, word arg);
  static void doWithCleanupStart(Context* ctx, word arg);
  static void doWithCleanupFinish(Context* ctx, word arg);
  static void doImportStar(Context* ctx, word arg);
  static void doSetupAnnotations(Context* ctx, word arg);
  static void doPopBlock(Context* ctx, word arg);
  static void doEndFinally(Context* ctx, word arg);
  static void doStoreName(Context* ctx, word arg);
  static void doDeleteName(Context* ctx, word arg);
  static void doUnpackSequence(Context* ctx, word arg);
  static void doForIter(Context* ctx, word arg);
  static void doUnpackEx(Context* ctx, word arg);
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
  static void doSetupFinally(Context* ctx, word arg);
  static void doLoadFast(Context* ctx, word arg);
  static void doStoreFast(Context* ctx, word arg);
  static void doDeleteFast(Context* ctx, word arg);
  static void doStoreAnnotation(Context* ctx, word arg);
  static void doCallFunction(Context* ctx, word arg);
  static void doMakeFunction(Context* ctx, word arg);
  static void doBuildSlice(Context* ctx, word arg);
  static void doLoadClosure(Context* ctx, word arg);
  static void doLoadDeref(Context* ctx, word arg);
  static void doStoreDeref(Context* ctx, word arg);
  static void doDeleteDeref(Context* ctx, word arg);
  static void doCallFunctionKw(Context* ctx, word arg);
  static void doCallFunctionEx(Context* ctx, word arg);
  static void doSetupWith(Context* ctx, word arg);
  static void doListAppend(Context* ctx, word arg);
  static void doSetAdd(Context* ctx, word arg);
  static void doMapAdd(Context* ctx, word arg);
  static void doLoadClassDeref(Context* ctx, word arg);
  static void doBuildListUnpack(Context* ctx, word arg);
  static void doBuildMapUnpack(Context* ctx, word arg);
  static void doBuildMapUnpackWithCall(Context* ctx, word arg);
  static void doBuildTupleUnpack(Context* ctx, word arg);
  static void doBuildSetUnpack(Context* ctx, word arg);
  static void doSetupAsyncWith(Context* ctx, word flags);
  static void doFormatValue(Context* ctx, word arg);
  static void doBuildConstKeyMap(Context* ctx, word arg);
  static void doBuildString(Context* ctx, word arg);
  static void doDeleteAttr(Context* ctx, word arg);

 private:
  // Re-arrange the stack so that it is in "normal" form.
  //
  // This is used when the target of a call or keyword call is not a concrete
  // Function object. It extracts the concrete function object from the
  // callable and re-arranges the stack so that the function is followed by its
  // arguments.
  //
  // Returns the concrete function that should be called. Updates nargs with the
  // number of items added to the stack.
  static RawObject prepareCallableCall(Thread* thread, Frame* frame,
                                       word callable_idx, word* nargs);

  // Common functionality for opcode handlers that dispatch to binary and
  // inplace operations
  static void doBinaryOperation(BinaryOp op, Context* ctx);

  static void doInplaceOperation(BinaryOp op, Context* ctx);

  static void doUnaryOperation(SymbolId selector, Context* ctx);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Interpreter);
};

}  // namespace python
