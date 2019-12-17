#include "mutex.h"

#include <pthread.h>

#include <cerrno>

#include "utils.h"

namespace py {

Mutex::Mutex() {
  pthread_mutex_t* lock = new pthread_mutex_t;
  DCHECK(lock != nullptr, "lock should not be null");
  int result = pthread_mutex_init(lock, nullptr);
  CHECK(result == 0, "lock creation failed");
  lock_ = lock;
}

Mutex::~Mutex() {
  pthread_mutex_t* lock = static_cast<pthread_mutex_t*>(lock_);
  int result = pthread_mutex_destroy(lock);
  DCHECK(result != EBUSY, "cannot destroy locked lock");
  CHECK(result == 0, "could not destroy lock");
  delete lock;
  lock_ = nullptr;
}

void Mutex::lock() {
  int result = pthread_mutex_lock(static_cast<pthread_mutex_t*>(lock_));
  DCHECK(result == 0, "failed to lock mutex with error code of %d", result);
}

bool Mutex::tryLock() {
  int result = pthread_mutex_trylock(static_cast<pthread_mutex_t*>(lock_));
  if (result == EBUSY) {
    return false;
  }
  DCHECK(result == 0, "failed to lock mutex with error code of %d", result);
  return true;
}

void Mutex::unlock() {
  int result = pthread_mutex_unlock(static_cast<pthread_mutex_t*>(lock_));
  DCHECK(result == 0, "failed to unlock mutex with error code of %d", result);
}

}  // namespace py
