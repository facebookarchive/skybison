#pragma once

#include "bytecode.h"
#include "frame.h"
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

  static RawObject isTrue(Thread* thread, Frame* caller, const Object& self);

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
  static RawObject callFunction2(Thread* thread, Frame* caller,
                                 const Object& func, const Object& arg1,
                                 const Object& arg2);
  static RawObject callFunction3(Thread* thread, Frame* caller,
                                 const Object& func, const Object& arg1,
                                 const Object& arg2, const Object& arg3);
  static RawObject callFunction4(Thread* thread, Frame* caller,
                                 const Object& func, const Object& arg1,
                                 const Object& arg2, const Object& arg3,
                                 const Object& arg4);
  static RawObject callFunction5(Thread* thread, Frame* caller,
                                 const Object& func, const Object& arg1,
                                 const Object& arg2, const Object& arg3,
                                 const Object& arg4, const Object& arg5);
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

  static RawObject unaryOperation(Thread* thread, const Object& receiver,
                                  SymbolId selector);

  static RawObject binaryOperation(Thread* thread, Frame* caller, BinaryOp op,
                                   const Object& left, const Object& right);

  static RawObject inplaceOperation(Thread* thread, Frame* caller, BinaryOp op,
                                    const Object& left, const Object& right);

  static RawObject compareOperation(Thread* thread, Frame* caller, CompareOp op,
                                    const Object& left, const Object& right);

  static RawObject sequenceIterSearch(Thread* thread, Frame* caller,
                                      const Object& value,
                                      const Object& container);

  static RawObject sequenceContains(Thread* thread, Frame* caller,
                                    const Object& value,
                                    const Object& container);

  static RawObject makeFunction(Thread* thread, const Object& qualname_str,
                                const Code& code, const Object& closure_tuple,
                                const Object& annotations_dict,
                                const Object& kw_defaults_dict,
                                const Object& defaults_tuple,
                                const Dict& globals, const Dict& builtins);

  struct Context {
    Thread* thread;
    Frame* frame;
    word pc;
  };

  // Process the operands to the RAISE_VARARGS bytecode into a pending exception
  // on ctx->thread.
  static void raise(Context* ctx, RawObject exc, RawObject cause);

  // Unwind the stack for a pending exception. Intended to be tail-called by a
  // bytecode handler that is raising an exception.
  //
  // Returns true if the exception escaped frames owned by the current
  // Interpreter instance, indicating that an Error should be returned to the
  // caller.
  static bool unwind(Context* ctx);

  // Unwind an ExceptHandler from the stack, restoring the previous handler
  // state.
  static void unwindExceptHandler(Thread* thread, Frame* frame, TryBlock block);

  // Pop a block off of the block stack and act appropriately.
  //
  // `why` should indicate the reason for the pop, and must not be
  // Why::kException (which is handled completely within unwind()). For
  // Why::kContinue, `value` should be the opcode's arg as a SmallInt; for
  // Why::kReturn, it should be the value to be returned. It is ignored for
  // other Whys.
  //
  // Returns true if a handler was found and the calling opcode handler should
  // return to the dispatch loop (the "handler" is either a loop for
  // break/continue, or a finally block for break/continue/return). Returns
  // false if the popped block was not relevant to the given Why.
  static bool popBlock(Context* ctx, TryBlock::Why why, const Object& value);

  // Pop from the block stack until a handler that cares about 'return' is
  // found, or the stack is emptied. The return value is meant to be used
  // directly as the return value of an opcode handler (see "Opcode handlers"
  // below for an explanation).
  static bool handleReturn(Context* ctx, const Object& retval);

  // Pop from the block stack until a handler that cares about 'break' or
  // 'continue' is found.
  static void handleLoopExit(Context* ctx, TryBlock::Why why,
                             const Object& retval);

  // Pseudo-opcodes
  static void doInvalidBytecode(Context* ctx, word arg);
  static void doNotImplemented(Context* ctx, word arg);

  // Opcode handlers
  //
  // Handlers that never exit the Frame return void, while those that could
  // return bool.
  //
  // A return value of true means the top Frame owned by this Interpreter is
  // finished. The dispatch loop will pop TOS, pop the Frame, and return the
  // popped value. For raised exceptions, this value will always be Error, and
  // for opcodes like RETURN_VALUE it will be the returned value.
  //
  // A return value of false means execution should continue as normal in the
  // current Frame.
  static void doPopTop(Context* ctx, word arg);
  static void doRotTwo(Context* ctx, word arg);
  static void doRotThree(Context* ctx, word arg);
  static void doDupTop(Context* ctx, word arg);
  static void doDupTopTwo(Context* ctx, word arg);
  static void doNop(Context* ctx, word arg);
  static bool doUnaryPositive(Context* ctx, word arg);
  static bool doUnaryNegative(Context* ctx, word arg);
  static bool doUnaryNot(Context* ctx, word arg);
  static bool doUnaryInvert(Context* ctx, word arg);
  static bool doBinaryMatrixMultiply(Context* ctx, word arg);
  static bool doInplaceMatrixMultiply(Context* ctx, word arg);
  static bool doBinaryPower(Context* ctx, word arg);
  static bool doBinaryMultiply(Context* ctx, word arg);
  static bool doBinaryModulo(Context* ctx, word arg);
  static bool doBinaryAdd(Context* ctx, word arg);
  static bool doBinarySubtract(Context* ctx, word arg);
  static bool doBinarySubscr(Context* ctx, word arg);
  static bool doBinaryFloorDivide(Context* ctx, word arg);
  static bool doBinaryTrueDivide(Context* ctx, word arg);
  static bool doInplaceFloorDivide(Context* ctx, word arg);
  static bool doInplaceTrueDivide(Context* ctx, word arg);
  static bool doGetAiter(Context* ctx, word arg);
  static bool doGetAnext(Context* ctx, word arg);
  static bool doBeforeAsyncWith(Context* ctx, word arg);
  static bool doInplaceAdd(Context* ctx, word arg);
  static bool doInplaceSubtract(Context* ctx, word arg);
  static bool doInplaceMultiply(Context* ctx, word arg);
  static bool doInplaceModulo(Context* ctx, word arg);
  static bool doStoreSubscr(Context* ctx, word arg);
  static bool doDeleteSubscr(Context* ctx, word arg);
  static bool doBinaryLshift(Context* ctx, word arg);
  static bool doBinaryRshift(Context* ctx, word arg);
  static bool doBinaryAnd(Context* ctx, word arg);
  static bool doBinaryXor(Context* ctx, word arg);
  static bool doBinaryOr(Context* ctx, word arg);
  static bool doInplacePower(Context* ctx, word arg);
  static bool doGetIter(Context* ctx, word arg);
  static bool doGetYieldFromIter(Context* ctx, word arg);
  static void doPrintExpr(Context* ctx, word);
  static void doLoadBuildClass(Context* ctx, word arg);
  static bool doYieldFrom(Context* ctx, word arg);
  static bool doGetAwaitable(Context* ctx, word arg);
  static bool doInplaceLshift(Context* ctx, word arg);
  static bool doInplaceRshift(Context* ctx, word arg);
  static bool doInplaceAnd(Context* ctx, word arg);
  static bool doInplaceXor(Context* ctx, word arg);
  static bool doInplaceOr(Context* ctx, word arg);
  static void doBreakLoop(Context* ctx, word arg);
  static bool doWithCleanupStart(Context* ctx, word arg);
  static bool doWithCleanupFinish(Context* ctx, word arg);
  static bool doReturnValue(Context* ctx, word arg);
  static void doImportStar(Context* ctx, word arg);
  static void doSetupAnnotations(Context* ctx, word arg);
  static bool doYieldValue(Context* ctx, word arg);
  static void doPopBlock(Context* ctx, word arg);
  static bool doEndFinally(Context* ctx, word arg);
  static bool doPopExcept(Context* ctx, word arg);
  static void doStoreName(Context* ctx, word arg);
  static void doDeleteName(Context* ctx, word arg);
  static bool doUnpackSequence(Context* ctx, word arg);
  static bool doForIter(Context* ctx, word arg);
  static bool doUnpackEx(Context* ctx, word arg);
  static bool doStoreAttr(Context* ctx, word arg);
  static void doStoreGlobal(Context* ctx, word arg);
  static void doDeleteGlobal(Context* ctx, word arg);
  static void doLoadConst(Context* ctx, word arg);
  static bool doLoadName(Context* ctx, word arg);
  static void doBuildTuple(Context* ctx, word arg);
  static void doBuildList(Context* ctx, word arg);
  static void doBuildSet(Context* ctx, word arg);
  static void doBuildMap(Context* ctx, word arg);
  static bool doLoadAttr(Context* ctx, word arg);
  static bool doCompareOp(Context* ctx, word arg);
  static bool doImportName(Context* ctx, word arg);
  static bool doImportFrom(Context* ctx, word arg);
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
  static bool doLoadFast(Context* ctx, word arg);
  static void doStoreFast(Context* ctx, word arg);
  static void doDeleteFast(Context* ctx, word arg);
  static void doStoreAnnotation(Context* ctx, word arg);
  static bool doRaiseVarargs(Context* ctx, word arg);
  static bool doCallFunction(Context* ctx, word arg);
  static void doMakeFunction(Context* ctx, word arg);
  static void doBuildSlice(Context* ctx, word arg);
  static void doLoadClosure(Context* ctx, word arg);
  static bool doLoadDeref(Context* ctx, word arg);
  static void doStoreDeref(Context* ctx, word arg);
  static void doDeleteDeref(Context* ctx, word arg);
  static bool doCallFunctionKw(Context* ctx, word arg);
  static bool doCallFunctionEx(Context* ctx, word arg);
  static bool doSetupWith(Context* ctx, word arg);
  static void doListAppend(Context* ctx, word arg);
  static void doSetAdd(Context* ctx, word arg);
  static void doMapAdd(Context* ctx, word arg);
  static void doLoadClassDeref(Context* ctx, word arg);
  static bool doBuildListUnpack(Context* ctx, word arg);
  static bool doBuildMapUnpack(Context* ctx, word arg);
  static bool doBuildMapUnpackWithCall(Context* ctx, word arg);
  static bool doBuildTupleUnpack(Context* ctx, word arg);
  static bool doBuildSetUnpack(Context* ctx, word arg);
  static void doSetupAsyncWith(Context* ctx, word flags);
  static bool doFormatValue(Context* ctx, word arg);
  static void doBuildConstKeyMap(Context* ctx, word arg);
  static void doBuildString(Context* ctx, word arg);
  static bool doDeleteAttr(Context* ctx, word arg);

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
  static bool doBinaryOperation(BinaryOp op, Context* ctx);

  static bool doInplaceOperation(BinaryOp op, Context* ctx);

  static bool doUnaryOperation(SymbolId selector, Context* ctx);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Interpreter);
};

}  // namespace python
