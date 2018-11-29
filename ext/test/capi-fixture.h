#pragma once

#include "Python.h"
#include "gtest/gtest.h"

namespace python {

class ExtensionApi : public ::testing::Test {
 protected:
  void SetUp() override { Py_Initialize(); }

  void TearDown() override { Py_FinalizeEx(); }
};

}  // namespace python
