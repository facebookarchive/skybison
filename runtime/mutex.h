#pragma once

#include <pthread.h>

namespace py {

class Mutex {
 public:
  Mutex();
  ~Mutex();
  void lock();
  bool tryLock();
  void unlock();

 private:
  void* lock_ = nullptr;
};

}  // namespace py
