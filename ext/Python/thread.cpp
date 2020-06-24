#include <pthread.h>

#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"

#include "globals.h"
#include "mutex.h"
#include "pythread.h"
#include "utils.h"

namespace py {

PY_EXPORT PyObject* PyThread_GetInfo() { UNIMPLEMENTED("PyThread_GetInfo"); }

PY_EXPORT void PyThread_ReInitTLS() { UNIMPLEMENTED("PyThread_ReInitTLS"); }

PY_EXPORT int PyThread_acquire_lock(PyThread_type_lock lock, int waitflag) {
  DCHECK(waitflag == WAIT_LOCK || waitflag == NOWAIT_LOCK,
         "waitflag should either be WAIT_LOCK or NOWAIT_LOCK");
  if (waitflag == WAIT_LOCK) {
    static_cast<Mutex*>(lock)->lock();
    return PY_LOCK_ACQUIRED;
  }
  return static_cast<Mutex*>(lock)->tryLock();
}

PY_EXPORT PyThread_type_lock PyThread_allocate_lock() {
  return static_cast<void*>(new Mutex());
}

PY_EXPORT int PyThread_create_key() { UNIMPLEMENTED("PyThread_create_key"); }

PY_EXPORT void PyThread_delete_key(int) {
  UNIMPLEMENTED("PyThread_delete_key");
}

PY_EXPORT void PyThread_delete_key_value(int) {
  UNIMPLEMENTED("PyThread_delete_key_value");
}

PY_EXPORT void PyThread_exit_thread() { UNIMPLEMENTED("PyThread_exit_thread"); }

PY_EXPORT void PyThread_free_lock(PyThread_type_lock lock) {
  delete static_cast<Mutex*>(lock);
}

PY_EXPORT void* PyThread_get_key_value(int) {
  UNIMPLEMENTED("PyThread_key_value");
}

PY_EXPORT size_t PyThread_get_stacksize() {
  UNIMPLEMENTED("PyThread_get_stacksize");
}

PY_EXPORT unsigned long PyThread_get_thread_ident() {
  return reinterpret_cast<unsigned long>(pthread_self());
}

PY_EXPORT void PyThread_init_thread() { UNIMPLEMENTED("PyThread_init_thread"); }

PY_EXPORT void PyThread_release_lock(PyThread_type_lock lock) {
  static_cast<Mutex*>(lock)->unlock();
}

PY_EXPORT int PyThread_set_key_value(int, void*) {
  UNIMPLEMENTED("PyThread_set_key_value");
}

PY_EXPORT int PyThread_set_stacksize(size_t) {
  UNIMPLEMENTED("PyThread_set_stacksize");
}

PY_EXPORT long PyThread_start_new_thread(void (*)(void*), void*) {
  UNIMPLEMENTED("PyThread_start_new_thread");
}

}  // namespace py
