#include "gtest/gtest.h"

#include "assembler-x64.h"
#include "memory-region.h"

namespace python {
namespace x64 {

using namespace testing;

template <size_t N>
::testing::AssertionResult assemblerContainsBytes(Assembler* a,
                                                  const byte (&expected)[N]) {
  if (a->codeSize() != N) {
    return ::testing::AssertionFailure() << "Code size of " << a->codeSize()
                                         << " doesn't match expected " << N;
  }

  byte buf[N];
  a->finalizeInstructions(MemoryRegion(buf, N));
  for (size_t i = 0; i < sizeof(expected); ++i) {
    if (buf[i] != expected[i]) {
      // AssertionFailure()'s streaming doesn't support std::hex.
      std::ostringstream msg;
      msg << "Mismatch at index " << i << ": expected 0x" << std::hex
          << int{expected[i]} << ", got 0x" << int{buf[i]} << std::dec;
      return ::testing::AssertionFailure() << msg.str();
    }
  }

  return ::testing::AssertionSuccess();
}

TEST(AssemblerTest, Empty) {
  byte buf[] = {0x12, 0x34, 0x56, 0x78};
  Assembler as;
  EXPECT_EQ(as.codeSize(), 0);
  as.finalizeInstructions(MemoryRegion(buf, sizeof(buf)));

  // Make sure the buffer wasn't changed.
  EXPECT_EQ(buf[0], 0x12);
  EXPECT_EQ(buf[1], 0x34);
  EXPECT_EQ(buf[2], 0x56);
  EXPECT_EQ(buf[3], 0x78);
}

TEST(AssemblerTest, emit) {
  const byte expected[] = {0x90};
  Assembler as;
  as.nop();
  EXPECT_TRUE(assemblerContainsBytes(&as, expected));
}

TEST(AssemblerTest, Labels) {
  const byte expected[] = {0x90, 0xeb, 0xfd};
  Assembler as;
  Label loop;
  as.bind(&loop);
  as.nop();
  as.jmp(&loop);
  EXPECT_TRUE(assemblerContainsBytes(&as, expected));
}

TEST(AssemblerTest, Align) {
  const byte expected[] = {0xcc, 0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00};
  Assembler as;
  as.int3();
  as.align(8);
  EXPECT_TRUE(assemblerContainsBytes(&as, expected));
}

}  // namespace x64
}  // namespace python
