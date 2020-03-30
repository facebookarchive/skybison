#include "mmap-module.h"

#include <sys/mman.h>
#include <sys/stat.h>

#include "builtins.h"
#include "file.h"
#include "frozen-modules.h"
#include "handles.h"
#include "module-builtins.h"
#include "modules.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "symbols.h"
#include "thread.h"

namespace py {

const BuiltinType MmapModule::kBuiltinTypes[] = {
    {ID(mmap), LayoutId::kMmap},
    {SymbolId::kSentinelId, LayoutId::kSentinelId},
};

void MmapModule::initialize(Thread* thread, const Module& module) {
  HandleScope scope(thread);
  Object prot_exec(&scope, SmallInt::fromWord(static_cast<word>(PROT_EXEC)));
  moduleAtPutById(thread, module, ID(PROT_EXEC), prot_exec);

  Object prot_read(&scope, SmallInt::fromWord(static_cast<word>(PROT_READ)));
  moduleAtPutById(thread, module, ID(PROT_READ), prot_read);

  Object prot_write(&scope, SmallInt::fromWord(static_cast<word>(PROT_WRITE)));
  moduleAtPutById(thread, module, ID(PROT_WRITE), prot_write);

  Object map_shared(&scope, SmallInt::fromWord(static_cast<word>(MAP_SHARED)));
  moduleAtPutById(thread, module, ID(MAP_SHARED), map_shared);

  moduleAddBuiltinTypes(thread, module, kBuiltinTypes);
  executeFrozenModule(thread, &kMmapModuleData, module);
}

RawObject FUNC(mmap, _mmap_new)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();

  word fd = intUnderlying(args.get(1)).asWord();
  word length = intUnderlying(args.get(2)).asWord();
  word flags = intUnderlying(args.get(3)).asWord();
  word prot = intUnderlying(args.get(4)).asWord();
  word offset = intUnderlying(args.get(5)).asWord();

  Type type(&scope, args.get(0));
  void* address = OS::mmap(nullptr, length, prot, flags, fd, offset);
  if (address == MAP_FAILED) {
    return thread->raiseWithFmt(LayoutId::kOSError, "Call to mmap failed");
  }

  Layout layout(&scope, type.instanceLayout());
  Mmap result(&scope, runtime->newInstance(layout));
  result.setAccess(0);
  if ((prot & PROT_READ) != 0) {
    result.setReadable();
  }
  if ((prot & PROT_WRITE) != 0) {
    result.setWritable();
  }
  if ((flags == MAP_PRIVATE) != 0) {
    result.setCopyOnWrite();
  }
  result.setData(runtime->newPointer(address, length));
  result.setFd(runtime->newInt(fd));
  return *result;
}

RawObject METH(mmap, close)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfMmap(*self)) {
    return thread->raiseRequiresType(self, ID(mmap));
  }
  Mmap mmap_obj(&scope, *self);
  // TODO(T64468928): Take into account exporters
  word fd = Int::cast(mmap_obj.fd()).asWord();
  if (fd >= 0) {
    int close_result = File::close(fd);
    if (close_result < 0) return thread->raiseOSErrorFromErrno(-close_result);
  }
  mmap_obj.setFd(SmallInt::fromWord(-1));
  Pointer pointer(&scope, mmap_obj.data());
  void* address = pointer.cptr();
  if (address != nullptr) {
    OS::freeMemory(static_cast<byte*>(address), pointer.length());
    mmap_obj.setData(NoneType::object());
  }
  return NoneType::object();
}

const BuiltinAttribute MmapBuiltins::kAttributes[] = {
    {ID(_mmap__access), Mmap::kAccessOffset, AttributeFlags::kHidden},
    {ID(_mmap__data), Mmap::kDataOffset, AttributeFlags::kHidden},
    {ID(_mmap__fd), Mmap::kFdOffset, AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

}  // namespace py
