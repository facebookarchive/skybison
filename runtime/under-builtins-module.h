#pragma once

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

// Copy the code, entry, and interpreter info from base to patch.
void copyFunctionEntries(Thread* thread, const Function& base,
                         const Function& patch);

class UnderBuiltinsModule {
 public:
  static RawObject underAddress(Thread* thread, Frame* frame, word nargs);
  static RawObject underBoundMethod(Thread* thread, Frame* frame, word nargs);
  static RawObject underByteArrayCheck(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underByteArrayClear(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underByteArrayJoin(Thread* thread, Frame* frame, word nargs);
  static RawObject underByteArraySetItem(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underBytesCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesFromInts(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesGetItem(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesGetSlice(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesJoin(Thread* thread, Frame* frame, word nargs);
  static RawObject underBytesMaketrans(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underBytesRepeat(Thread* thread, Frame* frame, word nargs);
  static RawObject underByteslikeFindByteslike(Thread* thread, Frame* frame,
                                               word nargs);
  static RawObject underByteslikeFindInt(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underClassMethod(Thread* thread, Frame* frame, word nargs);
  static RawObject underComplexImag(Thread* thread, Frame* frame, word nargs);
  static RawObject underComplexReal(Thread* thread, Frame* frame, word nargs);
  static RawObject underDictBucketInsert(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underDictBucketKey(Thread* thread, Frame* frame, word nargs);
  static RawObject underDictBucketUpdate(Thread* thread, Frame* frame,
                                         word nargs);
  static RawObject underDictBucketValue(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underDictCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underDictLookup(Thread* thread, Frame* frame, word nargs);
  static RawObject underDictLookupNext(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underDictUpdateMapping(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underFloatCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underFrozenSetCheck(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underGetMemberByte(Thread* thread, Frame* frame, word nargs);
  static RawObject underGetMemberChar(Thread* thread, Frame* frame, word nargs);
  static RawObject underGetMemberDouble(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underGetMemberFloat(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underGetMemberInt(Thread* thread, Frame* frame, word nargs);
  static RawObject underGetMemberLong(Thread* thread, Frame* frame, word nargs);
  static RawObject underGetMemberPyObject(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underGetMemberShort(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underGetMemberString(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underGetMemberUByte(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underGetMemberUInt(Thread* thread, Frame* frame, word nargs);
  static RawObject underGetMemberULong(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underGetMemberUShort(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underInstanceGetattr(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underInstanceSetattr(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underIntCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underIntFromBytes(Thread* thread, Frame* frame, word nargs);
  static RawObject underIntNewFromByteArray(Thread* thread, Frame* frame,
                                            word nargs);
  static RawObject underIntNewFromBytes(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underIntNewFromInt(Thread* thread, Frame* frame, word nargs);
  static RawObject underIntNewFromStr(Thread* thread, Frame* frame, word nargs);
  static RawObject underListCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underListCheckExact(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underListDelItem(Thread* thread, Frame* frame, word nargs);
  static RawObject underListDelSlice(Thread* thread, Frame* frame, word nargs);
  static RawObject underListSort(Thread* thread, Frame* frame, word nargs);
  static RawObject underObjectTypeHasattr(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underPatch(Thread* thread, Frame* frame, word nargs);
  static RawObject underProperty(Thread* thread, Frame* frame, word nargs);
  static RawObject underPyObjectOffset(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underReprEnter(Thread* thread, Frame* frame, word nargs);
  static RawObject underReprLeave(Thread* thread, Frame* frame, word nargs);
  static RawObject underSetCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underSetMemberDouble(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underSetMemberFloat(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underSetMemberIntegral(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underSetMemberPyObject(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underSliceCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrArrayIadd(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrEscapeNonAscii(Thread* thread, Frame* frame,
                                          word nargs);
  static RawObject underStrFind(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrFromStr(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrReplace(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrRFind(Thread* thread, Frame* frame, word nargs);
  static RawObject underStrSplitlines(Thread* thread, Frame* frame, word nargs);
  static RawObject underTupleCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underTupleCheckExact(Thread* thread, Frame* frame,
                                        word nargs);
  static RawObject underTupleNew(Thread* thread, Frame* frame, word nargs);
  static RawObject underType(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeAbstractMethodsDel(Thread* thread, Frame* frame,
                                               word nargs);
  static RawObject underTypeAbstractMethodsGet(Thread* thread, Frame* frame,
                                               word nargs);
  static RawObject underTypeAbstractMethodsSet(Thread* thread, Frame* frame,
                                               word nargs);
  static RawObject underTypeCheck(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeCheckExact(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underTypeDictKeys(Thread* thread, Frame* frame, word nargs);
  static RawObject underTypeIsSubclass(Thread* thread, Frame* frame,
                                       word nargs);
  static RawObject underUnimplemented(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinType kBuiltinTypes[];
  static const char* const kFrozenData;
  static const SymbolId kIntrinsicIds[];
};

}  // namespace python
