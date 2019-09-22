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
    {SymbolId::kBufferedReader, LayoutId::kBufferedReader},
    {SymbolId::kBytesIO, LayoutId::kBytesIO},
    {SymbolId::kFileIO, LayoutId::kFileIO},
    {SymbolId::kIncrementalNewlineDecoder,
     LayoutId::kIncrementalNewlineDecoder},
    {SymbolId::kTextIOWrapper, LayoutId::kTextIOWrapper},
    {SymbolId::kUnderBufferedIOBase, LayoutId::kUnderBufferedIOBase},
    {SymbolId::kUnderBufferedIOMixin, LayoutId::kUnderBufferedIOMixin},
    {SymbolId::kUnderIOBase, LayoutId::kUnderIOBase},
    {SymbolId::kUnderRawIOBase, LayoutId::kUnderRawIOBase},
    {SymbolId::kUnderTextIOBase, LayoutId::kUnderTextIOBase},
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

const BuiltinAttribute IncrementalNewlineDecoderBuiltins::kAttributes[] = {
    {SymbolId::kUnderErrors, IncrementalNewlineDecoder::kErrorsOffset},
    {SymbolId::kUnderTranslate, IncrementalNewlineDecoder::kTranslateOffset},
    {SymbolId::kUnderDecoder, IncrementalNewlineDecoder::kDecoderOffset},
    {SymbolId::kUnderSeennl, IncrementalNewlineDecoder::kSeennlOffset},
    {SymbolId::kUnderPendingcr, IncrementalNewlineDecoder::kPendingcrOffset},
    {SymbolId::kSentinelId, 0},
};

void UnderRawIOBaseBuiltins::postInitialize(Runtime*, const Type& new_type) {
  new_type.setBuiltinBase(kSuperType);
}

void UnderBufferedIOBaseBuiltins::postInitialize(Runtime*,
                                                 const Type& new_type) {
  new_type.setBuiltinBase(kSuperType);
}

const BuiltinAttribute UnderBufferedIOMixinBuiltins::kAttributes[] = {
    {SymbolId::kUnderRaw, UnderBufferedIOMixin::kUnderlyingOffset},
    {SymbolId::kSentinelId, 0},
};

const BuiltinAttribute BufferedReaderBuiltins::kAttributes[] = {
    {SymbolId::kUnderRaw, BufferedReader::kUnderlyingOffset},
    {SymbolId::kBufferSize, BufferedReader::kBufferSizeOffset},
    {SymbolId::kUnderReadLock, BufferedReader::kReadLockOffset},
    {SymbolId::kUnderReadBuf, BufferedReader::kReadBufOffset},
    {SymbolId::kUnderReadPos, BufferedReader::kReadPosOffset},
    {SymbolId::kSentinelId, 0},
};

const BuiltinAttribute BytesIOBuiltins::kAttributes[] = {
    {SymbolId::kDunderDict, BytesIO::kDictOffset},
    {SymbolId::kUnderBuffer, BytesIO::kBufferOffset},
    {SymbolId::kUnderPos, BytesIO::kPosOffset},
    {SymbolId::kSentinelId, 0},
};

void BytesIOBuiltins::postInitialize(Runtime*, const Type& new_type) {
  new_type.setBuiltinBase(kSuperType);
}

const BuiltinAttribute FileIOBuiltins::kAttributes[] = {
    {SymbolId::kUnderFd, FileIO::kFdOffset},
    {SymbolId::kName, FileIO::kNameOffset},
    {SymbolId::kUnderCreated, FileIO::kCreatedOffset},
    {SymbolId::kUnderReadable, FileIO::kReadableOffset},
    {SymbolId::kUnderWritable, FileIO::kWritableOffset},
    {SymbolId::kUnderAppending, FileIO::kAppendingOffset},
    {SymbolId::kUnderSeekable, FileIO::kSeekableOffset},
    {SymbolId::kUnderCloseFd, FileIO::kCloseFdOffset},
    {SymbolId::kSentinelId, 0},
};

const BuiltinAttribute TextIOWrapperBuiltins::kAttributes[] = {
    {SymbolId::kUnderB2cratio, TextIOWrapper::kB2cratioOffset},
    {SymbolId::kUnderBuffer, TextIOWrapper::kBufferOffset},
    {SymbolId::kUnderDecodedChars, TextIOWrapper::kDecodedCharsOffset},
    {SymbolId::kUnderDecodedCharsUsed, TextIOWrapper::kDecodedCharsUsedOffset},
    {SymbolId::kUnderDecoder, TextIOWrapper::kDecoderOffset},
    {SymbolId::kUnderEncoder, TextIOWrapper::kEncoderOffset},
    {SymbolId::kUnderEncoding, TextIOWrapper::kEncodingOffset},
    {SymbolId::kUnderErrors, TextIOWrapper::kErrorsOffset},
    {SymbolId::kUnderHasRead1, TextIOWrapper::kHasRead1Offset},
    {SymbolId::kUnderLineBuffering, TextIOWrapper::kLineBufferingOffset},
    {SymbolId::kUnderReadnl, TextIOWrapper::kReadnlOffset},
    {SymbolId::kUnderReadtranslate, TextIOWrapper::kReadtranslateOffset},
    {SymbolId::kUnderReaduniversal, TextIOWrapper::kReaduniversalOffset},
    {SymbolId::kUnderSeekable, TextIOWrapper::kSeekableOffset},
    {SymbolId::kUnderSnapshot, TextIOWrapper::kSnapshotOffset},
    {SymbolId::kUnderTelling, TextIOWrapper::kTellingOffset},
    {SymbolId::kUnderWritenl, TextIOWrapper::kWritenlOffset},
    {SymbolId::kUnderWritetranslate, TextIOWrapper::kWritetranslateOffset},
    {SymbolId::kSentinelId, 0},
};

}  // namespace python
