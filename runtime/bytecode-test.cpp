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

TEST_F(BytecodeTest, OpargFromObject) {
  EXPECT_EQ(NoneType::object(),
            objectFromOparg(opargFromObject(NoneType::object())));
  EXPECT_EQ(Bool::trueObj(), objectFromOparg(opargFromObject(Bool::trueObj())));
  EXPECT_EQ(Bool::falseObj(),
            objectFromOparg(opargFromObject(Bool::falseObj())));
  EXPECT_EQ(SmallInt::fromWord(-1),
            objectFromOparg(opargFromObject(SmallInt::fromWord(-1))));
  EXPECT_EQ(SmallInt::fromWord(-64),
            objectFromOparg(opargFromObject(SmallInt::fromWord(-64))));
  EXPECT_EQ(SmallInt::fromWord(0),
            objectFromOparg(opargFromObject(SmallInt::fromWord(0))));
  EXPECT_EQ(SmallInt::fromWord(63),
            objectFromOparg(opargFromObject(SmallInt::fromWord(63))));
  EXPECT_EQ(SmallStr::fromCStr(""),
            objectFromOparg(opargFromObject(SmallStr::fromCStr(""))));
  // Not immediate since it doesn't fit in byte.
  EXPECT_NE(SmallInt::fromWord(64),
            objectFromOparg(opargFromObject(SmallInt::fromWord(64))));
}

}  // namespace python
