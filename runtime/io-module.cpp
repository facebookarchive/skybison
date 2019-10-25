#include "io-module.h"

#include "bytes-builtins.h"
#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"

namespace py {

const BuiltinType UnderIoModule::kBuiltinTypes[] = {
    {SymbolId::kBufferedRandom, LayoutId::kBufferedRandom},
    {SymbolId::kBufferedReader, LayoutId::kBufferedReader},
    {SymbolId::kBufferedWriter, LayoutId::kBufferedWriter},
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

const BuiltinAttribute BufferedRandomBuiltins::kAttributes[] = {
    {SymbolId::kUnderRaw, BufferedRandom::kUnderlyingOffset},
    {SymbolId::kUnderReadBuf, BufferedRandom::kReadBufOffset},
    {SymbolId::kUnderReadLock, BufferedRandom::kReadLockOffset},
    {SymbolId::kUnderReadPos, BufferedRandom::kReadPosOffset},
    {SymbolId::kUnderWriteBuf, BufferedRandom::kWriteBufOffset},
    {SymbolId::kUnderWriteLock, BufferedRandom::kWriteLockOffset},
    {SymbolId::kBufferSize, BufferedRandom::kBufferSizeOffset},
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

const BuiltinAttribute BufferedWriterBuiltins::kAttributes[] = {
    {SymbolId::kUnderRaw, BufferedWriter::kUnderlyingOffset},
    {SymbolId::kUnderWriteBuf, BufferedWriter::kWriteBufOffset},
    {SymbolId::kUnderWriteLock, BufferedWriter::kWriteLockOffset},
    {SymbolId::kBufferSize, BufferedWriter::kBufferSizeOffset},
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
    {SymbolId::kMode, TextIOWrapper::kModeOffset},  // TODO(T54575279): remove
    {SymbolId::kSentinelId, 0},
};

}  // namespace py
