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

}  // namespace python
