// Copyright (c) 2013, the Dart project authors and Facebook, Inc. and its
// affiliates. Please see the AUTHORS-Dart file for details. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE-Dart file.

#pragma once

#include "assembler-utils.h"
#include "globals.h"
#include "utils.h"

namespace py {
namespace x64 {

enum Register {
  RAX = 0,
  RCX = 1,
  RDX = 2,
  RBX = 3,
  RSP = 4,
  RBP = 5,
  RSI = 6,
  RDI = 7,
  R8 = 8,
  R9 = 9,
  R10 = 10,
  R11 = 11,
  R12 = 12,
  R13 = 13,
  R14 = 14,
  R15 = 15,
  kNoRegister = -1,  // Signals an illegal register.
};

enum XmmRegister {
  XMM0 = 0,
  XMM1 = 1,
  XMM2 = 2,
  XMM3 = 3,
  XMM4 = 4,
  XMM5 = 5,
  XMM6 = 6,
  XMM7 = 7,
  XMM8 = 8,
  XMM9 = 9,
  XMM10 = 10,
  XMM11 = 11,
  XMM12 = 12,
  XMM13 = 13,
  XMM14 = 14,
  XMM15 = 15,
  kNoXmmRegister = -1  // Signals an illegal register.
};

enum RexBits {
  REX_NONE = 0,
  REX_B = 1 << 0,
  REX_X = 1 << 1,
  REX_R = 1 << 2,
  REX_W = 1 << 3,
  REX_PREFIX = 1 << 6
};

enum Prefix {
  LOCK = 0xf0,
  REPNZ = 0xf2,
  REP = 0xf3,
};

enum Condition {
  // This weird name avoids conflicts with the OVERFLOW macro in math.h on some
  // platforms.
  YES_OVERFLOW = 0,
  NOT_OVERFLOW = 1,
  BELOW = 2,
  ABOVE_EQUAL = 3,
  EQUAL = 4,
  NOT_EQUAL = 5,
  BELOW_EQUAL = 6,
  ABOVE = 7,
  SIGN = 8,
  NOT_SIGN = 9,
  PARITY = 10,
  NOT_PARITY = 11,
  LESS = 12,
  GREATER_EQUAL = 13,
  LESS_EQUAL = 14,
  GREATER = 15,

  ZERO = EQUAL,
  NOT_ZERO = NOT_EQUAL,
  CARRY = BELOW,
  NOT_CARRY = ABOVE_EQUAL,
  PARITY_EVEN = PARITY,
  PARITY_ODD = NOT_PARITY,

  // Platform-independent variants declared for all platforms
  UNSIGNED_LESS = BELOW,
  UNSIGNED_LESS_EQUAL = BELOW_EQUAL,
  UNSIGNED_GREATER = ABOVE,
  UNSIGNED_GREATER_EQUAL = ABOVE_EQUAL,

  INVALID_CONDITION = -1
};

enum ScaleFactor {
  TIMES_1 = 0,
  TIMES_2 = 1,
  TIMES_4 = 2,
  TIMES_8 = 3,
  TIMES_16 = 4,
};

// The largest multibyte nop we will emit.  This could go up to 15 if it
// becomes important to us.
const int kMaxNopSize = 8;

class Immediate {
 public:
  explicit Immediate(int64_t value) : value_(value) {}

  Immediate(const Immediate& other) : value_(other.value_) {}

  int64_t value() const { return value_; }

  bool isInt8() const { return Utils::fits<int8_t>(value_); }
  bool isUint8() const { return Utils::fits<uint8_t>(value_); }
  bool isInt16() const { return Utils::fits<int16_t>(value_); }
  bool isUint16() const { return Utils::fits<uint16_t>(value_); }
  bool isInt32() const { return Utils::fits<int32_t>(value_); }
  bool isUint32() const { return Utils::fits<uint32_t>(value_); }

 private:
  const int64_t value_;
};

class Operand {
 public:
  uint8_t rex() const { return rex_; }

  uint8_t mod() const { return (encodingAt(0) >> 6) & 3; }

  Register rm() const {
    int rm_rex = (rex_ & REX_B) << 3;
    return static_cast<Register>(rm_rex + (encodingAt(0) & 7));
  }

  ScaleFactor scale() const {
    return static_cast<ScaleFactor>((encodingAt(1) >> 6) & 3);
  }

  Register index() const {
    int index_rex = (rex_ & REX_X) << 2;
    return static_cast<Register>(index_rex + ((encodingAt(1) >> 3) & 7));
  }

  Register base() const {
    int base_rex = (rex_ & REX_B) << 3;
    return static_cast<Register>(base_rex + (encodingAt(1) & 7));
  }

  int8_t disp8() const {
    DCHECK(length_ >= 2, "assert()");
    return static_cast<int8_t>(encoding_[length_ - 1]);
  }

  int32_t disp32() const {
    DCHECK(length_ >= 5, "assert()");
    int32_t result;
    std::memcpy(&result, &encoding_[length_ - 4], sizeof(result));
    return result;
  }

  Operand(const Operand& other) : length_(other.length_), rex_(other.rex_) {
    std::memmove(&encoding_[0], &other.encoding_[0], other.length_);
  }

  Operand& operator=(const Operand& other) {
    length_ = other.length_;
    rex_ = other.rex_;
    memmove(&encoding_[0], &other.encoding_[0], other.length_);
    return *this;
  }

  bool operator==(const Operand& other) const {
    if (length_ != other.length_) return false;
    if (rex_ != other.rex_) return false;
    for (uint8_t i = 0; i < length_; i++) {
      if (encoding_[i] != other.encoding_[i]) return false;
    }
    return true;
  }

 protected:
  Operand() : length_(0), rex_(REX_NONE) {}  // Needed by subclass Address.

  void setModRM(int mod, Register rm) {
    DCHECK((mod & ~3) == 0, "assert()");
    if ((rm > 7) && !((rm == R12) && (mod != 3))) {
      rex_ |= REX_B;
    }
    encoding_[0] = (mod << 6) | (rm & 7);
    length_ = 1;
  }

  void setSIB(ScaleFactor scale, Register index, Register base) {
    DCHECK(length_ == 1, "assert()");
    DCHECK((scale & ~3) == 0, "assert()");
    if (base > 7) {
      DCHECK((rex_ & REX_B) == 0,
             "assert()");  // Must not have REX.B already set.
      rex_ |= REX_B;
    }
    if (index > 7) rex_ |= REX_X;
    encoding_[1] = (scale << 6) | ((index & 7) << 3) | (base & 7);
    length_ = 2;
  }

  void setDisp8(int8_t disp) {
    DCHECK(length_ == 1 || length_ == 2, "assert()");
    encoding_[length_++] = static_cast<uint8_t>(disp);
  }

  void setDisp32(int32_t disp) {
    DCHECK(length_ == 1 || length_ == 2, "assert()");
    memmove(&encoding_[length_], &disp, sizeof(disp));
    length_ += sizeof(disp);
  }

 private:
  uint8_t length_;
  uint8_t rex_;
  uint8_t encoding_[6];

  explicit Operand(Register reg) : rex_(REX_NONE) { setModRM(3, reg); }

  // Get the operand encoding byte at the given index.
  uint8_t encodingAt(word index) const {
    DCHECK_BOUND(index, length_);
    return encoding_[index];
  }

  // Returns whether or not this register is a direct register operand
  // referencing a specific register. Used from the assembler to generate better
  // encodings.
  bool hasRegister(Register reg) const {
    return isRegister() && this->reg() == reg;
  }

  // Returns whether or not this operand represents a direct register operand.
  bool isRegister() const {
    return (encodingAt(0) & 0xf8) == 0xc0;  // mod bits of ModR/M
  }

  // Returns the register represented by the rm field of this operand.
  Register reg() const {
    DCHECK(isRegister(), "reg() called on non-register Operand");
    return static_cast<Register>(
        (encodingAt(0) & 0x7) |         // r/m bits of ModR/M
        ((rex_ & REX_B) ? 0x4 : 0x0));  // REX.B extension
  }

  friend class Assembler;
};

class Address : public Operand {
 public:
  Address(Register base, int32_t disp) {
    if ((disp == 0) && ((base & 7) != RBP)) {
      setModRM(0, base);
      if ((base & 7) == RSP) {
        setSIB(TIMES_1, RSP, base);
      }
    } else if (Utils::fits<int8_t>(disp)) {
      setModRM(1, base);
      if ((base & 7) == RSP) {
        setSIB(TIMES_1, RSP, base);
      }
      setDisp8(disp);
    } else {
      setModRM(2, base);
      if ((base & 7) == RSP) {
        setSIB(TIMES_1, RSP, base);
      }
      setDisp32(disp);
    }
  }

  Address(Register index, ScaleFactor scale, int32_t disp) {
    DCHECK(index != RSP, "Illegal addressing mode");
    setModRM(0, RSP);
    setSIB(scale, index, RBP);
    setDisp32(disp);
  }

  Address(Register base, Register index, ScaleFactor scale, int32_t disp) {
    DCHECK(index != RSP, "Illegal addressing mode");
    if ((disp == 0) && ((base & 7) != RBP)) {
      setModRM(0, RSP);
      setSIB(scale, index, base);
    } else if (Utils::fits<int8_t>(disp)) {
      setModRM(1, RSP);
      setSIB(scale, index, base);
      setDisp8(disp);
    } else {
      setModRM(2, RSP);
      setSIB(scale, index, base);
      setDisp32(disp);
    }
  }

  Address(const Address& other) : Operand(other) {}

  Address& operator=(const Address& other) {
    Operand::operator=(other);
    return *this;
  }

  static Address addressRIPRelative(int32_t disp) {
    return Address(RIPRelativeDisp(disp));
  }
  static Address addressBaseImm32(Register base, int32_t disp) {
    return Address(base, disp, ForceDisp32{});
  }

 private:
  // Only used to invoke the alternate constructor below.
  enum class ForceDisp32 {};

  Address(Register base, int32_t disp, ForceDisp32) {
    setModRM(2, base);
    if ((base & 7) == RSP) {
      setSIB(TIMES_1, RSP, base);
    }
    setDisp32(disp);
  }

  struct RIPRelativeDisp {
    explicit RIPRelativeDisp(int32_t disp) : disp_(disp) {}
    const int32_t disp_;
  };

  explicit Address(const RIPRelativeDisp& disp) {
    setModRM(0, static_cast<Register>(0x5));
    setDisp32(disp.disp_);
  }
};

class Assembler {
 public:
  Assembler() = default;

  static const bool kNearJump = true;
  static const bool kFarJump = false;

  word codeSize() const { return buffer_.size(); }

  uword codeAddress(word offset) { return buffer_.address(offset); }

  void finalizeInstructions(MemoryRegion instructions) {
    buffer_.finalizeInstructions(instructions);
  }

  void call(Register reg) { emitUnaryL(reg, 0xff, 2); }
  void call(Address address) { emitUnaryL(address, 0xff, 2); }
  void call(Label* label);

  void pushq(Register reg);
  void pushq(Address address) { emitUnaryL(address, 0xff, 6); }
  void pushq(Immediate imm);

  void popq(Register reg);
  void popq(Address address) { emitUnaryL(address, 0x8f, 0); }

  void setcc(Condition condition, Register dst);

#define X86_ZERO_OPERAND_1_BYTE_INSTRUCTIONS(F)                                \
  F(ret, 0xc3)                                                                 \
  F(leave, 0xc9)                                                               \
  F(hlt, 0xf4)                                                                 \
  F(cld, 0xfc)                                                                 \
  F(int3, 0xcc)                                                                \
  F(pushad, 0x60)                                                              \
  F(popad, 0x61)                                                               \
  F(pushfd, 0x9c)                                                              \
  F(popfd, 0x9d)                                                               \
  F(sahf, 0x9e)                                                                \
  F(cdq, 0x99)                                                                 \
  F(fwait, 0x9b)                                                               \
  F(cmpsb, 0xa6)                                                               \
  F(cmpsl, 0xa7)

#define X86_ALU_CODES(F)                                                       \
  F(and, 4)                                                                    \
  F(or, 1)                                                                     \
  F(xor, 6)                                                                    \
  F(add, 0)                                                                    \
  F(adc, 2)                                                                    \
  F(sub, 5)                                                                    \
  F(sbb, 3)                                                                    \
  F(cmp, 7)

#define XMM_ALU_CODES(F)                                                       \
  F(bad0, 0)                                                                   \
  F(sqrt, 1)                                                                   \
  F(rsqrt, 2)                                                                  \
  F(rcp, 3)                                                                    \
  F(and, 4)                                                                    \
  F(bad1, 5)                                                                   \
  F(or, 6)                                                                     \
  F(xor, 7)                                                                    \
  F(add, 8)                                                                    \
  F(mul, 9)                                                                    \
  F(bad2, 0xa)                                                                 \
  F(bad3, 0xb)                                                                 \
  F(sub, 0xc)                                                                  \
  F(min, 0xd)                                                                  \
  F(div, 0xe)                                                                  \
  F(max, 0xf)

// Table 3-1, first part
#define XMM_CONDITIONAL_CODES(F)                                               \
  F(eq, 0)                                                                     \
  F(lt, 1)                                                                     \
  F(le, 2)                                                                     \
  F(unord, 3)                                                                  \
  F(neq, 4)                                                                    \
  F(nlt, 5)                                                                    \
  F(nle, 6)                                                                    \
  F(ord, 7)

#define X86_CONDITIONAL_SUFFIXES(F)                                            \
  F(o, YES_OVERFLOW)                                                           \
  F(no, NOT_OVERFLOW)                                                          \
  F(c, CARRY)                                                                  \
  F(nc, NOT_CARRY)                                                             \
  F(z, ZERO)                                                                   \
  F(nz, NOT_ZERO)                                                              \
  F(na, BELOW_EQUAL)                                                           \
  F(a, ABOVE)                                                                  \
  F(s, SIGN)                                                                   \
  F(ns, NOT_SIGN)                                                              \
  F(pe, PARITY_EVEN)                                                           \
  F(po, PARITY_ODD)                                                            \
  F(l, LESS)                                                                   \
  F(ge, GREATER_EQUAL)                                                         \
  F(le, LESS_EQUAL)                                                            \
  F(g, GREATER)                                                                \
  /* Some alternative names */                                                 \
  F(e, EQUAL)                                                                  \
  F(ne, NOT_EQUAL)

// Register-register, register-address and address-register instructions.
#define RR(width, name, ...)                                                   \
  void name(Register dst, Register src) { emit##width(dst, src, __VA_ARGS__); }
#define RA(width, name, ...)                                                   \
  void name(Register dst, Address src) { emit##width(dst, src, __VA_ARGS__); }
#define AR(width, name, ...)                                                   \
  void name(Address dst, Register src) { emit##width(src, dst, __VA_ARGS__); }
#define REGULAR_INSTRUCTION(name, ...)                                         \
  RA(W, name##w, __VA_ARGS__)                                                  \
  RA(L, name##l, __VA_ARGS__)                                                  \
  RA(Q, name##q, __VA_ARGS__)                                                  \
  RR(W, name##w, __VA_ARGS__)                                                  \
  RR(L, name##l, __VA_ARGS__)                                                  \
  RR(Q, name##q, __VA_ARGS__)
  REGULAR_INSTRUCTION(test, 0x85)
  REGULAR_INSTRUCTION(xchg, 0x87)
  REGULAR_INSTRUCTION(imul, 0xaf, 0x0f)
  REGULAR_INSTRUCTION(bsr, 0xbd, 0x0f)
#undef REGULAR_INSTRUCTION
  RA(Q, movsxd, 0x63)
  RR(Q, movsxd, 0x63)
  AR(B, movb, 0x88)
  AR(L, movl, 0x89)
  AR(Q, movq, 0x89)
  AR(W, movw, 0x89)
  RA(B, movb, 0x8a)
  RA(L, movl, 0x8b)
  RA(Q, movq, 0x8b)
  RR(L, movl, 0x8b)
  RA(Q, leaq, 0x8d)
  RA(L, leal, 0x8d)
  AR(L, cmpxchgl, 0xb1, 0x0f)
  AR(Q, cmpxchgq, 0xb1, 0x0f)
  RA(L, cmpxchgl, 0xb1, 0x0f)
  RA(Q, cmpxchgq, 0xb1, 0x0f)
  RR(L, cmpxchgl, 0xb1, 0x0f)
  RR(Q, cmpxchgq, 0xb1, 0x0f)
  RA(L, movzbl, 0xb6, 0x0f)
  RR(L, movzbl, 0xb6, 0x0f)
  RA(Q, movzbq, 0xb6, 0x0f)
  RR(Q, movzbq, 0xb6, 0x0f)
  RA(Q, movzwq, 0xb7, 0x0f)
  RR(Q, movzwq, 0xb7, 0x0f)
  RA(Q, movsbq, 0xbe, 0x0f)
  RR(Q, movsbq, 0xbe, 0x0f)
  RA(Q, movswq, 0xbf, 0x0f)
  RR(Q, movswq, 0xbf, 0x0f)
#define DECLARE_CMOV(name, code)                                               \
  RR(Q, cmov##name##q, 0x40 + code, 0x0f)                                      \
  RR(L, cmov##name##l, 0x40 + code, 0x0f)                                      \
  RA(Q, cmov##name##q, 0x40 + code, 0x0f)                                      \
  RA(L, cmov##name##l, 0x40 + code, 0x0f)
  X86_CONDITIONAL_SUFFIXES(DECLARE_CMOV)
#undef DECLARE_CMOV
#undef AA
#undef RA
#undef AR

#define SIMPLE(name, ...)                                                      \
  void name() { emitSimple(__VA_ARGS__); }
  SIMPLE(cpuid, 0x0f, 0xa2)
  SIMPLE(fcos, 0xd9, 0xff)
  SIMPLE(fincstp, 0xd9, 0xf7)
  SIMPLE(fsin, 0xd9, 0xfe)
#undef SIMPLE
// XmmRegister operations with another register or an address.
#define XX(width, name, ...)                                                   \
  void name(XmmRegister dst, XmmRegister src) {                                \
    emit##width(dst, src, __VA_ARGS__);                                        \
  }
#define XA(width, name, ...)                                                   \
  void name(XmmRegister dst, Address src) {                                    \
    emit##width(dst, src, __VA_ARGS__);                                        \
  }
#define AX(width, name, ...)                                                   \
  void name(Address dst, XmmRegister src) {                                    \
    emit##width(src, dst, __VA_ARGS__);                                        \
  }
  // We could add movupd here, but movups does the same and is shorter.
  XA(L, movups, 0x10, 0x0f);
  XA(L, movsd, 0x10, 0x0f, 0xf2)
  XA(L, movss, 0x10, 0x0f, 0xf3)
  AX(L, movups, 0x11, 0x0f);
  AX(L, movsd, 0x11, 0x0f, 0xf2)
  AX(L, movss, 0x11, 0x0f, 0xf3)
  XX(L, movhlps, 0x12, 0x0f)
  XX(L, unpcklps, 0x14, 0x0f)
  XX(L, unpcklpd, 0x14, 0x0f, 0x66)
  XX(L, unpckhps, 0x15, 0x0f)
  XX(L, unpckhpd, 0x15, 0x0f, 0x66)
  XX(L, movlhps, 0x16, 0x0f)
  XX(L, movaps, 0x28, 0x0f)
  XX(L, comisd, 0x2f, 0x0f, 0x66)
#define DECLARE_XMM(name, code)                                                \
  XX(L, name##ps, 0x50 + code, 0x0f)                                           \
  XA(L, name##ps, 0x50 + code, 0x0f)                                           \
  AX(L, name##ps, 0x50 + code, 0x0f)                                           \
  XX(L, name##pd, 0x50 + code, 0x0f, 0x66)                                     \
  XA(L, name##pd, 0x50 + code, 0x0f, 0x66)                                     \
  AX(L, name##pd, 0x50 + code, 0x0f, 0x66)                                     \
  XX(L, name##sd, 0x50 + code, 0x0f, 0xf2)                                     \
  XA(L, name##sd, 0x50 + code, 0x0f, 0xf2)                                     \
  AX(L, name##sd, 0x50 + code, 0x0f, 0xf2)                                     \
  XX(L, name##ss, 0x50 + code, 0x0f, 0xf3)                                     \
  XA(L, name##ss, 0x50 + code, 0x0f, 0xf3)                                     \
  AX(L, name##ss, 0x50 + code, 0x0f, 0xf3)
  XMM_ALU_CODES(DECLARE_XMM)
#undef DECLARE_XMM
  XX(L, cvtps2pd, 0x5a, 0x0f)
  XX(L, cvtpd2ps, 0x5a, 0x0f, 0x66)
  XX(L, cvtsd2ss, 0x5a, 0x0f, 0xf2)
  XX(L, cvtss2sd, 0x5a, 0x0f, 0xf3)
  XX(L, pxor, 0xef, 0x0f, 0x66)
  XX(L, subpl, 0xfa, 0x0f, 0x66)
  XX(L, addpl, 0xfe, 0x0f, 0x66)
#undef XX
#undef AX
#undef XA

#define DECLARE_CMPPS(name, code)                                              \
  void cmpps##name(XmmRegister dst, XmmRegister src) {                         \
    emitL(dst, src, 0xc2, 0x0f);                                               \
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);                         \
    emitUint8(code);                                                           \
  }
  XMM_CONDITIONAL_CODES(DECLARE_CMPPS)
#undef DECLARE_CMPPS

#define DECLARE_SIMPLE(name, opcode)                                           \
  void name() { emitSimple(opcode); }
  X86_ZERO_OPERAND_1_BYTE_INSTRUCTIONS(DECLARE_SIMPLE)
#undef DECLARE_SIMPLE

  void movl(Register dst, Immediate imm);
  void movl(Address dst, Immediate imm);

  void movb(Address dst, Immediate imm);

  void movw(Register dst, Address src);
  void movw(Address dst, Immediate imm);

  void movq(Register dst, Immediate imm);
  void movq(Address dst, Immediate imm);

  // Destination and source are reversed for some reason.
  void movq(Register dst, XmmRegister src) {
    emitQ(src, dst, 0x7e, 0x0f, 0x66);
  }
  void movl(Register dst, XmmRegister src) {
    emitL(src, dst, 0x7e, 0x0f, 0x66);
  }
  void movss(XmmRegister dst, XmmRegister src) {
    emitL(src, dst, 0x11, 0x0f, 0xf3);
  }
  void movsd(XmmRegister dst, XmmRegister src) {
    emitL(src, dst, 0x11, 0x0f, 0xf2);
  }

  // Use the reversed operand order and the 0x89 bytecode instead of the
  // obvious 0x88 encoding for this some, because it is expected by gdb older
  // than 7.3.1 when disassembling a function's prologue (movq rbp, rsp).
  void movq(Register dst, Register src) { emitQ(src, dst, 0x89); }

  void movq(XmmRegister dst, Register src) {
    emitQ(dst, src, 0x6e, 0x0f, 0x66);
  }

  void movd(XmmRegister dst, Register src) {
    emitL(dst, src, 0x6e, 0x0f, 0x66);
  }
  void cvtsi2sdq(XmmRegister dst, Register src) {
    emitQ(dst, src, 0x2a, 0x0f, 0xf2);
  }
  void cvtsi2sdl(XmmRegister dst, Register src) {
    emitL(dst, src, 0x2a, 0x0f, 0xf2);
  }
  void cvttsd2siq(Register dst, XmmRegister src) {
    emitQ(dst, src, 0x2c, 0x0f, 0xf2);
  }
  void cvttsd2sil(Register dst, XmmRegister src) {
    emitL(dst, src, 0x2c, 0x0f, 0xf2);
  }
  void movmskpd(Register dst, XmmRegister src) {
    emitL(dst, src, 0x50, 0x0f, 0x66);
  }
  void movmskps(Register dst, XmmRegister src) { emitL(dst, src, 0x50, 0x0f); }

  void movsb();
  void movsl();
  void movsq();
  void movsw();
  void repMovsb();
  void repMovsl();
  void repMovsq();
  void repMovsw();
  void repnzMovsb();
  void repnzMovsl();
  void repnzMovsq();
  void repnzMovsw();

  void btl(Register dst, Register src) { emitL(src, dst, 0xa3, 0x0f); }
  void btq(Register dst, Register src) { emitQ(src, dst, 0xa3, 0x0f); }

  void notps(XmmRegister dst, XmmRegister src);
  void negateps(XmmRegister dst, XmmRegister src);
  void absps(XmmRegister dst, XmmRegister src);
  void zerowps(XmmRegister dst, XmmRegister src);

  void set1ps(XmmRegister dst, Register tmp, Immediate imm);
  void shufps(XmmRegister dst, XmmRegister src, Immediate mask);

  void negatepd(XmmRegister dst, XmmRegister src);
  void abspd(XmmRegister dst, XmmRegister src);
  void shufpd(XmmRegister dst, XmmRegister src, Immediate mask);

  enum RoundingMode {
    kRoundToNearest = 0x0,
    kRoundDown = 0x1,
    kRoundUp = 0x2,
    kRoundToZero = 0x3
  };
  void roundsd(XmmRegister dst, XmmRegister src, RoundingMode mode);

  void testb(Register dst, Register src);
  void testb(Register reg, Immediate imm);
  void testb(Address address, Immediate imm);
  void testb(Address address, Register reg);

  void testl(Register reg, Immediate imm) { testq(reg, imm); }

  // TODO(T47100904): These functions will emit a testl or a testb when possible
  // based on the value of `imm`. This behavior is desired in most cases, but
  // probably belongs in a differently-named function.
  void testq(Register reg, Immediate imm);
  void testq(Address address, Immediate imm);

  void shldq(Register dst, Register src, Register shifter) {
    DCHECK(shifter == RCX, "assert()");
    emitQ(src, dst, 0xa5, 0x0f);
  }
  void shrdq(Register dst, Register src, Register shifter) {
    DCHECK(shifter == RCX, "assert()");
    emitQ(src, dst, 0xad, 0x0f);
  }

#define DECLARE_ALU(op, c)                                                     \
  void op##w(Register dst, Register src) { emitW(dst, src, c * 8 + 3); }       \
  void op##l(Register dst, Register src) { emitL(dst, src, c * 8 + 3); }       \
  void op##q(Register dst, Register src) { emitQ(dst, src, c * 8 + 3); }       \
  void op##w(Register dst, Address src) { emitW(dst, src, c * 8 + 3); }        \
  void op##l(Register dst, Address src) { emitL(dst, src, c * 8 + 3); }        \
  void op##q(Register dst, Address src) { emitQ(dst, src, c * 8 + 3); }        \
  void op##w(Address dst, Register src) { emitW(src, dst, c * 8 + 1); }        \
  void op##l(Address dst, Register src) { emitL(src, dst, c * 8 + 1); }        \
  void op##q(Address dst, Register src) { emitQ(src, dst, c * 8 + 1); }        \
  void op##l(Register dst, Immediate imm) { aluL(c, dst, imm); }               \
  void op##q(Register dst, Immediate imm) { aluQ(c, c * 8 + 3, dst, imm); }    \
  void op##b(Register dst, Immediate imm) { aluB(c, dst, imm); }               \
  void op##b(Address dst, Immediate imm) { aluB(c, dst, imm); }                \
  void op##w(Address dst, Immediate imm) { aluW(c, dst, imm); }                \
  void op##l(Address dst, Immediate imm) { aluL(c, dst, imm); }                \
  void op##q(Address dst, Immediate imm) { aluQ(c, c * 8 + 3, dst, imm); }

  X86_ALU_CODES(DECLARE_ALU)

#undef DECLARE_ALU
#undef ALU_OPS

  void cqo();

#define REGULAR_UNARY(name, opcode, modrm)                                     \
  void name##q(Register reg) { emitUnaryQ(reg, opcode, modrm); }               \
  void name##l(Register reg) { emitUnaryL(reg, opcode, modrm); }               \
  void name##q(Address address) { emitUnaryQ(address, opcode, modrm); }        \
  void name##l(Address address) { emitUnaryL(address, opcode, modrm); }
  REGULAR_UNARY(not, 0xf7, 2)
  REGULAR_UNARY(neg, 0xf7, 3)
  REGULAR_UNARY(mul, 0xf7, 4)
  REGULAR_UNARY(div, 0xf7, 6)
  REGULAR_UNARY(idiv, 0xf7, 7)
  REGULAR_UNARY(inc, 0xff, 0)
  REGULAR_UNARY(dec, 0xff, 1)
#undef REGULAR_UNARY

  void imull(Register reg, Immediate imm);
  void imulq(Register dst, Immediate imm);

  void shll(Register reg, Immediate imm);
  void shll(Register operand, Register shifter);
  void shrl(Register reg, Immediate imm);
  void shrl(Register operand, Register shifter);
  void sarl(Register reg, Immediate imm);
  void sarl(Register operand, Register shifter);
  void shldl(Register dst, Register src, Immediate imm);

  void shlq(Register reg, Immediate imm);
  void shlq(Register operand, Register shifter);
  void shrq(Register reg, Immediate imm);
  void shrq(Register operand, Register shifter);
  void sarq(Register reg, Immediate imm);
  void sarq(Register operand, Register shifter);
  void shldq(Register dst, Register src, Immediate imm);

  void btq(Register base, int bit);

  void enter(Immediate imm);

  void fldl(Address src);
  void fstpl(Address dst);

  void ffree(word value);

  // 'size' indicates size in bytes and must be in the range 1..8.
  void nop(int size = 1);

  // 'size' may be arbitrarily large, and multiple nops will be used if needed.
  void nops(int size);

  void ud2();

  void jcc(Condition condition, Label* label, bool near);
  void jmp(Register reg) { emitUnaryL(reg, 0xff, 4); }
  void jmp(Address address) { emitUnaryL(address, 0xff, 4); }
  void jmp(Label* label, bool near);

  void lockCmpxchgq(Address address, Register reg) {
    emitUint8(LOCK);
    cmpxchgq(address, reg);
  }

  void lockCmpxchgl(Address address, Register reg) {
    emitUint8(LOCK);
    cmpxchgl(address, reg);
  }

  void align(int alignment);
  void bind(Label* label);

  // Debugging and bringup support.
  void breakpoint() { int3(); }

  static void initializeMemoryWithBreakpoints(uword data, word length);

 private:
  void aluB(uint8_t modrm_opcode, Register dst, Immediate imm);
  void aluL(uint8_t modrm_opcode, Register dst, Immediate imm);
  void aluB(uint8_t modrm_opcode, Address dst, Immediate imm);
  void aluW(uint8_t modrm_opcode, Address dst, Immediate imm);
  void aluL(uint8_t modrm_opcode, Address dst, Immediate imm);
  void aluQ(uint8_t modrm_opcode, uint8_t opcode, Register dst, Immediate imm);
  void aluQ(uint8_t modrm_opcode, uint8_t opcode, Address dst, Immediate imm);

  void emitSimple(int opcode, int opcode2 = -1);
  void emitUnaryQ(Register reg, int opcode, int modrm_code);
  void emitUnaryL(Register reg, int opcode, int modrm_code);
  void emitUnaryQ(Address address, int opcode, int modrm_code);
  void emitUnaryL(Address address, int opcode, int modrm_code);
  // The prefixes are in reverse order due to the rules of default arguments in
  // C++.
  void emitQ(int reg, Address address, int opcode, int prefix2 = -1,
             int prefix1 = -1);
  void emitL(int reg, Address address, int opcode, int prefix2 = -1,
             int prefix1 = -1);
  void emitB(Register reg, Address address, int opcode, int prefix2 = -1,
             int prefix1 = -1);
  void emitW(Register reg, Address address, int opcode, int prefix2 = -1,
             int prefix1 = -1);
  void emitQ(int dst, int src, int opcode, int prefix2 = -1, int prefix1 = -1);
  void emitL(int dst, int src, int opcode, int prefix2 = -1, int prefix1 = -1);
  void emitW(Register dst, Register src, int opcode, int prefix2 = -1,
             int prefix1 = -1);
  void cmpPS(XmmRegister dst, XmmRegister src, int condition);
  void emitTestB(Operand operand, Immediate imm);
  void emitTestQ(Operand operand, Immediate imm);

  void emitUint8(uint8_t value);
  void emitInt32(int32_t value);
  void emitUInt32(uint32_t value);
  void emitInt64(int64_t value);

  static uint8_t byteRegisterREX(Register reg);
  static uint8_t byteOperandREX(Operand operand);

  void emitRegisterREX(Register reg, uint8_t rex);
  void emitOperandREX(int rm, Operand operand, uint8_t rex);
  void emitRegisterOperand(int rm, int reg);
  void emitFixup(AssemblerFixup* fixup);
  void emitOperandSizeOverride();
  void emitRegRegREX(int reg, int base, uint8_t rex = REX_NONE);
  void emitOperand(int rm, Operand operand);
  void emitImmediate(Immediate imm);
  void emitComplexB(int rm, Operand operand, Immediate imm);
  void emitComplex(int rm, Operand operand, Immediate immediate);
  void emitSignExtendedInt8(int rm, Operand operand, Immediate immediate);
  void emitLabel(Label* label, word instruction_size);
  void emitLabelLink(Label* label);
  void emitNearLabelLink(Label* label);

  void emitGenericShift(bool wide, int rm, Register reg, Immediate imm);
  void emitGenericShift(bool wide, int rm, Register operand, Register shifter);

  AssemblerBuffer buffer_;  // Contains position independent code.

  DISALLOW_COPY_AND_ASSIGN(Assembler);
};

inline void Assembler::emitUint8(uint8_t value) {
  buffer_.emit<uint8_t>(value);
}

inline void Assembler::emitInt32(int32_t value) {
  buffer_.emit<int32_t>(value);
}

inline void Assembler::emitUInt32(uint32_t value) {
  buffer_.emit<uint32_t>(value);
}

inline void Assembler::emitInt64(int64_t value) {
  buffer_.emit<int64_t>(value);
}

inline uint8_t Assembler::byteRegisterREX(Register reg) {
  // SPL, BPL, SIL, or DIL require a REX prefix.
  return reg >= RSP && reg <= RDI ? REX_PREFIX : REX_NONE;
}

inline uint8_t Assembler::byteOperandREX(Operand operand) {
  return operand.isRegister() ? byteRegisterREX(operand.reg())
                              : uint8_t{REX_NONE};
}

inline void Assembler::emitRegisterREX(Register reg, uint8_t rex) {
  DCHECK(reg != kNoRegister && reg <= R15, "assert()");
  DCHECK(rex == REX_NONE || rex == REX_W, "assert()");
  rex |= (reg > 7 ? REX_B : REX_NONE);
  if (rex != REX_NONE) emitUint8(REX_PREFIX | rex);
}

inline void Assembler::emitOperandREX(int rm, Operand operand, uint8_t rex) {
  rex |= (rm > 7 ? REX_R : REX_NONE) | operand.rex();
  if (rex != REX_NONE) emitUint8(REX_PREFIX | rex);
}

inline void Assembler::emitRegRegREX(int reg, int base, uint8_t rex) {
  DCHECK(reg != kNoRegister && reg <= R15, "assert()");
  DCHECK(base != kNoRegister && base <= R15, "assert()");
  DCHECK(rex == REX_NONE || rex == REX_W || rex == REX_PREFIX, "assert()");
  if (reg > 7) rex |= REX_R;
  if (base > 7) rex |= REX_B;
  if (rex != REX_NONE) emitUint8(REX_PREFIX | rex);
}

inline void Assembler::emitFixup(AssemblerFixup* fixup) {
  buffer_.emitFixup(fixup);
}

inline void Assembler::emitOperandSizeOverride() { emitUint8(0x66); }

}  // namespace x64
}  // namespace py
