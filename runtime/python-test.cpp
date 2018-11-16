#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

extern std::string FLAGS_benchmark_filter;

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);

  // Default to running no benchmarks.
  FLAGS_benchmark_filter.clear();
  benchmark::Initialize(&argc, argv);
  if (FLAGS_benchmark_filter.empty()) {
    return RUN_ALL_TESTS();
  }
  return (benchmark::RunSpecifiedBenchmarks() == 0);
}
