#pragma once

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

// Copy the code, entry, and interpreter info from base to patch.
void copyFunctionEntries(Thread* thread, const Function& base,
                         const Function& patch);

class UnderBuiltinsModule {
 public:
  static void initialize(Thread* thread, const Module& module);

  static RawObject underAddress(Thread* thread, Frame* frame, word nargs);
  static RawObject underBoolCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underBoolGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underBoundMethod(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytearrayAppend(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underBytearrayCheck(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underBytearrayClear(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underBytearrayContains(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underBytearrayDelitem(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underBytearrayDelslice(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underBytearrayGetitem(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underBytearrayGetslice(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underBytearrayGuard(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underBytearrayJoin(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytearrayLen(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytearraySetitem(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underBytearraySetslice(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underBytesCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesContains(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesDecode(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesDecodeASCII(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underBytesDecodeUTF8(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underBytesFromBytes(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underBytesFromInts(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesGetitem(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesGetslice(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesJoin(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesLen(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesMaketrans(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underBytesRepeat(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesSplit(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesSplitWhitespace(Thread* thread, Frame* frame,
                                             word nargs);
  static RawObject underByteslikeCheck(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underByteslikeCompareDigest(Thread* thread, Frame* frame,
                                               word nargs);
  static RawObject underByteslikeCount(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underByteslikeEndswith(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underByteslikeFindByteslike(Thread* thread, Frame* frame,
                                               word nargs);
  static RawObject underByteslikeFindInt(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underByteslikeGuard(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underByteslikeRfindByteslike(Thread* thread, Frame* frame,
                                                word nargs);
  static RawObject underByteslikeRfindInt(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underByteslikeStartswith(Thread* thread, Frame* frame,
                                            word nargs);
  static RawObject underClassmethod(Thread* thread, Frame* frame, word nargs);
  static RawObject underClassmethodIsabstract(Thread* thread, Frame* frame,
                                              word nargs);
  static RawObject underCodeCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underCodeGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underCodeSetPosonlyargcount(Thread* thread, Frame* frame,
                                               word nargs);
  static RawObject underComplexCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underComplexCheckexact(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underComplexImag(Thread* thread, Frame* frame, word nargs);
  static RawObject underComplexNew(Thread* thread, Frame* frame, word nargs);
  static RawObject underComplexNewFromStr(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underComplexReal(Thread* thread, Frame* frame, word nargs);
  static RawObject underDictBucketInsert(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underDictBucketKey(Thread* thread, Frame* frame, word nargs);
  static RawObject underDictBucketSetValue(Thread* thread, Frame* frame,
                                           word nargs);
  static RawObject underDictBucketValue(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underDictCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underDictCheckExact(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underDictGet(Thread* thread, Frame* frame, word nargs);
  static RawObject underDictGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underDictLen(Thread* thread, Frame* frame, word nargs);
  static RawObject underDictLookup(Thread* thread, Frame* frame, word nargs);
  static RawObject underDictLookupNext(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underDictUpdate(Thread* thread, Frame* frame, word nargs);
  static RawObject underDictPopitem(Thread* thread, Frame* frame, word nargs);
  static RawObject underDictSetitem(Thread* thread, Frame* frame, word nargs);
  static RawObject underDivmod(Thread* thread, Frame* frame, word nargs);
  static RawObject underExec(Thread* thread, Frame* frame, word nargs);
  static RawObject underFloatCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underFloatCheckExact(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underFloatDivmod(Thread* thread, Frame* frame, word nargs);
  static RawObject underFloatFormat(Thread* thread, Frame* frame, word nargs);
  static RawObject underFloatGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underFloatNewFromByteslike(Thread* thread, Frame* frame,
                                              word nargs);
  static RawObject underFloatNewFromFloat(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underFloatNewFromStr(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underFloatSignbit(Thread* thread, Frame* frame, word nargs);
  static RawObject underFrozensetCheck(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underFrozensetGuard(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underFunctionGlobals(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underFunctionGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underGc(Thread* thread, Frame* frame, word nargs);
  static RawObject underGetframeFunction(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underGetframeLineno(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underGetframeLocals(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underGetMemberByte(Thread* thread, Frame* frame, word nargs);
  static RawObject underGetMemberChar(Thread* thread, Frame* frame, word nargs);
  static RawObject underGetMemberDouble(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underGetMemberFloat(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underGetMemberInt(Thread* thread, Frame* frame, word nargs);
  static RawObject underGetMemberLong(Thread* thread, Frame* frame, word nargs);
  static RawObject underGetMemberPyobject(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underGetMemberShort(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underGetMemberString(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underGetMemberUbyte(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underGetMemberUint(Thread* thread, Frame* frame, word nargs);
  static RawObject underGetMemberUlong(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underGetMemberUshort(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underInstanceDelattr(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underInstanceGetattr(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underInstanceGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underInstanceOverflowDict(Thread* thread, Frame* frame,
                                             word nargs);
  static RawObject underInstanceSetattr(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underIntCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underIntCheckExact(Thread* thread, Frame* frame, word nargs);
  static RawObject underIntFromBytes(Thread* thread, Frame* frame, word nargs);
  static RawObject underIntGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underIntNewFromBytearray(Thread* thread, Frame* frame,
                                            word nargs);
  static RawObject underIntNewFromBytes(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underIntNewFromInt(Thread* thread, Frame* frame, word nargs);
  static RawObject underIntNewFromStr(Thread* thread, Frame* frame, word nargs);
  static RawObject underIter(Thread* thread, Frame* frame, word nargs);
  static RawObject underListCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underListCheckExact(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underListDelitem(Thread* thread, Frame* frame, word nargs);
  static RawObject underListDelslice(Thread* thread, Frame* frame, word nargs);
  static RawObject underListExtend(Thread* thread, Frame* frame, word nargs);
  static RawObject underListGetitem(Thread* thread, Frame* frame, word nargs);
  static RawObject underListGetslice(Thread* thread, Frame* frame, word nargs);
  static RawObject underListGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underListLen(Thread* thread, Frame* frame, word nargs);
  static RawObject underListNew(Thread* thread, Frame* frame, word nargs);
  static RawObject underListSetItem(Thread* thread, Frame* frame, word nargs);
  static RawObject underListSetSlice(Thread* thread, Frame* frame, word nargs);
  static RawObject underListSort(Thread* thread, Frame* frame, word nargs);
  static RawObject underListSwap(Thread* thread, Frame* frame, word nargs);
  static RawObject underMappingproxyGuard(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underMappingproxyMapping(Thread* thread, Frame* frame,
                                            word nargs);
  static RawObject underMappingproxySetMapping(Thread* thread, Frame* frame,
                                               word nargs);
  static RawObject underMemoryviewCheck(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underMemoryviewGuard(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underMemoryviewItemsize(Thread* thread, Frame* frame,
                                           word nargs);
  static RawObject underMemoryviewNbytes(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underMemoryviewSetitem(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underMemoryviewSetslice(Thread* thread, Frame* frame,
                                           word nargs);
  static RawObject underModuleDir(Thread* thread, Frame* frame, word nargs);
  static RawObject underModuleProxy(Thread* thread, Frame* frame, word nargs);
  static RawObject underModuleProxyCheck(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underModuleProxyGet(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underModuleProxyGuard(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underModuleProxyKeys(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underModuleProxyLen(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underModuleProxyDelitem(Thread* thread, Frame* frame,
                                           word nargs);
  static RawObject underModuleProxySetitem(Thread* thread, Frame* frame,
                                           word nargs);
  static RawObject underModuleProxyValues(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underObjectKeys(Thread* thread, Frame* frame, word nargs);
  static RawObject underObjectTypeGetattr(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underObjectTypeHasattr(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underOsWrite(Thread* thread, Frame* frame, word nargs);
  static RawObject underPatch(Thread* thread, Frame* frame, word nargs);
  static RawObject underProperty(Thread* thread, Frame* frame, word nargs);
  static RawObject underPropertyIsabstract(Thread* thread, Frame* frame,
                                           word nargs);
  static RawObject underPyobjectOffset(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underRangeCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underRangeGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underRangeLen(Thread* thread, Frame* frame, word nargs);
  static RawObject underReprEnter(Thread* thread, Frame* frame, word nargs);
  static RawObject underReprLeave(Thread* thread, Frame* frame, word nargs);
  static RawObject underSeqIndex(Thread* thread, Frame* frame, word nargs);
  static RawObject underSeqIterable(Thread* thread, Frame* frame, word nargs);
  static RawObject underSeqSetIndex(Thread* thread, Frame* frame, word nargs);
  static RawObject underSeqSetIterable(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underSetCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underSetGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underSetLen(Thread* thread, Frame* frame, word nargs);
  static RawObject underSetMemberDouble(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underSetMemberFloat(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underSetMemberIntegral(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underSetMemberPyobject(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underSliceCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underSliceGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underSliceStart(Thread* thread, Frame* frame, word nargs);
  static RawObject underSliceStartLong(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underSliceStep(Thread* thread, Frame* frame, word nargs);
  static RawObject underSliceStepLong(Thread* thread, Frame* frame, word nargs);
  static RawObject underSliceStop(Thread* thread, Frame* frame, word nargs);
  static RawObject underSliceStopLong(Thread* thread, Frame* frame, word nargs);
  static RawObject underStaticmethodIsabstract(Thread* thread, Frame* frame,
                                               word nargs);
  static RawObject underStopIterationCtor(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underStrarrayClear(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrarrayCtor(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrarrayIadd(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrCheckExact(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrCompareDigest(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underStrCount(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrEncode(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrEncodeASCII(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underStrEndswith(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrEscapeNonAscii(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underStrFind(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrFromStr(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrGetitem(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrGetslice(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrIschr(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrJoin(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrLen(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrModFastPath(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underStrPartition(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrReplace(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrRfind(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrRpartition(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrSplit(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrSplitlines(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrStartswith(Thread* thread, Frame* frame, word nargs);
  static RawObject underTupleCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underTupleCheckExact(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underTupleGetitem(Thread* thread, Frame* frame, word nargs);
  static RawObject underTupleGetslice(Thread* thread, Frame* frame, word nargs);
  static RawObject underTupleGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underTupleLen(Thread* thread, Frame* frame, word nargs);
  static RawObject underTupleNew(Thread* thread, Frame* frame, word nargs);
  static RawObject underType(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeAbstractmethodsDel(Thread* thread, Frame* frame,
                                               word nargs);
  static RawObject underTypeAbstractmethodsGet(Thread* thread, Frame* frame,
                                               word nargs);
  static RawObject underTypeAbstractmethodsSet(Thread* thread, Frame* frame,
                                               word nargs);
  static RawObject underTypeBasesDel(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeBasesGet(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeBasesSet(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeCheckExact(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underTypeDunderCall(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underTypeGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeInit(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeIssubclass(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underTypeNew(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeProxy(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeProxyCheck(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underTypeProxyGet(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeProxyGuard(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underTypeProxyKeys(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeProxyLen(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeProxyValues(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underTypeSubclassGuard(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underUnimplemented(Thread* thread, Frame* frame, word nargs);
  static RawObject underWarn(Thread* thread, Frame* frame, word nargs);
  static RawObject underWeakrefCallback(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underWeakrefCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underWeakrefGuard(Thread* thread, Frame* frame, word nargs);
  static RawObject underWeakrefReferent(Thread* thread, Frame* frame,
                                        word nargs);

  static const BuiltinFunction kBuiltinFunctions[];

 private:
  static const SymbolId kIntrinsicIds[];
};

}  // namespace py
