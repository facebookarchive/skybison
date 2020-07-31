#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

extern std::string FLAGS_benchmark_filter;

namespace py {
namespace testing {
extern const char* argv0;
}  // namespace testing
}  // namespace py

// CPython leaks when Py_Initialize is called multiple times:
// https://bugs.python.org/issue1635741
extern "C" const char* __asan_default_options() { return "detect_leaks=0"; }

int main(int argc, char* argv[]) {
  // Run benchmarks instead of tests if there was a --benchmark_filter argument.
  FLAGS_benchmark_filter.clear();
  benchmark::Initialize(&argc, argv);
  if (!FLAGS_benchmark_filter.empty()) {
    return (benchmark::RunSpecifiedBenchmarks() == 0);
  }

  py::testing::argv0 = argv[0];
  testing::InitGoogleTest(&argc, argv);
  // Skip all tests whose name ends in "Pyro".
  if (testing::GTEST_FLAG(filter).find('-') == std::string::npos) {
    testing::GTEST_FLAG(filter) += "-*Pyro";
  } else {
    testing::GTEST_FLAG(filter) += ":*Pyro";
  }
  return RUN_ALL_TESTS();
}
