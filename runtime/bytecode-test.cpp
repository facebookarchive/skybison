#include "gtest/gtest.h"

#include <cstdint>

#include "bytecode.h"
#include "globals.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {
using namespace testing;

TEST(ByteCode, Name) {
  EXPECT_STREQ(bytecode::name(LOAD_FAST), "LOAD_FAST");
  EXPECT_STREQ(bytecode::name(STORE_NAME), "STORE_NAME");
  EXPECT_STREQ(
      bytecode::name(static_cast<Bytecode>(500)), "invalid byte code 500");
}
} // namespace python
