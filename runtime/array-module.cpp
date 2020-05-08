#include "array-module.h"

#include "builtins.h"
#include "frozen-modules.h"
#include "handles.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "symbols.h"
#include "thread.h"
#include "type-builtins.h"

namespace py {

const BuiltinType ArrayModule::kBuiltinTypes[] = {
    {ID(array), LayoutId::kArray},
    {SymbolId::kSentinelId, LayoutId::kSentinelId},
};

void ArrayModule::initialize(Thread* thread, const Module& module) {
  moduleAddBuiltinTypes(thread, module, kBuiltinTypes);
  executeFrozenModule(thread, &kArrayModuleData, module);
}

RawObject FUNC(array, _array_check)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfArray(args.get(0)));
}

static word itemSize(byte typecode) {
  switch (typecode) {
    case 'b':
    case 'B':
      return kByteSize;
    case 'u':
      return kWcharSize;
    case 'h':
    case 'H':
      return kShortSize;
    case 'i':
    case 'I':
      return kIntSize;
    case 'l':
    case 'L':
      return kLongSize;
    case 'q':
    case 'Q':
      return kLongLongSize;
    case 'f':
      return kFloatSize;
    case 'd':
      return kDoubleSize;
    default:
      return -1;
  }
}

RawObject FUNC(array, _array_new)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Str typecode_str(&scope, strUnderlying(args.get(1)));
  DCHECK(typecode_str.length() == 1, "typecode must be a single-char str");
  byte typecode = typecode_str.byteAt(0);
  word item_size = itemSize(typecode);
  if (item_size == -1) {
    return thread->raiseWithFmt(
        LayoutId::kValueError,
        "bad typecode (must be b, B, u, h, H, i, I, l, L, q, Q, f or d)");
  }
  word len = SmallInt::cast(args.get(2)).value() * item_size;
  Runtime* runtime = thread->runtime();

  Type array_type(&scope, args.get(0));
  Layout layout(&scope, array_type.instanceLayout());
  Array result(&scope, runtime->newInstance(layout));
  result.setTypecode(*typecode_str);
  result.setLength(len);
  result.setBuffer(runtime->mutableBytesWith(len, 0));
  return *result;
}

static const BuiltinAttribute kArrayAttributes[] = {
    {ID(_array__buffer), RawArray::kBufferOffset, AttributeFlags::kHidden},
    {ID(_array__length), RawArray::kLengthOffset, AttributeFlags::kHidden},
    {ID(typecode), RawArray::kTypecodeOffset, AttributeFlags::kReadOnly},
};

void initializeArrayType(Thread* thread) {
  addBuiltinType(thread, ID(array), LayoutId::kArray,
                 /*superclass_id=*/LayoutId::kObject, kArrayAttributes);
}

}  // namespace py
