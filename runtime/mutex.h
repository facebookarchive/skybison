#pragma once

#include "globals.h"

namespace py {

class Mutex {
 public:
  Mutex();
  ~Mutex();
  void lock();
  bool tryLock();
  void unlock();

 private:
  union Data {
    char data[64];
    word _;  // here to increase alignment.
  };
  Data data_;

  void* mutex();
};

class MutexGuard {
 public:
  MutexGuard(Mutex* mutex);
  ~MutexGuard();
  Mutex* mutex_;
};

inline void* Mutex::mutex() { return &data_.data; }

inline MutexGuard::MutexGuard(Mutex* mutex) : mutex_(mutex) { mutex_->lock(); }

inline MutexGuard::~MutexGuard() { mutex_->unlock(); }

}  // namespace py
