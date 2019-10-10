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
#include "tuple-builtins.h"
#include "type-builtins.h"
#include "unicode.h"
#include "vector.h"

namespace python {

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
    {SymbolId::kUnderByteArrayCheck, underByteArrayCheck},
    {SymbolId::kUnderByteArrayClear, underByteArrayClear},
    {SymbolId::kUnderByteArrayDelitem, underByteArrayDelItem},
    {SymbolId::kUnderByteArrayDelslice, underByteArrayDelSlice},
    {SymbolId::kUnderByteArrayGuard, underByteArrayGuard},
    {SymbolId::kUnderByteArrayJoin, underByteArrayJoin},
    {SymbolId::kUnderByteArrayLen, underByteArrayLen},
    {SymbolId::kUnderByteArraySetitem, underByteArraySetItem},
    {SymbolId::kUnderByteArraySetslice, underByteArraySetSlice},
    {SymbolId::kUnderBytesCheck, underBytesCheck},
    {SymbolId::kUnderBytesFromBytes, underBytesFromBytes},
    {SymbolId::kUnderBytesFromInts, underBytesFromInts},
    {SymbolId::kUnderBytesGetitem, underBytesGetItem},
    {SymbolId::kUnderBytesGetslice, underBytesGetSlice},
    {SymbolId::kUnderBytesGuard, underBytesGuard},
    {SymbolId::kUnderBytesJoin, underBytesJoin},
    {SymbolId::kUnderBytesLen, underBytesLen},
    {SymbolId::kUnderBytesMaketrans, underBytesMaketrans},
    {SymbolId::kUnderBytesRepeat, underBytesRepeat},
    {SymbolId::kUnderBytesSplit, underBytesSplit},
    {SymbolId::kUnderBytesSplitWhitespace, underBytesSplitWhitespace},
    {SymbolId::kUnderByteslikeCheck, underByteslikeCheck},
    {SymbolId::kUnderByteslikeCount, underByteslikeCount},
    {SymbolId::kUnderByteslikeEndsWith, underByteslikeEndsWith},
    {SymbolId::kUnderByteslikeFindByteslike, underByteslikeFindByteslike},
    {SymbolId::kUnderByteslikeFindInt, underByteslikeFindInt},
    {SymbolId::kUnderByteslikeGuard, underByteslikeGuard},
    {SymbolId::kUnderByteslikeRfindByteslike, underByteslikeRFindByteslike},
    {SymbolId::kUnderByteslikeRfindInt, underByteslikeRFindInt},
    {SymbolId::kUnderByteslikeStartsWith, underByteslikeStartsWith},
    {SymbolId::kUnderClassMethod, underClassMethod},
    {SymbolId::kUnderClassMethodIsAbstract, underClassMethodIsAbstract},
    {SymbolId::kUnderCodeGuard, underCodeGuard},
    {SymbolId::kUnderComplexImag, underComplexImag},
    {SymbolId::kUnderComplexReal, underComplexReal},
    {SymbolId::kUnderDelattr, underDelattr},
    {SymbolId::kUnderDictBucketInsert, underDictBucketInsert},
    {SymbolId::kUnderDictBucketKey, underDictBucketKey},
    {SymbolId::kUnderDictBucketSetValue, underDictBucketSetValue},
    {SymbolId::kUnderDictBucketValue, underDictBucketValue},
    {SymbolId::kUnderDictCheck, underDictCheck},
    {SymbolId::kUnderDictCheckExact, underDictCheckExact},
    {SymbolId::kUnderDictGuard, underDictGuard},
    {SymbolId::kUnderDictLen, underDictLen},
    {SymbolId::kUnderDictLookup, underDictLookup},
    {SymbolId::kUnderDictLookupNext, underDictLookupNext},
    {SymbolId::kUnderDictPopitem, underDictPopitem},
    {SymbolId::kUnderDivmod, underDivmod},
    {SymbolId::kUnderFloatCheck, underFloatCheck},
    {SymbolId::kUnderFloatDivmod, underFloatDivmod},
    {SymbolId::kUnderFloatFormat, underFloatFormat},
    {SymbolId::kUnderFloatGuard, underFloatGuard},
    {SymbolId::kUnderFloatSignbit, underFloatSignbit},
    {SymbolId::kUnderFrozenSetCheck, underFrozenSetCheck},
    {SymbolId::kUnderFrozenSetGuard, underFrozenSetGuard},
    {SymbolId::kUnderFunctionGlobals, underFunctionGlobals},
    {SymbolId::kUnderGc, underGc},
    {SymbolId::kUnderGetMemberByte, underGetMemberByte},
    {SymbolId::kUnderGetMemberChar, underGetMemberChar},
    {SymbolId::kUnderGetMemberDouble, underGetMemberDouble},
    {SymbolId::kUnderGetMemberFloat, underGetMemberFloat},
    {SymbolId::kUnderGetMemberInt, underGetMemberInt},
    {SymbolId::kUnderGetMemberLong, underGetMemberLong},
    {SymbolId::kUnderGetMemberPyObject, underGetMemberPyObject},
    {SymbolId::kUnderGetMemberShort, underGetMemberShort},
    {SymbolId::kUnderGetMemberString, underGetMemberString},
    {SymbolId::kUnderGetMemberUByte, underGetMemberUByte},
    {SymbolId::kUnderGetMemberUInt, underGetMemberUInt},
    {SymbolId::kUnderGetMemberULong, underGetMemberULong},
    {SymbolId::kUnderGetMemberUShort, underGetMemberUShort},
    {SymbolId::kUnderInstanceDelattr, underInstanceDelattr},
    {SymbolId::kUnderInstanceGetattr, underInstanceGetattr},
    {SymbolId::kUnderInstanceKeys, underInstanceKeys},
    {SymbolId::kUnderInstanceSetattr, underInstanceSetattr},
    {SymbolId::kUnderIntCheck, underIntCheck},
    {SymbolId::kUnderIntCheckExact, underIntCheckExact},
    {SymbolId::kUnderIntFromBytes, underIntFromBytes},
    {SymbolId::kUnderIntGuard, underIntGuard},
    {SymbolId::kUnderIntNewFromByteArray, underIntNewFromByteArray},
    {SymbolId::kUnderIntNewFromBytes, underIntNewFromBytes},
    {SymbolId::kUnderIntNewFromInt, underIntNewFromInt},
    {SymbolId::kUnderIntNewFromStr, underIntNewFromStr},
    {SymbolId::kUnderListCheck, underListCheck},
    {SymbolId::kUnderListCheckExact, underListCheckExact},
    {SymbolId::kUnderListDelitem, underListDelItem},
    {SymbolId::kUnderListDelslice, underListDelSlice},
    {SymbolId::kUnderListExtend, underListExtend},
    {SymbolId::kUnderListGetitem, underListGetItem},
    {SymbolId::kUnderListGetslice, underListGetSlice},
    {SymbolId::kUnderListGuard, underListGuard},
    {SymbolId::kUnderListLen, underListLen},
    {SymbolId::kUnderListSort, underListSort},
    {SymbolId::kUnderListSwap, underListSwap},
    {SymbolId::kUnderMappingProxyGuard, underMappingProxyGuard},
    {SymbolId::kUnderMappingProxyMapping, underMappingProxyMapping},
    {SymbolId::kUnderMappingProxySetMapping, underMappingProxySetMapping},
    {SymbolId::kUnderMemoryviewCheck, underMemoryviewCheck},
    {SymbolId::kUnderMemoryviewGuard, underMemoryviewGuard},
    {SymbolId::kUnderMemoryviewItemsize, underMemoryviewItemsize},
    {SymbolId::kUnderMemoryviewNbytes, underMemoryviewNbytes},
    {SymbolId::kUnderModuleDir, underModuleDir},
    {SymbolId::kUnderModuleProxy, underModuleProxy},
    {SymbolId::kUnderModuleProxyDelitem, underModuleProxyDelitem},
    {SymbolId::kUnderModuleProxyGet, underModuleProxyGet},
    {SymbolId::kUnderModuleProxyGuard, underModuleProxyGuard},
    {SymbolId::kUnderModuleProxyKeys, underModuleProxyKeys},
    {SymbolId::kUnderModuleProxyLen, underModuleProxyLen},
    {SymbolId::kUnderModuleProxySetitem, underModuleProxySetitem},
    {SymbolId::kUnderModuleProxyValues, underModuleProxyValues},
    {SymbolId::kUnderObjectCreateIterator, underObjectCreateIterator},
    {SymbolId::kUnderObjectTypeGetattr, underObjectTypeGetAttr},
    {SymbolId::kUnderObjectTypeHasattr, underObjectTypeHasattr},
    {SymbolId::kUnderOsWrite, underOsWrite},
    {SymbolId::kUnderProperty, underProperty},
    {SymbolId::kUnderPropertyIsAbstract, underPropertyIsAbstract},
    {SymbolId::kUnderPyObjectOffset, underPyObjectOffset},
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
    {SymbolId::kUnderSetMemberPyObject, underSetMemberPyObject},
    {SymbolId::kUnderSliceCheck, underSliceCheck},
    {SymbolId::kUnderSliceGuard, underSliceGuard},
    {SymbolId::kUnderSliceStart, underSliceStart},
    {SymbolId::kUnderSliceStep, underSliceStep},
    {SymbolId::kUnderSliceStop, underSliceStop},
    {SymbolId::kUnderStaticMethodIsAbstract, underStaticMethodIsAbstract},
    {SymbolId::kUnderStrArrayIadd, underStrArrayIadd},
    {SymbolId::kUnderStrCheck, underStrCheck},
    {SymbolId::kUnderStrCheckExact, underStrCheckExact},
    {SymbolId::kUnderStrCount, underStrCount},
    {SymbolId::kUnderStrEndswith, underStrEndsWith},
    {SymbolId::kUnderStrGuard, underStrGuard},
    {SymbolId::kUnderStrJoin, underStrJoin},
    {SymbolId::kUnderStrEscapeNonAscii, underStrEscapeNonAscii},
    {SymbolId::kUnderStrFind, underStrFind},
    {SymbolId::kUnderStrFromStr, underStrFromStr},
    {SymbolId::kUnderStrLen, underStrLen},
    {SymbolId::kUnderStrReplace, underStrReplace},
    {SymbolId::kUnderStrRFind, underStrRFind},
    {SymbolId::kUnderStrRPartition, underStrRPartition},
    {SymbolId::kUnderStrSplitlines, underStrSplitlines},
    {SymbolId::kUnderStrStartswith, underStrStartsWith},
    {SymbolId::kUnderTupleCheck, underTupleCheck},
    {SymbolId::kUnderTupleCheckExact, underTupleCheckExact},
    {SymbolId::kUnderTupleGetitem, underTupleGetItem},
    {SymbolId::kUnderTupleGetslice, underTupleGetSlice},
    {SymbolId::kUnderTupleGuard, underTupleGuard},
    {SymbolId::kUnderTupleLen, underTupleLen},
    {SymbolId::kUnderTupleNew, underTupleNew},
    {SymbolId::kUnderType, underType},
    {SymbolId::kUnderTypeAbstractMethodsDel, underTypeAbstractMethodsDel},
    {SymbolId::kUnderTypeAbstractMethodsGet, underTypeAbstractMethodsGet},
    {SymbolId::kUnderTypeAbstractMethodsSet, underTypeAbstractMethodsSet},
    {SymbolId::kUnderTypeBasesDel, underTypeBasesDel},
    {SymbolId::kUnderTypeBasesGet, underTypeBasesGet},
    {SymbolId::kUnderTypeBasesSet, underTypeBasesSet},
    {SymbolId::kUnderTypeCheck, underTypeCheck},
    {SymbolId::kUnderTypeCheckExact, underTypeCheckExact},
    {SymbolId::kUnderTypeGuard, underTypeGuard},
    {SymbolId::kUnderTypeInit, underTypeInit},
    {SymbolId::kUnderTypeIsSubclass, underTypeIsSubclass},
    {SymbolId::kUnderTypeNew, underTypeNew},
    {SymbolId::kUnderTypeProxy, underTypeProxy},
    {SymbolId::kUnderTypeProxyCheck, underTypeProxyCheck},
    {SymbolId::kUnderTypeProxyGet, underTypeProxyGet},
    {SymbolId::kUnderTypeProxyGuard, underTypeProxyGuard},
    {SymbolId::kUnderTypeProxyKeys, underTypeProxyKeys},
    {SymbolId::kUnderTypeProxyLen, underTypeProxyLen},
    {SymbolId::kUnderTypeProxyValues, underTypeProxyValues},
    {SymbolId::kUnderUnimplemented, underUnimplemented},
    {SymbolId::kUnderWeakRefCallback, underWeakRefCallback},
    {SymbolId::kSentinelId, nullptr},
};

const BuiltinType UnderBuiltinsModule::kBuiltinTypes[] = {
    {SymbolId::kUnderTraceback, LayoutId::kTraceback},
    {SymbolId::kSentinelId, LayoutId::kSentinelId},
};

const char* const UnderBuiltinsModule::kFrozenData = kUnderBuiltinsModuleData;

// clang-format off
const SymbolId UnderBuiltinsModule::kIntrinsicIds[] = {
    SymbolId::kUnderByteArrayCheck,
    SymbolId::kUnderByteArrayGuard,
    SymbolId::kUnderByteArrayLen,
    SymbolId::kUnderBytesCheck,
    SymbolId::kUnderBytesGuard,
    SymbolId::kUnderBytesLen,
    SymbolId::kUnderByteslikeCheck,
    SymbolId::kUnderByteslikeGuard,
    SymbolId::kUnderDictCheck,
    SymbolId::kUnderDictCheckExact,
    SymbolId::kUnderDictGuard,
    SymbolId::kUnderDictLen,
    SymbolId::kUnderFloatCheck,
    SymbolId::kUnderFloatGuard,
    SymbolId::kUnderFrozenSetCheck,
    SymbolId::kUnderFrozenSetGuard,
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

RawObject UnderBuiltinsModule::underAddress(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  return thread->runtime()->newInt(args.get(0).raw());
}

RawObject UnderBuiltinsModule::underBoundMethod(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object function(&scope, args.get(0));
  Object owner(&scope, args.get(1));
  return thread->runtime()->newBoundMethod(function, owner);
}

RawObject UnderBuiltinsModule::underByteArrayClear(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray self(&scope, args.get(0));
  self.downsize(0);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underByteArrayCheck(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfByteArray(args.get(0)));
}

RawObject UnderBuiltinsModule::underByteArrayGuard(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfByteArray(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kByteArray);
}

RawObject UnderBuiltinsModule::underByteArrayDelItem(Thread* thread,
                                                     Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  ByteArray self(&scope, args.get(0));
  word length = self.numItems();
  Object index_obj(&scope, args.get(1));
  word idx = Int::cast(intUnderlying(thread, index_obj)).asWordSaturated();
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

RawObject UnderBuiltinsModule::underByteArrayDelSlice(Thread* thread,
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

  Object start_obj(&scope, args.get(1));
  Int start_int(&scope, intUnderlying(thread, start_obj));
  word start = start_int.asWord();

  Object stop_obj(&scope, args.get(2));
  Int stop_int(&scope, intUnderlying(thread, stop_obj));
  word stop = stop_int.asWord();

  Object step_obj(&scope, args.get(3));
  Int step_int(&scope, intUnderlying(thread, step_obj));
  // Lossy truncation of step to a word is expected.
  word step = step_int.asWordSaturated();

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

RawObject UnderBuiltinsModule::underByteArraySetItem(Thread* thread,
                                                     Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray self(&scope, args.get(0));
  Object key_obj(&scope, args.get(1));
  Int key(&scope, intUnderlying(thread, key_obj));
  Object value_obj(&scope, args.get(2));
  Int value(&scope, intUnderlying(thread, value_obj));
  word index = key.asWordSaturated();
  if (!SmallInt::isValid(index)) {
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
  word val = value.asWordSaturated();
  if (val < 0 || val > kMaxByte) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "byte must be in range(0, 256)");
  }
  self.byteAtPut(index, val);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underByteArraySetSlice(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray self(&scope, args.get(0));
  Object start_obj(&scope, args.get(1));
  word start = Int::cast(intUnderlying(thread, start_obj)).asWord();
  Object stop_obj(&scope, args.get(2));
  word stop = Int::cast(intUnderlying(thread, stop_obj)).asWord();
  Object step_obj(&scope, args.get(3));
  word step = Int::cast(intUnderlying(thread, step_obj)).asWord();
  Object src_obj(&scope, args.get(4));
  Bytes src_bytes(&scope, Bytes::empty());
  word src_length;

  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfBytes(*src_obj)) {
    Bytes src(&scope, bytesUnderlying(thread, src_obj));
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

RawObject UnderBuiltinsModule::underBytesGuard(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfBytes(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kBytes);
}

RawObject UnderBuiltinsModule::underByteArrayJoin(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray sep(&scope, args.get(0));
  Bytes sep_bytes(&scope, sep.bytes());
  Object iterable(&scope, args.get(1));
  Object joined(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  if (iterable.isList()) {
    List list(&scope, *iterable);
    Tuple src(&scope, list.items());
    joined = runtime->bytesJoin(thread, sep_bytes, sep.numItems(), src,
                                list.numItems());
  } else if (iterable.isTuple()) {
    Tuple src(&scope, *iterable);
    joined = runtime->bytesJoin(thread, sep_bytes, sep.numItems(), src,
                                src.length());
  }
  // Check for error or slow path
  if (!joined.isMutableBytes()) return *joined;
  MutableBytes joined_bytes(&scope, *joined);
  ByteArray result(&scope, runtime->newByteArray());
  result.setBytes(*joined_bytes);
  result.setNumItems(joined_bytes.length());
  return *result;
}

RawObject UnderBuiltinsModule::underByteArrayLen(Thread* thread, Frame* frame,
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
  Object value(&scope, args.get(1));
  value = bytesUnderlying(thread, value);
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

RawObject UnderBuiltinsModule::underBytesGetItem(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Object key_obj(&scope, args.get(1));
  Int key(&scope, intUnderlying(thread, key_obj));
  word index = key.asWordSaturated();
  if (!SmallInt::isValid(index)) {
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

RawObject UnderBuiltinsModule::underBytesGetSlice(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Object obj(&scope, args.get(1));
  Int start(&scope, intUnderlying(thread, obj));
  obj = args.get(2);
  Int stop(&scope, intUnderlying(thread, obj));
  obj = args.get(3);
  Int step(&scope, intUnderlying(thread, obj));
  return thread->runtime()->bytesSlice(thread, self, start.asWord(),
                                       stop.asWord(), step.asWord());
}

RawObject UnderBuiltinsModule::underBytesJoin(Thread* thread, Frame* frame,
                                              word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Object iterable(&scope, args.get(1));
  if (iterable.isList()) {
    List list(&scope, *iterable);
    Tuple src(&scope, list.items());
    return thread->runtime()->bytesJoin(thread, self, self.length(), src,
                                        list.numItems());
  }
  if (iterable.isTuple()) {
    Tuple src(&scope, *iterable);
    return thread->runtime()->bytesJoin(thread, self, self.length(), src,
                                        src.length());
  }
  // Slow path: collect items into list in Python and call again
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underBytesLen(Thread* thread, Frame* frame,
                                             word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  return SmallInt::fromWord(self.length());
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
    Bytes bytes(&scope, bytesUnderlying(thread, from_obj));
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
    Bytes bytes(&scope, bytesUnderlying(thread, to_obj));
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
  Object self_obj(&scope, args.get(0));
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Object count_obj(&scope, args.get(1));
  Int count_int(&scope, intUnderlying(thread, count_obj));
  word count = count_int.asWordSaturated();
  if (!SmallInt::isValid(count)) {
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
  Object self_obj(&scope, args.get(0));
  Object sep_obj(&scope, args.get(1));
  Object maxsplit_obj(&scope, args.get(2));
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Int maxsplit_int(&scope, intUnderlying(thread, maxsplit_obj));
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
    Bytes sep(&scope, bytesUnderlying(thread, sep_obj));
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
  Object self_obj(&scope, args.get(0));
  Object maxsplit_obj(&scope, args.get(1));
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Int maxsplit_int(&scope, intUnderlying(thread, maxsplit_obj));
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

RawObject UnderBuiltinsModule::underByteslikeCount(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  word haystack_len;
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes self(&scope, bytesUnderlying(thread, self_obj));
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
    Bytes sub(&scope, bytesUnderlying(thread, sub_obj));
    sub_obj = *sub;
    needle_len = sub.length();
  } else if (runtime->isInstanceOfByteArray(*sub_obj)) {
    ByteArray sub(&scope, *sub_obj);
    sub_obj = sub.bytes();
    needle_len = sub.numItems();
  } else if (runtime->isInstanceOfInt(*sub_obj)) {
    word sub = Int(&scope, intUnderlying(thread, sub_obj)).asWordSaturated();
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
  word start = Int::cast(intUnderlying(thread, start_obj)).asWordSaturated();
  word end = Int::cast(intUnderlying(thread, stop_obj)).asWordSaturated();
  return SmallInt::fromWord(
      bytesCount(haystack, haystack_len, needle, needle_len, start, end));
}

RawObject UnderBuiltinsModule::underByteslikeEndsWith(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  word self_len;
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes self(&scope, bytesUnderlying(thread, self_obj));
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
    Bytes suffix(&scope, bytesUnderlying(thread, suffix_obj));
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
  Int start(&scope, start_obj.isUnbound() ? SmallInt::fromWord(0)
                                          : intUnderlying(thread, start_obj));
  Int end(&scope, end_obj.isUnbound() ? SmallInt::fromWord(self_len)
                                      : intUnderlying(thread, end_obj));
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
    Bytes self(&scope, bytesUnderlying(thread, self_obj));
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
    Bytes sub(&scope, bytesUnderlying(thread, sub_obj));
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
  Object start_obj(&scope, args.get(2));
  Object stop_obj(&scope, args.get(3));
  Int start(&scope, intUnderlying(thread, start_obj));
  Int end(&scope, intUnderlying(thread, stop_obj));
  return SmallInt::fromWord(bytesFind(haystack, haystack_len, needle,
                                      needle_len, start.asWordSaturated(),
                                      end.asWordSaturated()));
}

RawObject UnderBuiltinsModule::underByteslikeFindInt(Thread* thread,
                                                     Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object sub_obj(&scope, args.get(1));
  Int sub_int(&scope, intUnderlying(thread, sub_obj));
  word sub = sub_int.asWordSaturated();
  if (sub < 0 || sub > kMaxByte) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "byte must be in range(0, 256)");
  }
  Bytes needle(&scope, runtime->newBytes(1, sub));
  Object self_obj(&scope, args.get(0));
  Object start_obj(&scope, args.get(2));
  Object stop_obj(&scope, args.get(3));
  Int start(&scope, intUnderlying(thread, start_obj));
  Int end(&scope, intUnderlying(thread, stop_obj));
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes haystack(&scope, bytesUnderlying(thread, self_obj));
    return SmallInt::fromWord(
        bytesFind(haystack, haystack.length(), needle, needle.length(),
                  start.asWordSaturated(), end.asWordSaturated()));
  }
  if (runtime->isInstanceOfByteArray(*self_obj)) {
    ByteArray self(&scope, *self_obj);
    Bytes haystack(&scope, self.bytes());
    return SmallInt::fromWord(
        bytesFind(haystack, self.numItems(), needle, needle.length(),
                  start.asWordSaturated(), end.asWordSaturated()));
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

RawObject UnderBuiltinsModule::underByteslikeRFindByteslike(Thread* thread,
                                                            Frame* frame,
                                                            word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  word haystack_len;
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes self(&scope, bytesUnderlying(thread, self_obj));
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
    Bytes sub(&scope, bytesUnderlying(thread, sub_obj));
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
  Object start_obj(&scope, args.get(2));
  Object stop_obj(&scope, args.get(3));
  Int start(&scope, intUnderlying(thread, start_obj));
  Int end(&scope, intUnderlying(thread, stop_obj));
  return SmallInt::fromWord(bytesRFind(haystack, haystack_len, needle,
                                       needle_len, start.asWordSaturated(),
                                       end.asWordSaturated()));
}

RawObject UnderBuiltinsModule::underByteslikeRFindInt(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object sub_obj(&scope, args.get(1));
  Int sub_int(&scope, intUnderlying(thread, sub_obj));
  word sub = sub_int.asWordSaturated();
  if (sub < 0 || sub > kMaxByte) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "byte must be in range(0, 256)");
  }
  Bytes needle(&scope, runtime->newBytes(1, sub));
  Object self_obj(&scope, args.get(0));
  Object start_obj(&scope, args.get(2));
  Object stop_obj(&scope, args.get(3));
  Int start(&scope, intUnderlying(thread, start_obj));
  Int end(&scope, intUnderlying(thread, stop_obj));
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes haystack(&scope, bytesUnderlying(thread, self_obj));
    return SmallInt::fromWord(
        bytesRFind(haystack, haystack.length(), needle, needle.length(),
                   start.asWordSaturated(), end.asWordSaturated()));
  }
  if (runtime->isInstanceOfByteArray(*self_obj)) {
    ByteArray self(&scope, *self_obj);
    Bytes haystack(&scope, self.bytes());
    return SmallInt::fromWord(
        bytesRFind(haystack, self.numItems(), needle, needle.length(),
                   start.asWordSaturated(), end.asWordSaturated()));
  }
  UNIMPLEMENTED("bytes-like other than bytes, bytearray");
}

RawObject UnderBuiltinsModule::underByteslikeStartsWith(Thread* thread,
                                                        Frame* frame,
                                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  word self_len;
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes self(&scope, bytesUnderlying(thread, self_obj));
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
    Bytes prefix(&scope, bytesUnderlying(thread, prefix_obj));
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
  Object start_obj(&scope, args.get(2));
  Object end_obj(&scope, args.get(3));
  word start = Int::cast(intUnderlying(thread, start_obj)).asWordSaturated();
  word end = Int::cast(intUnderlying(thread, end_obj)).asWordSaturated();
  return runtime->bytesStartsWith(self, self_len, prefix, prefix_len, start,
                                  end);
}

RawObject UnderBuiltinsModule::underClassMethod(Thread* thread, Frame* frame,
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
                              thread, obj, SymbolId::kDunderIsAbstractMethod));
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

RawObject UnderBuiltinsModule::underClassMethodIsAbstract(Thread* thread,
                                                          Frame* frame,
                                                          word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ClassMethod self(&scope, args.get(0));
  Object func(&scope, self.function());
  return isAbstract(thread, func);
}

RawObject UnderBuiltinsModule::underCodeGuard(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  if (args.get(0).isCode()) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kCode);
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

RawObject UnderBuiltinsModule::underDelattr(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  Object name_obj(&scope, args.get(1));
  Str name(&scope, strUnderlying(thread, name_obj));
  Object result(&scope, thread->runtime()->attributeDel(thread, obj, name));
  if (result.isError()) {
    return *result;
  }
  return NoneType::object();
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
  Object key_hash(&scope, args.get(3));
  key_hash = SmallInt::fromWordTruncated(
      Int::cast(intUnderlying(thread, key_hash)).digitAt(0));
  Object value(&scope, args.get(4));
  bool has_empty_slot = Dict::Bucket::isEmpty(*data, index);
  Dict::Bucket::set(*data, index, *key_hash, *key, *value);
  dict.setNumItems(dict.numItems() + 1);
  if (has_empty_slot) {
    dict.decrementNumUsableItems();
    thread->runtime()->dictEnsureCapacity(thread, dict);
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
  Object key_hash(&scope, args.get(2));
  if (!runtime->isInstanceOfInt(*key_hash)) {
    return thread->raiseRequiresType(key_hash, SymbolId::kInt);
  }
  key_hash = intUnderlying(thread, key_hash);
  key_hash = SmallInt::fromWordTruncated(Int::cast(*key_hash).digitAt(0));
  if (dict.capacity() == 0) {
    dict.setData(runtime->newMutableTuple(Runtime::kInitialDictCapacity *
                                          Dict::Bucket::kNumPointers));
    dict.resetNumUsableItems();
  }
  Tuple data(&scope, dict.data());
  word bucket_mask = Dict::Bucket::bucketMask(data.length());
  uword perturb = static_cast<uword>(RawSmallInt::cast(*key_hash).value());
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
      if (SmallInt::cast(*key_hash).value() ==
          SmallInt::cast(Dict::Bucket::hash(*data, index)).value()) {
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
  Object key_hash(&scope, args.get(3));
  key_hash = SmallInt::fromWordTruncated(
      Int::cast(intUnderlying(thread, key_hash)).digitAt(0));
  uword perturb;
  if (args.get(4).isUnbound()) {
    perturb = static_cast<uword>(RawSmallInt::cast(*key_hash).value());
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
    if (SmallInt::cast(*key_hash).value() ==
        SmallInt::cast(Dict::Bucket::hash(*data, index)).value()) {
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

RawObject UnderBuiltinsModule::underDivmod(Thread* thread, Frame* frame,
                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object number(&scope, args.get(0));
  Object divisor(&scope, args.get(1));
  return Interpreter::binaryOperation(
      thread, frame, Interpreter::BinaryOp::DIVMOD, number, divisor);
}

RawObject UnderBuiltinsModule::underFloatCheck(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfFloat(args.get(0)));
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

  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  Float self_float(&scope, floatUnderlying(thread, self_obj));
  double left = self_float.value();

  Object other_obj(&scope, args.get(1));
  Float other_float(&scope, floatUnderlying(thread, other_obj));
  double divisor = other_float.value();
  if (divisor == 0.0) {
    return thread->raiseWithFmt(LayoutId::kZeroDivisionError, "float divmod()");
  }

  double remainder;
  double quotient = floatDivmod(left, divisor, &remainder);
  Tuple result(&scope, runtime->newTuple(2));
  result.atPut(0, runtime->newFloat(quotient));
  result.atPut(1, runtime->newFloat(remainder));
  return *result;
}

RawObject UnderBuiltinsModule::underFloatFormat(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object value_obj(&scope, args.get(0));
  Float value(&scope, floatUnderlying(thread, value_obj));
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
  unique_c_ptr<char> c_str(
      formatFloat(value.value(), format_code_char, precision.value(),
                  always_add_sign.value(), add_dot_0.value(),
                  use_alt_formatting.value(), nullptr));
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

RawObject UnderBuiltinsModule::underFloatSignbit(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object value_obj(&scope, args.get(0));
  Float value(&scope, floatUnderlying(thread, value_obj));
  return Bool::fromBool(std::signbit(value.value()));
}

RawObject UnderBuiltinsModule::underFrozenSetCheck(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfFrozenSet(args.get(0)));
}

RawObject UnderBuiltinsModule::underFrozenSetGuard(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfFrozenSet(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kFrozenSet);
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

RawObject UnderBuiltinsModule::underGc(Thread* thread, Frame* /* frame */,
                                       word /* nargs */) {
  thread->runtime()->collectGarbage();
  return NoneType::object();
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

RawObject UnderBuiltinsModule::underGetMemberPyObject(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  auto pyobject = reinterpret_cast<PyObject**>(addr);
  if (*pyobject == nullptr) {
    if (args.get(1).isNoneType()) return NoneType::object();
    HandleScope scope(thread);
    Str name(&scope, args.get(1));
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "Object attribute '%S' is nullptr", &name);
  }
  return ApiHandle::fromPyObject(*pyobject)->asObject();
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

RawObject UnderBuiltinsModule::underGetMemberUByte(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  unsigned char value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newIntFromUnsigned(value);
}

RawObject UnderBuiltinsModule::underGetMemberUInt(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  unsigned int value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newIntFromUnsigned(value);
}

RawObject UnderBuiltinsModule::underGetMemberULong(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  unsigned long value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newIntFromUnsigned(value);
}

RawObject UnderBuiltinsModule::underGetMemberUShort(Thread* thread,
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
  HeapObject instance(&scope, args.get(0));
  Object name(&scope, args.get(1));
  // TODO(T53626118) Raise an exception when `name_str` is a string subclass
  // that overrides `__eq__` or `__hash__`.
  Str name_str(&scope, strUnderlying(thread, name));
  Runtime* runtime = thread->runtime();
  Str name_interned(&scope, runtime->internStr(thread, name_str));
  return instanceDelAttr(thread, instance, name_interned);
}

RawObject UnderBuiltinsModule::underInstanceGetattr(Thread* thread,
                                                    Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  HeapObject instance(&scope, args.get(0));
  Object name(&scope, args.get(1));
  // TODO(T53626118) Raise an exception when `name_str` is a string subclass
  // that overrides `__eq__` or `__hash__`.
  Str name_str(&scope, strUnderlying(thread, name));
  Str name_interned(&scope, thread->runtime()->internStr(thread, name_str));
  Object result(&scope, instanceGetAttribute(thread, instance, name_interned));
  return result.isErrorNotFound() ? Unbound::object() : *result;
}

RawObject UnderBuiltinsModule::underInstanceKeys(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object object(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, runtime->layoutAt(object.layoutId()));
  List result(&scope, runtime->newList());
  // Add in-object attributes
  Tuple in_object(&scope, layout.inObjectAttributes());
  for (word i = 0, length = in_object.length(); i < length; i++) {
    Tuple pair(&scope, in_object.at(i));
    Object name(&scope, pair.at(0));
    if (name.isNoneType()) continue;
    runtime->listAdd(thread, result, name);
  }
  // Add overflow attributes
  if (layout.hasTupleOverflow()) {
    Tuple overflow(&scope, layout.overflowAttributes());
    for (word i = 0; i < overflow.length(); i++) {
      Tuple pair(&scope, overflow.at(i));
      Object name(&scope, pair.at(0));
      if (name.isNoneType()) continue;
      runtime->listAdd(thread, result, name);
    }
  } else {
    // Dict overflow should be handled by a __dict__ descriptor on the type,
    // like `type` or `function`
    CHECK(layout.overflowAttributes().isNoneType(), "no overflow");
  }
  return *result;
}

RawObject UnderBuiltinsModule::underInstanceSetattr(Thread* thread,
                                                    Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  HeapObject instance(&scope, args.get(0));
  Object name(&scope, args.get(1));
  // TODO(T53626118) Raise an exception when `name_str` is a string subclass
  // that overrides `__eq__` or `__hash__`.
  Str name_str(&scope, strUnderlying(thread, name));
  Str name_interned(&scope, thread->runtime()->internStr(thread, name_str));
  Object value(&scope, args.get(2));
  return instanceSetAttr(thread, instance, name_interned, value);
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

static RawObject intFromBytes(Thread* /* t */, const Bytes& bytes, word length,
                              word base) {
  DCHECK_BOUND(length, bytes.length());
  DCHECK(base == 0 || (base >= 2 && base <= 36), "invalid base");
  if (length == 0) {
    return Error::error();
  }
  unique_c_ptr<char[]> str(reinterpret_cast<char*>(std::malloc(length + 1)));
  bytes.copyTo(reinterpret_cast<byte*>(str.get()), length);
  str[length] = '\0';
  char* end;
  errno = 0;
  const word result = std::strtoll(str.get(), &end, base);
  const int saved_errno = errno;
  if (end != str.get() + length || saved_errno == EINVAL) {
    return Error::error();
  }
  if (SmallInt::isValid(result) && saved_errno != ERANGE) {
    return SmallInt::fromWord(result);
  }
  UNIMPLEMENTED("LargeInt from bytes-like");
}

RawObject UnderBuiltinsModule::underIntNewFromByteArray(Thread* thread,
                                                        Frame* frame,
                                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  ByteArray array(&scope, args.get(1));
  Bytes bytes(&scope, array.bytes());
  Object base_obj(&scope, args.get(2));
  Int base_int(&scope, intUnderlying(thread, base_obj));
  DCHECK(base_int.numDigits() == 1, "invalid base");
  word base = base_int.asWord();
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
  Object bytes_obj(&scope, args.get(1));
  Bytes bytes(&scope, bytesUnderlying(thread, bytes_obj));
  Object base_obj(&scope, args.get(2));
  Int base_int(&scope, intUnderlying(thread, base_obj));
  DCHECK(base_int.numDigits() == 1, "invalid base");
  word base = base_int.asWord();
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
    value = intUnderlying(thread, value);
  }
  return intOrUserSubclass(thread, type, value);
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

static word inferBase(const Str& str, word start) {
  if (str.charAt(start) == '0' && start + 1 < str.charLength()) {
    switch (str.charAt(start + 1)) {
      case 'x':
      case 'X':
        return 16;
      case 'o':
      case 'O':
        return 8;
      case 'b':
      case 'B':
        return 2;
    }
  }
  return 10;
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
  word inferred_base = inferBase(str, start);
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
  Object base_obj(&scope, args.get(2));
  Int base_int(&scope, intUnderlying(thread, base_obj));
  DCHECK(base_int.numDigits() == 1, "invalid base");
  word base = base_int.asWord();
  Object result(&scope, intFromStr(thread, str, base));
  if (result.isError()) {
    Str repr(&scope, thread->invokeMethod1(str, SymbolId::kDunderRepr));
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "invalid literal for int() with base %w: %S",
                                base == 0 ? 10 : base, &repr);
  }
  return intOrUserSubclass(thread, type, result);
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

RawObject UnderBuiltinsModule::underListDelItem(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  List self(&scope, args.get(0));
  word length = self.numItems();
  Object index_obj(&scope, args.get(1));
  Int index_int(&scope, intUnderlying(thread, index_obj));
  word idx = index_int.asWordSaturated();
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

RawObject UnderBuiltinsModule::underListDelSlice(Thread* thread, Frame* frame,
                                                 word nargs) {
  // This function deletes elements that are specified by a slice by copying.
  // It compacts to the left elements in the slice range and then copies
  // elements after the slice into the free area.  The list element count is
  // decremented and elements in the unused part of the list are overwritten
  // with None.
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  List list(&scope, args.get(0));

  Object start_obj(&scope, args.get(1));
  Int start_int(&scope, intUnderlying(thread, start_obj));
  word start = start_int.asWord();

  Object stop_obj(&scope, args.get(2));
  Int stop_int(&scope, intUnderlying(thread, stop_obj));
  word stop = stop_int.asWord();

  Object step_obj(&scope, args.get(3));
  Int step_int(&scope, intUnderlying(thread, step_obj));
  // Lossy truncation of step to a word is expected.
  word step = step_int.asWordSaturated();

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

RawObject UnderBuiltinsModule::underListGetItem(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  List self(&scope, args.get(0));
  Object key_obj(&scope, args.get(1));
  Int key(&scope, intUnderlying(thread, key_obj));
  word index = key.asWordSaturated();
  if (!SmallInt::isValid(index)) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "cannot fit '%T' into an index-sized integer",
                                &key_obj);
  }
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

RawObject UnderBuiltinsModule::underListGetSlice(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  List self(&scope, args.get(0));
  Object obj(&scope, args.get(1));
  Int start(&scope, intUnderlying(thread, obj));
  obj = args.get(2);
  Int stop(&scope, intUnderlying(thread, obj));
  obj = args.get(3);
  Int step(&scope, intUnderlying(thread, obj));
  return listSlice(thread, self, start.asWord(), stop.asWord(), step.asWord());
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

RawObject UnderBuiltinsModule::underMappingProxyGuard(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  if (args.get(0).isMappingProxy()) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kMappingProxy);
}

RawObject UnderBuiltinsModule::underMappingProxyMapping(Thread* thread,
                                                        Frame* frame,
                                                        word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  MappingProxy mappingproxy(&scope, args.get(0));
  return mappingproxy.mapping();
}

RawObject UnderBuiltinsModule::underMappingProxySetMapping(Thread* thread,
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

RawObject UnderBuiltinsModule::underModuleProxyDelitem(Thread* thread,
                                                       Frame* frame,
                                                       word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  ModuleProxy self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  Module module(&scope, self.module());
  DCHECK(module.moduleProxy() == self, "module.proxy != proxy.module");
  Object key_hash(&scope, Interpreter::hash(thread, key));
  if (key_hash.isErrorException()) return *key_hash;
  Object result(&scope, moduleRemove(thread, module, key, key_hash));
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
  Object key(&scope, args.get(1));
  Object default_obj(&scope, args.get(2));
  Module module(&scope, self.module());
  DCHECK(module.moduleProxy() == self, "module.proxy != proxy.module");
  Object key_hash(&scope, Interpreter::hash(thread, key));
  if (key_hash.isErrorException()) return *key_hash;
  Object result(&scope, moduleAt(thread, module, key, key_hash));
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
  Object key(&scope, args.get(1));
  Object value(&scope, args.get(2));
  Module module(&scope, self.module());
  DCHECK(module.moduleProxy() == self, "module.proxy != proxy.module");
  Object key_hash(&scope, Interpreter::hash(thread, key));
  if (key_hash.isErrorException()) return *key_hash;
  return moduleAtPut(thread, module, key, key_hash, value);
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

RawObject UnderBuiltinsModule::underObjectCreateIterator(Thread* thread,
                                                         Frame* frame,
                                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object object(&scope, args.get(0));
  return Interpreter::createIterator(thread, thread->currentFrame(), object);
}

RawObject UnderBuiltinsModule::underObjectTypeGetAttr(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object instance(&scope, args.get(0));
  Type type(&scope, thread->runtime()->typeOf(*instance));
  Str name(&scope, args.get(1));
  Object attr(&scope, typeLookupInMroByStr(thread, type, name));
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
  Str name(&scope, args.get(1));
  Object result(&scope, typeLookupInMroByStr(thread, type, name));
  return Bool::fromBool(!result.isErrorNotFound());
}

RawObject UnderBuiltinsModule::underOsWrite(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object fd_obj(&scope, args.get(0));
  CHECK(fd_obj.isSmallInt(), "fd must be small int");
  Bytes bytes_buf(&scope, args.get(1));
  size_t count = bytes_buf.length();
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
  Str fn_name(&scope, patch_fn.name());
  Runtime* runtime = thread->runtime();
  Object module_name(&scope, patch_fn.module());
  Module module(&scope, runtime->findModule(module_name));
  Object base_fn_obj(&scope, moduleAtByStr(thread, module, fn_name));
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

RawObject UnderBuiltinsModule::underPropertyIsAbstract(Thread* thread,
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

RawObject UnderBuiltinsModule::underPyObjectOffset(Thread* thread, Frame* frame,
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

RawObject UnderBuiltinsModule::underSetMemberPyObject(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(1));
  std::memcpy(reinterpret_cast<void*>(addr), &value, sizeof(value));
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

RawObject UnderBuiltinsModule::underSliceStart(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object step_obj(&scope, args.get(1));
  Int step(&scope, intUnderlying(thread, step_obj));
  Object length_obj(&scope, args.get(2));
  Int length(&scope, intUnderlying(thread, length_obj));
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
  Int start(&scope, intUnderlying(thread, start_obj));
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
  HandleScope scope(thread);
  Object step_obj(&scope, args.get(0));
  if (step_obj.isNoneType()) return SmallInt::fromWord(1);
  Int step(&scope, intUnderlying(thread, step_obj));
  if (step.isZero()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "slice step cannot be zero");
  }
  return *step;
}

RawObject UnderBuiltinsModule::underSliceStop(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object step_obj(&scope, args.get(1));
  Int step(&scope, intUnderlying(thread, step_obj));
  Object length_obj(&scope, args.get(2));
  Int length(&scope, intUnderlying(thread, length_obj));
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
  Int stop(&scope, intUnderlying(thread, stop_obj));
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

RawObject UnderBuiltinsModule::underStaticMethodIsAbstract(Thread* thread,
                                                           Frame* frame,
                                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  StaticMethod self(&scope, args.get(0));
  Object func(&scope, self.function());
  return isAbstract(thread, func);
}

RawObject UnderBuiltinsModule::underStrArrayIadd(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  StrArray self(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  Str other(&scope, strUnderlying(thread, other_obj));
  thread->runtime()->strArrayAddStr(thread, self, other);
  return *self;
}

RawObject UnderBuiltinsModule::underStrCheck(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfStr(args.get(0)));
}

RawObject UnderBuiltinsModule::underStrCheckExact(Thread*, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isStr());
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
    Int start_int(&scope, intUnderlying(thread, start_obj));
    start = start_int.asWordSaturated();
  }
  word end = kMaxWord;
  if (!end_obj.isNoneType()) {
    Int end_int(&scope, intUnderlying(thread, end_obj));
    end = end_int.asWordSaturated();
  }
  return strCount(haystack, needle, start, end);
}

RawObject UnderBuiltinsModule::underStrEndsWith(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Object suffix_obj(&scope, args.get(1));
  Object start_obj(&scope, args.get(2));
  Object end_obj(&scope, args.get(3));
  Str self(&scope, strUnderlying(thread, self_obj));
  Str suffix(&scope, strUnderlying(thread, suffix_obj));

  word len = self.codePointLength();
  word start = 0;
  word end = len;
  if (!start_obj.isNoneType()) {
    Int start_int(&scope, intUnderlying(thread, start_obj));
    start = start_int.asWordSaturated();  // TODO(T55084422): bounds checking
  }
  if (!end_obj.isNoneType()) {
    Int end_int(&scope, intUnderlying(thread, end_obj));
    end = end_int.asWordSaturated();  // TODO(T55084422): bounds checking
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
    Int start_int(&scope, intUnderlying(thread, start_obj));
    start = start_int.asWordSaturated();
  }
  word end = kMaxWord;
  if (!end_obj.isNoneType()) {
    Int end_int(&scope, intUnderlying(thread, end_obj));
    end = end_int.asWordSaturated();
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
  Object value_obj(&scope, args.get(1));
  Str value(&scope, strUnderlying(thread, value_obj));
  if (type.isBuiltin()) return *value;
  Layout type_layout(&scope, type.instanceLayout());
  UserStrBase instance(&scope, thread->runtime()->newInstance(type_layout));
  instance.setValue(*value);
  return *instance;
}

RawObject UnderBuiltinsModule::underStrGuard(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfStr(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kStr);
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
  Object self_obj(&scope, args.get(0));
  Str self(&scope, strUnderlying(thread, self_obj));
  return SmallInt::fromWord(self.codePointLength());
}

RawObject UnderBuiltinsModule::underStrReplace(Thread* thread, Frame* frame,
                                               word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object oldstr_obj(&scope, args.get(1));
  Object newstr_obj(&scope, args.get(2));
  Str self(&scope, strUnderlying(thread, self_obj));
  Str oldstr(&scope, strUnderlying(thread, oldstr_obj));
  Str newstr(&scope, strUnderlying(thread, newstr_obj));
  Object count_obj(&scope, args.get(3));
  Int count(&scope, intUnderlying(thread, count_obj));
  return runtime->strReplace(thread, self, oldstr, newstr,
                             count.asWordSaturated());
}

RawObject UnderBuiltinsModule::underStrRFind(Thread* thread, Frame* frame,
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
    Int start_int(&scope, intUnderlying(thread, start_obj));
    start = start_int.asWordSaturated();
  }
  word end = kMaxWord;
  if (!end_obj.isNoneType()) {
    Int end_int(&scope, intUnderlying(thread, end_obj));
    end = end_int.asWordSaturated();
  }
  word result = strRFind(haystack, needle, start, end);
  return SmallInt::fromWord(result);
}

static word findReverse(const Str& haystack, word haystack_len,
                        const Str& needle, word needle_len) {
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
  return Utils::memoryFindReverse(haystack_ptr, haystack_len, needle_ptr,
                                  needle_len);
}

// Look for needle in haystack, starting from the right. Return a tuple
// containing:
// * haystack up to but not including needle
// * needle
// * haystack after and not including needle
// If needle is not found in haystack, return ("", "", haystack)
RawObject UnderBuiltinsModule::underStrRPartition(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object haystack_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfStr(*haystack_obj),
         "_str_rfind requires 'str' instance");
  Object needle_obj(&scope, args.get(1));
  DCHECK(runtime->isInstanceOfStr(*needle_obj),
         "_str_rfind requires 'str' instance");
  Str haystack(&scope, strUnderlying(thread, haystack_obj));
  Str needle(&scope, strUnderlying(thread, needle_obj));
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
  word prefix_len = findReverse(haystack, haystack_len, needle, needle_len);
  if (prefix_len < 0) return result.becomeImmutable();
  result.atPut(0, runtime->strSubstr(thread, haystack, 0, prefix_len));
  result.atPut(1, *needle);
  word suffix_start = prefix_len + needle_len;
  word suffix_len = haystack_len - suffix_start;
  result.atPut(2,
               runtime->strSubstr(thread, haystack, suffix_start, suffix_len));
  return result.becomeImmutable();
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
  Object keepends_obj(&scope, args.get(1));
  Int keepends_int(&scope, intUnderlying(thread, keepends_obj));
  bool keepends = !keepends_int.isZero();
  return strSplitlines(thread, self, keepends);
}

RawObject UnderBuiltinsModule::underStrStartsWith(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Object prefix_obj(&scope, args.get(1));
  Object start_obj(&scope, args.get(2));
  Object end_obj(&scope, args.get(3));
  Str self(&scope, strUnderlying(thread, self_obj));
  Str prefix(&scope, strUnderlying(thread, prefix_obj));

  word len = self.codePointLength();
  word start = 0;
  word end = len;
  if (!start_obj.isNoneType()) {
    Int start_int(&scope, intUnderlying(thread, start_obj));
    start = start_int.asWordSaturated();  // TODO(T55084422): bounds checking
  }
  if (!end_obj.isNoneType()) {
    Int end_int(&scope, intUnderlying(thread, end_obj));
    end = end_int.asWordSaturated();  // TODO(T55084422): bounds checking
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

RawObject UnderBuiltinsModule::underTupleGetItem(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Tuple self(&scope, tupleUnderlying(thread, self_obj));
  Object key_obj(&scope, args.get(1));
  Int key(&scope, intUnderlying(thread, key_obj));
  word index = key.asWordSaturated();
  if (!SmallInt::isValid(index)) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "cannot fit '%T' into an index-sized integer",
                                &key_obj);
  }
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

RawObject UnderBuiltinsModule::underTupleGetSlice(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Tuple self(&scope, tupleUnderlying(thread, self_obj));
  Int start(&scope, args.get(1));
  Int stop(&scope, args.get(2));
  Int step(&scope, args.get(3));
  return tupleSlice(thread, self, start.asWordSaturated(),
                    stop.asWordSaturated(), step.asWordSaturated());
}

RawObject UnderBuiltinsModule::underTupleGuard(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfTuple(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kTuple);
}

RawObject UnderBuiltinsModule::underTupleLen(Thread* thread, Frame* frame,
                                             word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Tuple self(&scope, tupleUnderlying(thread, self_obj));
  return SmallInt::fromWord(self.length());
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
  UserTupleBase instance(&scope, thread->runtime()->newInstance(layout));
  instance.setTupleValue(args.get(1));
  return *instance;
}

RawObject UnderBuiltinsModule::underType(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  return thread->runtime()->typeOf(args.get(0));
}

RawObject UnderBuiltinsModule::underTypeAbstractMethodsDel(Thread* thread,
                                                           Frame* frame,
                                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  if (type.abstractMethods().isUnbound()) {
    return thread->raiseWithId(LayoutId::kAttributeError,
                               SymbolId::kDunderAbstractMethods);
  }
  type.setAbstractMethods(Unbound::object());
  type.setFlagsAndBuiltinBase(
      static_cast<Type::Flag>(type.flags() & ~Type::Flag::kIsAbstract),
      type.builtinBase());
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underTypeAbstractMethodsGet(Thread* thread,
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
                             SymbolId::kDunderAbstractMethods);
}

RawObject UnderBuiltinsModule::underTypeAbstractMethodsSet(Thread* thread,
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

RawObject UnderBuiltinsModule::underTypeGuard(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfType(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kType);
}

RawObject UnderBuiltinsModule::underTypeIsSubclass(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type subclass(&scope, args.get(0));
  Type superclass(&scope, args.get(1));
  return Bool::fromBool(thread->runtime()->isSubclass(subclass, superclass));
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
  Object key(&scope, args.get(1));
  Object default_obj(&scope, args.get(2));
  Type type(&scope, self.type());
  Object key_hash(&scope, Interpreter::hash(thread, key));
  if (key_hash.isErrorException()) return *key_hash;
  Object result(&scope, typeAt(thread, type, key, key_hash));
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
  Tuple bases(&scope, args.get(2));
  Dict dict(&scope, args.get(3));
  Tuple mro(&scope, thread->runtime()->emptyTuple());
  if (args.get(4).isUnbound()) {
    Object mro_obj(&scope, computeMro(thread, type, bases));
    if (mro_obj.isError()) return *mro_obj;
    mro = *mro_obj;
  } else {
    mro = args.get(4);
  }
  return typeInit(thread, type, name, bases, dict, mro);
}

RawObject UnderBuiltinsModule::underUnimplemented(Thread* thread, Frame* frame,
                                                  word) {
  python::Utils::printTracebackToStderr();

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

RawObject UnderBuiltinsModule::underWeakRefCallback(Thread* thread,
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfWeakRef(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kUnderWeakRef);
  }
  WeakRef self(&scope, *self_obj);
  return self.callback();
}

}  // namespace python
