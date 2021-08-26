// Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
#include "disassembler.h"

#include "gtest/gtest.h"

#include "assembler-x64.h"

namespace py {
namespace x64 {
namespace testing {

static const int kMaxDisassemblySize = 4096;

std::string disassembleToString(View<byte> code) {
  // Some padding in case it's longer than expected.
  char buffer[kMaxDisassemblySize];
  std::memset(buffer, 0, sizeof buffer);
  DisassembleToMemory formatter(buffer, sizeof buffer);
  Disassembler::disassemble(
      reinterpret_cast<uword>(code.data()),
      reinterpret_cast<uword>(code.data()) + code.length(), &formatter);
  return buffer;
}

static byte modrm(byte mod, byte reg, byte rm) {
  return ((mod & 0x3) << 6) | ((reg & 0x7) << 3) | (rm & 0x7);
}

TEST(DisassemblerTest, Cmpb) {
  byte code[] = {0x41, 0x80, modrm(3, 7, R10), 0x78, 0x80, modrm(3, 7, RBX),
                 0x34};
  EXPECT_EQ(disassembleToString(code), R"(cmpb r10,0X78
cmpb rbx,0X34
)");
}

TEST(DisassemblerTest, Movq) {
  byte code[] = {0xc7, modrm(3, 0, RBX), 0xaa, 0xbb, 0xcc, 0x0d};
  EXPECT_EQ(disassembleToString(code), "movl rbx,0X0DCCBBAA\n");
}

TEST(DisassemblerTest, Orb) {
  byte code[] = {0x41, 0x80, modrm(3, 1, R10), 0x78, 0x80, modrm(3, 1, RBX),
                 0x34};
  EXPECT_EQ(disassembleToString(code), R"(orb r10,0X78
orb rbx,0X34
)");
}

TEST(DisassemblerTest, Ud2) {
  byte code[] = {0x0f, 0x0b};
  EXPECT_EQ(disassembleToString(code), "ud2\n");
}

}  // namespace testing
}  // namespace x64
}  // namespace py
