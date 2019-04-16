#include "gtest/gtest.h"

#include "mutex.h"

namespace python {

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

}  // namespace python
