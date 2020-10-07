#include "builtins.h"
#include "bytes-builtins.h"
#include "frame.h"
#include "globals.h"
#include "marshal.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

RawObject FUNC(marshal, loads)(Thread* thread, Arguments args) {
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
  Marshal::Reader reader(&scope, thread, View<byte>(buffer.get(), length));
  return reader.readObject();
}

}  // namespace py
