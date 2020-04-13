#pragma once

#include <clocale>

#include "Python.h"
#include "gtest/gtest.h"

namespace py {

class ExtensionApi : public ::testing::Test {
 protected:
  void SetUp() override {
    saved_locale_ = ::strdup(std::setlocale(LC_CTYPE, nullptr));
    std::setlocale(LC_CTYPE, "en_US.UTF-8");
    Py_NoSiteFlag = 1;
    Py_Initialize();
  }

  void TearDown() override {
    Py_FinalizeEx();
    std::setlocale(LC_CTYPE, saved_locale_);
    std::free(saved_locale_);
  }

 private:
  char* saved_locale_;
};

}  // namespace py
