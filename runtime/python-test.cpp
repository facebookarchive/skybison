#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);

  // Default to running no benchmarks.
  extern std::string FLAGS_benchmark_filter;
  FLAGS_benchmark_filter = ".^";
  benchmark::Initialize(&argc, argv);
  if (benchmark::RunSpecifiedBenchmarks() != 0) {
    return 0;
  }
  return RUN_ALL_TESTS();
}
