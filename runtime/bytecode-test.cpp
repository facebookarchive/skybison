#include "gtest/gtest.h"

#include "bytecode.h"
#include "test-utils.h"

namespace python {

using namespace testing;

using BytecodeTest = RuntimeFixture;

TEST_F(BytecodeTest, NextBytecodeOpReturnsNextBytecodeOpPair) {
  HandleScope scope(thread_);
  byte bytecode_raw[] = {
      NOP,          99, EXTENDED_ARG, 0xca, LOAD_ATTR,    0xfe, LOAD_GLOBAL, 10,
      EXTENDED_ARG, 1,  EXTENDED_ARG, 2,    EXTENDED_ARG, 3,    LOAD_ATTR,   4};
  Bytes original_bytecode(&scope, runtime_.newBytesWithAll(bytecode_raw));
  MutableBytes bytecode(
      &scope, runtime_.mutableBytesFromBytes(thread_, original_bytecode));
  word index = 0;
  BytecodeOp bc = nextBytecodeOp(bytecode, &index);
  EXPECT_EQ(bc.bc, NOP);
  EXPECT_EQ(bc.arg, 99);

  bc = nextBytecodeOp(bytecode, &index);
  EXPECT_EQ(bc.bc, LOAD_ATTR);
  EXPECT_EQ(bc.arg, 0xcafe);

  bc = nextBytecodeOp(bytecode, &index);
  EXPECT_EQ(bc.bc, LOAD_GLOBAL);
  EXPECT_EQ(bc.arg, 10);

  bc = nextBytecodeOp(bytecode, &index);
  EXPECT_EQ(bc.bc, LOAD_ATTR);
  EXPECT_EQ(bc.arg, 0x01020304);
}

}  // namespace python
