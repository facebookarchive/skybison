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
  static void raise(Thread* thread, RawObject exc_obj, RawObject cause_obj);

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
  static bool unwind(Thread* thread, Frame* entry_frame);

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
  static bool popBlock(Thread* thread, TryBlock::Why why, RawObject value);

  // Pop from the block stack until a handler that cares about 'return' is
  // found, or the stack is emptied. The return value is meant to be used
  // directly as the return value of an opcode handler (see "Opcode handlers"
  // below for an explanation).
  static bool handleReturn(Thread* thread, RawObject retval,
                           Frame* entry_frame);

  // Pop from the block stack until a handler that cares about 'break' or
  // 'continue' is found.
  static void handleLoopExit(Thread* thread, TryBlock::Why why,
                             RawObject retval);

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
  static Continue doBeforeAsyncWith(Thread* thread, word arg);
  static Continue doBinaryAdd(Thread* thread, word arg);
  static Continue doBinaryAnd(Thread* thread, word arg);
  static Continue doBinaryFloorDivide(Thread* thread, word arg);
  static Continue doBinaryLshift(Thread* thread, word arg);
  static Continue doBinaryMatrixMultiply(Thread* thread, word arg);
  static Continue doBinaryModulo(Thread* thread, word arg);
  static Continue doBinaryMultiply(Thread* thread, word arg);
  static Continue doBinaryOpCached(Thread* thread, word arg);
  static Continue doBinaryOr(Thread* thread, word arg);
  static Continue doBinaryPower(Thread* thread, word arg);
  static Continue doBinaryRshift(Thread* thread, word arg);
  static Continue doBinarySubscr(Thread* thread, word arg);
  static Continue doBinarySubscrCached(Thread* thread, word arg);
  static Continue doBinarySubtract(Thread* thread, word arg);
  static Continue doBinaryTrueDivide(Thread* thread, word arg);
  static Continue doBinaryXor(Thread* thread, word arg);
  static Continue doBuildListUnpack(Thread* thread, word arg);
  static Continue doBuildMap(Thread* thread, word arg);
  static Continue doBuildMapUnpack(Thread* thread, word arg);
  static Continue doBuildMapUnpackWithCall(Thread* thread, word arg);
  static Continue doBuildSet(Thread* thread, word arg);
  static Continue doBuildSetUnpack(Thread* thread, word arg);
  static Continue doBuildTupleUnpack(Thread* thread, word arg);
  static Continue doCallFunction(Thread* thread, word arg);
  static Continue doCallFunctionEx(Thread* thread, word arg);
  static Continue doCallFunctionKw(Thread* thread, word arg);
  static Continue doCallMethod(Thread* thread, word arg);
  static Continue doCompareOp(Thread* thread, word arg);
  static Continue doCompareOpCached(Thread* thread, word arg);
  static Continue doDeleteAttr(Thread* thread, word arg);
  static Continue doDeleteSubscr(Thread* thread, word arg);
  static Continue doEndFinally(Thread* thread, word arg);
  static Continue doForIter(Thread* thread, word arg);
  static Continue doForIterCached(Thread* thread, word arg);
  static Continue doFormatValue(Thread* thread, word arg);
  static Continue doGetAiter(Thread* thread, word arg);
  static Continue doGetAnext(Thread* thread, word arg);
  static Continue doGetAwaitable(Thread* thread, word arg);
  static Continue doGetIter(Thread* thread, word arg);
  static Continue doGetYieldFromIter(Thread* thread, word arg);
  static Continue doImportFrom(Thread* thread, word arg);
  static Continue doImportName(Thread* thread, word arg);
  static Continue doInplaceAdd(Thread* thread, word arg);
  static Continue doInplaceAnd(Thread* thread, word arg);
  static Continue doInplaceFloorDivide(Thread* thread, word arg);
  static Continue doInplaceLshift(Thread* thread, word arg);
  static Continue doInplaceMatrixMultiply(Thread* thread, word arg);
  static Continue doInplaceModulo(Thread* thread, word arg);
  static Continue doInplaceMultiply(Thread* thread, word arg);
  static Continue doInplaceOpCached(Thread* thread, word arg);
  static Continue doInplaceOr(Thread* thread, word arg);
  static Continue doInplacePower(Thread* thread, word arg);
  static Continue doInplaceRshift(Thread* thread, word arg);
  static Continue doInplaceSubtract(Thread* thread, word arg);
  static Continue doInplaceTrueDivide(Thread* thread, word arg);
  static Continue doInplaceXor(Thread* thread, word arg);
  static Continue doInvalidBytecode(Thread* thread, word arg);
  static Continue doLoadAttr(Thread* thread, word arg);
  static Continue doLoadAttrCached(Thread* thread, word arg);
  static Continue doLoadDeref(Thread* thread, word arg);
  static Continue doLoadFast(Thread* thread, word arg);
  static Continue doLoadFastReverse(Thread* thread, word arg);
  static Continue doLoadMethod(Thread* thread, word arg);
  static Continue doLoadMethodCached(Thread* thread, word arg);
  static Continue doLoadName(Thread* thread, word arg);
  static Continue doPopExcept(Thread* thread, word arg);
  static Continue doRaiseVarargs(Thread* thread, word arg);
  static Continue doReturnValue(Thread* thread, word arg);
  static Continue doSetupWith(Thread* thread, word arg);
  static Continue doStoreAttr(Thread* thread, word arg);
  static Continue doStoreAttrCached(Thread* thread, word arg);
  static Continue doStoreSubscr(Thread* thread, word arg);
  static Continue doUnaryInvert(Thread* thread, word arg);
  static Continue doUnaryNegative(Thread* thread, word arg);
  static Continue doUnaryNot(Thread* thread, word arg);
  static Continue doUnaryPositive(Thread* thread, word arg);
  static Continue doUnpackEx(Thread* thread, word arg);
  static Continue doUnpackSequence(Thread* thread, word arg);
  static Continue doWithCleanupFinish(Thread* thread, word arg);
  static Continue doWithCleanupStart(Thread* thread, word arg);
  static Continue doYieldFrom(Thread* thread, word arg);
  static Continue doYieldValue(Thread* thread, word arg);
  static Continue doBreakLoop(Thread* thread, word arg);
  static Continue doBuildConstKeyMap(Thread* thread, word arg);
  static Continue doBuildList(Thread* thread, word arg);
  static Continue doBuildSlice(Thread* thread, word arg);
  static Continue doBuildString(Thread* thread, word arg);
  static Continue doBuildTuple(Thread* thread, word arg);
  static Continue doContinueLoop(Thread* thread, word arg);
  static Continue doDeleteDeref(Thread* thread, word arg);
  static Continue doDeleteFast(Thread* thread, word arg);
  static Continue doDeleteGlobal(Thread* thread, word arg);
  static Continue doDeleteName(Thread* thread, word arg);
  static Continue doDupTop(Thread* thread, word arg);
  static Continue doDupTopTwo(Thread* thread, word arg);
  static Continue doImportStar(Thread* thread, word arg);
  static Continue doJumpAbsolute(Thread* thread, word arg);
  static Continue doJumpForward(Thread* thread, word arg);
  static Continue doJumpIfFalseOrPop(Thread* thread, word arg);
  static Continue doJumpIfTrueOrPop(Thread* thread, word arg);
  static Continue doListAppend(Thread* thread, word arg);
  static Continue doLoadBuildClass(Thread* thread, word arg);
  static Continue doLoadClassDeref(Thread* thread, word arg);
  static Continue doLoadClosure(Thread* thread, word arg);
  static Continue doLoadConst(Thread* thread, word arg);
  static Continue doLoadGlobal(Thread* thread, word arg);
  static Continue doLoadGlobalCached(Thread* thread, word arg);
  static Continue doLoadImmediate(Thread* thread, word arg);
  static Continue doMakeFunction(Thread* thread, word arg);
  static Continue doMapAdd(Thread* thread, word arg);
  static Continue doNop(Thread* thread, word arg);
  static Continue doPopBlock(Thread* thread, word arg);
  static Continue doPopJumpIfFalse(Thread* thread, word arg);
  static Continue doPopJumpIfTrue(Thread* thread, word arg);
  static Continue doPopTop(Thread* thread, word arg);
  static Continue doPrintExpr(Thread* thread, word arg);
  static Continue doRotThree(Thread* thread, word arg);
  static Continue doRotTwo(Thread* thread, word arg);
  static Continue doSetAdd(Thread* thread, word arg);
  static Continue doSetupAnnotations(Thread* thread, word arg);
  static Continue doSetupAsyncWith(Thread* thread, word arg);
  static Continue doSetupExcept(Thread* thread, word arg);
  static Continue doSetupFinally(Thread* thread, word arg);
  static Continue doSetupLoop(Thread* thread, word arg);
  static Continue doStoreAnnotation(Thread* thread, word arg);
  static Continue doStoreDeref(Thread* thread, word arg);
  static Continue doStoreFast(Thread* thread, word arg);
  static Continue doStoreFastReverse(Thread* thread, word arg);
  static Continue doStoreGlobal(Thread* thread, word arg);
  static Continue doStoreGlobalCached(Thread* thread, word arg);
  static Continue doStoreName(Thread* thread, word arg);

  // Common functionality for opcode handlers that dispatch to binary and
  // inplace operations
  static Continue doBinaryOperation(BinaryOp op, Thread* thread);
  static Continue doInplaceOperation(BinaryOp op, Thread* thread);
  static Continue doUnaryOperation(SymbolId selector, Thread* thread);

  // Slow path for the BINARY_SUBSCR opcode that updates the cache at the given
  // index when appropriate. May also be used as a non-caching slow path by
  // passing a negative index.
  static Continue binarySubscrUpdateCache(Thread* thread, word index);

  // Slow path for the FOR_ITER opcode that updates the cache at the given index
  // when appropriate. May also be used as a non-caching slow path by passing a
  // negative index.
  static bool forIterUpdateCache(Thread* thread, word arg, word index);

  // Slow path for isTrue check. Does a __bool__ method call, etc.
  static RawObject isTrueSlowPath(Thread* thread, RawObject value_obj);

  // Perform a method call at the end of an opcode handler. The benefit over
  // using `callMethod1()` is that we avoid recursively starting a new
  // interpreter loop when the target is a python function.
  static Continue tailcallMethod1(Thread* thread, RawObject method,
                                  RawObject self);

  // Call method with 2 parameters at the end of an opcode handler. See
  // `tailcallMethod1()`.
  static Continue tailcallMethod2(Thread* thread, RawObject method,
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
  static Continue handleCall(Thread* thread, word argc, word callable_idx,
                             word num_extra_pop, PrepareCallFunc prepare_args,
                             Function::Entry (RawFunction::*get_entry)() const);

  // Call a function through its trampoline, pushing the result on the stack.
  static Continue callTrampoline(Thread* thread, Function::Entry entry,
                                 word argc, RawObject* post_call_sp);

  // After a callable is prepared and all arguments are processed, push a frame
  // for the callee and update the Context to begin executing it.
  static Frame* pushFrame(Thread* thread, RawFunction function,
                          RawObject* post_call_sp);

  // Pop the current Frame, restoring the execution context of the previous
  // Frame.
  static Frame* popFrame(Thread* thread);

  // Resolve a callable object to a function (resolving `__call__` descriptors
  // as necessary).
  // This is only a helper for the `prepareCallableCall` implementation:
  // `prepareCallableCall` starts out with shortcuts with the common cases and
  // only calls this function for the remaining rare cases with the expectation
  // that this function is not inlined.
  static RawObject prepareCallableCallDunderCall(Thread* thread, Frame* frame,
                                                 word callable_idx,
                                                 word* nargs);

  static Continue loadAttrUpdateCache(Thread* thread, word arg);
  static Continue storeAttrUpdateCache(Thread* thread, word arg);

  using OpcodeHandler = Continue (*)(Thread* thread, word arg);
  using BinopFallbackHandler = Continue (*)(Thread* thread, word arg,
                                            IcBinopFlags flags);
  static Continue cachedBinaryOpImpl(Thread* thread, word arg,
                                     OpcodeHandler update_cache,
                                     BinopFallbackHandler fallback);

  static Continue binaryOpUpdateCache(Thread* thread, word arg);
  static Continue binaryOpFallback(Thread* thread, word arg,
                                   IcBinopFlags flags);
  static Continue compareOpUpdateCache(Thread* thread, word arg);
  static Continue compareOpFallback(Thread* thread, word arg,
                                    IcBinopFlags flags);
  static Continue inplaceOpUpdateCache(Thread* thread, word arg);
  static Continue inplaceOpFallback(Thread* thread, word arg,
                                    IcBinopFlags flags);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Interpreter);
};

}  // namespace python
