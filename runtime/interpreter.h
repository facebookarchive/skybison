#pragma once

#include "bytecode.h"
#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "ic.h"
#include "symbols.h"
#include "trampolines.h"

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

  enum class Continue {
    NEXT,
    UNWIND,
    RETURN,
    YIELD,
  };

  // Interpreter-internal execution state, containing the information necessary
  // for running bytecode.
  struct Context {
    // The current thread.
    Thread* thread;

    // The frame currently being executed. Unless there is another interpreter
    // nested below this one, and except for a brief window during calls and
    // returns, this is the same as thread->currentFrame().
    Frame* frame;

    DISALLOW_HEAP_ALLOCATION();
  };

  static RawObject execute(Thread* thread, Frame* entry_frame,
                           const Function& function);

  static RawObject call(Thread* thread, Frame* frame, word nargs);
  static RawObject callKw(Thread* thread, Frame* frame, word nargs);
  static RawObject callEx(Thread* thread, Frame* frame, word flags);

  // batch concat/join <num> string objects on the stack (no conversion)
  static RawObject stringJoin(Thread* thread, RawObject* sp, word num);

  static bool isCacheEnabledForCurrentFunction(Frame* frame) {
    return frame->caches().length() > 0;
  }

  static RawObject isTrue(Thread* thread, RawObject value_obj);

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
  static RawObject callFunction6(Thread* thread, Frame* caller,
                                 const Object& func, const Object& arg1,
                                 const Object& arg2, const Object& arg3,
                                 const Object& arg4, const Object& arg5,
                                 const Object& arg6);
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

  // Prepare the stack to for a positional or keyword call by normalizing the
  // callable object using prepareCallableObject().
  //
  // Returns the concrete Function that should be called. Updates nargs if a
  // self object was unpacked from the callable and inserted into the stack.
  //
  // Not intended for public use; only here for testing purposes.
  static RawObject prepareCallableCall(Thread* thread, Frame* frame,
                                       word callable_idx, word* nargs);

  static RawObject unaryOperation(Thread* thread, const Object& self,
                                  SymbolId selector);

  static RawObject binaryOperation(Thread* thread, Frame* caller, BinaryOp op,
                                   const Object& left, const Object& right);

  // Lookup and invoke a binary operation (like `__add__`, `__sub__`, ...).
  // Sets `method_out` and `flags_out` to the lookup result if it is possible
  // to cache it.
  static RawObject binaryOperationSetMethod(Thread* thread, Frame* caller,
                                            BinaryOp op, const Object& left,
                                            const Object& right,
                                            Object* method_out,
                                            IcBinopFlags* flags_out);

  // Calls a previously cached binary operation. Note that the caller still
  // needs to check for a `NotImplemented` result and call
  // `binaryOperationRetry()` if necessary.
  static RawObject binaryOperationWithMethod(Thread* thread, Frame* caller,
                                             RawObject method,
                                             IcBinopFlags flags, RawObject left,
                                             RawObject right);

  // Calls the normal binary operation if `flags` has the `IC_BINOP_REFLECTED`
  // and the `IC_BINOP_NOTIMPLEMENTED_RETRY` bits are set; calls the reflected
  // operation if just `IC_BINOP_NOTIMPLEMENTED_RETRY` is set. Raises an error
  // if any of the two operations raised `NotImplemented` or none was called.
  //
  // This represents the second half of the binary operation calling mechanism
  // after we attempted a first lookup and call. It is a separate function so we
  // can use it independently of the first lookup using inline caching.
  static RawObject binaryOperationRetry(Thread* thread, Frame* caller,
                                        BinaryOp op, IcBinopFlags flags,
                                        const Object& left,
                                        const Object& right);

  static RawObject inplaceOperation(Thread* thread, Frame* caller, BinaryOp op,
                                    const Object& left, const Object& right);

  static RawObject inplaceOperationSetMethod(Thread* thread, Frame* caller,
                                             BinaryOp op, const Object& left,
                                             const Object& right,
                                             Object* method_out,
                                             IcBinopFlags* flags_out);

  static RawObject compareOperationRetry(Thread* thread, Frame* caller,
                                         CompareOp op, IcBinopFlags flags,
                                         const Object& left,
                                         const Object& right);

  static RawObject compareOperationSetMethod(Thread* thread, Frame* caller,
                                             CompareOp op, const Object& left,
                                             const Object& right,
                                             Object* method_out,
                                             IcBinopFlags* flags_out);

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
                                const Dict& globals);

  static RawObject loadAttrWithLocation(Thread* thread, RawObject receiver,
                                        RawObject location);
  static RawObject loadAttrSetLocation(Thread* thread, const Object& object,
                                       const Object& name_str,
                                       Object* location_out);

  // Process the operands to the RAISE_VARARGS bytecode into a pending exception
  // on ctx->thread.
  static void raise(Context* ctx, RawObject exc_obj, RawObject cause_obj);

  static RawObject storeAttrSetLocation(Thread* thread, const Object& object,
                                        const Object& name_str,
                                        const Object& value,
                                        Object* location_out);
  static void storeAttrWithLocation(Thread* thread, RawObject receiver,
                                    RawObject location, RawObject value);

  // Unwind the stack for a pending exception. Intended to be tail-called by a
  // bytecode handler that is raising an exception.
  //
  // Returns true if the exception escaped frames owned by the current
  // Interpreter instance, indicating that an Error should be returned to the
  // caller.
  static bool unwind(Context* ctx, Frame* entry_frame);

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
  static bool popBlock(Context* ctx, TryBlock::Why why, RawObject value);

  // Pop from the block stack until a handler that cares about 'return' is
  // found, or the stack is emptied. The return value is meant to be used
  // directly as the return value of an opcode handler (see "Opcode handlers"
  // below for an explanation).
  static bool handleReturn(Context* ctx, RawObject retval, Frame* entry_frame);

  // Pop from the block stack until a handler that cares about 'break' or
  // 'continue' is found.
  static void handleLoopExit(Context* ctx, TryBlock::Why why, RawObject retval);

 private:
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
  static Continue doBeforeAsyncWith(Context* ctx, word arg);
  static Continue doBinaryAdd(Context* ctx, word arg);
  static Continue doBinaryAnd(Context* ctx, word arg);
  static Continue doBinaryFloorDivide(Context* ctx, word arg);
  static Continue doBinaryLshift(Context* ctx, word arg);
  static Continue doBinaryMatrixMultiply(Context* ctx, word arg);
  static Continue doBinaryModulo(Context* ctx, word arg);
  static Continue doBinaryMultiply(Context* ctx, word arg);
  static Continue doBinaryOpCached(Context* ctx, word arg);
  static Continue doBinaryOr(Context* ctx, word arg);
  static Continue doBinaryPower(Context* ctx, word arg);
  static Continue doBinaryRshift(Context* ctx, word arg);
  static Continue doBinarySubscr(Context* ctx, word arg);
  static Continue doBinarySubscrCached(Context* ctx, word arg);
  static Continue doBinarySubtract(Context* ctx, word arg);
  static Continue doBinaryTrueDivide(Context* ctx, word arg);
  static Continue doBinaryXor(Context* ctx, word arg);
  static Continue doBuildListUnpack(Context* ctx, word arg);
  static Continue doBuildMap(Context* ctx, word arg);
  static Continue doBuildMapUnpack(Context* ctx, word arg);
  static Continue doBuildMapUnpackWithCall(Context* ctx, word arg);
  static Continue doBuildSet(Context* ctx, word arg);
  static Continue doBuildSetUnpack(Context* ctx, word arg);
  static Continue doBuildTupleUnpack(Context* ctx, word arg);
  static Continue doCallFunction(Context* ctx, word arg);
  static Continue doCallFunctionEx(Context* ctx, word arg);
  static Continue doCallFunctionKw(Context* ctx, word arg);
  static Continue doCallMethod(Context* ctx, word arg);
  static Continue doCompareOp(Context* ctx, word arg);
  static Continue doCompareOpCached(Context* ctx, word arg);
  static Continue doDeleteAttr(Context* ctx, word arg);
  static Continue doDeleteSubscr(Context* ctx, word arg);
  static Continue doEndFinally(Context* ctx, word arg);
  static Continue doForIter(Context* ctx, word arg);
  static Continue doForIterCached(Context* ctx, word arg);
  static Continue doFormatValue(Context* ctx, word arg);
  static Continue doGetAiter(Context* ctx, word arg);
  static Continue doGetAnext(Context* ctx, word arg);
  static Continue doGetAwaitable(Context* ctx, word arg);
  static Continue doGetIter(Context* ctx, word arg);
  static Continue doGetYieldFromIter(Context* ctx, word arg);
  static Continue doImportFrom(Context* ctx, word arg);
  static Continue doImportName(Context* ctx, word arg);
  static Continue doInplaceAdd(Context* ctx, word arg);
  static Continue doInplaceAnd(Context* ctx, word arg);
  static Continue doInplaceFloorDivide(Context* ctx, word arg);
  static Continue doInplaceLshift(Context* ctx, word arg);
  static Continue doInplaceMatrixMultiply(Context* ctx, word arg);
  static Continue doInplaceModulo(Context* ctx, word arg);
  static Continue doInplaceMultiply(Context* ctx, word arg);
  static Continue doInplaceOpCached(Context* ctx, word arg);
  static Continue doInplaceOr(Context* ctx, word arg);
  static Continue doInplacePower(Context* ctx, word arg);
  static Continue doInplaceRshift(Context* ctx, word arg);
  static Continue doInplaceSubtract(Context* ctx, word arg);
  static Continue doInplaceTrueDivide(Context* ctx, word arg);
  static Continue doInplaceXor(Context* ctx, word arg);
  static Continue doInvalidBytecode(Context* ctx, word arg);
  static Continue doLoadAttr(Context* ctx, word arg);
  static Continue doLoadAttrCached(Context* ctx, word arg);
  static Continue doLoadDeref(Context* ctx, word arg);
  static Continue doLoadFast(Context* ctx, word arg);
  static Continue doLoadFastReverse(Context* ctx, word arg);
  static Continue doLoadMethod(Context* ctx, word arg);
  static Continue doLoadMethodCached(Context* ctx, word arg);
  static Continue doLoadName(Context* ctx, word arg);
  static Continue doPopExcept(Context* ctx, word arg);
  static Continue doRaiseVarargs(Context* ctx, word arg);
  static Continue doReturnValue(Context* ctx, word arg);
  static Continue doSetupWith(Context* ctx, word arg);
  static Continue doStoreAttr(Context* ctx, word arg);
  static Continue doStoreAttrCached(Context* ctx, word arg);
  static Continue doStoreSubscr(Context* ctx, word arg);
  static Continue doUnaryInvert(Context* ctx, word arg);
  static Continue doUnaryNegative(Context* ctx, word arg);
  static Continue doUnaryNot(Context* ctx, word arg);
  static Continue doUnaryPositive(Context* ctx, word arg);
  static Continue doUnpackEx(Context* ctx, word arg);
  static Continue doUnpackSequence(Context* ctx, word arg);
  static Continue doWithCleanupFinish(Context* ctx, word arg);
  static Continue doWithCleanupStart(Context* ctx, word arg);
  static Continue doYieldFrom(Context* ctx, word arg);
  static Continue doYieldValue(Context* ctx, word arg);
  static Continue doBreakLoop(Context* ctx, word arg);
  static Continue doBuildConstKeyMap(Context* ctx, word arg);
  static Continue doBuildList(Context* ctx, word arg);
  static Continue doBuildSlice(Context* ctx, word arg);
  static Continue doBuildString(Context* ctx, word arg);
  static Continue doBuildTuple(Context* ctx, word arg);
  static Continue doContinueLoop(Context* ctx, word arg);
  static Continue doDeleteDeref(Context* ctx, word arg);
  static Continue doDeleteFast(Context* ctx, word arg);
  static Continue doDeleteGlobal(Context* ctx, word arg);
  static Continue doDeleteName(Context* ctx, word arg);
  static Continue doDupTop(Context* ctx, word arg);
  static Continue doDupTopTwo(Context* ctx, word arg);
  static Continue doImportStar(Context* ctx, word arg);
  static Continue doJumpAbsolute(Context* ctx, word arg);
  static Continue doJumpForward(Context* ctx, word arg);
  static Continue doJumpIfFalseOrPop(Context* ctx, word arg);
  static Continue doJumpIfTrueOrPop(Context* ctx, word arg);
  static Continue doListAppend(Context* ctx, word arg);
  static Continue doLoadBuildClass(Context* ctx, word arg);
  static Continue doLoadClassDeref(Context* ctx, word arg);
  static Continue doLoadClosure(Context* ctx, word arg);
  static Continue doLoadConst(Context* ctx, word arg);
  static Continue doLoadGlobal(Context* ctx, word arg);
  static Continue doLoadGlobalCached(Context* ctx, word arg);
  static Continue doLoadImmediate(Context* ctx, word arg);
  static Continue doMakeFunction(Context* ctx, word arg);
  static Continue doMapAdd(Context* ctx, word arg);
  static Continue doNop(Context* ctx, word arg);
  static Continue doPopBlock(Context* ctx, word arg);
  static Continue doPopJumpIfFalse(Context* ctx, word arg);
  static Continue doPopJumpIfTrue(Context* ctx, word arg);
  static Continue doPopTop(Context* ctx, word arg);
  static Continue doPrintExpr(Context* ctx, word arg);
  static Continue doRotThree(Context* ctx, word arg);
  static Continue doRotTwo(Context* ctx, word arg);
  static Continue doSetAdd(Context* ctx, word arg);
  static Continue doSetupAnnotations(Context* ctx, word arg);
  static Continue doSetupAsyncWith(Context* ctx, word arg);
  static Continue doSetupExcept(Context* ctx, word arg);
  static Continue doSetupFinally(Context* ctx, word arg);
  static Continue doSetupLoop(Context* ctx, word arg);
  static Continue doStoreAnnotation(Context* ctx, word arg);
  static Continue doStoreDeref(Context* ctx, word arg);
  static Continue doStoreFast(Context* ctx, word arg);
  static Continue doStoreFastReverse(Context* ctx, word arg);
  static Continue doStoreGlobal(Context* ctx, word arg);
  static Continue doStoreGlobalCached(Context* ctx, word arg);
  static Continue doStoreName(Context* ctx, word arg);

  // Common functionality for opcode handlers that dispatch to binary and
  // inplace operations
  static Continue doBinaryOperation(BinaryOp op, Context* ctx);
  static Continue doInplaceOperation(BinaryOp op, Context* ctx);
  static Continue doUnaryOperation(SymbolId selector, Context* ctx);

  // Slow path for the BINARY_SUBSCR opcode that updates the cache at the given
  // index when appropriate. May also be used as a non-caching slow path by
  // passing a negative index.
  static Continue binarySubscrUpdateCache(Context* ctx, word index);

  // Slow path for the FOR_ITER opcode that updates the cache at the given index
  // when appropriate. May also be used as a non-caching slow path by passing a
  // negative index.
  static bool forIterUpdateCache(Context* ctx, word arg, word index);

  // Slow path for isTrue check. Does a __bool__ method call, etc.
  static RawObject isTrueSlowPath(Thread* thread, RawObject value_obj);

  // Perform a method call at the end of an opcode handler. The benefit over
  // using `callMethod1()` is that we avoid recursively starting a new
  // interpreter loop when the target is a python function.
  static Continue tailcallMethod1(Context* ctx, RawObject method,
                                  RawObject self);

  // Call method with 2 parameters at the end of an opcode handler. See
  // `tailcallMethod1()`.
  static Continue tailcallMethod2(Context* ctx, RawObject method,
                                  RawObject self, RawObject arg1);

  // Given a non-Function object in `callable`, attempt to normalize it to a
  // Function by either unpacking a BoundMethod or looking up the object's
  // __call__ method, iterating multiple times if necessary.
  //
  // On success, `callable` will contain the Function to call, and the return
  // value will be a bool indicating whether or not `self` was populated with an
  // object unpacked from a BoundMethod.
  //
  // On failure, Error is returned and `callable` may have been modified.
  static RawObject prepareCallable(Thread* thread, Frame* frame,
                                   Object* callable, Object* self);

  // Prepare the stack for an explode call by normalizing the callable object
  // using prepareCallableObject().
  //
  // Returns the concrete Function that should be called.
  static RawObject prepareCallableEx(Thread* thread, Frame* frame,
                                     word callable_idx);

  // Perform a positional or keyword call. Used by doCallFunction() and
  // doCallFunctionKw().
  static Continue handleCall(Context* ctx, word argc, word callable_idx,
                             word num_extra_pop, PrepareCallFunc prepare_args,
                             Function::Entry (RawFunction::*get_entry)() const);

  // Call a function through its trampoline, pushing the result on the stack.
  static Continue callTrampoline(Context* ctx, Function::Entry entry, word argc,
                                 RawObject* post_call_sp);

  // After a callable is prepared and all arguments are processed, push a frame
  // for the callee and update the Context to begin executing it.
  static Frame* pushFrame(Context* ctx, RawFunction function,
                          RawObject* post_call_sp);

  // Pop the current Frame, restoring the execution context of the previous
  // Frame.
  static void popFrame(Context* ctx);

  // Resolve a callable object to a function (resolving `__call__` descriptors
  // as necessary).
  // This is only a helper for the `prepareCallableCall` implementation:
  // `prepareCallableCall` starts out with shortcuts with the common cases and
  // only calls this function for the remaining rare cases with the expectation
  // that this function is not inlined.
  static RawObject prepareCallableCallDunderCall(Thread* thread, Frame* frame,
                                                 word callable_idx,
                                                 word* nargs);

  static Continue loadAttrUpdateCache(Context* ctx, word arg);
  static Continue storeAttrUpdateCache(Context* ctx, word arg);

  using OpcodeHandler = Continue (*)(Context* ctx, word arg);
  using BinopFallbackHandler = Continue (*)(Context* ctx, word arg,
                                            IcBinopFlags flags);
  static Continue cachedBinaryOpImpl(Context* ctx, word arg,
                                     OpcodeHandler update_cache,
                                     BinopFallbackHandler fallback);

  static Continue binaryOpUpdateCache(Context* ctx, word arg);
  static Continue binaryOpFallback(Context* ctx, word arg, IcBinopFlags flags);
  static Continue compareOpUpdateCache(Context* ctx, word arg);
  static Continue compareOpFallback(Context* ctx, word arg, IcBinopFlags flags);
  static Continue inplaceOpUpdateCache(Context* ctx, word arg);
  static Continue inplaceOpFallback(Context* ctx, word arg, IcBinopFlags flags);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Interpreter);
};

}  // namespace python
