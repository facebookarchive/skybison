#include "gtest/gtest.h"

#include "Python.h"
#include "pythread.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using ThreadExtensionApiTest = ExtensionApi;

TEST_F(ThreadExtensionApiTest, GetThreadIdentReturnsSameValue) {
  EXPECT_EQ(PyThread_get_thread_ident(), PyThread_get_thread_ident());
}

TEST_F(ThreadExtensionApiTest, TryLockWithBusyLockReturnsFailure) {
  PyThread_type_lock lock = PyThread_allocate_lock();
  ASSERT_NE(lock, nullptr);
  ASSERT_EQ(PyThread_acquire_lock(lock, 0), 1);
  EXPECT_EQ(PyThread_acquire_lock(lock, 0), 0);

  PyThread_release_lock(lock);
  PyThread_free_lock(lock);
}

TEST_F(ThreadExtensionApiTest, ReleaseWithBusyLockAllowsItToBeAcquiredAgain) {
  PyThread_type_lock lock = PyThread_allocate_lock();
  ASSERT_NE(lock, nullptr);
  ASSERT_EQ(PyThread_acquire_lock(lock, 0), 1);
  PyThread_release_lock(lock);
  EXPECT_EQ(PyThread_acquire_lock(lock, 0), 1);

  PyThread_release_lock(lock);
  PyThread_free_lock(lock);
}

}  // namespace python
