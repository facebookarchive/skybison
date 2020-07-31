#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

extern std::string FLAGS_benchmark_filter;

namespace py {
namespace testing {
extern const char* argv0;
}  // namespace testing
}  // namespace py

int main(int argc, char* argv[]) {
  // Run benchmarks instead of tests if there was a --benchmark_filter argument.
  FLAGS_benchmark_filter.clear();
  benchmark::Initialize(&argc, argv);
  if (!FLAGS_benchmark_filter.empty()) {
    return (benchmark::RunSpecifiedBenchmarks() == 0);
  }

  py::testing::argv0 = argv[0];
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
