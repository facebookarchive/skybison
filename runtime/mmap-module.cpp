#include "mmap-module.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <cerrno>

#include "builtins.h"
#include "file.h"
#include "handles.h"
#include "module-builtins.h"
#include "modules.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "symbols.h"
#include "thread.h"
#include "type-builtins.h"

namespace py {

void FUNC(mmap, __init_module__)(Thread* thread, const Module& module,
                                 View<byte> bytecode) {
  HandleScope scope(thread);
  Object page_size(&scope, SmallInt::fromWord(OS::pageSize()));
  moduleAtPutById(thread, module, ID(PAGESIZE), page_size);

  Object prot_exec(&scope, SmallInt::fromWord(static_cast<word>(PROT_EXEC)));
  moduleAtPutById(thread, module, ID(PROT_EXEC), prot_exec);

  Object prot_read(&scope, SmallInt::fromWord(static_cast<word>(PROT_READ)));
  moduleAtPutById(thread, module, ID(PROT_READ), prot_read);

  Object prot_write(&scope, SmallInt::fromWord(static_cast<word>(PROT_WRITE)));
  moduleAtPutById(thread, module, ID(PROT_WRITE), prot_write);

  Object map_shared(&scope, SmallInt::fromWord(static_cast<word>(MAP_SHARED)));
  moduleAtPutById(thread, module, ID(MAP_SHARED), map_shared);

  Object map_private(&scope,
                     SmallInt::fromWord(static_cast<word>(MAP_PRIVATE)));
  moduleAtPutById(thread, module, ID(MAP_PRIVATE), map_private);

  executeFrozenModule(thread, module, bytecode);
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

  if (fd != -1) {
    struct stat sbuf;
    if (::fstat(fd, &sbuf) == 0 && S_ISREG(sbuf.st_mode)) {
      if (length == 0) {
        if (sbuf.st_size == 0) {
          return thread->raiseWithFmt(LayoutId::kValueError,
                                      "cannot mmap an empty file");
        }
        if (offset >= sbuf.st_size) {
          return thread->raiseWithFmt(LayoutId::kValueError,
                                      "mmap offset is greater than file size");
        }
        length = sbuf.st_size - offset;
      } else if (offset > sbuf.st_size || sbuf.st_size - offset < length) {
        return thread->raiseWithFmt(LayoutId::kValueError,
                                    "mmap length is greater than file size");
      }
    }
    fd = ::fcntl(fd, F_DUPFD_CLOEXEC, 0);
    if (fd < 0) {
      return thread->raiseOSErrorFromErrno(errno);
    }
  } else {
    flags |= MAP_ANONYMOUS;
  }

  void* address = ::mmap(nullptr, length, prot, flags, fd, offset);
  if (address == MAP_FAILED) {
    return thread->raiseOSErrorFromErrno(errno);
  }

  Type type(&scope, args.get(0));
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

static const BuiltinAttribute kMmapAttributes[] = {
    {ID(_mmap__access), RawMmap::kAccessOffset, AttributeFlags::kHidden},
    {ID(_mmap__data), RawMmap::kDataOffset, AttributeFlags::kHidden},
    {ID(_mmap__fd), RawMmap::kFdOffset, AttributeFlags::kHidden},
};

void initializeMmapType(Thread* thread) {
  addBuiltinType(thread, ID(mmap), LayoutId::kMmap,
                 /*superclass_id=*/LayoutId::kObject, kMmapAttributes,
                 /*basetype=*/true);
}

}  // namespace py
