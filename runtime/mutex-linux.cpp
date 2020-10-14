#include "mutex.h"

#include <pthread.h>

#include <cerrno>

#include "utils.h"

namespace py {

Mutex::Mutex() {
  static_assert(sizeof(Mutex::Data) >= sizeof(pthread_mutex_t),
                "Mutex::Data too small for pthread_mutex_t");
  static_assert(alignof(Mutex::Data) >= alignof(pthread_mutex_t),
                "Mutex::Data alignment too small for pthread_mutex_t");

  pthread_mutex_t* lock = reinterpret_cast<pthread_mutex_t*>(mutex());
  DCHECK(lock != nullptr, "lock should not be null");
  int result = pthread_mutex_init(lock, nullptr);
  CHECK(result == 0, "lock creation failed");
}

Mutex::~Mutex() {
  CHECK(tryLock(), "cannot destroy locked lock");
  unlock();
  pthread_mutex_t* lock = reinterpret_cast<pthread_mutex_t*>(mutex());
  int result = pthread_mutex_destroy(lock);
  CHECK(result == 0, "could not destroy lock");
}

void Mutex::lock() {
  pthread_mutex_t* lock = reinterpret_cast<pthread_mutex_t*>(mutex());
  int result = pthread_mutex_lock(lock);
  DCHECK(result == 0, "failed to lock mutex with error code of %d", result);
}

bool Mutex::tryLock() {
  pthread_mutex_t* lock = reinterpret_cast<pthread_mutex_t*>(mutex());
  int result = pthread_mutex_trylock(lock);
  if (result == EBUSY) {
    return false;
  }
  DCHECK(result == 0, "failed to lock mutex with error code of %d", result);
  return true;
}

void Mutex::unlock() {
  pthread_mutex_t* lock = reinterpret_cast<pthread_mutex_t*>(mutex());
  int result = pthread_mutex_unlock(lock);
  DCHECK(result == 0, "failed to unlock mutex with error code of %d", result);
}

}  // namespace py
