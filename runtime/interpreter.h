#pragma once

#include "bytecode.h"
#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "symbols.h"
#include "trampolines.h"

namespace py {

class RawObject;
class Frame;
class Thread;

// Bitset indicating how a cached binary operation needs to be called.
enum BinaryOpFlags : uint8_t {
  kBinaryOpNone = 0,
  // Swap arguments when calling the method.
  kBinaryOpReflected = 1 << 0,
  // Retry alternative method when method returns `NotImplemented`.  Should try
  // the non-reflected op if the `kBinaryOpReflected` flag is set and vice
  // versa.
  kBinaryOpNotImplementedRetry = 1 << 1,
  // This flag is set when the cached method is an in-place operation (such as
  // __iadd__).
  kInplaceBinaryOpRetry = 1 << 2,
};

// Different caching strategies for the resolved location of a LOAD_ATTR
enum class LoadAttrKind {
  kInstanceOffset = 1,
  kInstanceFunction,
  kInstanceProperty,
  kInstanceSlotDescr,
  kInstanceType,
  kInstanceTypeDescr,
  kModule,
  kType,
  kUnknown,
};

enum class ICState {
  kAnamorphic,
  kMonomorphic,
  kPolymorphic,
};

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
  static const word kNumContinues = 4;

  using OpcodeHandler = Continue (*)(Thread* thread, word arg);

  virtual ~Interpreter();
  // Initialize per-thread interpreter state. Must be called on each thread
  // before the interpreter can execute code on it.
  virtual void setupThread(Thread* thread) = 0;

  virtual void* entryAsm(const Function& function) = 0;

  virtual void setOpcodeCounting(bool enabled) = 0;

  static RawObject execute(Thread* thread);
  static RawObject resumeGenerator(Thread* thread,
                                   const GeneratorBase& generator,
                                   const Object& send_value);
  static RawObject resumeGeneratorWithRaise(Thread* thread,
                                            const GeneratorBase& generator,
                                            const Object& type,
                                            const Object& value,
                                            const Object& traceback);

  static RawObject call(Thread* thread, word nargs);
  static RawObject callEx(Thread* thread, word flags);
  static RawObject callKw(Thread* thread, word nargs);

  // Perform positional call of function `function`. Does not work for arbitrary
  // callables objects; that case requires `call`.
  static RawObject callFunction(Thread* thread, word nargs, RawObject function);

  // Calls __hash__ on `value`, checks result and postprocesses.  Returns a
  // SmallInt or Error::exception().
  static RawObject hash(Thread* thread, const Object& value);

  // batch concat/join <num> string objects on the stack (no conversion)
  static RawObject stringJoin(Thread* thread, RawObject* sp, word num);

  static RawObject isTrue(Thread* thread, RawObject value_obj);

  static RawObject callDescriptorGet(Thread* thread, const Object& descriptor,
                                     const Object& receiver,
                                     const Object& receiver_type);

  static RawObject callDescriptorSet(Thread* thread, const Object& descriptor,
                                     const Object& receiver,
                                     const Object& value);

  static RawObject callDescriptorDelete(Thread* thread,
                                        const Object& descriptor,
                                        const Object& receiver);

  static RawObject lookupMethod(Thread* thread, const Object& receiver,
                                SymbolId selector);

  static RawObject call0(Thread* thread, const Object& callable);
  static RawObject call1(Thread* thread, const Object& callable,
                         const Object& arg1);
  static RawObject call2(Thread* thread, const Object& callable,
                         const Object& arg1, const Object& arg2);
  static RawObject call3(Thread* thread, const Object& callable,
                         const Object& arg1, const Object& arg2,
                         const Object& arg3);
  static RawObject call4(Thread* thread, const Object& callable,
                         const Object& arg1, const Object& arg2,
                         const Object& arg3, const Object& arg4);
  static RawObject call5(Thread* thread, const Object& callable,
                         const Object& arg1, const Object& arg2,
                         const Object& arg3, const Object& arg4,
                         const Object& arg5);
  static RawObject call6(Thread* thread, const Object& callable,
                         const Object& arg1, const Object& arg2,
                         const Object& arg3, const Object& arg4,
                         const Object& arg5, const Object& arg6);

  static RawObject callMethod1(Thread* thread, const Object& method,
                               const Object& self);

  static RawObject callMethod2(Thread* thread, const Object& method,
                               const Object& self, const Object& other);

  static RawObject callMethod3(Thread* thread, const Object& method,
                               const Object& self, const Object& arg1,
                               const Object& arg2);

  static RawObject callMethod4(Thread* thread, const Object& method,
                               const Object& self, const Object& arg1,
                               const Object& arg2, const Object& arg3);

  struct PrepareCallableResult {
    RawObject function;
    word nargs;
  };

  // Prepare the stack to for a positional or keyword call by normalizing the
  // callable object using prepareCallableObject().
  //
  // Returns the concrete Function that should be called. Updates nargs if a
  // self object was unpacked from the callable and inserted into the stack.
  //
  // Not intended for public use; only here for testing purposes.
  static PrepareCallableResult prepareCallableCall(Thread* thread, word nargs,
                                                   word callable_idx);

  static RawObject unaryOperation(Thread* thread, const Object& self,
                                  SymbolId selector);

  static RawObject binaryOperation(Thread* thread, BinaryOp op,
                                   const Object& left, const Object& right);
  static Continue binaryOpUpdateCache(Thread* thread, word arg);

  // Lookup and invoke a binary operation (like `__add__`, `__sub__`, ...).
  // Sets `method_out` and `flags_out` to the lookup result if it is possible
  // to cache it.
  static RawObject binaryOperationSetMethod(Thread* thread, BinaryOp op,
                                            const Object& left,
                                            const Object& right,
                                            Object* method_out,
                                            BinaryOpFlags* flags_out);

  // Calls a previously cached binary operation. Note that the caller still
  // needs to check for a `NotImplemented` result and call
  // `binaryOperationRetry()` if necessary.
  static RawObject binaryOperationWithMethod(Thread* thread, RawObject method,
                                             BinaryOpFlags flags,
                                             RawObject left, RawObject right);

  // Calls the normal binary operation if `flags` has the `kBinaryOpReflected`
  // and the `kBinaryOpNotImplementedRetry` bits are set; calls the reflected
  // operation if just `kBinaryOpNotImplementedRetry` is set. Raises an error
  // if any of the two operations raised `NotImplemented` or none was called.
  //
  // This represents the second half of the binary operation calling mechanism
  // after we attempted a first lookup and call. It is a separate function so we
  // can use it independently of the first lookup using inline caching.
  static RawObject binaryOperationRetry(Thread* thread, BinaryOp op,
                                        BinaryOpFlags flags, const Object& left,
                                        const Object& right);

  // Slow path for the BINARY_SUBSCR opcode that updates the cache at the given
  // index when appropriate. May also be used as a non-caching slow path by
  // passing a negative index.
  static Continue binarySubscrUpdateCache(Thread* thread, word index);

  // Slow path for the STORE_SUBSCR opcode that updates the cache at the given
  // index when appropriate. May also be used as a non-caching slow path by
  // passing a negative index.
  static Continue storeSubscrUpdateCache(Thread* thread, word arg);

  static Continue compareInUpdateCache(Thread* thread, word arg);

  static RawObject inplaceOperation(Thread* thread, BinaryOp op,
                                    const Object& left, const Object& right);
  static Continue inplaceOpUpdateCache(Thread* thread, word arg);

  static RawObject inplaceOperationSetMethod(Thread* thread, BinaryOp op,
                                             const Object& left,
                                             const Object& right,
                                             Object* method_out,
                                             BinaryOpFlags* flags_out);

  static RawObject compareOperationRetry(Thread* thread, CompareOp op,
                                         BinaryOpFlags flags,
                                         const Object& left,
                                         const Object& right);

  static RawObject compareOperationSetMethod(Thread* thread, CompareOp op,
                                             const Object& left,
                                             const Object& right,
                                             Object* method_out,
                                             BinaryOpFlags* flags_out);

  static RawObject compareOperation(Thread* thread, CompareOp op,
                                    const Object& left, const Object& right);
  static Continue compareOpUpdateCache(Thread* thread, word arg);

  static RawObject createIterator(Thread* thread, const Object& iterable);

  static RawObject sequenceIterSearch(Thread* thread, const Object& value,
                                      const Object& container);

  static RawObject sequenceContains(Thread* thread, const Object& value,
                                    const Object& container);

  static RawObject sequenceContainsSetMethod(Thread* thread,
                                             const Object& value,
                                             const Object& container,
                                             Object* method_out);

  static RawObject loadAttrWithLocation(Thread* thread, RawObject receiver,
                                        RawObject location);

  static RawObject importAllFrom(Thread* thread, Frame* frame,
                                 const Object& object);

  // Process the operands to the RAISE_VARARGS bytecode into a pending exception
  // on ctx->thread.
  static void raise(Thread* thread, RawObject exc_obj, RawObject cause_obj);

  static RawObject storeAttrSetLocation(Thread* thread, const Object& object,
                                        const Object& name, const Object& value,
                                        Object* location_out);
  static void storeAttrWithLocation(Thread* thread, RawObject receiver,
                                    RawObject location, RawObject value);

  // Unwind the stack for a pending exception. A return value that is not
  // `Error::error()` indicates that we should exit the interpreter loop and
  // return that value.
  static RawObject unwind(Thread* thread);

  // Unwind an ExceptHandler from the stack, restoring the previous handler
  // state.
  static void unwindExceptHandler(Thread* thread, TryBlock block);

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
  // found, or the stack is emptied. A return value that is not `Error::error()`
  // indicates that we should exit the interpreter loop and return that value.
  static RawObject handleReturn(Thread* thread);

  // Pop from the block stack until a handler that cares about 'break' or
  // 'continue' is found.
  static void handleLoopExit(Thread* thread, TryBlock::Why why,
                             RawObject retval);

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
  static Continue doBeginFinally(Thread* thread, word arg);
  static Continue doBinaryAdd(Thread* thread, word arg);
  static Continue doBinaryAnd(Thread* thread, word arg);
  static Continue doBinaryFloorDivide(Thread* thread, word arg);
  static Continue doBinaryLshift(Thread* thread, word arg);
  static Continue doBinaryMatrixMultiply(Thread* thread, word arg);
  static Continue doBinaryModulo(Thread* thread, word arg);
  static Continue doBinaryMultiply(Thread* thread, word arg);
  static Continue doBinaryOpMonomorphic(Thread* thread, word arg);
  static Continue doBinaryOpPolymorphic(Thread* thread, word arg);
  static Continue doBinaryAddSmallInt(Thread* thread, word arg);
  static Continue doBinaryAndSmallInt(Thread* thread, word arg);
  static Continue doBinaryMulSmallInt(Thread* thread, word arg);
  static Continue doBinaryFloordivSmallInt(Thread* thread, word arg);
  static Continue doBinarySubSmallInt(Thread* thread, word arg);
  static Continue doBinaryOrSmallInt(Thread* thread, word arg);
  static Continue doBinaryOpAnamorphic(Thread* thread, word arg);
  static Continue doBinaryOr(Thread* thread, word arg);
  static Continue doBinaryPower(Thread* thread, word arg);
  static Continue doBinaryRshift(Thread* thread, word arg);
  static Continue doBinarySubscr(Thread* thread, word arg);
  static Continue doBinarySubscrDict(Thread* thread, word arg);
  static Continue doBinarySubscrList(Thread* thread, word arg);
  static Continue doBinarySubscrTuple(Thread* thread, word arg);
  static Continue doBinarySubscrMonomorphic(Thread* thread, word arg);
  static Continue doBinarySubscrPolymorphic(Thread* thread, word arg);
  static Continue doBinarySubscrAnamorphic(Thread* thread, word arg);
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
  static Continue doCompareInAnamorphic(Thread* thread, word arg);
  static Continue doCompareInStr(Thread* thread, word arg);
  static Continue doCompareInTuple(Thread* thread, word arg);
  static Continue doCompareInDict(Thread* thread, word arg);
  static Continue doCompareInList(Thread* thread, word arg);
  static Continue doCompareInMonomorphic(Thread* thread, word arg);
  static Continue doCompareInPolymorphic(Thread* thread, word arg);
  static Continue doCompareIs(Thread* thread, word arg);
  static Continue doCompareIsNot(Thread* thread, word arg);
  static Continue doCompareOp(Thread* thread, word arg);
  static Continue doCompareOpMonomorphic(Thread* thread, word arg);
  static Continue doCompareOpPolymorphic(Thread* thread, word arg);
  static Continue doCompareEqSmallInt(Thread* thread, word arg);
  static Continue doCompareGtSmallInt(Thread* thread, word arg);
  static Continue doCompareLtSmallInt(Thread* thread, word arg);
  static Continue doCompareGeSmallInt(Thread* thread, word arg);
  static Continue doCompareNeSmallInt(Thread* thread, word arg);
  static Continue doCompareLeSmallInt(Thread* thread, word arg);
  static Continue doCompareEqStr(Thread* thread, word arg);
  static Continue doCompareNeStr(Thread* thread, word arg);
  static Continue doCompareOpAnamorphic(Thread* thread, word arg);
  static Continue doDeleteAttr(Thread* thread, word arg);
  static Continue doDeleteSubscr(Thread* thread, word arg);
  static Continue doEndFinally(Thread* thread, word arg);
  static Continue doForIter(Thread* thread, word arg);
  static Continue doForIterAnamorphic(Thread* thread, word arg);
  static Continue doForIterDict(Thread* thread, word arg);
  static Continue doForIterGenerator(Thread* thread, word arg);
  static Continue doForIterList(Thread* thread, word arg);
  static Continue doForIterMonomorphic(Thread* thread, word arg);
  static Continue doForIterPolymorphic(Thread* thread, word arg);
  static Continue doForIterRange(Thread* thread, word arg);
  static Continue doForIterStr(Thread* thread, word arg);
  static Continue doForIterTuple(Thread* thread, word arg);
  static Continue doForIterUncached(Thread* thread, word arg);
  static Continue doFormatValue(Thread* thread, word arg);
  static Continue doGetAiter(Thread* thread, word arg);
  static Continue doGetAnext(Thread* thread, word arg);
  static Continue doGetAwaitable(Thread* thread, word arg);
  static Continue doGetIter(Thread* thread, word arg);
  static Continue doGetYieldFromIter(Thread* thread, word arg);
  static Continue doImportFrom(Thread* thread, word arg);
  static Continue doImportName(Thread* thread, word arg);
  static Continue doInplaceAdd(Thread* thread, word arg);
  static Continue doInplaceAddSmallInt(Thread* thread, word arg);
  static Continue doInplaceAnd(Thread* thread, word arg);
  static Continue doInplaceFloorDivide(Thread* thread, word arg);
  static Continue doInplaceLshift(Thread* thread, word arg);
  static Continue doInplaceMatrixMultiply(Thread* thread, word arg);
  static Continue doInplaceModulo(Thread* thread, word arg);
  static Continue doInplaceMultiply(Thread* thread, word arg);
  static Continue doInplaceOpMonomorphic(Thread* thread, word arg);
  static Continue doInplaceOpPolymorphic(Thread* thread, word arg);
  static Continue doInplaceOpAnamorphic(Thread* thread, word arg);
  static Continue doInplaceOr(Thread* thread, word arg);
  static Continue doInplacePower(Thread* thread, word arg);
  static Continue doInplaceRshift(Thread* thread, word arg);
  static Continue doInplaceSubtract(Thread* thread, word arg);
  static Continue doInplaceSubSmallInt(Thread* thread, word arg);
  static Continue doInplaceTrueDivide(Thread* thread, word arg);
  static Continue doInplaceXor(Thread* thread, word arg);
  static Continue doInvalidBytecode(Thread* thread, word arg);
  static Continue doLoadAttr(Thread* thread, word arg);
  static Continue doLoadAttrAnamorphic(Thread* thread, word arg);
  static Continue doLoadAttrInstance(Thread* thread, word arg);
  static Continue doLoadAttrInstanceTypeBoundMethod(Thread* thread, word arg);
  static Continue doLoadAttrInstanceProperty(Thread* thread, word arg);
  static Continue doLoadAttrInstanceSlotDescr(Thread* thread, word arg);
  static Continue doLoadAttrInstanceType(Thread* thread, word arg);
  static Continue doLoadAttrInstanceTypeDescr(Thread* thread, word arg);
  static Continue doLoadAttrModule(Thread* thread, word arg);
  static Continue doLoadAttrPolymorphic(Thread* thread, word arg);
  static Continue doLoadAttrType(Thread* thread, word arg);
  static Continue doLoadBool(Thread* thread, word arg);
  static Continue doLoadDeref(Thread* thread, word arg);
  static Continue doLoadFast(Thread* thread, word arg);
  static Continue doLoadFastReverse(Thread* thread, word arg);
  static Continue doLoadFastReverseUnchecked(Thread* thread, word arg);
  static Continue doLoadMethod(Thread* thread, word arg);
  static Continue doLoadMethodAnamorphic(Thread* thread, word arg);
  static Continue doLoadMethodInstanceFunction(Thread* thread, word arg);
  static Continue doLoadMethodPolymorphic(Thread* thread, word arg);
  static Continue doLoadName(Thread* thread, word arg);
  static Continue doPopExcept(Thread* thread, word arg);
  static Continue doPopFinally(Thread* thread, word arg);
  static Continue doRaiseVarargs(Thread* thread, word arg);
  static Continue doReturnValue(Thread* thread, word arg);
  static Continue doSetupWith(Thread* thread, word arg);
  static Continue doStoreAttr(Thread* thread, word arg);
  static Continue doStoreAttrInstance(Thread* thread, word arg);
  static Continue doStoreAttrInstanceOverflow(Thread* thread, word arg);
  static Continue doStoreAttrInstanceOverflowUpdate(Thread* thread, word arg);
  static Continue doStoreAttrInstanceUpdate(Thread* thread, word arg);
  static Continue doStoreAttrPolymorphic(Thread* thread, word arg);
  static Continue doStoreAttrAnamorphic(Thread* thread, word arg);
  static Continue doStoreSubscr(Thread* thread, word arg);
  static Continue doStoreSubscrDict(Thread* thread, word arg);
  static Continue doStoreSubscrList(Thread* thread, word arg);
  static Continue doStoreSubscrMonomorphic(Thread* thread, word arg);
  static Continue doStoreSubscrPolymorphic(Thread* thread, word arg);
  static Continue doStoreSubscrAnamorphic(Thread* thread, word arg);
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
  static Continue doRotFour(Thread* thread, word arg);
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

  // Call an interpreted function (this captures the part of the CALL_FUNCTION
  // process after intrinsics are processed and it has been determined that the
  // callable is an interpreted function).
  static Continue callInterpreted(Thread* thread, word nargs,
                                  RawFunction function);

  // Resolve a callable object to a function (resolving `__call__` descriptors
  // as necessary).
  // This is a helper for `prepareCallableCall`: `prepareCallableCall` starts
  // out with shortcuts with the common cases and only calls this function for
  // the remaining rare cases with the expectation that this function is not
  // inlined.
  static PrepareCallableResult prepareCallableCallDunderCall(Thread* thread,
                                                             word nargs,
                                                             word callable_idx);

  // If the given GeneratorBase is suspended at a YIELD_FROM instruction, return
  // its subiterator. Otherwise, return None.
  static RawObject findYieldFrom(RawGeneratorBase gen);

 private:
  // Common functionality for opcode handlers that dispatch to binary and
  // inplace operations
  static Continue doBinaryOperation(BinaryOp op, Thread* thread);
  static Continue doInplaceOperation(BinaryOp op, Thread* thread);
  static Continue doUnaryOperation(SymbolId selector, Thread* thread);

  // Slow path for the FOR_ITER opcode that updates the cache at the given index
  // when appropriate. May also be used as a non-caching slow path by passing a
  // negative index.
  static Continue forIterUpdateCache(Thread* thread, word arg, word index);
  static Continue forIter(Thread* thread, RawObject next_method, word arg);

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

  // Call callable with `arg` parameters at the end of an opcode handler. Use
  // this when the number of parameters is more than 2.
  static Continue tailcall(Thread* thread, word arg);

  // Call function object at frame[nargs]. The result functionc all result
  // is pushed onto the stack.
  static Continue tailcallFunction(Thread* thread, word nargs,
                                   RawObject function_obj);

  // Given a non-Function object in `callable`, attempt to normalize it to a
  // Function by either unpacking a BoundMethod or looking up the object's
  // __call__ method, iterating multiple times if necessary.
  //
  // On success, `callable` will contain the Function to call, and the return
  // value will be a bool indicating whether or not `self` was populated with an
  // object unpacked from a BoundMethod.
  //
  // On failure, Error is returned and `callable` may have been modified.
  static RawObject prepareCallable(Thread* thread, Object* callable,
                                   Object* self);

  // Prepare the stack for an explode call by normalizing the callable object
  // using prepareCallableObject().
  //
  // Returns the concrete Function that should be called.
  static RawObject prepareCallableEx(Thread* thread, word callable_idx);

  // Perform a positional or keyword call. Used by doCallFunction() and
  // doCallFunctionKw().
  static Continue handleCall(Thread* thread, word nargs, word callable_idx,
                             PrepareCallFunc prepare_args,
                             Function::Entry (RawFunction::*get_entry)() const);

  // Call a function through its trampoline, pushing the result on the stack.
  static Continue callTrampoline(Thread* thread, Function::Entry entry,
                                 word nargs, RawObject* post_call_sp);

  static Continue retryLoadAttrCached(Thread* thread, word arg);
  static Continue loadAttrUpdateCache(Thread* thread, word arg);
  static Continue storeAttrUpdateCache(Thread* thread, word arg);
  static Continue storeSubscr(Thread* thread, RawObject set_item_method);

  static Continue loadMethodUpdateCache(Thread* thread, word arg);

  using BinaryOpFallbackHandler = Continue (*)(Thread* thread, word arg,
                                               BinaryOpFlags flags);
  static Continue cachedBinaryOpImpl(Thread* thread, word arg,
                                     OpcodeHandler update_cache,
                                     BinaryOpFallbackHandler fallback);

  static Continue binaryOp(Thread* thread, word arg, RawObject method,
                           BinaryOpFlags flags, RawObject left, RawObject right,
                           BinaryOpFallbackHandler fallback);
  static Continue binaryOpFallback(Thread* thread, word arg,
                                   BinaryOpFlags flags);

  static Continue compareOpFallback(Thread* thread, word arg,
                                    BinaryOpFlags flags);

  static Continue inplaceOpFallback(Thread* thread, word arg,
                                    BinaryOpFlags flags);

  static RawObject awaitableIter(Thread* thread,
                                 const char* invalid_type_message);
};

Interpreter* createCppInterpreter();

}  // namespace py
