#include <pthread.h>

#include "mutex.h"
#include "utils.h"

namespace python {

Mutex::Mutex() {
  pthread_mutex_t* lock = new pthread_mutex_t;
  DCHECK(lock != nullptr, "lock should not be null");
  int result = pthread_mutex_init(lock, nullptr);
  CHECK(result == 0, "lock creation failed");
  lock_ = lock;
}

Mutex::~Mutex() {
  CHECK(tryLock(), "cannot destroy locked lock");
  unlock();
  pthread_mutex_t* lock = static_cast<pthread_mutex_t*>(lock_);
  int result = pthread_mutex_destroy(lock);
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

}  // namespace python
