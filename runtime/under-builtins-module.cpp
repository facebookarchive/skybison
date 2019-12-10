#include "under-builtins-module.h"

#include <cerrno>
#include <cmath>

#include <unistd.h>

#include "bytearray-builtins.h"
#include "bytes-builtins.h"
#include "capi-handles.h"
#include "dict-builtins.h"
#include "exception-builtins.h"
#include "float-builtins.h"
#include "float-conversion.h"
#include "frozen-modules.h"
#include "int-builtins.h"
#include "list-builtins.h"
#include "memoryview-builtins.h"
#include "module-builtins.h"
#include "mro.h"
#include "object-builtins.h"
#include "range-builtins.h"
#include "str-builtins.h"
#include "strarray-builtins.h"
#include "tuple-builtins.h"
#include "type-builtins.h"
#include "unicode.h"
#include "vector.h"
#include "warnings-module.h"

namespace py {

static bool isPass(const Code& code) {
  HandleScope scope;
  Bytes bytes(&scope, code.code());
  // const_loaded is the index into the consts array that is returned
  word const_loaded = bytes.byteAt(1);
  return bytes.length() == 4 && bytes.byteAt(0) == LOAD_CONST &&
         Tuple::cast(code.consts()).at(const_loaded).isNoneType() &&
         bytes.byteAt(2) == RETURN_VALUE && bytes.byteAt(3) == 0;
}

void copyFunctionEntries(Thread* thread, const Function& base,
                         const Function& patch) {
  HandleScope scope(thread);
  Str method_name(&scope, base.qualname());
  Code patch_code(&scope, patch.code());
  Code base_code(&scope, base.code());
  CHECK(isPass(patch_code),
        "Redefinition of native code method '%s' in managed code",
        method_name.toCStr());
  CHECK(!base_code.code().isNoneType(),
        "Useless declaration of native code method %s in managed code",
        method_name.toCStr());
  patch_code.setCode(base_code.code());
  patch_code.setLnotab(Bytes::empty());
  patch.setEntry(base.entry());
  patch.setEntryKw(base.entryKw());
  patch.setEntryEx(base.entryEx());
  patch.setIsInterpreted(false);
  patch.setIntrinsicId(base.intrinsicId());
}

static RawObject raiseRequiresFromCaller(Thread* thread, Frame* frame,
                                         word nargs, SymbolId expected_type) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Function function(&scope, frame->previousFrame()->function());
  Str function_name(&scope, function.name());
  Object obj(&scope, args.get(0));
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "'%S' requires a '%Y' object but received a '%T'",
                              &function_name, expected_type, &obj);
}

const BuiltinMethod UnderBuiltinsModule::kBuiltinMethods[] = {
    {SymbolId::kUnderAddress, underAddress},
    {SymbolId::kUnderBoundMethod, underBoundMethod},
    {SymbolId::kUnderBoolCheck, underBoolCheck},
    {SymbolId::kUnderBytearrayCheck, underBytearrayCheck},
    {SymbolId::kUnderBytearrayClear, underBytearrayClear},
    {SymbolId::kUnderBytearrayDelitem, underBytearrayDelitem},
    {SymbolId::kUnderBytearrayDelslice, underBytearrayDelslice},
    {SymbolId::kUnderBytearrayGetitem, underBytearrayGetitem},
    {SymbolId::kUnderBytearrayGetslice, underBytearrayGetslice},
    {SymbolId::kUnderBytearrayGuard, underBytearrayGuard},
    {SymbolId::kUnderBytearrayJoin, underBytearrayJoin},
    {SymbolId::kUnderBytearrayLen, underBytearrayLen},
    {SymbolId::kUnderBytearraySetitem, underBytearraySetitem},
    {SymbolId::kUnderBytearraySetslice, underBytearraySetslice},
    {SymbolId::kUnderBytesCheck, underBytesCheck},
    {SymbolId::kUnderBytesDecode, underBytesDecode},
    {SymbolId::kUnderBytesDecodeASCII, underBytesDecodeASCII},
    {SymbolId::kUnderBytesDecodeUTF8, underBytesDecodeUTF8},
    {SymbolId::kUnderBytesFromBytes, underBytesFromBytes},
    {SymbolId::kUnderBytesFromInts, underBytesFromInts},
    {SymbolId::kUnderBytesGetitem, underBytesGetitem},
    {SymbolId::kUnderBytesGetslice, underBytesGetslice},
    {SymbolId::kUnderBytesGuard, underBytesGuard},
    {SymbolId::kUnderBytesJoin, underBytesJoin},
    {SymbolId::kUnderBytesLen, underBytesLen},
    {SymbolId::kUnderBytesMaketrans, underBytesMaketrans},
    {SymbolId::kUnderBytesRepeat, underBytesRepeat},
    {SymbolId::kUnderBytesSplit, underBytesSplit},
    {SymbolId::kUnderBytesSplitWhitespace, underBytesSplitWhitespace},
    {SymbolId::kUnderByteslikeCheck, underByteslikeCheck},
    {SymbolId::kUnderByteslikeCompareDigest, underByteslikeCompareDigest},
    {SymbolId::kUnderByteslikeCount, underByteslikeCount},
    {SymbolId::kUnderByteslikeEndswith, underByteslikeEndswith},
    {SymbolId::kUnderByteslikeFindByteslike, underByteslikeFindByteslike},
    {SymbolId::kUnderByteslikeFindInt, underByteslikeFindInt},
    {SymbolId::kUnderByteslikeGuard, underByteslikeGuard},
    {SymbolId::kUnderByteslikeRfindByteslike, underByteslikeRfindByteslike},
    {SymbolId::kUnderByteslikeRfindInt, underByteslikeRfindInt},
    {SymbolId::kUnderByteslikeStartswith, underByteslikeStartswith},
    {SymbolId::kUnderClassmethod, underClassmethod},
    {SymbolId::kUnderClassmethodIsabstract, underClassmethodIsabstract},
    {SymbolId::kUnderCodeCheck, underCodeCheck},
    {SymbolId::kUnderCodeGuard, underCodeGuard},
    {SymbolId::kUnderComplexCheck, underComplexCheck},
    {SymbolId::kUnderComplexImag, underComplexImag},
    {SymbolId::kUnderComplexReal, underComplexReal},
    {SymbolId::kUnderDictBucketInsert, underDictBucketInsert},
    {SymbolId::kUnderDictBucketKey, underDictBucketKey},
    {SymbolId::kUnderDictBucketSetValue, underDictBucketSetValue},
    {SymbolId::kUnderDictBucketValue, underDictBucketValue},
    {SymbolId::kUnderDictCheck, underDictCheck},
    {SymbolId::kUnderDictCheckExact, underDictCheckExact},
    {SymbolId::kUnderDictGet, underDictGet},
    {SymbolId::kUnderDictGuard, underDictGuard},
    {SymbolId::kUnderDictLen, underDictLen},
    {SymbolId::kUnderDictLookup, underDictLookup},
    {SymbolId::kUnderDictLookupNext, underDictLookupNext},
    {SymbolId::kUnderDictPopitem, underDictPopitem},
    {SymbolId::kUnderDictSetitem, underDictSetitem},
    {SymbolId::kUnderDictUpdate, underDictUpdate},
    {SymbolId::kUnderDivmod, underDivmod},
    {SymbolId::kUnderExec, underExec},
    {SymbolId::kUnderFloatCheck, underFloatCheck},
    {SymbolId::kUnderFloatCheckExact, underFloatCheckExact},
    {SymbolId::kUnderFloatDivmod, underFloatDivmod},
    {SymbolId::kUnderFloatFormat, underFloatFormat},
    {SymbolId::kUnderFloatGuard, underFloatGuard},
    {SymbolId::kUnderFloatNewFromByteslike, underFloatNewFromByteslike},
    {SymbolId::kUnderFloatNewFromFloat, underFloatNewFromFloat},
    {SymbolId::kUnderFloatNewFromStr, underFloatNewFromStr},
    {SymbolId::kUnderFloatSignbit, underFloatSignbit},
    {SymbolId::kUnderFrozensetCheck, underFrozensetCheck},
    {SymbolId::kUnderFrozensetGuard, underFrozensetGuard},
    {SymbolId::kUnderFunctionGlobals, underFunctionGlobals},
    {SymbolId::kUnderFunctionGuard, underFunctionGuard},
    {SymbolId::kUnderGc, underGc},
    {SymbolId::kUnderGetframeFunction, underGetframeFunction},
    {SymbolId::kUnderGetframeLineno, underGetframeLineno},
    {SymbolId::kUnderGetframeLocals, underGetframeLocals},
    {SymbolId::kUnderGetMemberByte, underGetMemberByte},
    {SymbolId::kUnderGetMemberChar, underGetMemberChar},
    {SymbolId::kUnderGetMemberDouble, underGetMemberDouble},
    {SymbolId::kUnderGetMemberFloat, underGetMemberFloat},
    {SymbolId::kUnderGetMemberInt, underGetMemberInt},
    {SymbolId::kUnderGetMemberLong, underGetMemberLong},
    {SymbolId::kUnderGetMemberPyobject, underGetMemberPyobject},
    {SymbolId::kUnderGetMemberShort, underGetMemberShort},
    {SymbolId::kUnderGetMemberString, underGetMemberString},
    {SymbolId::kUnderGetMemberUbyte, underGetMemberUbyte},
    {SymbolId::kUnderGetMemberUint, underGetMemberUint},
    {SymbolId::kUnderGetMemberUlong, underGetMemberUlong},
    {SymbolId::kUnderGetMemberUshort, underGetMemberUshort},
    {SymbolId::kUnderInstanceDelattr, underInstanceDelattr},
    {SymbolId::kUnderInstanceGetattr, underInstanceGetattr},
    {SymbolId::kUnderInstanceGuard, underInstanceGuard},
    {SymbolId::kUnderInstanceOverflowDict, underInstanceOverflowDict},
    {SymbolId::kUnderInstanceSetattr, underInstanceSetattr},
    {SymbolId::kUnderIntCheck, underIntCheck},
    {SymbolId::kUnderIntCheckExact, underIntCheckExact},
    {SymbolId::kUnderIntFromBytes, underIntFromBytes},
    {SymbolId::kUnderIntGuard, underIntGuard},
    {SymbolId::kUnderIntNewFromBytearray, underIntNewFromBytearray},
    {SymbolId::kUnderIntNewFromBytes, underIntNewFromBytes},
    {SymbolId::kUnderIntNewFromInt, underIntNewFromInt},
    {SymbolId::kUnderIntNewFromStr, underIntNewFromStr},
    {SymbolId::kUnderIter, underIter},
    {SymbolId::kUnderListCheck, underListCheck},
    {SymbolId::kUnderListCheckExact, underListCheckExact},
    {SymbolId::kUnderListDelitem, underListDelitem},
    {SymbolId::kUnderListDelslice, underListDelslice},
    {SymbolId::kUnderListExtend, underListExtend},
    {SymbolId::kUnderListGetitem, underListGetitem},
    {SymbolId::kUnderListGetslice, underListGetslice},
    {SymbolId::kUnderListGuard, underListGuard},
    {SymbolId::kUnderListLen, underListLen},
    {SymbolId::kUnderListSort, underListSort},
    {SymbolId::kUnderListSwap, underListSwap},
    {SymbolId::kUnderMappingproxyGuard, underMappingproxyGuard},
    {SymbolId::kUnderMappingproxyMapping, underMappingproxyMapping},
    {SymbolId::kUnderMappingproxySetMapping, underMappingproxySetMapping},
    {SymbolId::kUnderMemoryviewCheck, underMemoryviewCheck},
    {SymbolId::kUnderMemoryviewGuard, underMemoryviewGuard},
    {SymbolId::kUnderMemoryviewItemsize, underMemoryviewItemsize},
    {SymbolId::kUnderMemoryviewNbytes, underMemoryviewNbytes},
    {SymbolId::kUnderMemoryviewSetitem, underMemoryviewSetitem},
    {SymbolId::kUnderMemoryviewSetslice, underMemoryviewSetslice},
    {SymbolId::kUnderModuleDir, underModuleDir},
    {SymbolId::kUnderModuleProxy, underModuleProxy},
    {SymbolId::kUnderModuleProxyCheck, underModuleProxyCheck},
    {SymbolId::kUnderModuleProxyDelitem, underModuleProxyDelitem},
    {SymbolId::kUnderModuleProxyGet, underModuleProxyGet},
    {SymbolId::kUnderModuleProxyGuard, underModuleProxyGuard},
    {SymbolId::kUnderModuleProxyKeys, underModuleProxyKeys},
    {SymbolId::kUnderModuleProxyLen, underModuleProxyLen},
    {SymbolId::kUnderModuleProxySetitem, underModuleProxySetitem},
    {SymbolId::kUnderModuleProxyValues, underModuleProxyValues},
    {SymbolId::kUnderObjectKeys, underObjectKeys},
    {SymbolId::kUnderObjectTypeGetattr, underObjectTypeGetattr},
    {SymbolId::kUnderObjectTypeHasattr, underObjectTypeHasattr},
    {SymbolId::kUnderOsWrite, underOsWrite},
    {SymbolId::kUnderProperty, underProperty},
    {SymbolId::kUnderPropertyIsabstract, underPropertyIsabstract},
    {SymbolId::kUnderPyobjectOffset, underPyobjectOffset},
    {SymbolId::kUnderRangeCheck, underRangeCheck},
    {SymbolId::kUnderRangeGuard, underRangeGuard},
    {SymbolId::kUnderRangeLen, underRangeLen},
    {SymbolId::kUnderReprEnter, underReprEnter},
    {SymbolId::kUnderReprLeave, underReprLeave},
    {SymbolId::kUnderSeqIndex, underSeqIndex},
    {SymbolId::kUnderSeqIterable, underSeqIterable},
    {SymbolId::kUnderSeqSetIndex, underSeqSetIndex},
    {SymbolId::kUnderSeqSetIterable, underSeqSetIterable},
    {SymbolId::kUnderSetCheck, underSetCheck},
    {SymbolId::kUnderSetGuard, underSetGuard},
    {SymbolId::kUnderSetLen, underSetLen},
    {SymbolId::kUnderSetMemberDouble, underSetMemberDouble},
    {SymbolId::kUnderSetMemberFloat, underSetMemberFloat},
    {SymbolId::kUnderSetMemberIntegral, underSetMemberIntegral},
    {SymbolId::kUnderSetMemberPyobject, underSetMemberPyobject},
    {SymbolId::kUnderSliceCheck, underSliceCheck},
    {SymbolId::kUnderSliceGuard, underSliceGuard},
    {SymbolId::kUnderSliceStart, underSliceStart},
    {SymbolId::kUnderSliceStartLong, underSliceStartLong},
    {SymbolId::kUnderSliceStep, underSliceStep},
    {SymbolId::kUnderSliceStepLong, underSliceStepLong},
    {SymbolId::kUnderSliceStop, underSliceStop},
    {SymbolId::kUnderSliceStopLong, underSliceStopLong},
    {SymbolId::kUnderStaticmethodIsabstract, underStaticmethodIsabstract},
    {SymbolId::kUnderStopIterationCtor, underStopIterationCtor},
    {SymbolId::kUnderStrarrayClear, underStrarrayClear},
    {SymbolId::kUnderStrarrayCtor, underStrarrayCtor},
    {SymbolId::kUnderStrarrayIadd, underStrarrayIadd},
    {SymbolId::kUnderStrCheck, underStrCheck},
    {SymbolId::kUnderStrCheckExact, underStrCheckExact},
    {SymbolId::kUnderStrCompareDigest, underStrCompareDigest},
    {SymbolId::kUnderStrCount, underStrCount},
    {SymbolId::kUnderStrEncode, underStrEncode},
    {SymbolId::kUnderStrEncodeASCII, underStrEncodeASCII},
    {SymbolId::kUnderStrEndswith, underStrEndswith},
    {SymbolId::kUnderStrEscapeNonAscii, underStrEscapeNonAscii},
    {SymbolId::kUnderStrFind, underStrFind},
    {SymbolId::kUnderStrFromStr, underStrFromStr},
    {SymbolId::kUnderStrGetitem, underStrGetitem},
    {SymbolId::kUnderStrGetslice, underStrGetslice},
    {SymbolId::kUnderStrGuard, underStrGuard},
    {SymbolId::kUnderStrIschr, underStrIschr},
    {SymbolId::kUnderStrJoin, underStrJoin},
    {SymbolId::kUnderStrLen, underStrLen},
    {SymbolId::kUnderStrPartition, underStrPartition},
    {SymbolId::kUnderStrReplace, underStrReplace},
    {SymbolId::kUnderStrRfind, underStrRfind},
    {SymbolId::kUnderStrRpartition, underStrRpartition},
    {SymbolId::kUnderStrSplit, underStrSplit},
    {SymbolId::kUnderStrSplitlines, underStrSplitlines},
    {SymbolId::kUnderStrStartswith, underStrStartswith},
    {SymbolId::kUnderTupleCheck, underTupleCheck},
    {SymbolId::kUnderTupleCheckExact, underTupleCheckExact},
    {SymbolId::kUnderTupleGetitem, underTupleGetitem},
    {SymbolId::kUnderTupleGetslice, underTupleGetslice},
    {SymbolId::kUnderTupleGuard, underTupleGuard},
    {SymbolId::kUnderTupleLen, underTupleLen},
    {SymbolId::kUnderTupleNew, underTupleNew},
    {SymbolId::kUnderType, underType},
    {SymbolId::kUnderTypeAbstractmethodsDel, underTypeAbstractmethodsDel},
    {SymbolId::kUnderTypeAbstractmethodsGet, underTypeAbstractmethodsGet},
    {SymbolId::kUnderTypeAbstractmethodsSet, underTypeAbstractmethodsSet},
    {SymbolId::kUnderTypeBasesDel, underTypeBasesDel},
    {SymbolId::kUnderTypeBasesGet, underTypeBasesGet},
    {SymbolId::kUnderTypeBasesSet, underTypeBasesSet},
    {SymbolId::kUnderTypeCheck, underTypeCheck},
    {SymbolId::kUnderTypeCheckExact, underTypeCheckExact},
    {SymbolId::kUnderTypeDunderCall, underTypeDunderCall},
    {SymbolId::kUnderTypeGuard, underTypeGuard},
    {SymbolId::kUnderTypeInit, underTypeInit},
    {SymbolId::kUnderTypeIssubclass, underTypeIssubclass},
    {SymbolId::kUnderTypeSubclassGuard, underTypeSubclassGuard},
    {SymbolId::kUnderTypeNew, underTypeNew},
    {SymbolId::kUnderTypeProxy, underTypeProxy},
    {SymbolId::kUnderTypeProxyCheck, underTypeProxyCheck},
    {SymbolId::kUnderTypeProxyGet, underTypeProxyGet},
    {SymbolId::kUnderTypeProxyGuard, underTypeProxyGuard},
    {SymbolId::kUnderTypeProxyKeys, underTypeProxyKeys},
    {SymbolId::kUnderTypeProxyLen, underTypeProxyLen},
    {SymbolId::kUnderTypeProxyValues, underTypeProxyValues},
    {SymbolId::kUnderUnimplemented, underUnimplemented},
    {SymbolId::kUnderWarn, underWarn},
    {SymbolId::kUnderWeakrefCallback, underWeakrefCallback},
    {SymbolId::kUnderWeakrefCheck, underWeakrefCheck},
    {SymbolId::kUnderWeakrefGuard, underWeakrefGuard},
    {SymbolId::kUnderWeakrefReferent, underWeakrefReferent},
    {SymbolId::kSentinelId, nullptr},
};

const char* const UnderBuiltinsModule::kFrozenData = kUnderBuiltinsModuleData;

// clang-format off
const SymbolId UnderBuiltinsModule::kIntrinsicIds[] = {
    SymbolId::kUnderBoolCheck,
    SymbolId::kUnderBytearrayCheck,
    SymbolId::kUnderBytearrayGuard,
    SymbolId::kUnderBytearrayLen,
    SymbolId::kUnderBytesCheck,
    SymbolId::kUnderBytesGuard,
    SymbolId::kUnderBytesLen,
    SymbolId::kUnderByteslikeCheck,
    SymbolId::kUnderByteslikeGuard,
    SymbolId::kUnderComplexCheck,
    SymbolId::kUnderDictCheck,
    SymbolId::kUnderDictCheckExact,
    SymbolId::kUnderDictGuard,
    SymbolId::kUnderDictLen,
    SymbolId::kUnderFloatCheck,
    SymbolId::kUnderFloatCheckExact,
    SymbolId::kUnderFloatGuard,
    SymbolId::kUnderFrozensetCheck,
    SymbolId::kUnderFrozensetGuard,
    SymbolId::kUnderIntCheck,
    SymbolId::kUnderIntCheckExact,
    SymbolId::kUnderIntGuard,
    SymbolId::kUnderListCheck,
    SymbolId::kUnderListCheckExact,
    SymbolId::kUnderListGetitem,
    SymbolId::kUnderListGuard,
    SymbolId::kUnderListLen,
    SymbolId::kUnderMemoryviewGuard,
    SymbolId::kUnderRangeCheck,
    SymbolId::kUnderRangeGuard,
    SymbolId::kUnderSetCheck,
    SymbolId::kUnderSetGuard,
    SymbolId::kUnderSetLen,
    SymbolId::kUnderSliceCheck,
    SymbolId::kUnderSliceGuard,
    SymbolId::kUnderStrCheck,
    SymbolId::kUnderStrCheckExact,
    SymbolId::kUnderStrGuard,
    SymbolId::kUnderStrLen,
    SymbolId::kUnderTupleCheck,
    SymbolId::kUnderTupleCheckExact,
    SymbolId::kUnderTupleGetitem,
    SymbolId::kUnderTupleGuard,
    SymbolId::kUnderTupleLen,
    SymbolId::kUnderType,
    SymbolId::kUnderTypeCheck,
    SymbolId::kUnderTypeCheckExact,
    SymbolId::kUnderTypeGuard,
    SymbolId::kSentinelId,
};
// clang-format on

// Attempts to unpack a possibly-slice key. Returns true and sets start, stop if
// key is a slice with None step and None/SmallInt start and stop. The start and
// stop values must still be adjusted for the container's length. Returns false
// if key is not a slice or if the slice bounds are not the common types.
static bool trySlice(const Object& key, word* start, word* stop) {
  if (!key.isSlice()) {
    return false;
  }

  RawSlice slice = Slice::cast(*key);
  if (!slice.step().isNoneType()) {
    return false;
  }

  RawObject start_obj = slice.start();
  if (start_obj.isNoneType()) {
    *start = 0;
  } else if (start_obj.isSmallInt()) {
    *start = SmallInt::cast(start_obj).value();
  } else {
    return false;
  }

  RawObject stop_obj = slice.stop();
  if (stop_obj.isNoneType()) {
    *stop = kMaxWord;
  } else if (stop_obj.isSmallInt()) {
    *stop = SmallInt::cast(stop_obj).value();
  } else {
    return false;
  }

  return true;
}

RawObject UnderBuiltinsModule::underAddress(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  return thread->runtime()->newInt(args.get(0).raw());
}

RawObject UnderBuiltinsModule::underBoolCheck(Thread* /* t */, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isBool());
}

RawObject UnderBuiltinsModule::underBoundMethod(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object function(&scope, args.get(0));
  Object owner(&scope, args.get(1));
  return thread->runtime()->newBoundMethod(function, owner);
}

RawObject UnderBuiltinsModule::underBytearrayClear(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray self(&scope, args.get(0));
  self.downsize(0);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underBytearrayCheck(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfByteArray(args.get(0)));
}

RawObject UnderBuiltinsModule::underBytearrayGuard(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfByteArray(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kBytearray);
}

RawObject UnderBuiltinsModule::underBytearrayDelitem(Thread* thread,
                                                     Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  ByteArray self(&scope, args.get(0));
  word length = self.numItems();
  word idx = intUnderlying(args.get(1)).asWordSaturated();
  if (idx < 0) {
    idx += length;
  }
  if (idx < 0 || idx >= length) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "bytearray index out of range");
  }
  word last_idx = length - 1;
  MutableBytes self_bytes(&scope, self.bytes());
  self_bytes.replaceFromWithStartAt(idx, Bytes::cast(self.bytes()),
                                    last_idx - idx, idx + 1);
  self.setNumItems(last_idx);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underBytearrayDelslice(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  // This function deletes elements that are specified by a slice by copying.
  // It compacts to the left elements in the slice range and then copies
  // elements after the slice into the free area.  The self element count is
  // decremented and elements in the unused part of the self are overwritten
  // with None.
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  ByteArray self(&scope, args.get(0));

  word start = SmallInt::cast(args.get(1)).value();
  word stop = SmallInt::cast(args.get(2)).value();
  word step = SmallInt::cast(args.get(3)).value();

  word slice_length = Slice::length(start, stop, step);
  DCHECK_BOUND(slice_length, self.numItems());
  if (slice_length == 0) {
    // Nothing to delete
    return NoneType::object();
  }
  if (slice_length == self.numItems()) {
    // Delete all the items
    self.setNumItems(0);
    return NoneType::object();
  }
  if (step < 0) {
    // Adjust step to make iterating easier
    start = start + step * (slice_length - 1);
    step = -step;
  }
  DCHECK_INDEX(start, self.numItems());
  DCHECK(step <= self.numItems() || slice_length == 1,
         "Step should be in bounds or only one element should be sliced");
  // Sliding compaction of elements out of the slice to the left
  // Invariant: At each iteration of the loop, `fast` is the index of an
  // element addressed by the slice.
  // Invariant: At each iteration of the inner loop, `slow` is the index of a
  // location to where we are relocating a slice addressed element. It is *not*
  // addressed by the slice.
  word fast = start;
  MutableBytes self_bytes(&scope, self.bytes());
  for (word i = 1; i < slice_length; i++) {
    DCHECK_INDEX(fast, self.numItems());
    word slow = fast + 1;
    fast += step;
    for (; slow < fast; slow++) {
      self_bytes.byteAtPut(slow - i, self_bytes.byteAt(slow));
    }
  }
  // Copy elements into the space where the deleted elements were
  for (word i = fast + 1; i < self.numItems(); i++) {
    self_bytes.byteAtPut(i - slice_length, self_bytes.byteAt(i));
  }
  self.setNumItems(self.numItems() - slice_length);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underBytearrayGetitem(Thread* thread,
                                                     Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfByteArray(*self_obj)) {
    return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kBytearray);
  }
  Object key(&scope, args.get(1));
  if (runtime->isInstanceOfInt(*key)) {
    ByteArray self(&scope, *self_obj);
    word index = intUnderlying(*key).asWordSaturated();
    if (!SmallInt::isValid(index)) {
      return thread->raiseWithFmt(LayoutId::kIndexError,
                                  "cannot fit '%T' into an index-sized integer",
                                  &key);
    }
    word length = self.numItems();
    if (index < 0) {
      index += length;
    }
    if (index < 0 || index >= length) {
      return thread->raiseWithFmt(LayoutId::kIndexError,
                                  "bytearray index out of range");
    }
    return SmallInt::fromWord(self.byteAt(index));
  }

  word start, stop;
  if (!trySlice(key, &start, &stop)) {
    return Unbound::object();
  }

  ByteArray self(&scope, *self_obj);
  word result_len = Slice::adjustIndices(self.numItems(), &start, &stop, 1);
  if (result_len == 0) {
    return runtime->newByteArray();
  }

  ByteArray result(&scope, runtime->newByteArray());
  MutableBytes result_bytes(&scope,
                            runtime->newMutableBytesUninitialized(result_len));
  Bytes src_bytes(&scope, self.bytes());
  result_bytes.replaceFromWithStartAt(0, *src_bytes, result_len, start);
  result.setBytes(*result_bytes);
  result.setNumItems(result_len);
  return *result;
}

RawObject UnderBuiltinsModule::underBytearrayGetslice(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray self(&scope, args.get(0));
  word start = SmallInt::cast(args.get(1)).value();
  word stop = SmallInt::cast(args.get(2)).value();
  word step = SmallInt::cast(args.get(3)).value();
  word len = Slice::length(start, stop, step);
  Runtime* runtime = thread->runtime();
  ByteArray result(&scope, runtime->newByteArray());
  runtime->byteArrayEnsureCapacity(thread, result, len);
  result.setNumItems(len);
  for (word i = 0, idx = start; i < len; i++, idx += step) {
    result.byteAtPut(i, self.byteAt(idx));
  }
  return *result;
}

RawObject UnderBuiltinsModule::underBytearraySetitem(Thread* thread,
                                                     Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray self(&scope, args.get(0));
  word index = intUnderlying(args.get(1)).asWordSaturated();
  if (!SmallInt::isValid(index)) {
    Object key_obj(&scope, args.get(1));
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "cannot fit '%T' into an index-sized integer",
                                &key_obj);
  }
  word length = self.numItems();
  if (index < 0) {
    index += length;
  }
  if (index < 0 || index >= length) {
    return thread->raiseWithFmt(LayoutId::kIndexError, "index out of range");
  }
  word val = intUnderlying(args.get(2)).asWordSaturated();
  if (val < 0 || val > kMaxByte) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "byte must be in range(0, 256)");
  }
  self.byteAtPut(index, val);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underBytearraySetslice(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray self(&scope, args.get(0));
  word start = SmallInt::cast(args.get(1)).value();
  word stop = SmallInt::cast(args.get(2)).value();
  word step = SmallInt::cast(args.get(3)).value();
  Object src_obj(&scope, args.get(4));
  Bytes src_bytes(&scope, Bytes::empty());
  word src_length;

  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfBytes(*src_obj)) {
    Bytes src(&scope, bytesUnderlying(*src_obj));
    src_bytes = *src;
    src_length = src.length();
  } else if (runtime->isInstanceOfByteArray(*src_obj)) {
    ByteArray src(&scope, *src_obj);
    src_bytes = src.bytes();
    src_length = src.numItems();
  } else {
    // TODO(T38246066): support buffer protocol
    UNIMPLEMENTED("bytes-like other than bytes or bytearray");
  }
  // Make sure that the degenerate case of a slice assignment where start is
  // greater than stop inserts before the start and not the stop. For example,
  // b[5:2] = ... should inserts before 5, not before 2.
  if ((step < 0 && start < stop) || (step > 0 && start > stop)) {
    stop = start;
  }

  if (step == 1) {
    if (self == src_obj) {
      // This copy avoids complicated indexing logic in a rare case of
      // replacing lhs with elements of rhs when lhs == rhs. It can likely be
      // re-written to avoid allocation if necessary.
      src_bytes =
          thread->runtime()->bytesSubseq(thread, src_bytes, 0, src_length);
    }
    word growth = src_length - (stop - start);
    word new_length = self.numItems() + growth;
    if (growth == 0) {
      // Assignment does not change the length of the bytearray. Do nothing.
    } else if (growth > 0) {
      // Assignment grows the length of the bytearray. Ensure there is enough
      // free space in the underlying tuple for the new bytes and move stuff
      // out of the way.
      thread->runtime()->byteArrayEnsureCapacity(thread, self, new_length);
      // Make the free space part of the bytearray. Must happen before shifting
      // so we can index into the free space.
      self.setNumItems(new_length);
      // Shift some bytes to the right.
      self.replaceFromWithStartAt(start + growth, *self,
                                  new_length - growth - start, start);
    } else {
      // Growth is negative so assignment shrinks the length of the bytearray.
      // Shift some bytes to the left.
      self.replaceFromWithStartAt(start, *self, new_length - start,
                                  start - growth);
      // Remove the free space from the length of the bytearray. Must happen
      // after shifting and clearing so we can index into the free space.
      self.setNumItems(new_length);
    }
    // Copy new elements into the middle
    MutableBytes::cast(self.bytes())
        .replaceFromWith(start, *src_bytes, src_length);
    return NoneType::object();
  }

  word slice_length = Slice::length(start, stop, step);
  if (slice_length != src_length) {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "attempt to assign bytes of size %w to extended slice of size %w",
        src_length, slice_length);
  }

  MutableBytes dst_bytes(&scope, self.bytes());
  for (word dst_idx = start, src_idx = 0; src_idx < src_length;
       dst_idx += step, src_idx++) {
    dst_bytes.byteAtPut(dst_idx, src_bytes.byteAt(src_idx));
  }
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underBytesCheck(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfBytes(args.get(0)));
}

RawObject UnderBuiltinsModule::underBytesDecode(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object bytes_obj(&scope, args.get(0));
  if (!bytes_obj.isBytes()) {
    return Unbound::object();
  }
  Bytes bytes(&scope, *bytes_obj);
  static RawSmallStr ascii = SmallStr::fromCStr("ascii");
  static RawSmallStr utf8 = SmallStr::fromCStr("utf-8");
  static RawSmallStr latin1 = SmallStr::fromCStr("latin-1");
  Str enc(&scope, args.get(1));
  if (enc != ascii && enc != utf8 && enc != latin1 &&
      enc.compareCStr("iso-8859-1") != 0) {
    return Unbound::object();
  }
  return bytesDecodeASCII(thread, bytes);
}

RawObject UnderBuiltinsModule::underBytesDecodeASCII(Thread* thread,
                                                     Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object bytes_obj(&scope, args.get(0));
  if (!bytes_obj.isBytes()) {
    return Unbound::object();
  }
  Bytes bytes(&scope, *bytes_obj);
  return bytesDecodeASCII(thread, bytes);
}

RawObject UnderBuiltinsModule::underBytesDecodeUTF8(Thread* thread,
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object bytes_obj(&scope, args.get(0));
  if (!bytes_obj.isBytes()) {
    return Unbound::object();
  }
  Bytes bytes(&scope, *bytes_obj);
  return bytesDecodeASCII(thread, bytes);
}

RawObject UnderBuiltinsModule::underBytesGuard(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfBytes(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kBytes);
}

RawObject UnderBuiltinsModule::underBytearrayJoin(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object sep_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfByteArray(*sep_obj)) {
    return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kBytearray);
  }
  ByteArray sep(&scope, args.get(0));
  Bytes sep_bytes(&scope, sep.bytes());
  Object iterable(&scope, args.get(1));
  Tuple tuple(&scope, runtime->emptyTuple());
  word length;
  if (iterable.isList()) {
    tuple = List::cast(*iterable).items();
    length = List::cast(*iterable).numItems();
  } else if (iterable.isTuple()) {
    tuple = *iterable;
    length = tuple.length();
  } else {
    // Collect items into list in Python and call again
    return Unbound::object();
  }
  Object elt(&scope, NoneType::object());
  for (word i = 0; i < length; i++) {
    elt = tuple.at(i);
    if (!runtime->isInstanceOfBytes(*elt) &&
        !runtime->isInstanceOfByteArray(*elt)) {
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "sequence item %w: expected a bytes-like object, '%T' found", i,
          &elt);
    }
  }
  Bytes joined(&scope, runtime->bytesJoin(thread, sep_bytes, sep.numItems(),
                                          tuple, length));
  ByteArray result(&scope, runtime->newByteArray());
  result.setBytes(*joined);
  result.setNumItems(joined.length());
  return *result;
}

RawObject UnderBuiltinsModule::underBytearrayLen(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray self(&scope, args.get(0));
  return SmallInt::fromWord(self.numItems());
}

RawObject UnderBuiltinsModule::underBytesFromBytes(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  DCHECK(type.builtinBase() == LayoutId::kBytes, "type must subclass bytes");
  Object value(&scope, bytesUnderlying(args.get(1)));
  if (type.isBuiltin()) return *value;
  Layout type_layout(&scope, type.instanceLayout());
  UserBytesBase instance(&scope, thread->runtime()->newInstance(type_layout));
  instance.setValue(*value);
  return *instance;
}

RawObject UnderBuiltinsModule::underBytesFromInts(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object src(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  // TODO(T38246066): buffers other than bytes, bytearray
  if (runtime->isInstanceOfBytes(*src)) {
    return *src;
  }
  if (runtime->isInstanceOfByteArray(*src)) {
    ByteArray source(&scope, *src);
    return byteArrayAsBytes(thread, runtime, source);
  }
  if (src.isList()) {
    List source(&scope, *src);
    Tuple items(&scope, source.items());
    return runtime->bytesFromTuple(thread, items, source.numItems());
  }
  if (src.isTuple()) {
    Tuple source(&scope, *src);
    return runtime->bytesFromTuple(thread, source, source.length());
  }
  if (runtime->isInstanceOfStr(*src)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "cannot convert '%T' object to bytes", &src);
  }
  // Slow path: iterate over source in Python, collect into list, and call again
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underBytesGetitem(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Bytes self(&scope, bytesUnderlying(args.get(0)));
  word index = intUnderlying(args.get(1)).asWordSaturated();
  if (!SmallInt::isValid(index)) {
    Object key_obj(&scope, args.get(1));
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "cannot fit '%T' into an index-sized integer",
                                &key_obj);
  }
  word length = self.length();
  if (index < 0) {
    index += length;
  }
  if (index < 0 || index >= length) {
    return thread->raiseWithFmt(LayoutId::kIndexError, "index out of range");
  }
  return SmallInt::fromWord(self.byteAt(index));
}

RawObject UnderBuiltinsModule::underBytesGetslice(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Bytes self(&scope, bytesUnderlying(args.get(0)));
  word start = SmallInt::cast(args.get(1)).value();
  word stop = SmallInt::cast(args.get(2)).value();
  word step = SmallInt::cast(args.get(3)).value();
  return thread->runtime()->bytesSlice(thread, self, start, stop, step);
}

RawObject UnderBuiltinsModule::underBytesJoin(Thread* thread, Frame* frame,
                                              word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kBytes);
  }
  Bytes self(&scope, bytesUnderlying(*self_obj));
  Object iterable(&scope, args.get(1));
  Tuple tuple(&scope, runtime->emptyTuple());
  word length;
  if (iterable.isList()) {
    tuple = List::cast(*iterable).items();
    length = List::cast(*iterable).numItems();
  } else if (iterable.isTuple()) {
    tuple = *iterable;
    length = Tuple::cast(*iterable).length();
  } else {
    // Collect items into list in Python and call again
    return Unbound::object();
  }
  Object elt(&scope, NoneType::object());
  for (word i = 0; i < length; i++) {
    elt = tuple.at(i);
    if (!runtime->isInstanceOfBytes(*elt) &&
        !runtime->isInstanceOfByteArray(*elt)) {
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "sequence item %w: expected a bytes-like object, %T found", i, &elt);
    }
  }
  return runtime->bytesJoin(thread, self, self.length(), tuple, length);
}

RawObject UnderBuiltinsModule::underBytesLen(Thread*, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  return SmallInt::fromWord(bytesUnderlying(args.get(0)).length());
}

RawObject UnderBuiltinsModule::underBytesMaketrans(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object from_obj(&scope, args.get(0));
  Object to_obj(&scope, args.get(1));
  word length;
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfBytes(*from_obj)) {
    Bytes bytes(&scope, bytesUnderlying(*from_obj));
    length = bytes.length();
    from_obj = *bytes;
  } else if (runtime->isInstanceOfByteArray(*from_obj)) {
    ByteArray array(&scope, *from_obj);
    length = array.numItems();
    from_obj = array.bytes();
  } else {
    UNIMPLEMENTED("bytes-like other than bytes or bytearray");
  }
  if (runtime->isInstanceOfBytes(*to_obj)) {
    Bytes bytes(&scope, bytesUnderlying(*to_obj));
    DCHECK(bytes.length() == length, "lengths should already be the same");
    to_obj = *bytes;
  } else if (runtime->isInstanceOfByteArray(*to_obj)) {
    ByteArray array(&scope, *to_obj);
    DCHECK(array.numItems() == length, "lengths should already be the same");
    to_obj = array.bytes();
  } else {
    UNIMPLEMENTED("bytes-like other than bytes or bytearray");
  }
  Bytes from(&scope, *from_obj);
  Bytes to(&scope, *to_obj);
  byte table[BytesBuiltins::kTranslationTableLength];
  for (word i = 0; i < BytesBuiltins::kTranslationTableLength; i++) {
    table[i] = i;
  }
  for (word i = 0; i < length; i++) {
    table[from.byteAt(i)] = to.byteAt(i);
  }
  return runtime->newBytesWithAll(table);
}

RawObject UnderBuiltinsModule::underBytesRepeat(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Bytes self(&scope, bytesUnderlying(args.get(0)));
  // TODO(T55084422): unify bounds checking
  word count = intUnderlying(args.get(1)).asWordSaturated();
  if (!SmallInt::isValid(count)) {
    Object count_obj(&scope, args.get(1));
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit '%T' into an index-sized integer",
                                &count_obj);
  }
  // NOTE: unlike __mul__, we raise a value error for negative count
  if (count < 0) {
    return thread->raiseWithFmt(LayoutId::kValueError, "negative count");
  }
  return thread->runtime()->bytesRepeat(thread, self, self.length(), count);
}

RawObject UnderBuiltinsModule::underBytesSplit(Thread* thread, Frame* frame,
                                               word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Bytes self(&scope, bytesUnderlying(args.get(0)));
  Object sep_obj(&scope, args.get(1));
  Int maxsplit_int(&scope, intUnderlying(args.get(2)));
  if (maxsplit_int.numDigits() > 1) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C ssize_t");
  }
  word maxsplit = maxsplit_int.asWord();
  if (maxsplit < 0) {
    maxsplit = kMaxWord;
  }
  word sep_len;
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfBytes(*sep_obj)) {
    Bytes sep(&scope, bytesUnderlying(*sep_obj));
    sep_obj = *sep;
    sep_len = sep.length();
  } else if (runtime->isInstanceOfByteArray(*sep_obj)) {
    ByteArray sep(&scope, *sep_obj);
    sep_obj = sep.bytes();
    sep_len = sep.numItems();
  } else {
    // TODO(T38246066): support buffer protocol
    UNIMPLEMENTED("bytes-like other than bytes or bytearray");
  }
  if (sep_len == 0) {
    return thread->raiseWithFmt(LayoutId::kValueError, "empty separator");
  }
  Bytes sep(&scope, *sep_obj);
  word self_len = self.length();

  // First pass: calculate the length of the result list.
  word splits = 0;
  word start = 0;
  while (splits < maxsplit) {
    word end = bytesFind(self, self_len, sep, sep_len, start, self_len);
    if (end < 0) {
      break;
    }
    splits++;
    start = end + sep_len;
  }
  word result_len = splits + 1;

  // Second pass: write subsequences into result list.
  List result(&scope, runtime->newList());
  MutableTuple buffer(&scope, runtime->newMutableTuple(result_len));
  start = 0;
  for (word i = 0; i < splits; i++) {
    word end = bytesFind(self, self_len, sep, sep_len, start, self_len);
    DCHECK(end != -1, "already found in first pass");
    buffer.atPut(i, runtime->bytesSubseq(thread, self, start, end - start));
    start = end + sep_len;
  }
  buffer.atPut(splits,
               runtime->bytesSubseq(thread, self, start, self_len - start));
  result.setItems(*buffer);
  result.setNumItems(result_len);
  return *result;
}

RawObject UnderBuiltinsModule::underBytesSplitWhitespace(Thread* thread,
                                                         Frame* frame,
                                                         word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Bytes self(&scope, bytesUnderlying(args.get(0)));
  Int maxsplit_int(&scope, intUnderlying(args.get(1)));
  if (maxsplit_int.numDigits() > 1) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C ssize_t");
  }
  word self_len = self.length();
  word maxsplit = maxsplit_int.asWord();
  if (maxsplit < 0) {
    maxsplit = kMaxWord;
  }

  // First pass: calculate the length of the result list.
  word splits = 0;
  word index = 0;
  while (splits < maxsplit) {
    while (index < self_len && isSpaceASCII(self.byteAt(index))) {
      index++;
    }
    if (index == self_len) break;
    index++;
    while (index < self_len && !isSpaceASCII(self.byteAt(index))) {
      index++;
    }
    splits++;
  }
  while (index < self_len && isSpaceASCII(self.byteAt(index))) {
    index++;
  }
  bool has_remaining = index < self_len;
  word result_len = has_remaining ? splits + 1 : splits;

  // Second pass: write subsequences into result list.
  Runtime* runtime = thread->runtime();
  List result(&scope, runtime->newList());
  if (result_len == 0) return *result;
  MutableTuple buffer(&scope, runtime->newMutableTuple(result_len));
  index = 0;
  for (word i = 0; i < splits; i++) {
    while (isSpaceASCII(self.byteAt(index))) {
      index++;
    }
    word start = index++;
    while (!isSpaceASCII(self.byteAt(index))) {
      index++;
    }
    buffer.atPut(i, runtime->bytesSubseq(thread, self, start, index - start));
  }
  if (has_remaining) {
    while (isSpaceASCII(self.byteAt(index))) {
      index++;
    }
    buffer.atPut(splits,
                 runtime->bytesSubseq(thread, self, index, self_len - index));
  }
  result.setItems(*buffer);
  result.setNumItems(result_len);
  return *result;
}

RawObject UnderBuiltinsModule::underByteslikeCheck(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isByteslike(args.get(0)));
}

RawObject UnderBuiltinsModule::underByteslikeCompareDigest(Thread* thread,
                                                           Frame* frame,
                                                           word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object left_obj(&scope, args.get(0));
  Object right_obj(&scope, args.get(1));
  DCHECK(runtime->isInstanceOfBytes(*left_obj) ||
             runtime->isInstanceOfByteArray(*left_obj),
         "_byteslike_compare_digest requires 'bytes' or 'bytearray' instance");
  DCHECK(runtime->isInstanceOfBytes(*right_obj) ||
             runtime->isInstanceOfByteArray(*right_obj),
         "_byteslike_compare_digest requires 'bytes' or 'bytearray' instance");
  // TODO(T57794178): Use volatile
  Bytes left(&scope, Bytes::empty());
  Bytes right(&scope, Bytes::empty());
  word left_len = 0;
  word right_len = 0;
  if (runtime->isInstanceOfBytes(*left_obj)) {
    left = bytesUnderlying(*left_obj);
    left_len = left.length();
  } else {
    ByteArray byte_array(&scope, *left_obj);
    left = byte_array.bytes();
    left_len = byte_array.numItems();
  }
  if (runtime->isInstanceOfBytes(*right_obj)) {
    right = bytesUnderlying(*right_obj);
    right_len = right.length();
  } else {
    ByteArray byte_array(&scope, *right_obj);
    right = byte_array.bytes();
    right_len = byte_array.numItems();
  }
  word length = Utils::minimum(left_len, right_len);
  word result = (right_len == left_len) ? 0 : 1;
  for (word i = 0; i < length; i++) {
    result |= left.byteAt(i) ^ right.byteAt(i);
  }
  return Bool::fromBool(result == 0);
}

RawObject UnderBuiltinsModule::underByteslikeCount(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  word haystack_len;
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes self(&scope, bytesUnderlying(*self_obj));
    self_obj = *self;
    haystack_len = self.length();
  } else if (runtime->isInstanceOfByteArray(*self_obj)) {
    ByteArray self(&scope, *self_obj);
    self_obj = self.bytes();
    haystack_len = self.numItems();
  } else {
    // TODO(T38246066): support buffer protocol
    UNIMPLEMENTED("bytes-like other than bytes, bytearray");
  }
  Object sub_obj(&scope, args.get(1));
  word needle_len;
  if (runtime->isInstanceOfBytes(*sub_obj)) {
    Bytes sub(&scope, bytesUnderlying(*sub_obj));
    sub_obj = *sub;
    needle_len = sub.length();
  } else if (runtime->isInstanceOfByteArray(*sub_obj)) {
    ByteArray sub(&scope, *sub_obj);
    sub_obj = sub.bytes();
    needle_len = sub.numItems();
  } else if (runtime->isInstanceOfInt(*sub_obj)) {
    word sub = intUnderlying(*sub_obj).asWordSaturated();
    if (sub < 0 || sub > kMaxByte) {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "byte must be in range(0, 256)");
    }
    sub_obj = runtime->newBytes(1, sub);
    needle_len = 1;
  } else {
    // TODO(T38246066): support buffer protocol
    UNIMPLEMENTED("bytes-like other than bytes, bytearray");
  }
  Bytes haystack(&scope, *self_obj);
  Bytes needle(&scope, *sub_obj);
  Object start_obj(&scope, args.get(2));
  Object stop_obj(&scope, args.get(3));
  word start = intUnderlying(*start_obj).asWordSaturated();
  word end = intUnderlying(*stop_obj).asWordSaturated();
  return SmallInt::fromWord(
      bytesCount(haystack, haystack_len, needle, needle_len, start, end));
}

RawObject UnderBuiltinsModule::underByteslikeEndswith(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  word self_len;
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes self(&scope, bytesUnderlying(*self_obj));
    self_obj = *self;
    self_len = self.length();
  } else if (runtime->isInstanceOfByteArray(*self_obj)) {
    ByteArray self(&scope, *self_obj);
    self_obj = self.bytes();
    self_len = self.numItems();
  } else {
    UNREACHABLE("self has an unexpected type");
  }
  DCHECK(self_obj.isBytes(),
         "bytes-like object not resolved to underlying bytes");
  Object suffix_obj(&scope, args.get(1));
  word suffix_len;
  if (runtime->isInstanceOfBytes(*suffix_obj)) {
    Bytes suffix(&scope, bytesUnderlying(*suffix_obj));
    suffix_obj = *suffix;
    suffix_len = suffix.length();
  } else if (runtime->isInstanceOfByteArray(*suffix_obj)) {
    ByteArray suffix(&scope, *suffix_obj);
    suffix_obj = suffix.bytes();
    suffix_len = suffix.numItems();
  } else {
    // TODO(T38246066): support buffer protocol
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "endswith first arg must be bytes or a tuple of bytes, not %T",
        &suffix_obj);
  }
  Bytes self(&scope, *self_obj);
  Bytes suffix(&scope, *suffix_obj);
  Object start_obj(&scope, args.get(2));
  Object end_obj(&scope, args.get(3));
  Int start(&scope, start_obj.isUnbound() ? Int::cast(SmallInt::fromWord(0))
                                          : intUnderlying(*start_obj));
  Int end(&scope, end_obj.isUnbound() ? Int::cast(SmallInt::fromWord(self_len))
                                      : intUnderlying(*end_obj));
  return runtime->bytesEndsWith(self, self_len, suffix, suffix_len,
                                start.asWordSaturated(), end.asWordSaturated());
}

RawObject UnderBuiltinsModule::underByteslikeFindByteslike(Thread* thread,
                                                           Frame* frame,
                                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  word haystack_len;
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes self(&scope, bytesUnderlying(*self_obj));
    self_obj = *self;
    haystack_len = self.length();
  } else if (runtime->isInstanceOfByteArray(*self_obj)) {
    ByteArray self(&scope, *self_obj);
    self_obj = self.bytes();
    haystack_len = self.numItems();
  } else {
    UNIMPLEMENTED("bytes-like other than bytes, bytearray");
  }
  Object sub_obj(&scope, args.get(1));
  word needle_len;
  if (runtime->isInstanceOfBytes(*sub_obj)) {
    Bytes sub(&scope, bytesUnderlying(*sub_obj));
    sub_obj = *sub;
    needle_len = sub.length();
  } else if (runtime->isInstanceOfByteArray(*sub_obj)) {
    ByteArray sub(&scope, *sub_obj);
    sub_obj = sub.bytes();
    needle_len = sub.numItems();
  } else {
    UNIMPLEMENTED("bytes-like other than bytes, bytearray");
  }
  Bytes haystack(&scope, *self_obj);
  Bytes needle(&scope, *sub_obj);
  word start = intUnderlying(args.get(2)).asWordSaturated();
  word end = intUnderlying(args.get(3)).asWordSaturated();
  return SmallInt::fromWord(
      bytesFind(haystack, haystack_len, needle, needle_len, start, end));
}

RawObject UnderBuiltinsModule::underByteslikeFindInt(Thread* thread,
                                                     Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  word sub = intUnderlying(args.get(1)).asWordSaturated();
  if (sub < 0 || sub > kMaxByte) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "byte must be in range(0, 256)");
  }
  Bytes needle(&scope, runtime->newBytes(1, sub));
  Object self_obj(&scope, args.get(0));
  word start = intUnderlying(args.get(2)).asWordSaturated();
  word end = intUnderlying(args.get(3)).asWordSaturated();
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes haystack(&scope, bytesUnderlying(*self_obj));
    return SmallInt::fromWord(bytesFind(haystack, haystack.length(), needle,
                                        needle.length(), start, end));
  }
  if (runtime->isInstanceOfByteArray(*self_obj)) {
    ByteArray self(&scope, *self_obj);
    Bytes haystack(&scope, self.bytes());
    return SmallInt::fromWord(bytesFind(haystack, self.numItems(), needle,
                                        needle.length(), start, end));
  }
  UNIMPLEMENTED("bytes-like other than bytes, bytearray");
}

RawObject UnderBuiltinsModule::underByteslikeGuard(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(0));
  if (thread->runtime()->isByteslike(*obj)) {
    return NoneType::object();
  }
  return thread->raiseWithFmt(
      LayoutId::kTypeError, "a bytes-like object is required, not '%T'", &obj);
}

RawObject UnderBuiltinsModule::underByteslikeRfindByteslike(Thread* thread,
                                                            Frame* frame,
                                                            word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  word haystack_len;
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes self(&scope, bytesUnderlying(*self_obj));
    self_obj = *self;
    haystack_len = self.length();
  } else if (runtime->isInstanceOfByteArray(*self_obj)) {
    ByteArray self(&scope, *self_obj);
    self_obj = self.bytes();
    haystack_len = self.numItems();
  } else {
    UNIMPLEMENTED("bytes-like other than bytes, bytearray");
  }
  Object sub_obj(&scope, args.get(1));
  word needle_len;
  if (runtime->isInstanceOfBytes(*sub_obj)) {
    Bytes sub(&scope, bytesUnderlying(*sub_obj));
    sub_obj = *sub;
    needle_len = sub.length();
  } else if (runtime->isInstanceOfByteArray(*sub_obj)) {
    ByteArray sub(&scope, *sub_obj);
    sub_obj = sub.bytes();
    needle_len = sub.numItems();
  } else {
    UNIMPLEMENTED("bytes-like other than bytes, bytearray");
  }
  Bytes haystack(&scope, *self_obj);
  Bytes needle(&scope, *sub_obj);
  word start = intUnderlying(args.get(2)).asWordSaturated();
  word end = intUnderlying(args.get(3)).asWordSaturated();
  return SmallInt::fromWord(
      bytesRFind(haystack, haystack_len, needle, needle_len, start, end));
}

RawObject UnderBuiltinsModule::underByteslikeRfindInt(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  word sub = intUnderlying(args.get(1)).asWordSaturated();
  if (sub < 0 || sub > kMaxByte) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "byte must be in range(0, 256)");
  }
  Bytes needle(&scope, runtime->newBytes(1, sub));
  Object self_obj(&scope, args.get(0));
  word start = intUnderlying(args.get(2)).asWordSaturated();
  word end = intUnderlying(args.get(3)).asWordSaturated();
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes haystack(&scope, bytesUnderlying(*self_obj));
    return SmallInt::fromWord(bytesRFind(haystack, haystack.length(), needle,
                                         needle.length(), start, end));
  }
  if (runtime->isInstanceOfByteArray(*self_obj)) {
    ByteArray self(&scope, *self_obj);
    Bytes haystack(&scope, self.bytes());
    return SmallInt::fromWord(bytesRFind(haystack, self.numItems(), needle,
                                         needle.length(), start, end));
  }
  UNIMPLEMENTED("bytes-like other than bytes, bytearray");
}

RawObject UnderBuiltinsModule::underByteslikeStartswith(Thread* thread,
                                                        Frame* frame,
                                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  word self_len;
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes self(&scope, bytesUnderlying(*self_obj));
    self_obj = *self;
    self_len = self.length();
  } else if (runtime->isInstanceOfByteArray(*self_obj)) {
    ByteArray self(&scope, *self_obj);
    self_obj = self.bytes();
    self_len = self.numItems();
  } else {
    UNREACHABLE("self has an unexpected type");
  }
  DCHECK(self_obj.isBytes(),
         "bytes-like object not resolved to underlying bytes");
  Object prefix_obj(&scope, args.get(1));
  word prefix_len;
  if (runtime->isInstanceOfBytes(*prefix_obj)) {
    Bytes prefix(&scope, bytesUnderlying(*prefix_obj));
    prefix_obj = *prefix;
    prefix_len = prefix.length();
  } else if (runtime->isInstanceOfByteArray(*prefix_obj)) {
    ByteArray prefix(&scope, *prefix_obj);
    prefix_obj = prefix.bytes();
    prefix_len = prefix.numItems();
  } else {
    // TODO(T38246066): support buffer protocol
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "startswith first arg must be bytes or a tuple of bytes, not %T",
        &prefix_obj);
  }
  Bytes self(&scope, *self_obj);
  Bytes prefix(&scope, *prefix_obj);
  word start = intUnderlying(args.get(2)).asWordSaturated();
  word end = intUnderlying(args.get(3)).asWordSaturated();
  return runtime->bytesStartsWith(self, self_len, prefix, prefix_len, start,
                                  end);
}

RawObject UnderBuiltinsModule::underClassmethod(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ClassMethod result(&scope, thread->runtime()->newClassMethod());
  result.setFunction(args.get(0));
  return *result;
}

static RawObject isAbstract(Thread* thread, const Object& obj) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  // TODO(T47800709): make this lookup more efficient
  Object abstract(&scope, runtime->attributeAtById(
                              thread, obj, SymbolId::kDunderIsabstractMethod));
  if (abstract.isError()) {
    Object given(&scope, thread->pendingExceptionType());
    Object exc(&scope, runtime->typeAt(LayoutId::kAttributeError));
    if (givenExceptionMatches(thread, given, exc)) {
      thread->clearPendingException();
      return Bool::falseObj();
    }
    return *abstract;
  }
  return Interpreter::isTrue(thread, *abstract);
}

RawObject UnderBuiltinsModule::underClassmethodIsabstract(Thread* thread,
                                                          Frame* frame,
                                                          word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ClassMethod self(&scope, args.get(0));
  Object func(&scope, self.function());
  return isAbstract(thread, func);
}

RawObject UnderBuiltinsModule::underCodeCheck(Thread*, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isCode());
}

RawObject UnderBuiltinsModule::underCodeGuard(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  if (args.get(0).isCode()) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kCode);
}

RawObject UnderBuiltinsModule::underComplexCheck(Thread* thread, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfComplex(args.get(0)));
}

RawObject UnderBuiltinsModule::underComplexImag(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kComplex);
  }
  Complex self(&scope, *self_obj);
  return runtime->newFloat(self.imag());
}

RawObject UnderBuiltinsModule::underComplexReal(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kComplex);
  }
  Complex self(&scope, *self_obj);
  return runtime->newFloat(self.real());
}

// TODO(T46009010): Move this method body into the dictionary API
RawObject UnderBuiltinsModule::underDictBucketInsert(Thread* thread,
                                                     Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Dict dict(&scope, args.get(0));
  Tuple data(&scope, dict.data());
  word index = ~Int::cast(args.get(1)).asWord();
  Object key(&scope, args.get(2));
  SmallInt hash(&scope, args.get(3));
  Object value(&scope, args.get(4));
  bool has_empty_slot = Dict::Bucket::isEmpty(*data, index);
  Dict::Bucket::set(*data, index, hash.value(), *key, *value);
  dict.setNumItems(dict.numItems() + 1);
  if (has_empty_slot) {
    dict.decrementNumUsableItems();
    dictEnsureCapacity(thread, dict);
  }
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underDictBucketKey(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Dict dict(&scope, args.get(0));
  Tuple data(&scope, dict.data());
  word index = Int::cast(args.get(1)).asWord();
  return Dict::Bucket::key(*data, index);
}

RawObject UnderBuiltinsModule::underDictBucketValue(Thread* thread,
                                                    Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Dict dict(&scope, args.get(0));
  Tuple data(&scope, dict.data());
  word index = Int::cast(args.get(1)).asWord();
  return Dict::Bucket::value(*data, index);
}

RawObject UnderBuiltinsModule::underDictBucketSetValue(Thread* thread,
                                                       Frame* frame,
                                                       word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Dict dict(&scope, args.get(0));
  Tuple data(&scope, dict.data());
  word index = Int::cast(args.get(1)).asWord();
  Object value(&scope, args.get(2));
  Dict::Bucket::setValue(*data, index, *value);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underDictCheck(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfDict(args.get(0)));
}

RawObject UnderBuiltinsModule::underDictCheckExact(Thread*, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isDict());
}

RawObject UnderBuiltinsModule::underDictGet(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  Object default_obj(&scope, args.get(2));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kDict);
  }
  Dict dict(&scope, *self);

  // Check key hash
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) return *hash_obj;
  word hash = SmallInt::cast(*hash_obj).value();
  Object result(&scope, dictAt(thread, dict, key, hash));
  if (result.isErrorNotFound()) return *default_obj;
  return *result;
}

RawObject UnderBuiltinsModule::underDictGuard(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfDict(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kDict);
}

RawObject UnderBuiltinsModule::underDictLen(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Dict self(&scope, args.get(0));
  return SmallInt::fromWord(self.numItems());
}

RawObject UnderBuiltinsModule::underDictLookup(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object dict_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "_dict_lookup expected a 'dict' self but got '%T'", &dict_obj);
  }
  Dict dict(&scope, *dict_obj);
  Object key(&scope, args.get(1));
  word hash = SmallInt::cast(args.get(2)).value();
  if (dict.capacity() == 0) {
    dict.setData(runtime->newMutableTuple(Runtime::kInitialDictCapacity *
                                          Dict::Bucket::kNumPointers));
    dict.resetNumUsableItems();
  }
  Tuple data(&scope, dict.data());
  word bucket_mask = Dict::Bucket::bucketMask(data.length());
  uword perturb = static_cast<uword>(hash);
  word index = Dict::Bucket::reduceIndex(data.length(), perturb);
  // Track the first place where an item could be inserted. This might be
  // the index zero. Therefore, all negative insertion indexes will be
  // offset by one to distinguish the zero index.
  uword insert_idx = 0;
  for (;;) {
    if (Dict::Bucket::isEmpty(*data, index)) {
      if (insert_idx == 0) insert_idx = ~index;
      return SmallInt::fromWord(insert_idx);
    }
    if (Dict::Bucket::isTombstone(*data, index)) {
      if (insert_idx == 0) insert_idx = ~index;
    } else {
      if (key.raw() == Dict::Bucket::key(*data, index).raw()) {
        return SmallInt::fromWord(index);
      }
      if (Dict::Bucket::hash(*data, index) == hash) {
        return SmallInt::fromWord(index);
      }
    }
    index = Dict::Bucket::nextBucket(index / Dict::Bucket::kNumPointers,
                                     bucket_mask, &perturb) *
            Dict::Bucket::kNumPointers;
  }
}

RawObject UnderBuiltinsModule::underDictLookupNext(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Dict dict(&scope, args.get(0));
  Tuple data(&scope, dict.data());
  word index = Int::cast(args.get(1)).asWord();
  Object key(&scope, args.get(2));
  word hash = SmallInt::cast(args.get(3)).value();
  uword perturb;
  if (args.get(4).isUnbound()) {
    perturb = static_cast<uword>(hash);
  } else {
    perturb = Int::cast(args.get(4)).asWord();
  }
  word bucket_mask = Dict::Bucket::bucketMask(data.length());
  Tuple result(&scope, thread->runtime()->newTuple(2));
  word insert_idx = 0;
  for (;;) {
    index = Dict::Bucket::nextBucket(index / Dict::Bucket::kNumPointers,
                                     bucket_mask, &perturb) *
            Dict::Bucket::kNumPointers;
    if (Dict::Bucket::isEmpty(*data, index)) {
      if (insert_idx == 0) insert_idx = ~index;
      result.atPut(0, SmallInt::fromWord(insert_idx));
      result.atPut(1, SmallInt::fromWord(perturb));
      return *result;
    }
    if (Dict::Bucket::isTombstone(*data, index)) {
      if (insert_idx == 0) insert_idx = ~index;
      continue;
    }
    if (key.raw() == Dict::Bucket::key(*data, index).raw()) {
      result.atPut(0, SmallInt::fromWord(index));
      result.atPut(1, SmallInt::fromWord(perturb));
      return *result;
    }
    if (hash == Dict::Bucket::hash(*data, index)) {
      result.atPut(0, SmallInt::fromWord(index));
      result.atPut(1, SmallInt::fromWord(perturb));
      return *result;
    }
  }
}

RawObject UnderBuiltinsModule::underDictPopitem(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Dict dict(&scope, args.get(0));
  if (dict.numItems() == 0) {
    return NoneType::object();
  }
  // TODO(T44040673): Return the last item.
  Tuple data(&scope, dict.data());
  word index = Dict::Bucket::kFirst;
  bool has_item = Dict::Bucket::nextItem(*data, &index);
  DCHECK(has_item,
         "dict.numItems() > 0, but Dict::Bucket::nextItem() returned false");
  Tuple result(&scope, thread->runtime()->newTuple(2));
  result.atPut(0, Dict::Bucket::key(*data, index));
  result.atPut(1, Dict::Bucket::value(*data, index));
  Dict::Bucket::setTombstone(*data, index);
  dict.setNumItems(dict.numItems() - 1);
  return *result;
}

RawObject UnderBuiltinsModule::underDictSetitem(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  Object value(&scope, args.get(2));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kDict);
  }
  Dict dict(&scope, *self);
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) return *hash_obj;
  word hash = SmallInt::cast(*hash_obj).value();
  dictAtPut(thread, dict, key, hash, value);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underDictUpdate(Thread* thread, Frame* frame,
                                               word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self_obj)) {
    return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kDict);
  }
  Dict self(&scope, *self_obj);
  Object other_obj(&scope, args.get(1));
  if (!other_obj.isDict()) return Unbound::object();
  if (*other_obj != *self) {
    Object key(&scope, NoneType::object());
    Object value(&scope, NoneType::object());
    Dict other(&scope, *other_obj);
    Tuple other_data(&scope, other.data());
    for (word i = Dict::Bucket::kFirst;
         Dict::Bucket::nextItem(*other_data, &i);) {
      key = Dict::Bucket::key(*other_data, i);
      value = Dict::Bucket::value(*other_data, i);
      word hash = Dict::Bucket::hash(*other_data, i);
      dictAtPut(thread, self, key, hash, value);
    }
  }
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underDivmod(Thread* thread, Frame* frame,
                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object number(&scope, args.get(0));
  Object divisor(&scope, args.get(1));
  return Interpreter::binaryOperation(
      thread, frame, Interpreter::BinaryOp::DIVMOD, number, divisor);
}

RawObject UnderBuiltinsModule::underExec(Thread* thread, Frame* frame,
                                         word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Code code(&scope, args.get(0));
  Module module(&scope, args.get(1));
  Object implicit_globals(&scope, args.get(2));
  return thread->exec(code, module, implicit_globals);
}

RawObject UnderBuiltinsModule::underFloatCheck(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfFloat(args.get(0)));
}

RawObject UnderBuiltinsModule::underFloatCheckExact(Thread*, Frame* frame,
                                                    word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isFloat());
}

static double floatDivmod(double x, double y, double* remainder) {
  double mod = std::fmod(x, y);
  double div = (x - mod) / y;

  if (mod != 0.0) {
    if ((y < 0.0) != (mod < 0.0)) {
      mod += y;
      div -= 1.0;
    }
  } else {
    mod = std::copysign(0.0, y);
  }

  double floordiv = 0;
  if (div != 0.0) {
    floordiv = std::floor(div);
    if (div - floordiv > 0.5) {
      floordiv += 1.0;
    }
  } else {
    floordiv = std::copysign(0.0, x / y);
  }

  *remainder = mod;
  return floordiv;
}

RawObject UnderBuiltinsModule::underFloatDivmod(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);

  double left = floatUnderlying(args.get(0)).value();
  double divisor = floatUnderlying(args.get(1)).value();
  if (divisor == 0.0) {
    return thread->raiseWithFmt(LayoutId::kZeroDivisionError, "float divmod()");
  }

  double remainder;
  double quotient = floatDivmod(left, divisor, &remainder);
  Runtime* runtime = thread->runtime();
  Tuple result(&scope, runtime->newTuple(2));
  result.atPut(0, runtime->newFloat(quotient));
  result.atPut(1, runtime->newFloat(remainder));
  return *result;
}

RawObject UnderBuiltinsModule::underFloatFormat(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  double value = floatUnderlying(args.get(0)).value();
  Str format_code(&scope, args.get(1));
  DCHECK(format_code.charLength() == 1, "expected len(format_code) == 1");
  char format_code_char = format_code.charAt(0);
  DCHECK(format_code_char == 'e' || format_code_char == 'E' ||
             format_code_char == 'f' || format_code_char == 'F' ||
             format_code_char == 'g' || format_code_char == 'G' ||
             format_code_char == 'r',
         "expected format_code in 'eEfFgGr'");
  SmallInt precision(&scope, args.get(2));
  Bool always_add_sign(&scope, args.get(3));
  Bool add_dot_0(&scope, args.get(4));
  Bool use_alt_formatting(&scope, args.get(5));
  unique_c_ptr<char> c_str(formatFloat(
      value, format_code_char, precision.value(), always_add_sign.value(),
      add_dot_0.value(), use_alt_formatting.value(), nullptr));
  return thread->runtime()->newStrFromCStr(c_str.get());
}

RawObject UnderBuiltinsModule::underFloatGuard(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfFloat(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kFloat);
}

static RawObject floatNew(Thread* thread, const Type& type, RawObject flt) {
  DCHECK(flt.isFloat(), "unexpected type when creating float");
  if (type.isBuiltin()) return flt;
  HandleScope scope(thread);
  Layout type_layout(&scope, type.instanceLayout());
  UserFloatBase instance(&scope, thread->runtime()->newInstance(type_layout));
  instance.setValue(flt);
  return *instance;
}

RawObject UnderBuiltinsModule::underFloatNewFromByteslike(Thread* /* thread */,
                                                          Frame* /* frame */,
                                                          word /* nargs */) {
  // TODO(T57022841): follow full CPython conversion for bytes-like objects
  UNIMPLEMENTED("float.__new__ from byteslike");
}

RawObject UnderBuiltinsModule::underFloatNewFromFloat(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  return floatNew(thread, type, args.get(1));
}

RawObject UnderBuiltinsModule::underFloatNewFromStr(Thread* thread,
                                                    Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Object arg(&scope, args.get(1));
  Str str(&scope, strUnderlying(*arg));

  // TODO(T57022841): follow full CPython conversion for strings
  char* str_end = nullptr;
  unique_c_ptr<char> c_str(str.toCStr());
  double result = std::strtod(c_str.get(), &str_end);

  // Overflow, return infinity or negative infinity.
  if (result == HUGE_VAL) {
    return floatNew(
        thread, type,
        thread->runtime()->newFloat(std::numeric_limits<double>::infinity()));
  }
  if (result == -HUGE_VAL) {
    return floatNew(
        thread, type,
        thread->runtime()->newFloat(-std::numeric_limits<double>::infinity()));
  }

  // Conversion was incomplete; the string was not a valid float.
  word expected_length = str.charLength();
  if (expected_length == 0 || str_end - c_str.get() != expected_length) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "could not convert string to float");
  }
  return floatNew(thread, type, thread->runtime()->newFloat(result));
}

RawObject UnderBuiltinsModule::underFloatSignbit(Thread*, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  double value = floatUnderlying(args.get(0)).value();
  return Bool::fromBool(std::signbit(value));
}

RawObject UnderBuiltinsModule::underFrozensetCheck(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfFrozenSet(args.get(0)));
}

RawObject UnderBuiltinsModule::underFrozensetGuard(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfFrozenSet(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kFrozenset);
}

RawObject UnderBuiltinsModule::underFunctionGlobals(Thread* thread,
                                                    Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!self.isFunction()) {
    return thread->raiseRequiresType(self, SymbolId::kFunction);
  }
  Function function(&scope, *self);
  Module module(&scope, function.moduleObject());
  return module.moduleProxy();
}

RawObject UnderBuiltinsModule::underFunctionGuard(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  if (args.get(0).isFunction()) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kFunction);
}

RawObject UnderBuiltinsModule::underGc(Thread* thread, Frame* /* frame */,
                                       word /* nargs */) {
  thread->runtime()->collectGarbage();
  return NoneType::object();
}

namespace {

class UserVisibleFrameVisitor : public FrameVisitor {
 public:
  UserVisibleFrameVisitor(word depth) : target_depth_(depth) {}

  bool visit(Frame* frame) {
    if (current_depth_ == target_depth_) {
      target_ = frame;
      return false;
    }
    current_depth_++;
    return true;
  }

  Frame* target() { return target_; }

 private:
  word current_depth_ = 0;
  const word target_depth_;
  Frame* target_ = nullptr;
};

}  // namespace

static Frame* frameAtDepth(Thread* thread, word depth) {
  UserVisibleFrameVisitor visitor(depth + 1);
  thread->visitFrames(&visitor);
  return visitor.target();
}

RawObject UnderBuiltinsModule::underGetframeFunction(Thread* thread,
                                                     Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  word depth = intUnderlying(args.get(0)).asWordSaturated();
  if (depth < 0) {
    return thread->raiseWithFmt(LayoutId::kValueError, "negative stack level");
  }
  frame = frameAtDepth(thread, depth);
  if (frame == nullptr) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "call stack is not deep enough");
  }
  return frame->function();
}

RawObject UnderBuiltinsModule::underGetframeLineno(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  word depth = intUnderlying(args.get(0)).asWordSaturated();
  if (depth < 0) {
    return thread->raiseWithFmt(LayoutId::kValueError, "negative stack level");
  }
  frame = frameAtDepth(thread, depth);
  if (frame == nullptr) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "call stack is not deep enough");
  }
  Code code(&scope, frame->code());
  word pc = frame->virtualPC();
  word lineno = code.offsetToLineNum(pc);
  return SmallInt::fromWord(lineno);
}

RawObject UnderBuiltinsModule::underGetframeLocals(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  word depth = intUnderlying(args.get(0)).asWordSaturated();
  if (depth < 0) {
    return thread->raiseWithFmt(LayoutId::kValueError, "negative stack level");
  }
  frame = frameAtDepth(thread, depth);
  if (frame == nullptr) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "call stack is not deep enough");
  }
  return frameLocals(thread, frame);
}

RawObject UnderBuiltinsModule::underGetMemberByte(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  char value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), 1);
  return thread->runtime()->newInt(value);
}

RawObject UnderBuiltinsModule::underGetMemberChar(Thread*, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  return SmallStr::fromCodePoint(*reinterpret_cast<byte*>(addr));
}

RawObject UnderBuiltinsModule::underGetMemberDouble(Thread* thread,
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  double value = 0.0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newFloat(value);
}

RawObject UnderBuiltinsModule::underGetMemberFloat(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  float value = 0.0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newFloat(value);
}

RawObject UnderBuiltinsModule::underGetMemberInt(Thread* thread, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  int value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newInt(value);
}

RawObject UnderBuiltinsModule::underGetMemberLong(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  long value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newInt(value);
}

RawObject UnderBuiltinsModule::underGetMemberPyobject(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  ApiHandle* value =
      *reinterpret_cast<ApiHandle**>(Int::cast(args.get(0)).asCPtr());
  if (value == nullptr) {
    if (args.get(1).isNoneType()) return NoneType::object();
    HandleScope scope(thread);
    Str name(&scope, args.get(1));
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "Object attribute '%S' is nullptr", &name);
  }
  return value->asObject();
}

RawObject UnderBuiltinsModule::underGetMemberShort(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  short value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newInt(value);
}

RawObject UnderBuiltinsModule::underGetMemberString(Thread* thread,
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  return thread->runtime()->newStrFromCStr(*reinterpret_cast<char**>(addr));
}

RawObject UnderBuiltinsModule::underGetMemberUbyte(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  unsigned char value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newIntFromUnsigned(value);
}

RawObject UnderBuiltinsModule::underGetMemberUint(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  unsigned int value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newIntFromUnsigned(value);
}

RawObject UnderBuiltinsModule::underGetMemberUlong(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  unsigned long value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newIntFromUnsigned(value);
}

RawObject UnderBuiltinsModule::underGetMemberUshort(Thread* thread,
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  unsigned short value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newIntFromUnsigned(value);
}

RawObject UnderBuiltinsModule::underInstanceDelattr(Thread* thread,
                                                    Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Instance instance(&scope, args.get(0));
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  return instanceDelAttr(thread, instance, name);
}

RawObject UnderBuiltinsModule::underInstanceGetattr(Thread* thread,
                                                    Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Instance instance(&scope, args.get(0));
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object result(&scope, instanceGetAttribute(thread, instance, name));
  return result.isErrorNotFound() ? Unbound::object() : *result;
}

RawObject UnderBuiltinsModule::underInstanceGuard(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  if (args.get(0).isInstance()) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kInstance);
}

RawObject UnderBuiltinsModule::underInstanceOverflowDict(Thread* thread,
                                                         Frame* frame,
                                                         word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object object(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, runtime->layoutAt(object.layoutId()));
  CHECK(layout.hasDictOverflow(), "expected dict overflow layout");
  word offset = layout.dictOverflowOffset();
  Instance instance(&scope, *object);
  Object overflow_dict_obj(&scope, instance.instanceVariableAt(offset));
  if (overflow_dict_obj.isNoneType()) {
    overflow_dict_obj = runtime->newDict();
    instance.instanceVariableAtPut(offset, *overflow_dict_obj);
  }
  return *overflow_dict_obj;
}

RawObject UnderBuiltinsModule::underInstanceSetattr(Thread* thread,
                                                    Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Instance instance(&scope, args.get(0));
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object value(&scope, args.get(2));
  return instanceSetAttr(thread, instance, name, value);
}

RawObject UnderBuiltinsModule::underIntCheck(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfInt(args.get(0)));
}

RawObject UnderBuiltinsModule::underIntCheckExact(Thread*, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  RawObject arg = args.get(0);
  return Bool::fromBool(arg.isSmallInt() || arg.isLargeInt());
}

static RawObject intOrUserSubclass(Thread* thread, const Type& type,
                                   const Object& value) {
  DCHECK(value.isSmallInt() || value.isLargeInt(),
         "builtin value should have type int");
  DCHECK(type.builtinBase() == LayoutId::kInt, "type must subclass int");
  if (type.isBuiltin()) return *value;
  HandleScope scope(thread);
  Layout layout(&scope, type.instanceLayout());
  UserIntBase instance(&scope, thread->runtime()->newInstance(layout));
  instance.setValue(*value);
  return *instance;
}

RawObject UnderBuiltinsModule::underIntFromBytes(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();

  Type type(&scope, args.get(0));
  Bytes bytes(&scope, args.get(1));
  Bool byteorder_big(&scope, args.get(2));
  endian endianness = byteorder_big.value() ? endian::big : endian::little;
  Bool signed_arg(&scope, args.get(3));
  bool is_signed = signed_arg == Bool::trueObj();
  Int value(&scope, runtime->bytesToInt(thread, bytes, endianness, is_signed));
  return intOrUserSubclass(thread, type, value);
}

RawObject UnderBuiltinsModule::underIntGuard(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfInt(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kInt);
}

static word digitValue(byte digit, word base) {
  if ('0' <= digit && digit < '0' + base) return digit - '0';
  // Bases 2-10 are limited to numerals, but all greater bases can use letters
  // too.
  if (base <= 10) return -1;
  if ('a' <= digit && digit < 'a' + base) return digit - 'a' + 10;
  if ('A' <= digit && digit < 'A' + base) return digit - 'A' + 10;
  return -1;
}

static word inferBase(byte second_byte) {
  switch (second_byte) {
    case 'x':
    case 'X':
      return 16;
    case 'o':
    case 'O':
      return 8;
    case 'b':
    case 'B':
      return 2;
    default:
      return 10;
  }
}

static RawObject intFromBytes(Thread* thread, const Bytes& bytes, word length,
                              word base) {
  DCHECK_BOUND(length, bytes.length());
  DCHECK(base == 0 || (base >= 2 && base <= 36), "invalid base");
  // Functions the same as intFromString
  word idx = 0;
  if (idx >= length) return Error::error();
  byte b = bytes.byteAt(idx++);
  while (isSpaceASCII(b)) {
    if (idx >= length) return Error::error();
    b = bytes.byteAt(idx++);
  }
  word sign = 1;
  switch (b) {
    case '-':
      sign = -1;
      // fall through
    case '+':
      if (idx >= length) return Error::error();
      b = bytes.byteAt(idx++);
      break;
  }

  word inferred_base = 10;
  if (b == '0') {
    if (idx >= length) return SmallInt::fromWord(0);
    inferred_base = inferBase(bytes.byteAt(idx));
    if (base == 0) base = inferred_base;
    if (inferred_base != 10 && base == inferred_base) {
      if (++idx >= length) return Error::error();
      b = bytes.byteAt(idx++);
    }
  } else if (base == 0) {
    base = 10;
  }

  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Int result(&scope, SmallInt::fromWord(0));
  Int digit(&scope, SmallInt::fromWord(0));
  Int base_obj(&scope, SmallInt::fromWord(base));
  word num_start = idx;
  for (;;) {
    if (b == '_') {
      // No leading underscores unless the number has a prefix
      if (idx == num_start && inferred_base == 10) return Error::error();
      // No trailing underscores
      if (idx >= length) return Error::error();
      b = bytes.byteAt(idx++);
    }
    word digit_val = digitValue(b, base);
    if (digit_val == -1) return Error::error();
    digit = Int::cast(SmallInt::fromWord(digit_val));
    result = runtime->intAdd(thread, result, digit);
    if (idx >= length) break;
    b = bytes.byteAt(idx++);
    result = runtime->intMultiply(thread, result, base_obj);
  }
  if (sign < 0) {
    result = runtime->intNegate(thread, result);
  }
  return *result;
}

RawObject UnderBuiltinsModule::underIntNewFromBytearray(Thread* thread,
                                                        Frame* frame,
                                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  ByteArray array(&scope, args.get(1));
  Bytes bytes(&scope, array.bytes());
  word base = intUnderlying(args.get(2)).asWord();
  Object result(&scope, intFromBytes(thread, bytes, array.numItems(), base));
  if (result.isError()) {
    Runtime* runtime = thread->runtime();
    Bytes truncated(&scope, byteArrayAsBytes(thread, runtime, array));
    Str repr(&scope, bytesReprSmartQuotes(thread, truncated));
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "invalid literal for int() with base %w: %S",
                                base, &repr);
  }
  return intOrUserSubclass(thread, type, result);
}

RawObject UnderBuiltinsModule::underIntNewFromBytes(Thread* thread,
                                                    Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Bytes bytes(&scope, bytesUnderlying(args.get(1)));
  word base = intUnderlying(args.get(2)).asWord();
  Object result(&scope, intFromBytes(thread, bytes, bytes.length(), base));
  if (result.isError()) {
    Str repr(&scope, bytesReprSmartQuotes(thread, bytes));
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "invalid literal for int() with base %w: %S",
                                base, &repr);
  }
  return intOrUserSubclass(thread, type, result);
}

RawObject UnderBuiltinsModule::underIntNewFromInt(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Object value(&scope, args.get(1));
  if (value.isBool()) {
    value = convertBoolToInt(*value);
  } else if (!value.isSmallInt() && !value.isLargeInt()) {
    value = intUnderlying(*value);
  }
  return intOrUserSubclass(thread, type, value);
}

static RawObject intFromStr(Thread* thread, const Str& str, word base) {
  DCHECK(base == 0 || (base >= 2 && base <= 36), "invalid base");
  // CPython allows leading whitespace in the integer literal
  word start = strFindFirstNonWhitespace(str);
  if (str.charLength() - start == 0) {
    return Error::error();
  }
  word sign = 1;
  if (str.charAt(start) == '-') {
    sign = -1;
    start += 1;
  } else if (str.charAt(start) == '+') {
    start += 1;
  }
  if (str.charLength() - start == 0) {
    // Just the sign
    return Error::error();
  }
  if (str.charLength() - start == 1) {
    // Single digit, potentially with +/-
    word result = digitValue(str.charAt(start), base == 0 ? 10 : base);
    if (result == -1) return Error::error();
    return SmallInt::fromWord(sign * result);
  }
  // Decimal literals start at the index 0 (no prefix).
  // Octal literals (0oFOO), hex literals (0xFOO), and binary literals (0bFOO)
  // start at index 2.
  word inferred_base = 10;
  if (str.charAt(start) == '0' && start + 1 < str.charLength()) {
    inferred_base = inferBase(str.charAt(start + 1));
  }
  if (base == 0) {
    base = inferred_base;
  }
  if (base == 2 || base == 8 || base == 16) {
    if (base == inferred_base) {
      // This handles integer literals with a base prefix, e.g.
      // * int("0b1", 0) => 1, where the base is inferred from the prefix
      // * int("0b1", 2) => 1, where the prefix matches the provided base
      //
      // If the prefix does not match the provided base, then we treat it as
      // part as part of the number, e.g.
      // * int("0b1", 10) => ValueError
      // * int("0b1", 16) => 177
      start += 2;
    }
    if (str.charLength() - start == 0) {
      // Just the prefix: 0x, 0b, 0o, etc
      return Error::error();
    }
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Int result(&scope, SmallInt::fromWord(0));
  Int digit(&scope, SmallInt::fromWord(0));
  Int base_obj(&scope, SmallInt::fromWord(base));
  for (word i = start; i < str.charLength(); i++) {
    byte digit_char = str.charAt(i);
    if (digit_char == '_') {
      // No leading underscores unless the number has a prefix
      if (i == start && inferred_base == 10) return Error::error();
      // No trailing underscores
      if (i + 1 == str.charLength()) return Error::error();
      digit_char = str.charAt(++i);
    }
    word digit_val = digitValue(digit_char, base);
    if (digit_val == -1) return Error::error();
    digit = Int::cast(SmallInt::fromWord(digit_val));
    result = runtime->intMultiply(thread, result, base_obj);
    result = runtime->intAdd(thread, result, digit);
  }
  if (sign < 0) {
    result = runtime->intNegate(thread, result);
  }
  return *result;
}

RawObject UnderBuiltinsModule::underIntNewFromStr(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Str str(&scope, args.get(1));
  word base = intUnderlying(args.get(2)).asWord();
  Object result(&scope, intFromStr(thread, str, base));
  if (result.isError()) {
    Str repr(&scope, thread->invokeMethod1(str, SymbolId::kDunderRepr));
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "invalid literal for int() with base %w: %S",
                                base == 0 ? 10 : base, &repr);
  }
  return intOrUserSubclass(thread, type, result);
}

RawObject UnderBuiltinsModule::underIter(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object object(&scope, args.get(0));
  return Interpreter::createIterator(thread, thread->currentFrame(), object);
}

RawObject UnderBuiltinsModule::underListCheck(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfList(args.get(0)));
}

RawObject UnderBuiltinsModule::underListCheckExact(Thread*, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isList());
}

RawObject UnderBuiltinsModule::underListDelitem(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  List self(&scope, args.get(0));
  word length = self.numItems();
  word idx = intUnderlying(args.get(1)).asWordSaturated();
  if (idx < 0) {
    idx += length;
  }
  if (idx < 0 || idx >= length) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "list assignment index out of range");
  }
  listPop(thread, self, idx);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underListDelslice(Thread* thread, Frame* frame,
                                                 word nargs) {
  // This function deletes elements that are specified by a slice by copying.
  // It compacts to the left elements in the slice range and then copies
  // elements after the slice into the free area.  The list element count is
  // decremented and elements in the unused part of the list are overwritten
  // with None.
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  List list(&scope, args.get(0));

  word start = SmallInt::cast(args.get(1)).value();
  word stop = SmallInt::cast(args.get(2)).value();
  word step = SmallInt::cast(args.get(3)).value();

  word slice_length = Slice::length(start, stop, step);
  DCHECK(slice_length >= 0, "slice length should be positive");
  if (slice_length == 0) {
    // Nothing to delete
    return NoneType::object();
  }
  if (slice_length == list.numItems()) {
    // Delete all the items
    list.clearFrom(0);
    return NoneType::object();
  }
  if (step < 0) {
    // Adjust step to make iterating easier
    start = start + step * (slice_length - 1);
    step = -step;
  }
  DCHECK(start >= 0, "start should be positive");
  DCHECK(start < list.numItems(), "start should be in bounds");
  DCHECK(step <= list.numItems() || slice_length == 1,
         "Step should be in bounds or only one element should be sliced");
  // Sliding compaction of elements out of the slice to the left
  // Invariant: At each iteration of the loop, `fast` is the index of an
  // element addressed by the slice.
  // Invariant: At each iteration of the inner loop, `slow` is the index of a
  // location to where we are relocating a slice addressed element. It is *not*
  // addressed by the slice.
  word fast = start;
  for (word i = 1; i < slice_length; i++) {
    DCHECK_INDEX(fast, list.numItems());
    word slow = fast + 1;
    fast += step;
    for (; slow < fast; slow++) {
      list.atPut(slow - i, list.at(slow));
    }
  }
  // Copy elements into the space where the deleted elements were
  for (word i = fast + 1; i < list.numItems(); i++) {
    list.atPut(i - slice_length, list.at(i));
  }
  word new_length = list.numItems() - slice_length;
  DCHECK(new_length >= 0, "new_length must be positive");
  // Untrack all deleted elements
  list.clearFrom(new_length);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underListExtend(Thread* thread, Frame* frame,
                                               word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  List list(&scope, args.get(0));
  Object value(&scope, args.get(1));
  return listExtend(thread, list, value);
}

RawObject UnderBuiltinsModule::underListGetitem(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfList(*self_obj)) {
    return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kList);
  }
  Object key(&scope, args.get(1));
  if (runtime->isInstanceOfInt(*key)) {
    word index = intUnderlying(*key).asWordSaturated();
    if (!SmallInt::isValid(index)) {
      return thread->raiseWithFmt(LayoutId::kIndexError,
                                  "cannot fit '%T' into an index-sized integer",
                                  &key);
    }
    List self(&scope, *self_obj);
    word length = self.numItems();
    if (index < 0) {
      index += length;
    }
    if (index < 0 || index >= length) {
      return thread->raiseWithFmt(LayoutId::kIndexError,
                                  "list index out of range");
    }
    return self.at(index);
  }

  word start, stop;
  if (!trySlice(key, &start, &stop)) {
    return Unbound::object();
  }

  List self(&scope, *self_obj);
  word result_len = Slice::adjustIndices(self.numItems(), &start, &stop, 1);
  if (result_len == 0) {
    return runtime->newList();
  }
  Tuple src(&scope, self.items());
  MutableTuple dst(&scope, runtime->newMutableTuple(result_len));
  dst.replaceFromWithStartAt(0, *src, result_len, start);
  List result(&scope, runtime->newList());
  result.setItems(*dst);
  result.setNumItems(result_len);
  return *result;
}

RawObject UnderBuiltinsModule::underListGetslice(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  List self(&scope, args.get(0));
  word start = SmallInt::cast(args.get(1)).value();
  word stop = SmallInt::cast(args.get(2)).value();
  word step = SmallInt::cast(args.get(3)).value();
  return listSlice(thread, self, start, stop, step);
}

RawObject UnderBuiltinsModule::underListGuard(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfList(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kList);
}

RawObject UnderBuiltinsModule::underListLen(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  List self(&scope, args.get(0));
  return SmallInt::fromWord(self.numItems());
}

RawObject UnderBuiltinsModule::underListSort(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  CHECK(thread->runtime()->isInstanceOfList(args.get(0)),
        "Unsupported argument type for 'ls'");
  List list(&scope, args.get(0));
  return listSort(thread, list);
}

RawObject UnderBuiltinsModule::underListSwap(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  List list(&scope, args.get(0));
  word i = SmallInt::cast(args.get(1)).value();
  word j = SmallInt::cast(args.get(2)).value();
  RawObject tmp = list.at(i);
  list.atPut(i, list.at(j));
  list.atPut(j, tmp);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underMappingproxyGuard(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  if (args.get(0).isMappingProxy()) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kMappingproxy);
}

RawObject UnderBuiltinsModule::underMappingproxyMapping(Thread* thread,
                                                        Frame* frame,
                                                        word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  MappingProxy mappingproxy(&scope, args.get(0));
  return mappingproxy.mapping();
}

RawObject UnderBuiltinsModule::underMappingproxySetMapping(Thread* thread,
                                                           Frame* frame,
                                                           word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  MappingProxy mappingproxy(&scope, args.get(0));
  mappingproxy.setMapping(args.get(1));
  return *mappingproxy;
}

RawObject UnderBuiltinsModule::underMemoryviewCheck(Thread*, Frame* frame,
                                                    word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isMemoryView());
}

RawObject UnderBuiltinsModule::underMemoryviewGuard(Thread* thread,
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (args.get(0).isMemoryView()) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kMemoryView);
}

RawObject UnderBuiltinsModule::underMemoryviewItemsize(Thread* thread,
                                                       Frame* frame,
                                                       word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isMemoryView()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kMemoryView);
  }
  MemoryView self(&scope, *self_obj);
  return memoryviewItemsize(thread, self);
}

RawObject UnderBuiltinsModule::underMemoryviewNbytes(Thread* thread,
                                                     Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isMemoryView()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kMemoryView);
  }
  MemoryView self(&scope, *self_obj);
  return SmallInt::fromWord(self.length());
}

RawObject UnderBuiltinsModule::underMemoryviewSetitem(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isMemoryView()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kMemoryView);
  }
  MemoryView self(&scope, *self_obj);
  if (self.readOnly()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "cannot modify read-only memory");
  }
  Object index_obj(&scope, args.get(1));
  if (!index_obj.isInt()) return Unbound::object();
  Int index_int(&scope, *index_obj);
  word index = index_int.asWord();
  word item_size = SmallInt::cast(memoryviewItemsize(thread, self)).value();
  word byte_index = (index < 0 ? -index : index) * item_size;
  if (byte_index + item_size > self.length()) {
    return thread->raiseWithFmt(LayoutId::kIndexError, "index out of bounds");
  }
  if (index < 0) {
    byte_index = self.length() - byte_index;
  }

  Object value(&scope, args.get(2));
  Int bytes(&scope, SmallInt::fromWord(byte_index));
  return memoryviewSetitem(thread, self, bytes, value);
}

RawObject UnderBuiltinsModule::underMemoryviewSetslice(Thread* thread,
                                                       Frame* frame,
                                                       word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isMemoryView()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kMemoryView);
  }
  MemoryView self(&scope, *self_obj);
  if (self.readOnly()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "cannot modify read-only memory");
  }
  Int start_int(&scope, intUnderlying(args.get(1)));
  word start = start_int.asWord();
  Int stop_int(&scope, intUnderlying(args.get(2)));
  word stop = stop_int.asWord();
  Int step_int(&scope, intUnderlying(args.get(3)));
  word step = step_int.asWord();
  word slice_len = Slice::adjustIndices(self.length(), &start, &stop, step);
  Object value(&scope, args.get(4));
  return memoryviewSetslice(thread, self, start, stop, step, slice_len, value);
}

RawObject UnderBuiltinsModule::underModuleDir(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Module self(&scope, args.get(0));
  return moduleKeys(thread, self);
}

RawObject UnderBuiltinsModule::underModuleProxy(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Module module(&scope, args.get(0));
  return module.moduleProxy();
}

RawObject UnderBuiltinsModule::underModuleProxyCheck(Thread*, Frame* frame,
                                                     word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isModuleProxy());
}

RawObject UnderBuiltinsModule::underModuleProxyDelitem(Thread* thread,
                                                       Frame* frame,
                                                       word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  ModuleProxy self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  Module module(&scope, self.module());
  DCHECK(module.moduleProxy() == self, "module.proxy != proxy.module");
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) return *hash_obj;
  word hash = SmallInt::cast(*hash_obj).value();
  Object result(&scope, moduleRemove(thread, module, key, hash));
  if (result.isErrorNotFound()) {
    return thread->raiseWithFmt(LayoutId::kKeyError, "'%S'", &key);
  }
  return *result;
}

RawObject UnderBuiltinsModule::underModuleProxyGet(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  ModuleProxy self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object default_obj(&scope, args.get(2));
  Module module(&scope, self.module());
  DCHECK(module.moduleProxy() == self, "module.proxy != proxy.module");
  Object result(&scope, moduleAt(thread, module, name));
  if (result.isError()) {
    return *default_obj;
  }
  return *result;
}

RawObject UnderBuiltinsModule::underModuleProxyGuard(Thread* thread,
                                                     Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (args.get(0).isModuleProxy()) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kModuleProxy);
}

RawObject UnderBuiltinsModule::underModuleProxyKeys(Thread* thread,
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  ModuleProxy self(&scope, args.get(0));
  Module module(&scope, self.module());
  DCHECK(module.moduleProxy() == self, "module.proxy != proxy.module");
  return moduleKeys(thread, module);
}

RawObject UnderBuiltinsModule::underModuleProxyLen(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  ModuleProxy self(&scope, args.get(0));
  Module module(&scope, self.module());
  DCHECK(module.moduleProxy() == self, "module.proxy != proxy.module");
  return moduleLen(thread, module);
}

RawObject UnderBuiltinsModule::underModuleProxySetitem(Thread* thread,
                                                       Frame* frame,
                                                       word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  ModuleProxy self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object value(&scope, args.get(2));
  Module module(&scope, self.module());
  DCHECK(module.moduleProxy() == self, "module.proxy != proxy.module");
  return moduleAtPut(thread, module, name, value);
}

RawObject UnderBuiltinsModule::underModuleProxyValues(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  ModuleProxy self(&scope, args.get(0));
  Module module(&scope, self.module());
  DCHECK(module.moduleProxy() == self, "module.proxy != proxy.module");
  return moduleValues(thread, module);
}

RawObject UnderBuiltinsModule::underObjectKeys(Thread* thread, Frame* frame,
                                               word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object object(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, runtime->layoutAt(object.layoutId()));
  List result(&scope, runtime->newList());
  // Add in-object attributes
  Tuple in_object(&scope, layout.inObjectAttributes());
  word in_object_length = in_object.length();
  word result_length = in_object_length;
  if (layout.hasTupleOverflow()) {
    result_length += Tuple::cast(layout.overflowAttributes()).length();
  }
  for (word i = 0; i < in_object_length; i++) {
    Tuple pair(&scope, in_object.at(i));
    Object name(&scope, pair.at(0));
    if (name.isNoneType()) continue;
    runtime->listAdd(thread, result, name);
  }
  // Add overflow attributes
  if (layout.hasTupleOverflow()) {
    Tuple overflow(&scope, layout.overflowAttributes());
    for (word i = 0, length = overflow.length(); i < length; i++) {
      Tuple pair(&scope, overflow.at(i));
      Object name(&scope, pair.at(0));
      if (name.isNoneType()) continue;
      runtime->listAdd(thread, result, name);
    }
  } else {
    // TODO(T57446141): Dict overflow should be handled by a __dict__ descriptor
    // on the type, like `type` or `function`
    CHECK(layout.overflowAttributes().isNoneType(), "no overflow");
  }
  return *result;
}

RawObject UnderBuiltinsModule::underObjectTypeGetattr(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object instance(&scope, args.get(0));
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Type type(&scope, thread->runtime()->typeOf(*instance));
  Object attr(&scope, typeLookupInMro(thread, type, name));
  if (attr.isErrorNotFound()) {
    return Unbound::object();
  }
  return resolveDescriptorGet(thread, attr, instance, type);
}

RawObject UnderBuiltinsModule::underObjectTypeHasattr(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Type type(&scope, thread->runtime()->typeOf(args.get(0)));
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object result(&scope, typeLookupInMro(thread, type, name));
  return Bool::fromBool(!result.isErrorNotFound());
}

RawObject UnderBuiltinsModule::underOsWrite(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object fd_obj(&scope, args.get(0));
  CHECK(fd_obj.isSmallInt(), "fd must be small int");
  Object bytes_obj(&scope, args.get(1));
  Bytes bytes_buf(&scope, Bytes::empty());
  size_t count;
  // TODO(T55505775): Add support for more byteslike types instead of switching
  // on bytes/bytearray
  if (bytes_obj.isByteArray()) {
    bytes_buf = ByteArray::cast(*bytes_obj).bytes();
    count = ByteArray::cast(*bytes_obj).numItems();
  } else {
    bytes_buf = *bytes_obj;
    count = bytes_buf.length();
  }
  std::unique_ptr<byte[]> buffer(new byte[count]);
  bytes_buf.copyTo(buffer.get(), count);
  ssize_t result;
  {
    int fd = SmallInt::cast(*fd_obj).value();
    do {
      result = ::write(fd, buffer.get(), count);
    } while (result == -1 && errno == EINTR);
  }
  if (result == -1) {
    DCHECK(errno != EINTR, "this should have been handled in the loop");
    return thread->raiseOSErrorFromErrno(errno);
  }
  return SmallInt::fromWord(result);
}

RawObject UnderBuiltinsModule::underPatch(Thread* thread, Frame* frame,
                                          word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);

  Object patch_fn_obj(&scope, args.get(0));
  if (!patch_fn_obj.isFunction()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "_patch expects function argument");
  }
  Function patch_fn(&scope, *patch_fn_obj);
  Object fn_name(&scope, patch_fn.name());
  Runtime* runtime = thread->runtime();
  Object module_name(&scope, patch_fn.module());
  Module module(&scope, runtime->findModule(module_name));
  Object base_fn_obj(&scope, moduleAt(thread, module, fn_name));
  if (!base_fn_obj.isFunction()) {
    if (base_fn_obj.isErrorNotFound()) {
      return thread->raiseWithFmt(LayoutId::kAttributeError,
                                  "function %S not found in module %S",
                                  &fn_name, &module_name);
    }
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "_patch can only patch functions");
  }
  Function base_fn(&scope, *base_fn_obj);
  copyFunctionEntries(thread, base_fn, patch_fn);
  return *patch_fn;
}

RawObject UnderBuiltinsModule::underProperty(Thread* thread, Frame* frame,
                                             word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object getter(&scope, args.get(0));
  Object setter(&scope, args.get(1));
  Object deleter(&scope, args.get(2));
  // TODO(T42363565) Do something with the doc argument.
  return thread->runtime()->newProperty(getter, setter, deleter);
}

RawObject UnderBuiltinsModule::underPropertyIsabstract(Thread* thread,
                                                       Frame* frame,
                                                       word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Property self(&scope, args.get(0));
  Object getter(&scope, self.getter());
  Object abstract(&scope, isAbstract(thread, getter));
  if (abstract != Bool::falseObj()) {
    return *abstract;
  }
  Object setter(&scope, self.setter());
  if ((abstract = isAbstract(thread, setter)) != Bool::falseObj()) {
    return *abstract;
  }
  Object deleter(&scope, self.deleter());
  return isAbstract(thread, deleter);
}

RawObject UnderBuiltinsModule::underPyobjectOffset(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  auto addr =
      reinterpret_cast<uword>(thread->runtime()->nativeProxyPtr(args.get(0)));
  addr += RawInt::cast(args.get(1)).asWord();
  return thread->runtime()->newIntFromCPtr(bit_cast<void*>(addr));
}

RawObject UnderBuiltinsModule::underRangeCheck(Thread*, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isRange());
}

RawObject UnderBuiltinsModule::underRangeGuard(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  if (args.get(0).isRange()) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kRange);
}

RawObject UnderBuiltinsModule::underRangeLen(Thread* thread, Frame* frame,
                                             word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Range self(&scope, args.get(0));
  Object start(&scope, self.start());
  Object stop(&scope, self.stop());
  Object step(&scope, self.step());
  return rangeLen(thread, start, stop, step);
}

RawObject UnderBuiltinsModule::underReprEnter(Thread* thread, Frame* frame,
                                              word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(0));
  return thread->reprEnter(obj);
}

RawObject UnderBuiltinsModule::underReprLeave(Thread* thread, Frame* frame,
                                              word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(0));
  thread->reprLeave(obj);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underSeqIndex(Thread* thread, Frame* frame,
                                             word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  SeqIterator self(&scope, args.get(0));
  return SmallInt::fromWord(self.index());
}

RawObject UnderBuiltinsModule::underSeqIterable(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  SeqIterator self(&scope, args.get(0));
  return self.iterable();
}

RawObject UnderBuiltinsModule::underSeqSetIndex(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  SeqIterator self(&scope, args.get(0));
  Int index(&scope, args.get(1));
  self.setIndex(index.asWord());
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underSeqSetIterable(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  SeqIterator self(&scope, args.get(0));
  Object iterable(&scope, args.get(1));
  self.setIterable(*iterable);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underSetCheck(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfSet(args.get(0)));
}

RawObject UnderBuiltinsModule::underSetGuard(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfSet(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kSet);
}

RawObject UnderBuiltinsModule::underSetLen(Thread* thread, Frame* frame,
                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Set self(&scope, args.get(0));
  return SmallInt::fromWord(self.numItems());
}

RawObject UnderBuiltinsModule::underSetMemberDouble(Thread*, Frame* frame,
                                                    word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  double value = Float::cast(args.get(1)).value();
  std::memcpy(reinterpret_cast<void*>(addr), &value, sizeof(value));
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underSetMemberFloat(Thread*, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  float value = Float::cast(args.get(1)).value();
  std::memcpy(reinterpret_cast<void*>(addr), &value, sizeof(value));
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underSetMemberIntegral(Thread*, Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  auto value = RawInt::cast(args.get(1)).asWord();
  auto num_bytes = RawInt::cast(args.get(2)).asWord();
  std::memcpy(reinterpret_cast<void*>(addr), &value, num_bytes);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underSetMemberPyobject(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  ApiHandle* newvalue = ApiHandle::newReference(thread, args.get(1));
  ApiHandle** oldvalue =
      reinterpret_cast<ApiHandle**>(Int::cast(args.get(0)).asCPtr());
  (*oldvalue)->decref();
  *oldvalue = newvalue;
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underSliceCheck(Thread*, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isSlice());
}

RawObject UnderBuiltinsModule::underSliceGuard(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  if (args.get(0).isSlice()) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kSlice);
}

RawObject UnderBuiltinsModule::underSliceStart(Thread*, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  RawObject start_obj = args.get(0);
  word step = SmallInt::cast(args.get(1)).value();
  word length = SmallInt::cast(args.get(2)).value();
  if (start_obj.isNoneType()) {
    return SmallInt::fromWord(step < 0 ? length - 1 : 0);
  }

  word lower, upper;
  if (step < 0) {
    lower = -1;
    upper = length - 1;
  } else {
    lower = 0;
    upper = length;
  }

  word start = intUnderlying(start_obj).asWordSaturated();
  if (start < 0) {
    start = Utils::maximum(start + length, lower);
  } else {
    start = Utils::minimum(start, upper);
  }
  return SmallInt::fromWord(start);
}

RawObject UnderBuiltinsModule::underSliceStartLong(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Int step(&scope, intUnderlying(args.get(1)));
  Int length(&scope, intUnderlying(args.get(2)));
  bool negative_step = step.isNegative();
  Int lower(&scope, SmallInt::fromWord(negative_step ? -1 : 0));
  Runtime* runtime = thread->runtime();
  // upper = length + lower; if step < 0, then lower = 0 anyway
  Int upper(&scope,
            negative_step ? runtime->intAdd(thread, length, lower) : *length);
  Object start_obj(&scope, args.get(0));
  if (start_obj.isNoneType()) {
    return negative_step ? *upper : *lower;
  }
  Int start(&scope, intUnderlying(*start_obj));
  if (start.isNegative()) {
    start = runtime->intAdd(thread, start, length);
    if (start.compare(*lower) < 0) {
      start = *lower;
    }
  } else if (start.compare(*upper) > 0) {
    start = *upper;
  }
  return *start;
}

RawObject UnderBuiltinsModule::underSliceStep(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  RawObject step_obj = args.get(0);
  if (step_obj.isNoneType()) return SmallInt::fromWord(1);
  RawInt step = intUnderlying(step_obj);
  if (step == SmallInt::fromWord(0) || step == Bool::falseObj()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "slice step cannot be zero");
  }
  if (step.isSmallInt()) {
    return step;
  }
  if (step == Bool::trueObj()) {
    return SmallInt::fromWord(1);
  }
  return SmallInt::fromWord(step.isNegative() ? SmallInt::kMinValue
                                              : SmallInt::kMaxValue);
}

RawObject UnderBuiltinsModule::underSliceStepLong(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  RawObject step_obj = args.get(0);
  if (step_obj.isNoneType()) return SmallInt::fromWord(1);
  RawInt step = intUnderlying(step_obj);
  if (step == SmallInt::fromWord(0) || step == Bool::falseObj()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "slice step cannot be zero");
  }
  if (step.isSmallInt()) {
    return step;
  }
  if (step == Bool::trueObj()) {
    return SmallInt::fromWord(1);
  }
  return step;
}

RawObject UnderBuiltinsModule::underSliceStop(Thread*, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  RawObject stop_obj = args.get(0);
  word step = SmallInt::cast(args.get(1)).value();
  word length = SmallInt::cast(args.get(2)).value();
  if (stop_obj.isNoneType()) {
    return SmallInt::fromWord(step < 0 ? -1 : length);
  }

  word lower, upper;
  if (step < 0) {
    lower = -1;
    upper = length - 1;
  } else {
    lower = 0;
    upper = length;
  }

  word stop = intUnderlying(stop_obj).asWordSaturated();
  if (stop < 0) {
    stop = Utils::maximum(stop + length, lower);
  } else {
    stop = Utils::minimum(stop, upper);
  }
  return SmallInt::fromWord(stop);
}

RawObject UnderBuiltinsModule::underSliceStopLong(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Int step(&scope, intUnderlying(args.get(1)));
  Int length(&scope, intUnderlying(args.get(2)));
  bool negative_step = step.isNegative();
  Int lower(&scope, SmallInt::fromWord(negative_step ? -1 : 0));
  Runtime* runtime = thread->runtime();
  // upper = length + lower; if step < 0, then lower = 0 anyway
  Int upper(&scope,
            negative_step ? runtime->intAdd(thread, length, lower) : *length);
  Object stop_obj(&scope, args.get(0));
  if (stop_obj.isNoneType()) {
    return negative_step ? *lower : *upper;
  }
  Int stop(&scope, intUnderlying(*stop_obj));
  if (stop.isNegative()) {
    stop = runtime->intAdd(thread, stop, length);
    if (stop.compare(*lower) < 0) {
      stop = *lower;
    }
  } else if (stop.compare(*upper) > 0) {
    stop = *upper;
  }
  return *stop;
}

RawObject UnderBuiltinsModule::underStaticmethodIsabstract(Thread* thread,
                                                           Frame* frame,
                                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  StaticMethod self(&scope, args.get(0));
  Object func(&scope, self.function());
  return isAbstract(thread, func);
}

RawObject UnderBuiltinsModule::underStopIterationCtor(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  DCHECK(args.get(0) == runtime->typeAt(LayoutId::kStopIteration),
         "unexpected type; should be StopIteration");
  Layout layout(&scope, runtime->layoutAt(LayoutId::kStopIteration));
  StopIteration self(&scope, runtime->newInstance(layout));
  Object args_obj(&scope, args.get(1));
  self.setArgs(*args_obj);
  self.setCause(Unbound::object());
  self.setContext(Unbound::object());
  self.setTraceback(Unbound::object());
  self.setSuppressContext(RawBool::falseObj());
  Tuple tuple(&scope, self.args());
  if (tuple.length() > 0) self.setValue(tuple.at(0));
  return *self;
}

RawObject UnderBuiltinsModule::underStrarrayClear(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  StrArray self(&scope, args.get(0));
  self.setNumItems(0);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underStrarrayIadd(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  StrArray self(&scope, args.get(0));
  Str other(&scope, strUnderlying(args.get(1)));
  thread->runtime()->strArrayAddStr(thread, self, other);
  return *self;
}

RawObject UnderBuiltinsModule::underStrarrayCtor(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  DCHECK(args.get(0) == runtime->typeAt(LayoutId::kStrArray),
         "_strarray.__new__(X): X is not '_strarray'");
  Object self_obj(&scope, runtime->newStrArray());
  if (self_obj.isError()) return *self_obj;
  StrArray self(&scope, *self_obj);
  self.setNumItems(0);
  Object source_obj(&scope, args.get(1));
  if (source_obj.isUnbound()) {
    return *self;
  }
  if (!runtime->isInstanceOfStr(*source_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "_strarray can only be initialized with str");
  }
  Str source(&scope, strUnderlying(*source_obj));
  runtime->strArrayAddStr(thread, self, source);
  return *self;
}

RawObject UnderBuiltinsModule::underStrCheck(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfStr(args.get(0)));
}

RawObject UnderBuiltinsModule::underStrEncode(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object str_obj(&scope, args.get(0));
  if (!str_obj.isStr()) {
    return Unbound::object();
  }
  Str str(&scope, *str_obj);
  static RawSmallStr ascii = SmallStr::fromCStr("ascii");
  static RawSmallStr utf8 = SmallStr::fromCStr("utf-8");
  static RawSmallStr latin1 = SmallStr::fromCStr("latin-1");
  Str enc(&scope, args.get(1));
  if (enc != ascii && enc != utf8 && enc != latin1 &&
      enc.compareCStr("iso-8859-1") != 0) {
    return Unbound::object();
  }
  return strEncodeASCII(thread, str);
}

RawObject UnderBuiltinsModule::underStrEncodeASCII(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object str_obj(&scope, args.get(0));
  if (!str_obj.isStr()) {
    return Unbound::object();
  }
  Str str(&scope, *str_obj);
  return strEncodeASCII(thread, str);
}

RawObject UnderBuiltinsModule::underStrCheckExact(Thread*, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isStr());
}

RawObject UnderBuiltinsModule::underStrCompareDigest(Thread* thread,
                                                     Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  // TODO(T57794178): Use volatile
  Object left_obj(&scope, args.get(0));
  Object right_obj(&scope, args.get(1));
  DCHECK(runtime->isInstanceOfStr(*left_obj),
         "_str_compare_digest requires 'str' instance");
  DCHECK(runtime->isInstanceOfStr(*right_obj),
         "_str_compare_digest requires 'str' instance");
  Str left(&scope, strUnderlying(*left_obj));
  Str right(&scope, strUnderlying(*right_obj));
  word left_len = left.charLength();
  word right_len = right.charLength();
  word length = Utils::minimum(left_len, right_len);
  word result = (right_len == left_len) ? 0 : 1;
  for (word i = 0; i < length; i++) {
    result |= left.charAt(i) ^ right.charAt(i);
  }
  return Bool::fromBool(result == 0);
}

RawObject UnderBuiltinsModule::underStrCount(Thread* thread, Frame* frame,
                                             word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  DCHECK(runtime->isInstanceOfStr(args.get(0)),
         "_str_count requires 'str' instance");
  DCHECK(runtime->isInstanceOfStr(args.get(1)),
         "_str_count requires 'str' instance");
  HandleScope scope(thread);
  Str haystack(&scope, args.get(0));
  Str needle(&scope, args.get(1));
  Object start_obj(&scope, args.get(2));
  Object end_obj(&scope, args.get(3));
  word start = 0;
  if (!start_obj.isNoneType()) {
    start = intUnderlying(*start_obj).asWordSaturated();
  }
  word end = kMaxWord;
  if (!end_obj.isNoneType()) {
    end = intUnderlying(*end_obj).asWordSaturated();
  }
  return strCount(haystack, needle, start, end);
}

RawObject UnderBuiltinsModule::underStrEndswith(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object start_obj(&scope, args.get(2));
  Object end_obj(&scope, args.get(3));
  Str self(&scope, strUnderlying(args.get(0)));
  Str suffix(&scope, strUnderlying(args.get(1)));

  word len = self.codePointLength();
  word start = 0;
  word end = len;
  if (!start_obj.isNoneType()) {
    // TODO(T55084422): bounds checking
    start = intUnderlying(*start_obj).asWordSaturated();
  }
  if (!end_obj.isNoneType()) {
    // TODO(T55084422): bounds checking
    end = intUnderlying(*end_obj).asWordSaturated();
  }

  Slice::adjustSearchIndices(&start, &end, len);
  word suffix_len = suffix.codePointLength();
  if (start + suffix_len > end) {
    return Bool::falseObj();
  }
  word start_offset = self.offsetByCodePoints(0, end - suffix_len);
  word suffix_chars = suffix.charLength();
  for (word i = start_offset, j = 0; j < suffix_chars; i++, j++) {
    if (self.charAt(i) != suffix.charAt(j)) {
      return Bool::falseObj();
    }
  }
  return Bool::trueObj();
}

RawObject UnderBuiltinsModule::underStrEscapeNonAscii(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  CHECK(thread->runtime()->isInstanceOfStr(args.get(0)),
        "_str_escape_non_ascii expected str instance");
  Str obj(&scope, args.get(0));
  return strEscapeNonASCII(thread, obj);
}

RawObject UnderBuiltinsModule::underStrFind(Thread* thread, Frame* frame,
                                            word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  DCHECK(runtime->isInstanceOfStr(args.get(0)),
         "_str_find requires 'str' instance");
  DCHECK(runtime->isInstanceOfStr(args.get(1)),
         "_str_find requires 'str' instance");
  HandleScope scope(thread);
  Str haystack(&scope, args.get(0));
  Str needle(&scope, args.get(1));
  Object start_obj(&scope, args.get(2));
  Object end_obj(&scope, args.get(3));
  word start = 0;
  if (!start_obj.isNoneType()) {
    start = intUnderlying(*start_obj).asWordSaturated();
  }
  word end = kMaxWord;
  if (!end_obj.isNoneType()) {
    end = intUnderlying(*end_obj).asWordSaturated();
  }
  word result = strFind(haystack, needle, start, end);
  return SmallInt::fromWord(result);
}

RawObject UnderBuiltinsModule::underStrFromStr(Thread* thread, Frame* frame,
                                               word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  DCHECK(type.builtinBase() == LayoutId::kStr, "type must subclass str");
  Str value(&scope, strUnderlying(args.get(1)));
  if (type.isBuiltin()) return *value;
  Layout type_layout(&scope, type.instanceLayout());
  UserStrBase instance(&scope, thread->runtime()->newInstance(type_layout));
  instance.setValue(*value);
  return *instance;
}

RawObject UnderBuiltinsModule::underStrGetitem(Thread* thread, Frame* frame,
                                               word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStr(*self_obj)) {
    return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kStr);
  }
  Object key(&scope, args.get(1));
  if (runtime->isInstanceOfInt(*key)) {
    Str self(&scope, strUnderlying(*self_obj));
    word index = intUnderlying(*key).asWordSaturated();
    if (!SmallInt::isValid(index)) {
      return thread->raiseWithFmt(LayoutId::kIndexError,
                                  "cannot fit '%T' into an index-sized integer",
                                  &key);
    }
    if (index < 0) {
      index += self.codePointLength();
    }
    if (index >= 0) {
      word offset = self.offsetByCodePoints(0, index);
      if (offset < self.charLength()) {
        word ignored;
        return SmallStr::fromCodePoint(self.codePointAt(offset, &ignored));
      }
    }
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "string index out of range");
  }

  word start, stop;
  if (!trySlice(key, &start, &stop)) {
    return Unbound::object();
  }

  Str self(&scope, *self_obj);
  word length = self.codePointLength();
  word result_len = Slice::adjustIndices(length, &start, &stop, 1);
  if (result_len == length) {
    return *self;
  }
  return runtime->strSubstr(thread, self, start, result_len);
}

RawObject UnderBuiltinsModule::underStrGetslice(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Str self(&scope, strUnderlying(args.get(0)));
  word start = SmallInt::cast(args.get(1)).value();
  word stop = SmallInt::cast(args.get(2)).value();
  word step = SmallInt::cast(args.get(3)).value();
  return thread->runtime()->strSlice(thread, self, start, stop, step);
}

RawObject UnderBuiltinsModule::underStrGuard(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfStr(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kStr);
}

RawObject UnderBuiltinsModule::underStrIschr(Thread*, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  RawStr str = strUnderlying(args.get(0));
  return Bool::fromBool(str.isSmallStr() && str.codePointLength() == 1);
}

RawObject UnderBuiltinsModule::underStrJoin(Thread* thread, Frame* frame,
                                            word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Str sep(&scope, args.get(0));
  Object iterable(&scope, args.get(1));
  if (iterable.isTuple()) {
    Tuple tuple(&scope, *iterable);
    return runtime->strJoin(thread, sep, tuple, tuple.length());
  }
  DCHECK(iterable.isList(), "iterable must be tuple or list");
  List list(&scope, *iterable);
  Tuple tuple(&scope, list.items());
  return runtime->strJoin(thread, sep, tuple, list.numItems());
}

RawObject UnderBuiltinsModule::underStrLen(Thread* thread, Frame* frame,
                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Str self(&scope, strUnderlying(args.get(0)));
  return SmallInt::fromWord(self.codePointLength());
}

static word strScan(const Str& haystack, word haystack_len, const Str& needle,
                    word needle_len,
                    word (*find_func)(byte* haystack, word haystack_len,
                                      byte* needle, word needle_len)) {
  byte haystack_buf[SmallStr::kMaxLength];
  byte* haystack_ptr = haystack_buf;
  if (haystack.isSmallStr()) {
    haystack.copyTo(haystack_buf, haystack_len);
  } else {
    haystack_ptr = reinterpret_cast<byte*>(LargeStr::cast(*haystack).address());
  }
  byte needle_buf[SmallStr::kMaxLength];
  byte* needle_ptr = needle_buf;
  if (needle.isSmallStr()) {
    needle.copyTo(needle_buf, needle_len);
  } else {
    needle_ptr = reinterpret_cast<byte*>(LargeStr::cast(*needle).address());
  }
  return (*find_func)(haystack_ptr, haystack_len, needle_ptr, needle_len);
}

// Look for needle in haystack, starting from the left. Return a tuple
// containing:
// * haystack up to but not including needle
// * needle
// * haystack after and not including needle
// If needle is not found in haystack, return (haystack, "", "")
RawObject UnderBuiltinsModule::underStrPartition(Thread* thread, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Str haystack(&scope, strUnderlying(args.get(0)));
  Str needle(&scope, strUnderlying(args.get(1)));
  Runtime* runtime = thread->runtime();
  MutableTuple result(&scope, runtime->newMutableTuple(3));
  result.atPut(0, *haystack);
  result.atPut(1, Str::empty());
  result.atPut(2, Str::empty());
  word haystack_len = haystack.charLength();
  word needle_len = needle.charLength();
  if (haystack_len < needle_len) {
    // Fast path when needle is bigger than haystack
    return result.becomeImmutable();
  }
  word prefix_len =
      strScan(haystack, haystack_len, needle, needle_len, Utils::memoryFind);
  if (prefix_len < 0) return result.becomeImmutable();
  result.atPut(0, runtime->strSubstr(thread, haystack, 0, prefix_len));
  result.atPut(1, *needle);
  word suffix_start = prefix_len + needle_len;
  word suffix_len = haystack_len - suffix_start;
  result.atPut(2,
               runtime->strSubstr(thread, haystack, suffix_start, suffix_len));
  return result.becomeImmutable();
}

RawObject UnderBuiltinsModule::underStrReplace(Thread* thread, Frame* frame,
                                               word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Str self(&scope, strUnderlying(args.get(0)));
  Str oldstr(&scope, strUnderlying(args.get(1)));
  Str newstr(&scope, strUnderlying(args.get(2)));
  word count = intUnderlying(args.get(3)).asWordSaturated();
  return runtime->strReplace(thread, self, oldstr, newstr, count);
}

RawObject UnderBuiltinsModule::underStrRfind(Thread* thread, Frame* frame,
                                             word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  DCHECK(runtime->isInstanceOfStr(args.get(0)),
         "_str_rfind requires 'str' instance");
  DCHECK(runtime->isInstanceOfStr(args.get(1)),
         "_str_rfind requires 'str' instance");
  HandleScope scope(thread);
  Str haystack(&scope, args.get(0));
  Str needle(&scope, args.get(1));
  Object start_obj(&scope, args.get(2));
  Object end_obj(&scope, args.get(3));
  word start = 0;
  if (!start_obj.isNoneType()) {
    start = intUnderlying(*start_obj).asWordSaturated();
  }
  word end = kMaxWord;
  if (!end_obj.isNoneType()) {
    end = intUnderlying(*end_obj).asWordSaturated();
  }
  Slice::adjustSearchIndices(&start, &end, haystack.codePointLength());
  word result = strRFind(haystack, needle, start, end);
  return SmallInt::fromWord(result);
}

// Look for needle in haystack, starting from the right. Return a tuple
// containing:
// * haystack up to but not including needle
// * needle
// * haystack after and not including needle
// If needle is not found in haystack, return ("", "", haystack)
RawObject UnderBuiltinsModule::underStrRpartition(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Str haystack(&scope, strUnderlying(args.get(0)));
  Str needle(&scope, strUnderlying(args.get(1)));
  MutableTuple result(&scope, runtime->newMutableTuple(3));
  result.atPut(0, Str::empty());
  result.atPut(1, Str::empty());
  result.atPut(2, *haystack);
  word haystack_len = haystack.charLength();
  word needle_len = needle.charLength();
  if (haystack_len < needle_len) {
    // Fast path when needle is bigger than haystack
    return result.becomeImmutable();
  }
  word prefix_len = strScan(haystack, haystack_len, needle, needle_len,
                            Utils::memoryFindReverse);
  if (prefix_len < 0) return result.becomeImmutable();
  result.atPut(0, runtime->strSubstr(thread, haystack, 0, prefix_len));
  result.atPut(1, *needle);
  word suffix_start = prefix_len + needle_len;
  word suffix_len = haystack_len - suffix_start;
  result.atPut(2,
               runtime->strSubstr(thread, haystack, suffix_start, suffix_len));
  return result.becomeImmutable();
}

static RawObject strSplitWhitespace(Thread* thread, const Str& self,
                                    word maxsplit) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  List result(&scope, runtime->newList());
  if (maxsplit < 0) {
    maxsplit = kMaxWord;
  }
  word self_length = self.charLength();
  word num_split = 0;
  Str substr(&scope, Str::empty());
  for (word i = 0, j = 0; j < self_length; i = self.offsetByCodePoints(j, 1)) {
    // Find beginning of next word
    {
      word num_bytes;
      while (i < self_length && isSpace(self.codePointAt(i, &num_bytes))) {
        i += num_bytes;
      }
    }
    if (i == self_length) {
      // End of string; finished
      break;
    }

    // Find end of next word
    if (maxsplit == num_split) {
      // Take the rest of the string
      j = self_length;
    } else {
      j = self.offsetByCodePoints(i, 1);
      {
        word num_bytes;
        while (j < self_length && !isSpace(self.codePointAt(j, &num_bytes))) {
          j += num_bytes;
        }
      }
      num_split += 1;
    }
    substr = runtime->strSubstr(thread, self, i, j - i);
    runtime->listAdd(thread, result, substr);
  }
  return *result;
}

RawObject UnderBuiltinsModule::underStrSplit(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Str self(&scope, strUnderlying(args.get(0)));
  Object sep_obj(&scope, args.get(1));
  word maxsplit = intUnderlying(args.get(2)).asWordSaturated();
  if (sep_obj.isNoneType()) {
    return strSplitWhitespace(thread, self, maxsplit);
  }
  Str sep(&scope, strUnderlying(*sep_obj));
  if (sep.charLength() == 0) {
    return thread->raiseWithFmt(LayoutId::kValueError, "empty separator");
  }
  if (maxsplit < 0) {
    maxsplit = kMaxWord;
  }
  return strSplit(thread, self, sep, maxsplit);
}

RawObject UnderBuiltinsModule::underStrSplitlines(Thread* thread, Frame* frame,
                                                  word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  DCHECK(runtime->isInstanceOfStr(args.get(0)),
         "_str_splitlines requires 'str' instance");
  DCHECK(runtime->isInstanceOfInt(args.get(1)),
         "_str_splitlines requires 'int' instance");
  HandleScope scope(thread);
  Str self(&scope, args.get(0));
  bool keepends = !intUnderlying(args.get(1)).isZero();
  return strSplitlines(thread, self, keepends);
}

RawObject UnderBuiltinsModule::underStrStartswith(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object start_obj(&scope, args.get(2));
  Object end_obj(&scope, args.get(3));
  Str self(&scope, strUnderlying(args.get(0)));
  Str prefix(&scope, strUnderlying(args.get(1)));

  word len = self.codePointLength();
  word start = 0;
  word end = len;
  if (!start_obj.isNoneType()) {
    // TODO(T55084422): bounds checking
    start = intUnderlying(*start_obj).asWordSaturated();
  }
  if (!end_obj.isNoneType()) {
    // TODO(T55084422): bounds checking
    end = intUnderlying(*end_obj).asWordSaturated();
  }

  Slice::adjustSearchIndices(&start, &end, len);
  if (start + prefix.codePointLength() > end) {
    return Bool::falseObj();
  }
  word start_offset = self.offsetByCodePoints(0, start);
  word prefix_chars = prefix.charLength();
  for (word i = start_offset, j = 0; j < prefix_chars; i++, j++) {
    if (self.charAt(i) != prefix.charAt(j)) {
      return Bool::falseObj();
    }
  }
  return Bool::trueObj();
}

RawObject UnderBuiltinsModule::underTupleCheck(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfTuple(args.get(0)));
}

RawObject UnderBuiltinsModule::underTupleCheckExact(Thread*, Frame* frame,
                                                    word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isTuple());
}

RawObject UnderBuiltinsModule::underTupleGetitem(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(*self_obj)) {
    return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kTuple);
  }
  Object key(&scope, args.get(1));
  if (runtime->isInstanceOfInt(*key)) {
    word index = intUnderlying(*key).asWordSaturated();
    if (!SmallInt::isValid(index)) {
      return thread->raiseWithFmt(LayoutId::kIndexError,
                                  "cannot fit '%T' into an index-sized integer",
                                  &key);
    }
    Tuple self(&scope, tupleUnderlying(*self_obj));
    word length = self.length();
    if (index < 0) {
      index += length;
    }
    if (index < 0 || index >= length) {
      return thread->raiseWithFmt(LayoutId::kIndexError,
                                  "tuple index out of range");
    }
    return self.at(index);
  }

  word start, stop;
  if (!trySlice(key, &start, &stop)) {
    return Unbound::object();
  }

  Tuple self(&scope, tupleUnderlying(*self_obj));
  word length = self.length();
  word result_len = Slice::adjustIndices(length, &start, &stop, 1);
  if (result_len == length) {
    return *self;
  }
  return runtime->tupleSubseq(thread, self, start, result_len);
}

RawObject UnderBuiltinsModule::underTupleGetslice(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Tuple self(&scope, tupleUnderlying(args.get(0)));
  word start = SmallInt::cast(args.get(1)).value();
  word stop = SmallInt::cast(args.get(2)).value();
  word step = SmallInt::cast(args.get(3)).value();
  return tupleSlice(thread, self, start, stop, step);
}

RawObject UnderBuiltinsModule::underTupleGuard(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfTuple(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kTuple);
}

RawObject UnderBuiltinsModule::underTupleLen(Thread*, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  return SmallInt::fromWord(tupleUnderlying(args.get(0)).length());
}

RawObject UnderBuiltinsModule::underTupleNew(Thread* thread, Frame* frame,
                                             word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  DCHECK(type != runtime->typeAt(LayoutId::kTuple), "cls must not be tuple");
  DCHECK(args.get(1).isTuple(), "old_tuple must be exact tuple");
  Layout layout(&scope, type.instanceLayout());
  UserTupleBase instance(&scope, runtime->newInstance(layout));
  instance.setValue(args.get(1));
  return *instance;
}

RawObject UnderBuiltinsModule::underType(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  return thread->runtime()->typeOf(args.get(0));
}

RawObject UnderBuiltinsModule::underTypeAbstractmethodsDel(Thread* thread,
                                                           Frame* frame,
                                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  if (type.abstractMethods().isUnbound()) {
    return thread->raiseWithId(LayoutId::kAttributeError,
                               SymbolId::kDunderAbstractmethods);
  }
  type.setAbstractMethods(Unbound::object());
  type.setFlagsAndBuiltinBase(
      static_cast<Type::Flag>(type.flags() & ~Type::Flag::kIsAbstract),
      type.builtinBase());
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underTypeAbstractmethodsGet(Thread* thread,
                                                           Frame* frame,
                                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Object methods(&scope, type.abstractMethods());
  if (!methods.isUnbound()) {
    return *methods;
  }
  return thread->raiseWithId(LayoutId::kAttributeError,
                             SymbolId::kDunderAbstractmethods);
}

RawObject UnderBuiltinsModule::underTypeAbstractmethodsSet(Thread* thread,
                                                           Frame* frame,
                                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Object abstract(&scope, Interpreter::isTrue(thread, args.get(1)));
  if (abstract.isError()) return *abstract;
  type.setAbstractMethods(args.get(1));
  if (Bool::cast(*abstract).value()) {
    type.setFlagsAndBuiltinBase(
        static_cast<Type::Flag>(type.flags() | Type::Flag::kIsAbstract),
        type.builtinBase());
  }
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underTypeBasesDel(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Str name(&scope, type.name());
  return thread->raiseWithFmt(LayoutId::kTypeError, "can't delete %S.__bases__",
                              &name);
}

RawObject UnderBuiltinsModule::underTypeBasesGet(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  return Type(&scope, args.get(0)).bases();
}

RawObject UnderBuiltinsModule::underTypeBasesSet(Thread*, Frame*, word) {
  UNIMPLEMENTED("type.__bases__ setter");
}

RawObject UnderBuiltinsModule::underTypeCheck(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfType(args.get(0)));
}

RawObject UnderBuiltinsModule::underTypeCheckExact(Thread*, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isType());
}

RawObject UnderBuiltinsModule::underTypeDunderCall(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  Tuple pargs(&scope, args.get(1));
  Dict kwargs(&scope, args.get(2));
  word pargs_length = pargs.length();
  bool is_kwargs_empty = kwargs.numItems() == 0;
  // Shortcut for type(x) calls.
  if (pargs_length == 1 && is_kwargs_empty &&
      self_obj == runtime->typeAt(LayoutId::kType)) {
    return runtime->typeOf(pargs.at(0));
  }

  if (!runtime->isInstanceOfType(*self_obj)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "'__call__' requires a '%Y' object but got '%T'",
        SymbolId::kType, &self_obj);
  }
  Type self(&scope, *self_obj);

  // `instance = self.__new__(...)`
  Object dunder_new_name(&scope, runtime->symbols()->DunderNew());
  Object dunder_new(&scope, typeGetAttribute(thread, self, dunder_new_name));
  Object instance(&scope, NoneType::object());
  Object call_args_obj(&scope, NoneType::object());
  if (dunder_new == runtime->objectDunderNew()) {
    // Fast path when `__new__` was not overridden and is just `object.__new__`.
    instance = objectNew(thread, self);
    if (instance.isErrorException()) return *instance;
  } else {
    CHECK(!dunder_new.isError(), "self must have __new__");
    frame->pushValue(*dunder_new);
    if (is_kwargs_empty) {
      frame->pushValue(*self);
      for (word i = 0; i < pargs_length; ++i) {
        frame->pushValue(pargs.at(i));
      }
      instance = Interpreter::call(thread, frame, pargs_length + 1);
    } else {
      MutableTuple call_args(&scope,
                             runtime->newMutableTuple(pargs_length + 1));
      call_args.atPut(0, *self);
      call_args.replaceFromWith(1, *pargs, pargs_length);
      frame->pushValue(call_args.becomeImmutable());
      frame->pushValue(*kwargs);
      instance =
          Interpreter::callEx(thread, frame, CallFunctionExFlag::VAR_KEYWORDS);
      call_args_obj = *call_args;
    }
    if (instance.isErrorException()) return *instance;
    Type type(&scope, runtime->typeOf(*instance));
    if (!typeIsSubclass(type, self)) {
      return *instance;
    }
  }

  // instance.__init__(...)
  Object dunder_init_name(&scope, runtime->symbols()->DunderInit());
  Object dunder_init(&scope, typeGetAttribute(thread, self, dunder_init_name));
  // `object.__init__` does nothing, we may be able to just skip things.
  // The exception to the rule being `object.__init__` raising errors when
  // arguments are provided and nothing is overridden.
  if (dunder_init != runtime->objectDunderInit() ||
      (dunder_new == runtime->objectDunderNew() &&
       (pargs.length() != 0 || kwargs.numItems() != 0))) {
    CHECK(!dunder_init.isError(), "self must have __init__");
    Object result(&scope, NoneType::object());
    frame->pushValue(*dunder_init);
    if (is_kwargs_empty) {
      frame->pushValue(*instance);
      for (word i = 0; i < pargs_length; ++i) {
        frame->pushValue(pargs.at(i));
      }
      result = Interpreter::call(thread, frame, pargs_length + 1);
    } else {
      if (!call_args_obj.isMutableTuple()) {
        MutableTuple call_args(&scope,
                               runtime->newMutableTuple(pargs_length + 1));
        call_args.atPut(0, *instance);
        call_args.replaceFromWith(1, *pargs, pargs_length);
        call_args_obj = *call_args;
      } else {
        MutableTuple::cast(*call_args_obj).atPut(0, *instance);
      }
      frame->pushValue(*call_args_obj);
      frame->pushValue(*kwargs);
      result =
          Interpreter::callEx(thread, frame, CallFunctionExFlag::VAR_KEYWORDS);
    }
    if (result.isErrorException()) return *result;
    if (!result.isNoneType()) {
      Object type_name(&scope, self.name());
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "%S.__init__ returned non None", &type_name);
    }
  }
  return *instance;
}

RawObject UnderBuiltinsModule::underTypeGuard(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfType(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kType);
}

RawObject UnderBuiltinsModule::underTypeIssubclass(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type subclass(&scope, args.get(0));
  Type superclass(&scope, args.get(1));
  return Bool::fromBool(typeIsSubclass(subclass, superclass));
}

RawObject UnderBuiltinsModule::underTypeNew(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type metaclass(&scope, args.get(0));
  Tuple bases(&scope, args.get(1));
  LayoutId metaclass_id = Layout::cast(metaclass.instanceLayout()).id();
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->newTypeWithMetaclass(metaclass_id));
  type.setBases(bases.length() > 0 ? *bases : runtime->implicitBases());
  Function type_dunder_call(
      &scope, runtime->lookupNameInModule(thread, SymbolId::kUnderBuiltins,
                                          SymbolId::kUnderTypeDunderCall));
  type.setCtor(*type_dunder_call);
  return *type;
}

RawObject UnderBuiltinsModule::underTypeProxy(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (type.proxy().isNoneType()) {
    type.setProxy(thread->runtime()->newTypeProxy(type));
  }
  return type.proxy();
}

RawObject UnderBuiltinsModule::underTypeProxyCheck(Thread*, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isTypeProxy());
}

RawObject UnderBuiltinsModule::underTypeProxyGet(Thread* thread, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  TypeProxy self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object default_obj(&scope, args.get(2));
  Type type(&scope, self.type());
  Object result(&scope, typeAt(type, name));
  if (result.isError()) {
    return *default_obj;
  }
  return *result;
}

RawObject UnderBuiltinsModule::underTypeProxyGuard(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  if (args.get(0).isTypeProxy()) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kTypeProxy);
}

RawObject UnderBuiltinsModule::underTypeProxyKeys(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  TypeProxy self(&scope, args.get(0));
  Type type(&scope, self.type());
  return typeKeys(thread, type);
}

RawObject UnderBuiltinsModule::underTypeProxyLen(Thread* thread, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  TypeProxy self(&scope, args.get(0));
  Type type(&scope, self.type());
  return typeLen(thread, type);
}

RawObject UnderBuiltinsModule::underTypeProxyValues(Thread* thread,
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  TypeProxy self(&scope, args.get(0));
  Type type(&scope, self.type());
  return typeValues(thread, type);
}

RawObject UnderBuiltinsModule::underTypeInit(Thread* thread, Frame* frame,
                                             word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Str name(&scope, args.get(1));
  Dict dict(&scope, args.get(2));
  Tuple mro(&scope, thread->runtime()->emptyTuple());
  if (args.get(3).isUnbound()) {
    Object mro_obj(&scope, computeMro(thread, type));
    if (mro_obj.isError()) return *mro_obj;
    mro = *mro_obj;
  } else {
    mro = args.get(3);
  }
  return typeInit(thread, type, name, dict, mro);
}

RawObject UnderBuiltinsModule::underTypeSubclassGuard(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  if (!thread->runtime()->isInstanceOfType(args.get(0))) {
    return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kType);
  }
  Type subclass(&scope, args.get(0));
  Type superclass(&scope, args.get(1));
  if (typeIsSubclass(subclass, superclass)) {
    return NoneType::object();
  }
  Function function(&scope, frame->previousFrame()->function());
  Str function_name(&scope, function.name());
  Str subclass_name(&scope, subclass.name());
  Str superclass_name(&scope, superclass.name());
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "'%S': '%S' is not a subclass of '%S'",
                              &function_name, &subclass_name, &superclass_name);
}

RawObject UnderBuiltinsModule::underUnimplemented(Thread* thread, Frame* frame,
                                                  word) {
  py::Utils::printTracebackToStderr();

  // Attempt to identify the calling function.
  HandleScope scope(thread);
  Object function_obj(&scope, frame->previousFrame()->function());
  if (!function_obj.isError()) {
    Function function(&scope, *function_obj);
    Str function_name(&scope, function.name());
    unique_c_ptr<char> name_cstr(function_name.toCStr());
    fprintf(stderr, "\n'_unimplemented' called in function '%s'.\n",
            name_cstr.get());
  } else {
    fputs("\n'_unimplemented' called.\n", stderr);
  }

  std::abort();
}

RawObject UnderBuiltinsModule::underWarn(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object message(&scope, args.get(0));
  Object category(&scope, args.get(1));
  Object stacklevel(&scope, args.get(2));
  Object source(&scope, args.get(3));
  return thread->invokeFunction4(SymbolId::kWarnings, SymbolId::kWarn, message,
                                 category, stacklevel, source);
}

RawObject UnderBuiltinsModule::underWeakrefCallback(Thread* thread,
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfWeakRef(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kRef);
  }
  WeakRef self(&scope, *self_obj);
  return self.callback();
}

RawObject UnderBuiltinsModule::underWeakrefCheck(Thread* thread, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfWeakRef(args.get(0)));
}

RawObject UnderBuiltinsModule::underWeakrefGuard(Thread* thread, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfWeakRef(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kRef);
}

RawObject UnderBuiltinsModule::underWeakrefReferent(Thread* thread,
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfWeakRef(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kRef);
  }
  WeakRef self(&scope, *self_obj);
  return self.referent();
}

}  // namespace py
