#include <pthread.h>

#include "cpython-func.h"
#include "cpython-types.h"
#include "globals.h"
#include "mutex.h"
#include "pythread.h"
#include "utils.h"

namespace py {

PY_EXPORT PyThread_type_lock PyThread_allocate_lock(void) {
  return static_cast<void*>(new Mutex());
}

PY_EXPORT void PyThread_free_lock(PyThread_type_lock lock) {
  delete static_cast<Mutex*>(lock);
}

PY_EXPORT int PyThread_acquire_lock(PyThread_type_lock lock, int waitflag) {
  DCHECK(waitflag == WAIT_LOCK || waitflag == NOWAIT_LOCK,
         "waitflag should either be WAIT_LOCK or NOWAIT_LOCK");
  if (waitflag == WAIT_LOCK) {
    static_cast<Mutex*>(lock)->lock();
    return PY_LOCK_ACQUIRED;
  }
  return static_cast<Mutex*>(lock)->tryLock();
}

PY_EXPORT void PyThread_release_lock(PyThread_type_lock lock) {
  static_cast<Mutex*>(lock)->unlock();
}

PY_EXPORT long PyThread_get_thread_ident() {
  return bit_cast<long>(pthread_self());
}

}  // namespace py
