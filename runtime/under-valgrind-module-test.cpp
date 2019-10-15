#include "gtest/gtest.h"

#include <cmath>

#include "test-utils.h"
#include "under-valgrind-module.h"

namespace python {

using namespace testing;

using UnderValgrindModuleTest = RuntimeFixture;

TEST_F(UnderValgrindModuleTest, UnderCallgrindDumpStatsAtDoesNothing) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import _valgrind
_valgrind.callgrind_dump_stats_at()
_valgrind.callgrind_dump_stats_at("service_load")
_valgrind.callgrind_dump_stats_at("a" * 1024)
)")
                   .isError());
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
