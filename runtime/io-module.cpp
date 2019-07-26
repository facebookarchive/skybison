#include "io-module.h"

#include "bytes-builtins.h"
#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"

namespace python {

const BuiltinMethod UnderIoModule::kBuiltinMethods[] = {
    {SymbolId::kUnderReadBytes, underReadBytes},
    {SymbolId::kUnderReadFile, underReadFile},
    {SymbolId::kSentinelId, nullptr},
};

const BuiltinType UnderIoModule::kBuiltinTypes[] = {
    {SymbolId::kBytesIO, LayoutId::kBytesIO},
    {SymbolId::kUnderBufferedIOBase, LayoutId::kUnderBufferedIOBase},
    {SymbolId::kUnderIOBase, LayoutId::kUnderIOBase},
    {SymbolId::kUnderRawIOBase, LayoutId::kUnderRawIOBase},
    {SymbolId::kSentinelId, LayoutId::kSentinelId},
};

const char* const UnderIoModule::kFrozenData = kUnderIoModuleData;

RawObject UnderIoModule::underReadFile(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Str path(&scope, args.get(0));
  unique_c_ptr<char> c_path(path.toCStr());
  word length;
  std::unique_ptr<const char[]> c_filedata(OS::readFile(c_path.get(), &length));
  View<byte> data(reinterpret_cast<const byte*>(c_filedata.get()), length);
  Bytes bytes(&scope, thread->runtime()->newBytesWithAll(data));
  return *bytes;
}

RawObject UnderIoModule::underReadBytes(Thread* thread, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object bytes_obj(&scope, args.get(0));
  Bytes bytes(&scope, bytesUnderlying(thread, bytes_obj));
  word length = bytes.length();
  std::unique_ptr<char[]> data(new char[length + 1]);
  for (word idx = 0; idx < length; idx++) data[idx] = bytes.byteAt(idx);
  data[length] = '\0';
  Str result(&scope, thread->runtime()->newStrFromCStr(data.get()));
  return *result;
}

const BuiltinAttribute UnderIOBaseBuiltins::kAttributes[] = {
    {SymbolId::kUnderClosed, UnderIOBase::kClosedOffset},
    {SymbolId::kSentinelId, 0},
};

void UnderRawIOBaseBuiltins::postInitialize(Runtime*, const Type& new_type) {
  new_type.setBuiltinBase(kSuperType);
}

void UnderBufferedIOBaseBuiltins::postInitialize(Runtime*,
                                                 const Type& new_type) {
  new_type.setBuiltinBase(kSuperType);
}

const BuiltinAttribute BytesIOBuiltins::kAttributes[] = {
    {SymbolId::kDunderDict, BytesIO::kDictOffset},
    {SymbolId::kUnderBuffer, BytesIO::kBufferOffset},
    {SymbolId::kUnderPos, BytesIO::kPosOffset},
    {SymbolId::kSentinelId, 0},
};

void BytesIOBuiltins::postInitialize(Runtime*, const Type& new_type) {
  new_type.setBuiltinBase(kSuperType);
}

}  // namespace python
