#include "marshal-module.h"

#include "bytes-builtins.h"
#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "marshal.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

const BuiltinFunction MarshalModule::kBuiltinFunctions[] = {
    {SymbolId::kLoads, loads},
    {SymbolId::kSentinelId, nullptr},
};

const char* const MarshalModule::kFrozenData = kMarshalModuleData;

RawObject MarshalModule::loads(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object bytes_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*bytes_obj)) {
    // TODO(T38902048): Load from buffer protocol objects
    UNIMPLEMENTED("marshal.loads with non-bytes objects");
  }
  Bytes bytes(&scope, bytesUnderlying(*bytes_obj));
  word length = bytes.length();
  // TODO(T38902583): Use updated Marshal reader to read from Bytes object
  // directly
  std::unique_ptr<byte[]> buffer(new byte[length]);
  bytes.copyTo(buffer.get(), length);
  Marshal::Reader reader(&scope, runtime, View<byte>(buffer.get(), length));
  return reader.readObject();
}

}  // namespace py
