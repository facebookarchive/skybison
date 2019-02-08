#include "marshal-module.h"

#include "frame.h"
#include "globals.h"
#include "marshal.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

RawObject MarshalModule::loads(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("marshal.loads takes 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object bytes_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!bytes_obj.isBytes()) {
    // TODO(T38902048): Load from buffer protocol objects
    UNIMPLEMENTED("marshal.loads with non-bytes objects");
  }
  Bytes bytes(&scope, *bytes_obj);
  word length = bytes.length();
  // TODO(T38902583): Use updated Marshal reader to read from Bytes object
  // directly
  std::unique_ptr<char[]> buffer(new char[length]);
  for (word i = 0; i < length; i++) {
    buffer[i] = bytes.byteAt(i);
  }
  Marshal::Reader reader(&scope, runtime, buffer.get());
  return reader.readObject();
}

}  // namespace python
