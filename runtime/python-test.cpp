#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

extern std::string FLAGS_benchmark_filter;

namespace {

using testing::EmptyTestEventListener;
using testing::TestCase;
using testing::TestInfo;
using testing::TestPartResult;
using testing::UnitTest;

// TestEventListener that ignores any events related to successful tests.
class FailurePrinter : public EmptyTestEventListener {
 public:
  FailurePrinter(TestEventListener* fallback);
  ~FailurePrinter() override;

  void OnTestIterationStart(const UnitTest& test, int iteration) override;
  void OnEnvironmentsSetUpStart(const UnitTest& test) override;
  void OnTestPartResult(const TestPartResult& part) override;
  void OnTestEnd(const TestInfo& test) override;
  void OnTestCaseEnd(const TestCase& test) override;
  void OnEnvironmentsTearDownStart(const UnitTest& test) override;
  void OnTestIterationEnd(const UnitTest& test, int iteration) override;

 private:
  TestEventListener* fallback_;
};

FailurePrinter::FailurePrinter(TestEventListener* fallback)
    : fallback_{fallback} {}

FailurePrinter::~FailurePrinter() { delete fallback_; }

void FailurePrinter::OnTestIterationStart(const UnitTest& test, int iteration) {
  fallback_->OnTestIterationStart(test, iteration);
}

void FailurePrinter::OnEnvironmentsSetUpStart(const UnitTest& test) {
  fallback_->OnEnvironmentsSetUpStart(test);
}

void FailurePrinter::OnTestPartResult(const TestPartResult& part) {
  if (part.failed()) {
    fallback_->OnTestPartResult(part);
  }
}

void FailurePrinter::OnTestEnd(const TestInfo& test) {
  if (test.result()->Failed()) {
    fallback_->OnTestEnd(test);
  }
}

void FailurePrinter::OnTestCaseEnd(const TestCase& test) {
  if (test.Failed()) {
    fallback_->OnTestCaseEnd(test);
  }
}

void FailurePrinter::OnEnvironmentsTearDownStart(const UnitTest& test) {
  fallback_->OnEnvironmentsTearDownStart(test);
}

void FailurePrinter::OnTestIterationEnd(const UnitTest& test, int iteration) {
  fallback_->OnTestIterationEnd(test, iteration);
}
}  // namespace

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);

  bool hide_success = false;
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "--hide_success") == 0) {
      hide_success = true;
      break;
    }
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      std::cout << "\nThis binary shows successful test names by default. To "
                   "override this, use:\n  --hide_success\n\n";
      break;
    }
  }

  if (hide_success) {
    auto& listeners = testing::UnitTest::GetInstance()->listeners();
    auto default_printer =
        listeners.Release(listeners.default_result_printer());
    listeners.Append(new FailurePrinter(default_printer));
  }

  // Default to running no benchmarks.
  FLAGS_benchmark_filter.clear();
  benchmark::Initialize(&argc, argv);
  if (FLAGS_benchmark_filter.empty()) {
    return RUN_ALL_TESTS();
  }
  return (benchmark::RunSpecifiedBenchmarks() == 0);
}
