#include "gtest/gtest.h"

#include <cmath>

#include "test-utils.h"
#include "under-valgrind-module.h"

namespace python {

using namespace testing;

using UnderValgrindModuleTest = RuntimeFixture;

TEST_F(UnderValgrindModuleTest, UnderCallgrindDumpStatsWithNoneDoesNothing) {
  HandleScope scope(thread_);
  Object none(&scope, NoneType::object());
  runBuiltin(UnderValgrindModule::callgrindDumpStats, none);
}

TEST_F(UnderValgrindModuleTest, UnderCallgrindDumpStatsWithStringDoesNothing) {
  HandleScope scope(thread_);
  Object string(&scope, runtime_.newStrFromCStr("service_load"));
  runBuiltin(UnderValgrindModule::callgrindDumpStats, string);
}

TEST_F(UnderValgrindModuleTest, UnderCallgrindStartInstrumentationDoesNothing) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import _valgrind
_valgrind.callgrind_start_instrumentation()
)")
                   .isError());
}

TEST_F(UnderValgrindModuleTest, UnderCallgrindStopInstrumentationDoesNothing) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import _valgrind
_valgrind.callgrind_stop_instrumentation()
)")
                   .isError());
}

TEST_F(UnderValgrindModuleTest, UnderCallgrindZeroStatsDoesNothing) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import _valgrind
_valgrind.callgrind_zero_stats()
)")
                   .isError());
}

}  // namespace python
