#include "Python.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {
using PyhashExtensionApiTest = ExtensionApi;

TEST_F(PyhashExtensionApiTest, PyHashPointerReturnsHash) {
  // We currently use the same hash algorithm as cpython so we can test for a
  // specific result. This is optional and we can change the test when we want
  // to use a different hashing algorithm.
  if (sizeof(void*) == 8) {
    Py_hash_t result =
        _Py_HashPointer(reinterpret_cast<void*>(0xcafebabebadf00d));
    EXPECT_EQ(result, static_cast<Py_hash_t>(0xd0cafebabebadf00));
  }
}

TEST_F(PyhashExtensionApiTest, _Py_HashDoubleReturnsHash) {
  PyRun_SimpleString(R"(
hash_value = hash(-42.42)
)");
  PyObjectPtr hash_value(moduleGet("__main__", "hash_value"));
  Py_hash_t result = _Py_HashDouble(-42.42);
  EXPECT_TRUE(isLongEqualsLong(hash_value, result));
}

TEST_F(PyhashExtensionApiTest, _Py_HashBytesWithSmallBytesReturnsHash) {
  PyRun_SimpleString(R"(
hash_value = hash(b"jo")
)");
  PyObjectPtr hash_value(moduleGet("__main__", "hash_value"));
  Py_hash_t result = _Py_HashBytes("jo", 2);
  EXPECT_TRUE(isLongEqualsLong(hash_value, result));
}

TEST_F(PyhashExtensionApiTest, _Py_HashBytesWithLargeBytesReturnsHash) {
  PyRun_SimpleString(R"(
hash_value = hash(b"Monty Python")
)");
  PyObjectPtr hash_value(moduleGet("__main__", "hash_value"));
  Py_hash_t result = _Py_HashBytes("Monty Python", 12);
  EXPECT_TRUE(isLongEqualsLong(hash_value, result));
}

}  // namespace testing
}  // namespace py
