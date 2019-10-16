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
  as.jmp(&loop, Assembler::kNearJump);
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

TEST(AssemblerTest, TestbWithRexPrefix) {
  // 40 84 3e        testb   %dil, (%rsi)
  // 41 84 2c 1a     testb   %bpl, (%r10,%rbx)
  const byte expected[] = {0x40, 0x84, 0x3e, 0x41, 0x84, 0x2c, 0x1a};

  Assembler as;
  as.testb(Address(RSI, 0), RDI);
  as.testb(Address(R10, RBX, TIMES_1, 0), RBP);
  EXPECT_TRUE(assemblerContainsBytes(&as, expected));
}

TEST(AssemblerTest, TestqWithByteImm) {
  // a8 12                   test   $0x12,%al
  // f6 c3 34                test   $0x34,%bl
  // 40 f6 c6 56             test   $0x56,%sil
  // 41 f6 c2 78             test   $0x78,%r10b
  // f6 00 90                testb  $0x90,(%rax)
  // 41 f6 02 12             testb  $0x12,(%r10)
  const byte expected[] = {0xa8, 0x12, 0xf6, 0xc3, 0x34, 0x40, 0xf6,
                           0xc6, 0x56, 0x41, 0xf6, 0xc2, 0x78, 0xf6,
                           0x00, 0x90, 0x41, 0xf6, 0x02, 0x12};

  Assembler as;
  as.testq(RAX, Immediate(0x12));
  as.testq(RBX, Immediate(0x34));
  as.testq(RSI, Immediate(0x56));
  as.testq(R10, Immediate(0x78));
  as.testq(Address(RAX, 0), Immediate(0x90));
  as.testq(Address(R10, 0), Immediate(0x12));
  EXPECT_TRUE(assemblerContainsBytes(&as, expected));
}

TEST(AssemblerTest, TestqWithUnsignedLongImm) {
  // a9 34 12 00 00          test   $0x1234,%eax
  // f7 c3 78 56 00 00       test   $0x5678,%ebx
  // 41 f7 c0 12 90 00 00    test   $0x9012,%r10d
  // f7 00 56 34 00 00       testl  $0x3456,(%rax)
  // 41 f7 00 00 00 00 40    testl  $0x40000000,(%r10)
  const byte expected[] = {0xa9, 0x34, 0x12, 0x00, 0x00, 0xf7, 0xc3, 0x78,
                           0x56, 0x00, 0x00, 0x41, 0xf7, 0xc2, 0x12, 0x90,
                           0x00, 0x00, 0xf7, 0x00, 0x56, 0x34, 0x00, 0x00,
                           0x41, 0xf7, 0x02, 0x00, 0x00, 0x00, 0x40};

  Assembler as;
  as.testq(RAX, Immediate(0x1234));
  as.testq(RBX, Immediate(0x5678));
  as.testq(R10, Immediate(0x9012));
  as.testq(Address(RAX, 0), Immediate(0x3456));
  as.testq(Address(R10, 0), Immediate(0x40000000));
  EXPECT_TRUE(assemblerContainsBytes(&as, expected));
}

TEST(AssemblerTest, TestqWithSignedLongImm) {
  // 48 a9 ff ff ff ff       test   $0xffffffffffffffff,%rax
  // 48 f7 c3 fe ff ff ff    test   $0xfffffffffffffffe,%rbx
  // 49 f7 c2 fd ff ff ff    test   $0xfffffffffffffffd,%r10
  // 48 f7 00 fc ff ff ff    testq  $0xfffffffffffffffc,(%rax)
  // 49 f7 02 fb ff ff ff    testq  $0xfffffffffffffffb,(%r10)
  const byte expected[] = {0x48, 0xa9, 0xff, 0xff, 0xff, 0xff, 0x48, 0xf7, 0xc3,
                           0xfe, 0xff, 0xff, 0xff, 0x49, 0xf7, 0xc2, 0xfd, 0xff,
                           0xff, 0xff, 0x48, 0xf7, 0x00, 0xfc, 0xff, 0xff, 0xff,
                           0x49, 0xf7, 0x02, 0xfb, 0xff, 0xff, 0xff};

  Assembler as;
  as.testq(RAX, Immediate(-1));
  as.testq(RBX, Immediate(-2));
  as.testq(R10, Immediate(-3));
  as.testq(Address(RAX, 0), Immediate(-4));
  as.testq(Address(R10, 0), Immediate(-5));
  EXPECT_TRUE(assemblerContainsBytes(&as, expected));
}

}  // namespace x64
}  // namespace python
