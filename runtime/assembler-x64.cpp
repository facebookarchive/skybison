// Copyright (c) 2013, the Dart project authors and Facebook, Inc. and its
// affiliates. Please see the AUTHORS-Dart file for details. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE-Dart file.

#include "assembler-x64.h"

#include <cstring>

#include "globals.h"

namespace py {
namespace x64 {

void Assembler::initializeMemoryWithBreakpoints(uword data, word length) {
  std::memset(reinterpret_cast<void*>(data), 0xcc, length);
}

void Assembler::call(Label* label) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  static const int size = 5;
  emitUint8(0xe8);
  emitLabel(label, size);
}

void Assembler::pushq(Register reg) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitRegisterREX(reg, REX_NONE);
  emitUint8(0x50 | (reg & 7));
}

void Assembler::pushq(Immediate imm) {
  if (imm.isInt8()) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    emitUint8(0x6a);
    emitUint8(imm.value() & 0xff);
  } else {
    DCHECK(imm.isInt32(), "assert()");
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    emitUint8(0x68);
    emitImmediate(imm);
  }
}

void Assembler::popq(Register reg) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitRegisterREX(reg, REX_NONE);
  emitUint8(0x58 | (reg & 7));
}

void Assembler::setcc(Condition condition, Register dst) {
  DCHECK(dst != kNoRegister, "assert()");
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  if (dst >= 8) {
    emitUint8(REX_PREFIX | (((dst & 0x08) != 0) ? REX_B : REX_NONE));
  }
  emitUint8(0x0f);
  emitUint8(0x90 + condition);
  emitUint8(0xc0 + (dst & 0x07));
}

void Assembler::emitQ(int reg, Address address, int opcode, int prefix2,
                      int prefix1) {
  DCHECK(reg <= XMM15, "assert()");
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  if (prefix1 >= 0) {
    emitUint8(prefix1);
  }
  emitOperandREX(reg, address, REX_W);
  if (prefix2 >= 0) {
    emitUint8(prefix2);
  }
  emitUint8(opcode);
  emitOperand(reg & 7, address);
}

void Assembler::emitB(Register reg, Address address, int opcode, int prefix2,
                      int prefix1) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  if (prefix1 >= 0) {
    emitUint8(prefix1);
  }
  emitOperandREX(reg, address, byteRegisterREX(reg));
  if (prefix2 >= 0) {
    emitUint8(prefix2);
  }
  emitUint8(opcode);
  emitOperand(reg & 7, address);
}

void Assembler::emitL(int reg, Address address, int opcode, int prefix2,
                      int prefix1) {
  DCHECK(reg <= XMM15, "assert()");
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  if (prefix1 >= 0) {
    emitUint8(prefix1);
  }
  emitOperandREX(reg, address, REX_NONE);
  if (prefix2 >= 0) {
    emitUint8(prefix2);
  }
  emitUint8(opcode);
  emitOperand(reg & 7, address);
}

void Assembler::emitW(Register reg, Address address, int opcode, int prefix2,
                      int prefix1) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  if (prefix1 >= 0) {
    emitUint8(prefix1);
  }
  emitOperandSizeOverride();
  emitOperandREX(reg, address, REX_NONE);
  if (prefix2 >= 0) {
    emitUint8(prefix2);
  }
  emitUint8(opcode);
  emitOperand(reg & 7, address);
}

void Assembler::movl(Register dst, Immediate imm) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  Operand operand(dst);
  emitOperandREX(0, operand, REX_NONE);
  emitUint8(0xc7);
  emitOperand(0, operand);
  DCHECK(imm.isInt32(), "assert()");
  emitImmediate(imm);
}

void Assembler::movl(Address dst, Immediate imm) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  Operand operand(dst);
  emitOperandREX(0, dst, REX_NONE);
  emitUint8(0xc7);
  emitOperand(0, dst);
  DCHECK(imm.isInt32(), "assert()");
  emitImmediate(imm);
}

void Assembler::movb(Address dst, Immediate imm) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitOperandREX(0, dst, REX_NONE);
  emitUint8(0xc6);
  emitOperand(0, dst);
  DCHECK(imm.isInt8(), "assert()");
  emitUint8(imm.value() & 0xff);
}

void Assembler::movw(Register /*dst*/, Address /*src*/) {
  UNIMPLEMENTED("Use movzxw or movsxw instead.");
}

void Assembler::movw(Address dst, Immediate imm) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitOperandSizeOverride();
  emitOperandREX(0, dst, REX_NONE);
  emitUint8(0xc7);
  emitOperand(0, dst);
  emitUint8(imm.value() & 0xff);
  emitUint8((imm.value() >> 8) & 0xff);
}

void Assembler::leaq(Register dst, Label* label) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  // Emit RIP-relative lea
  Operand operand(dst);
  emitOperandREX(0, operand, REX_W);
  emitUint8(0x8d);
  static const int size = 7;
  Address address(Address::addressRIPRelative(0xdeadbeef));
  emitOperand(0, address);
  // Overwrite the fake displacement with a label or label link
  buffer_.remit<uint32_t>();
  emitLabel(label, size);
}

void Assembler::movq(Register dst, Immediate imm) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  if (imm.isUint32()) {
    // Pick single byte B8 encoding if possible. If dst < 8 then we also omit
    // the Rex byte.
    emitRegisterREX(dst, REX_NONE);
    emitUint8(0xb8 | (dst & 7));
    emitUInt32(imm.value());
  } else if (imm.isInt32()) {
    // Sign extended C7 Cx encoding if we have a negative input.
    Operand operand(dst);
    emitOperandREX(0, operand, REX_W);
    emitUint8(0xc7);
    emitOperand(0, operand);
    emitImmediate(imm);
  } else {
    // Full 64 bit immediate encoding.
    emitRegisterREX(dst, REX_W);
    emitUint8(0xb8 | (dst & 7));
    emitImmediate(imm);
  }
}

void Assembler::movq(Address dst, Immediate imm) {
  CHECK(imm.isInt32(), "this instruction only exists for 32bit immediates");
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitOperandREX(0, dst, REX_W);
  emitUint8(0xC7);
  emitOperand(0, dst);
  emitImmediate(imm);
}

void Assembler::movsb() {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(0xa4);
}

void Assembler::movsw() {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitOperandSizeOverride();
  emitUint8(0xa5);
}

void Assembler::movsl() {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(0xa5);
}

void Assembler::movsq() {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(REX_PREFIX | REX_W);
  emitUint8(0xa5);
}

void Assembler::repMovsb() {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(REP);
  emitUint8(0xa4);
}

void Assembler::repMovsw() {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(REP);
  emitOperandSizeOverride();
  emitUint8(0xa5);
}

void Assembler::repMovsl() {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(REP);
  emitUint8(0xa5);
}

void Assembler::repMovsq() {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(REP);
  emitUint8(REX_PREFIX | REX_W);
  emitUint8(0xa5);
}

void Assembler::repnzMovsb() {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(REPNZ);
  emitUint8(0xa4);
}

void Assembler::repnzMovsw() {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(REPNZ);
  emitOperandSizeOverride();
  emitUint8(0xa5);
}

void Assembler::repnzMovsl() {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(REPNZ);
  emitUint8(0xa5);
}

void Assembler::repnzMovsq() {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(REPNZ);
  emitUint8(REX_PREFIX | REX_W);
  emitUint8(0xa5);
}

void Assembler::emitSimple(int opcode, int opcode2) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(opcode);
  if (opcode2 != -1) {
    emitUint8(opcode2);
  }
}

void Assembler::emitQ(int dst, int src, int opcode, int prefix2, int prefix1) {
  DCHECK(src <= XMM15, "assert()");
  DCHECK(dst <= XMM15, "assert()");
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  if (prefix1 >= 0) {
    emitUint8(prefix1);
  }
  emitRegRegREX(dst, src, REX_W);
  if (prefix2 >= 0) {
    emitUint8(prefix2);
  }
  emitUint8(opcode);
  emitRegisterOperand(dst & 7, src);
}

void Assembler::emitL(int dst, int src, int opcode, int prefix2, int prefix1) {
  DCHECK(src <= XMM15, "assert()");
  DCHECK(dst <= XMM15, "assert()");
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  if (prefix1 >= 0) {
    emitUint8(prefix1);
  }
  emitRegRegREX(dst, src);
  if (prefix2 >= 0) {
    emitUint8(prefix2);
  }
  emitUint8(opcode);
  emitRegisterOperand(dst & 7, src);
}

void Assembler::emitW(Register dst, Register src, int opcode, int prefix2,
                      int prefix1) {
  DCHECK(src <= R15, "assert()");
  DCHECK(dst <= R15, "assert()");
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  if (prefix1 >= 0) {
    emitUint8(prefix1);
  }
  emitOperandSizeOverride();
  emitRegRegREX(dst, src);
  if (prefix2 >= 0) {
    emitUint8(prefix2);
  }
  emitUint8(opcode);
  emitRegisterOperand(dst & 7, src);
}

void Assembler::cmpPS(XmmRegister dst, XmmRegister src, int condition) {
  emitL(dst, src, 0xc2, 0x0f);
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(condition);
}

void Assembler::set1ps(XmmRegister dst, Register tmp, Immediate imm) {
  // load 32-bit immediate value into tmp1.
  movl(tmp, imm);
  // Move value from tmp1 into dst.
  movd(dst, tmp);
  // Broadcast low lane into other three lanes.
  shufps(dst, dst, Immediate(0x0));
}

void Assembler::shufps(XmmRegister dst, XmmRegister src, Immediate mask) {
  emitL(dst, src, 0xc6, 0x0f);
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  DCHECK(mask.isUint8(), "assert()");
  emitUint8(mask.value());
}

void Assembler::shufpd(XmmRegister dst, XmmRegister src, Immediate mask) {
  emitL(dst, src, 0xc6, 0x0f, 0x66);
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  DCHECK(mask.isUint8(), "assert()");
  emitUint8(mask.value());
}

void Assembler::roundsd(XmmRegister dst, XmmRegister src, RoundingMode mode) {
  DCHECK(src <= XMM15, "assert()");
  DCHECK(dst <= XMM15, "assert()");
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(0x66);
  emitRegRegREX(dst, src);
  emitUint8(0x0f);
  emitUint8(0x3a);
  emitUint8(0x0b);
  emitRegisterOperand(dst & 7, src);
  // Mask precision exeption.
  emitUint8(static_cast<uint8_t>(mode) | 0x8);
}

void Assembler::fldl(Address src) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(0xdd);
  emitOperand(0, src);
}

void Assembler::fstpl(Address dst) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(0xdd);
  emitOperand(3, dst);
}

void Assembler::ffree(word value) {
  DCHECK(value < 7, "assert()");
  emitSimple(0xdd, 0xc0 + value);
}

void Assembler::emitTestB(Operand operand, Immediate imm) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  if (operand.hasRegister(RAX)) {
    emitUint8(0xa8);
  } else {
    emitOperandREX(0, operand, byteOperandREX(operand));
    emitUint8(0xf6);
    emitOperand(0, operand);
  }
  DCHECK(imm.isInt8(), "immediate too large");
  emitUint8(imm.value());
}

void Assembler::testb(Register dst, Register src) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitRegRegREX(dst, src, byteRegisterREX(dst) | byteRegisterREX(src));
  emitUint8(0x84);
  emitRegisterOperand(dst & 7, src);
}

void Assembler::testb(Register reg, Immediate imm) {
  emitTestB(Operand(reg), imm);
}

void Assembler::testb(Address address, Immediate imm) {
  emitTestB(address, imm);
}

void Assembler::testb(Address address, Register reg) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitOperandREX(reg, address, byteRegisterREX(reg));
  emitUint8(0x84);
  emitOperand(reg & 7, address);
}

void Assembler::emitTestQ(Operand operand, Immediate imm) {
  // Try to emit a small instruction if the value of the immediate lets us. For
  // Address operands, this relies on the fact that x86 is little-endian.
  if (imm.isUint8()) {
    emitTestB(operand, Immediate(static_cast<int8_t>(imm.value())));
  } else if (imm.isUint32()) {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    if (operand.hasRegister(RAX)) {
      emitUint8(0xa9);
    } else {
      emitOperandREX(0, operand, REX_NONE);
      emitUint8(0xf7);
      emitOperand(0, operand);
    }
    emitUInt32(imm.value());
  } else {
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    // Sign extended version of 32 bit test.
    DCHECK(imm.isInt32(), "assert()");
    emitOperandREX(0, operand, REX_W);
    if (operand.hasRegister(RAX)) {
      emitUint8(0xa9);
    } else {
      emitUint8(0xf7);
      emitOperand(0, operand);
    }
    emitInt32(imm.value());
  }
}

void Assembler::testq(Register reg, Immediate imm) {
  emitTestQ(Operand(reg), imm);
}

void Assembler::testq(Address address, Immediate imm) {
  emitTestQ(address, imm);
}

void Assembler::aluB(uint8_t modrm_opcode, Register dst, Immediate imm) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  Operand dst_operand(dst);
  emitOperandREX(modrm_opcode, dst_operand, byteRegisterREX(dst));
  emitComplexB(modrm_opcode, dst_operand, imm);
}

void Assembler::aluL(uint8_t modrm_opcode, Register dst, Immediate imm) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitRegisterREX(dst, REX_NONE);
  emitComplex(modrm_opcode, Operand(dst), imm);
}

void Assembler::aluB(uint8_t modrm_opcode, Address dst, Immediate imm) {
  DCHECK(imm.isUint8() || imm.isInt8(), "assert()");
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitOperandREX(modrm_opcode, dst, REX_NONE);
  emitUint8(0x80);
  emitOperand(modrm_opcode, dst);
  emitUint8(imm.value() & 0xff);
}

void Assembler::aluW(uint8_t modrm_opcode, Address dst, Immediate imm) {
  DCHECK(imm.isInt16() || imm.isUint16(), "assert()");
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitOperandSizeOverride();
  emitOperandREX(modrm_opcode, dst, REX_NONE);
  if (imm.isInt8()) {
    emitSignExtendedInt8(modrm_opcode, dst, imm);
  } else {
    emitUint8(0x81);
    emitOperand(modrm_opcode, dst);
    emitUint8(imm.value() & 0xff);
    emitUint8((imm.value() >> 8) & 0xff);
  }
}

void Assembler::aluL(uint8_t modrm_opcode, Address dst, Immediate imm) {
  DCHECK(imm.isInt32(), "assert()");
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitOperandREX(modrm_opcode, dst, REX_NONE);
  emitComplex(modrm_opcode, dst, imm);
}

void Assembler::aluQ(uint8_t modrm_opcode, uint8_t /*opcode*/, Register dst,
                     Immediate imm) {
  Operand operand(dst);
  if (modrm_opcode == 4 && imm.isUint32()) {
    // We can use andl for andq.
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    emitRegisterREX(dst, REX_NONE);
    // Would like to use emitComplex here, but it doesn't like uint32
    // immediates.
    if (imm.isInt8()) {
      emitSignExtendedInt8(modrm_opcode, operand, imm);
    } else {
      if (dst == RAX) {
        emitUint8(0x25);
      } else {
        emitUint8(0x81);
        emitOperand(modrm_opcode, operand);
      }
      emitUInt32(imm.value());
    }
  } else {
    DCHECK(imm.isInt32(), "assert()");
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);
    emitRegisterREX(dst, REX_W);
    emitComplex(modrm_opcode, operand, imm);
  }
}

void Assembler::aluQ(uint8_t modrm_opcode, uint8_t /*opcode*/, Address dst,
                     Immediate imm) {
  DCHECK(imm.isInt32(), "assert()");
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitOperandREX(modrm_opcode, dst, REX_W);
  emitComplex(modrm_opcode, dst, imm);
}

void Assembler::cqo() {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitRegisterREX(RAX, REX_W);
  emitUint8(0x99);
}

void Assembler::emitUnaryQ(Register reg, int opcode, int modrm_code) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitRegisterREX(reg, REX_W);
  emitUint8(opcode);
  emitOperand(modrm_code, Operand(reg));
}

void Assembler::emitUnaryL(Register reg, int opcode, int modrm_code) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitRegisterREX(reg, REX_NONE);
  emitUint8(opcode);
  emitOperand(modrm_code, Operand(reg));
}

void Assembler::emitUnaryQ(Address address, int opcode, int modrm_code) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  Operand operand(address);
  emitOperandREX(modrm_code, operand, REX_W);
  emitUint8(opcode);
  emitOperand(modrm_code, operand);
}

void Assembler::emitUnaryL(Address address, int opcode, int modrm_code) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  Operand operand(address);
  emitOperandREX(modrm_code, operand, REX_NONE);
  emitUint8(opcode);
  emitOperand(modrm_code, operand);
}

void Assembler::imull(Register reg, Immediate imm) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  Operand operand(reg);
  emitOperandREX(reg, operand, REX_NONE);
  emitUint8(0x69);
  emitOperand(reg & 7, Operand(reg));
  emitImmediate(imm);
}

void Assembler::shll(Register reg, Immediate imm) {
  emitGenericShift(false, 4, reg, imm);
}

void Assembler::shll(Register operand, Register shifter) {
  emitGenericShift(false, 4, operand, shifter);
}

void Assembler::shrl(Register reg, Immediate imm) {
  emitGenericShift(false, 5, reg, imm);
}

void Assembler::shrl(Register operand, Register shifter) {
  emitGenericShift(false, 5, operand, shifter);
}

void Assembler::sarl(Register reg, Immediate imm) {
  emitGenericShift(false, 7, reg, imm);
}

void Assembler::sarl(Register operand, Register shifter) {
  emitGenericShift(false, 7, operand, shifter);
}

void Assembler::shldl(Register dst, Register src, Immediate imm) {
  emitL(src, dst, 0xa4, 0x0f);
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  DCHECK(imm.isInt8(), "assert()");
  emitUint8(imm.value() & 0xff);
}

void Assembler::shlq(Register reg, Immediate imm) {
  emitGenericShift(true, 4, reg, imm);
}

void Assembler::shlq(Register operand, Register shifter) {
  emitGenericShift(true, 4, operand, shifter);
}

void Assembler::shrq(Register reg, Immediate imm) {
  emitGenericShift(true, 5, reg, imm);
}

void Assembler::shrq(Register operand, Register shifter) {
  emitGenericShift(true, 5, operand, shifter);
}

void Assembler::sarq(Register reg, Immediate imm) {
  emitGenericShift(true, 7, reg, imm);
}

void Assembler::sarq(Register operand, Register shifter) {
  emitGenericShift(true, 7, operand, shifter);
}

void Assembler::shldq(Register dst, Register src, Immediate imm) {
  emitQ(src, dst, 0xa4, 0x0f);
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  DCHECK(imm.isInt8(), "assert()");
  emitUint8(imm.value() & 0xff);
}

void Assembler::btq(Register base, int bit) {
  DCHECK(bit >= 0 && bit < 64, "assert()");
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  Operand operand(base);
  emitOperandREX(4, operand, bit >= 32 ? REX_W : REX_NONE);
  emitUint8(0x0f);
  emitUint8(0xba);
  emitOperand(4, operand);
  emitUint8(bit);
}

void Assembler::enter(Immediate imm) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(0xc8);
  DCHECK(imm.isUint16(), "assert()");
  emitUint8(imm.value() & 0xff);
  emitUint8((imm.value() >> 8) & 0xff);
  emitUint8(0x00);
}

void Assembler::nop(int size) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  // There are nops up to size 15, but for now just provide up to size 8.
  DCHECK(0 < size && size <= kMaxNopSize, "assert()");
  switch (size) {
    case 1:
      emitUint8(0x90);
      break;
    case 2:
      emitUint8(0x66);
      emitUint8(0x90);
      break;
    case 3:
      emitUint8(0x0f);
      emitUint8(0x1f);
      emitUint8(0x00);
      break;
    case 4:
      emitUint8(0x0f);
      emitUint8(0x1f);
      emitUint8(0x40);
      emitUint8(0x00);
      break;
    case 5:
      emitUint8(0x0f);
      emitUint8(0x1f);
      emitUint8(0x44);
      emitUint8(0x00);
      emitUint8(0x00);
      break;
    case 6:
      emitUint8(0x66);
      emitUint8(0x0f);
      emitUint8(0x1f);
      emitUint8(0x44);
      emitUint8(0x00);
      emitUint8(0x00);
      break;
    case 7:
      emitUint8(0x0f);
      emitUint8(0x1f);
      emitUint8(0x80);
      emitUint8(0x00);
      emitUint8(0x00);
      emitUint8(0x00);
      emitUint8(0x00);
      break;
    case 8:
      emitUint8(0x0f);
      emitUint8(0x1f);
      emitUint8(0x84);
      emitUint8(0x00);
      emitUint8(0x00);
      emitUint8(0x00);
      emitUint8(0x00);
      emitUint8(0x00);
      break;
    default:
      UNIMPLEMENTED("default");
  }
}

void Assembler::nops(int size) {
  DCHECK(size >= 0, "Can't emit negative nops");
  if (size == 0) return;
  while (size > kMaxNopSize) {
    nop(kMaxNopSize);
    size -= kMaxNopSize;
  }
  nop(size);
}

void Assembler::ud2() {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  emitUint8(0x0f);
  emitUint8(0x0b);
}

void Assembler::jcc(Condition condition, Label* label, bool near) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  if (label->isBound()) {
    static const int short_size = 2;
    static const int long_size = 6;
    word offset = label->position() - buffer_.size();
    DCHECK(offset <= 0, "assert()");
    if (Utils::fits<int8_t>(offset - short_size)) {
      emitUint8(0x70 + condition);
      emitUint8((offset - short_size) & 0xff);
    } else {
      emitUint8(0x0f);
      emitUint8(0x80 + condition);
      emitInt32(offset - long_size);
    }
  } else if (near) {
    emitUint8(0x70 + condition);
    emitNearLabelLink(label);
  } else {
    emitUint8(0x0f);
    emitUint8(0x80 + condition);
    emitLabelLink(label);
  }
}

void Assembler::jmp(Label* label, bool near) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  if (label->isBound()) {
    static const int short_size = 2;
    static const int long_size = 5;
    word offset = label->position() - buffer_.size();
    DCHECK(offset <= 0, "assert()");
    if (Utils::fits<int8_t>(offset - short_size)) {
      emitUint8(0xeb);
      emitUint8((offset - short_size) & 0xff);
    } else {
      emitUint8(0xe9);
      emitInt32(offset - long_size);
    }
  } else if (near) {
    emitUint8(0xeb);
    emitNearLabelLink(label);
  } else {
    emitUint8(0xe9);
    emitLabelLink(label);
  }
}

void Assembler::bind(Label* label) {
  word bound = buffer_.size();
  DCHECK(!label->isBound(), "assert()");  // Labels can only be bound once.
  while (label->isLinked()) {
    word position = label->linkPosition();
    word next = buffer_.load<int32_t>(position);
    buffer_.store<int32_t>(position, bound - (position + 4));
    label->position_ = next;
  }
  while (label->hasNear()) {
    word position = label->nearPosition();
    word offset = bound - (position + 1);
    DCHECK(Utils::fits<int8_t>(offset), "assert()");
    buffer_.store<int8_t>(position, offset);
  }
  label->bindTo(bound);
}

void Assembler::align(int alignment) {
  DCHECK(Utils::isPowerOfTwo(alignment), "assert()");
  word pos = buffer_.getPosition();
  int mod = pos & (alignment - 1);
  if (mod == 0) {
    return;
  }
  word bytes_needed = alignment - mod;
  while (bytes_needed > kMaxNopSize) {
    nop(kMaxNopSize);
    bytes_needed -= kMaxNopSize;
  }
  if (bytes_needed) {
    nop(bytes_needed);
  }
  DCHECK((buffer_.getPosition() & (alignment - 1)) == 0, "assert()");
}

void Assembler::emitOperand(int rm, Operand operand) {
  DCHECK(rm >= 0 && rm < 8, "assert()");
  const word length = operand.length_;
  DCHECK(length > 0, "assert()");
  // emit the ModRM byte updated with the given RM value.
  DCHECK((operand.encoding_[0] & 0x38) == 0, "assert()");
  emitUint8(operand.encoding_[0] + (rm << 3));
  // emit the rest of the encoded operand.
  for (word i = 1; i < length; i++) {
    emitUint8(operand.encoding_[i]);
  }
}

void Assembler::emitRegisterOperand(int rm, int reg) {
  Operand operand;
  operand.setModRM(3, static_cast<Register>(reg));
  emitOperand(rm, operand);
}

void Assembler::emitImmediate(Immediate imm) {
  if (imm.isInt32()) {
    emitInt32(static_cast<int32_t>(imm.value()));
  } else {
    emitInt64(imm.value());
  }
}

void Assembler::emitSignExtendedInt8(int rm, Operand operand,
                                     Immediate immediate) {
  emitUint8(0x83);
  emitOperand(rm, operand);
  emitUint8(immediate.value() & 0xff);
}

void Assembler::emitComplexB(int rm, Operand operand, Immediate imm) {
  DCHECK(rm >= 0 && rm < 8, "assert()");
  DCHECK(imm.isUint8() || imm.isInt8(), "immediate too large");
  if (operand.hasRegister(RAX)) {
    // Use short form if the destination is al.
    emitUint8(0x04 + (rm << 3));
  } else {
    emitUint8(0x80);
    emitOperand(rm, operand);
  }
  emitUint8(imm.value());
}

void Assembler::emitComplex(int rm, Operand operand, Immediate immediate) {
  DCHECK(rm >= 0 && rm < 8, "assert()");
  DCHECK(immediate.isInt32(), "assert()");
  if (immediate.isInt8()) {
    emitSignExtendedInt8(rm, operand, immediate);
  } else if (operand.hasRegister(RAX)) {
    // Use short form if the destination is rax.
    emitUint8(0x05 + (rm << 3));
    emitImmediate(immediate);
  } else {
    emitUint8(0x81);
    emitOperand(rm, operand);
    emitImmediate(immediate);
  }
}

void Assembler::emitLabel(Label* label, word instruction_size) {
  if (label->isBound()) {
    word offset = label->position() - buffer_.size();
    DCHECK(offset <= 0, "assert()");
    emitInt32(offset - instruction_size);
  } else {
    emitLabelLink(label);
  }
}

void Assembler::emitLabelLink(Label* label) {
  DCHECK(!label->isBound(), "assert()");
  word position = buffer_.size();
  emitInt32(label->position_);
  label->linkTo(position);
}

void Assembler::emitNearLabelLink(Label* label) {
  DCHECK(!label->isBound(), "assert()");
  word position = buffer_.size();
  emitUint8(0);
  label->nearLinkTo(position);
}

void Assembler::emitGenericShift(bool wide, int rm, Register reg,
                                 Immediate imm) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  DCHECK(imm.isInt8(), "assert()");
  if (wide) {
    emitRegisterREX(reg, REX_W);
  } else {
    emitRegisterREX(reg, REX_NONE);
  }
  if (imm.value() == 1) {
    emitUint8(0xd1);
    emitOperand(rm, Operand(reg));
  } else {
    emitUint8(0xc1);
    emitOperand(rm, Operand(reg));
    emitUint8(imm.value() & 0xff);
  }
}

void Assembler::emitGenericShift(bool wide, int rm, Register operand,
                                 Register shifter) {
  AssemblerBuffer::EnsureCapacity ensured(&buffer_);
  DCHECK(shifter == RCX, "assert()");
  emitRegisterREX(operand, wide ? REX_W : REX_NONE);
  emitUint8(0xd3);
  emitOperand(rm, Operand(operand));
}

}  // namespace x64
}  // namespace py
