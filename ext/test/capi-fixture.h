#pragma once

#include "gtest/gtest.h"

#include "cpython-func.h"

namespace python {

class ExtensionApi : public ::testing::Test {
 protected:
  void SetUp() override { Py_Initialize(); }

  void TearDown() override { Py_FinalizeEx(); }
};

}  // namespace python
