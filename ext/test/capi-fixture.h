#pragma once

#include <clocale>

#include "Python.h"
#include "gtest/gtest.h"

namespace py {
namespace testing {

extern const char* argv0;

void resetPythonEnv();

class ExtensionApi : public ::testing::Test {
 protected:
  void SetUp() override;

  void TearDown() override;
};

}  // namespace testing
}  // namespace py
