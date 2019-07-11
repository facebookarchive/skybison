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

TEST(AssemblerTest, Cmpb) {
  // 3c 12   cmpb    $18, %al
  // 80 fb 34        cmpb    $52, %bl
  // 40 80 ff 56     cmpb    $86, %dil
  // 41 80 fa 78     cmpb    $120, %r10b
  const byte expected[] = {0x3c, 0x12, 0x80, 0xfb, 0x34, 0x40, 0x80,
                           0xff, 0x56, 0x41, 0x80, 0xfa, 0x78};
  Assembler as;
  as.cmpb(RAX, Immediate(0x12));
  as.cmpb(RBX, Immediate(0x34));
  as.cmpb(RDI, Immediate(0x56));
  as.cmpb(R10, Immediate(0x78));
  EXPECT_TRUE(assemblerContainsBytes(&as, expected));
}

TEST(AssemblerTest, MovbRexPrefix) {
  // 40 8a 7a 12     movb    18(%rdx), %dil
  // 41 8a 74 24 34  movb    52(%r12), %sil
  // 40 88 6f 56     movb    %bpl, 86(%rdi)
  // 44 88 7e 78     movb    %r15b, 120(%rsi)
  const byte expected[] = {0x40, 0x8a, 0x7a, 0x12, 0x41, 0x8a, 0x74, 0x24, 0x34,
                           0x40, 0x88, 0x6f, 0x56, 0x44, 0x88, 0x7e, 0x78};
  Assembler as;
  as.movb(RDI, Address(RDX, 0x12));
  as.movb(RSI, Address(R12, 0x34));
  as.movb(Address(RDI, 0x56), RBP);
  as.movb(Address(RSI, 0x78), R15);
  EXPECT_TRUE(assemblerContainsBytes(&as, expected));
}

}  // namespace x64
}  // namespace python
