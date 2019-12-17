#include "mutex.h"

#include "gtest/gtest.h"

namespace py {

TEST(MutexTest, TryLockWithBusyLockReturnsFailure) {
  Mutex mu;
  mu.lock();
  EXPECT_FALSE(mu.tryLock());

  mu.unlock();
}

TEST(MutexTest, ReleaseWithBusyLockAllowsItToBeAcquiredAgain) {
  Mutex mu;
  mu.lock();
  mu.unlock();
  EXPECT_TRUE(mu.tryLock());

  mu.unlock();
}

}  // namespace py
