// Copyright (c) 2013, the Dart project authors and Facebook, Inc. and its
// affiliates. Please see the AUTHORS-Dart file for details. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE-Dart file.

// A combined disassembler for IA32 and X64.

#include "disassembler.h"

#include <cinttypes>
#include <cstdarg>

#include "assembler-x64.h"

namespace py {

namespace x64 {

enum OperandType {
  UNSET_OP_ORDER = 0,
  // Operand size decides between 16, 32 and 64 bit operands.
  REG_OPER_OP_ORDER = 1,  // Register destination, operand source.
  OPER_REG_OP_ORDER = 2,  // Operand destination, register source.
  // Fixed 8-bit operands.
  BYTE_SIZE_OPERAND_FLAG = 4,
  BYTE_REG_OPER_OP_ORDER = REG_OPER_OP_ORDER | BYTE_SIZE_OPERAND_FLAG,
  BYTE_OPER_REG_OP_ORDER = OPER_REG_OP_ORDER | BYTE_SIZE_OPERAND_FLAG
};

//------------------------------------------------------------------
// Tables
//------------------------------------------------------------------
struct ByteMnemonic {
  int b;  // -1 terminates, otherwise must be in range (0..255)
  OperandType op_order_;
  const char* mnem;
};

#define ALU_ENTRY(name, code)                                                  \
  {code * 8 + 0, BYTE_OPER_REG_OP_ORDER, #name},                               \
      {code * 8 + 1, OPER_REG_OP_ORDER, #name},                                \
      {code * 8 + 2, BYTE_REG_OPER_OP_ORDER, #name},                           \
      {code * 8 + 3, REG_OPER_OP_ORDER, #name},
static const ByteMnemonic kTwoOperandInstructions[] = {
    X86_ALU_CODES(ALU_ENTRY){0x63, REG_OPER_OP_ORDER, "movsxd"},
    {0x84, BYTE_REG_OPER_OP_ORDER, "test"},
    {0x85, REG_OPER_OP_ORDER, "test"},
    {0x86, BYTE_REG_OPER_OP_ORDER, "xchg"},
    {0x87, REG_OPER_OP_ORDER, "xchg"},
    {0x88, BYTE_OPER_REG_OP_ORDER, "mov"},
    {0x89, OPER_REG_OP_ORDER, "mov"},
    {0x8A, BYTE_REG_OPER_OP_ORDER, "mov"},
    {0x8B, REG_OPER_OP_ORDER, "mov"},
    {0x8D, REG_OPER_OP_ORDER, "lea"},
    {-1, UNSET_OP_ORDER, ""}};

#define ZERO_OPERAND_ENTRY(name, opcode) {opcode, UNSET_OP_ORDER, #name},
static const ByteMnemonic kZeroOperandInstructions[] = {
    X86_ZERO_OPERAND_1_BYTE_INSTRUCTIONS(ZERO_OPERAND_ENTRY){-1, UNSET_OP_ORDER,
                                                             ""}};

static const ByteMnemonic kCallJumpInstructions[] = {
    {0xE8, UNSET_OP_ORDER, "call"},
    {0xE9, UNSET_OP_ORDER, "jmp"},
    {-1, UNSET_OP_ORDER, ""}};

#define SHORT_IMMEDIATE_ENTRY(name, code) {code * 8 + 5, UNSET_OP_ORDER, #name},
static const ByteMnemonic kShortImmediateInstructions[] = {
    X86_ALU_CODES(SHORT_IMMEDIATE_ENTRY){-1, UNSET_OP_ORDER, ""}};

static const char* const kConditionalCodeSuffix[] = {
#define STRINGIFY(name, number) #name,
    X86_CONDITIONAL_SUFFIXES(STRINGIFY)
#undef STRINGIFY
};

#define STRINGIFY_NAME(name, code) #name,
static const char* const kXmmConditionalCodeSuffix[] = {
    XMM_CONDITIONAL_CODES(STRINGIFY_NAME)};
#undef STRINGIFY_NAME

enum InstructionType {
  NO_INSTR,
  ZERO_OPERANDS_INSTR,
  TWO_OPERANDS_INSTR,
  JUMP_CONDITIONAL_SHORT_INSTR,
  REGISTER_INSTR,
  PUSHPOP_INSTR,  // Has implicit 64-bit operand size.
  MOVE_REG_INSTR,
  CALL_JUMP_INSTR,
  SHORT_IMMEDIATE_INSTR
};

enum Prefixes {
  ESCAPE_PREFIX = 0x0F,
  OPERAND_SIZE_OVERRIDE_PREFIX = 0x66,
  ADDRESS_SIZE_OVERRIDE_PREFIX = 0x67,
  REPNE_PREFIX = 0xF2,
  REP_PREFIX = 0xF3,
  REPEQ_PREFIX = REP_PREFIX
};

struct XmmMnemonic {
  const char* ps_name;
  const char* pd_name;
  const char* ss_name;
  const char* sd_name;
};

#define XMM_INSTRUCTION_ENTRY(name, code)                                      \
  {#name "ps", #name "pd", #name "ss", #name "sd"},
static const XmmMnemonic kXmmInstructions[] = {
    XMM_ALU_CODES(XMM_INSTRUCTION_ENTRY)};

struct InstructionDesc {
  const char* mnem;
  InstructionType type;
  OperandType op_order_;
  bool byte_size_operation;  // Fixed 8-bit operation.
};

class InstructionTable {
 public:
  InstructionTable();
  const InstructionDesc& get(uint8_t x) const { return instructions_[x]; }

 private:
  InstructionDesc instructions_[256];
  void clear();
  void init();
  void copyTable(const ByteMnemonic bm[], InstructionType type);
  void setTableRange(InstructionType type, uint8_t start, uint8_t end,
                     bool byte_size, const char* mnem);
  void addJumpConditionalShort();

  DISALLOW_COPY_AND_ASSIGN(InstructionTable);
};

InstructionTable::InstructionTable() {
  clear();
  init();
}

void InstructionTable::clear() {
  for (int i = 0; i < 256; i++) {
    instructions_[i].mnem = "(bad)";
    instructions_[i].type = NO_INSTR;
    instructions_[i].op_order_ = UNSET_OP_ORDER;
    instructions_[i].byte_size_operation = false;
  }
}

void InstructionTable::init() {
  copyTable(kTwoOperandInstructions, TWO_OPERANDS_INSTR);
  copyTable(kZeroOperandInstructions, ZERO_OPERANDS_INSTR);
  copyTable(kCallJumpInstructions, CALL_JUMP_INSTR);
  copyTable(kShortImmediateInstructions, SHORT_IMMEDIATE_INSTR);
  addJumpConditionalShort();
  setTableRange(PUSHPOP_INSTR, 0x50, 0x57, false, "push");
  setTableRange(PUSHPOP_INSTR, 0x58, 0x5F, false, "pop");
  setTableRange(MOVE_REG_INSTR, 0xB8, 0xBF, false, "mov");
}

void InstructionTable::copyTable(const ByteMnemonic bm[],
                                 InstructionType type) {
  for (int i = 0; bm[i].b >= 0; i++) {
    InstructionDesc* id = &instructions_[bm[i].b];
    id->mnem = bm[i].mnem;
    OperandType op_order = bm[i].op_order_;
    id->op_order_ =
        static_cast<OperandType>(op_order & ~BYTE_SIZE_OPERAND_FLAG);
    DCHECK(NO_INSTR == id->type, "");  // Information not already entered
    id->type = type;
    id->byte_size_operation = ((op_order & BYTE_SIZE_OPERAND_FLAG) != 0);
  }
}

void InstructionTable::setTableRange(InstructionType type, uint8_t start,
                                     uint8_t end, bool byte_size,
                                     const char* mnem) {
  for (uint8_t b = start; b <= end; b++) {
    InstructionDesc* id = &instructions_[b];
    DCHECK(NO_INSTR == id->type, "");  // Information not already entered
    id->mnem = mnem;
    id->type = type;
    id->byte_size_operation = byte_size;
  }
}

void InstructionTable::addJumpConditionalShort() {
  for (uint8_t b = 0x70; b <= 0x7F; b++) {
    InstructionDesc* id = &instructions_[b];
    DCHECK(NO_INSTR == id->type, "");  // Information not already entered
    id->mnem = nullptr;                // Computed depending on condition code.
    id->type = JUMP_CONDITIONAL_SHORT_INSTR;
  }
}

static InstructionTable instruction_table;

static InstructionDesc cmov_instructions[16] = {
    {"cmovo", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false},
    {"cmovno", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false},
    {"cmovc", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false},
    {"cmovnc", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false},
    {"cmovz", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false},
    {"cmovnz", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false},
    {"cmovna", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false},
    {"cmova", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false},
    {"cmovs", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false},
    {"cmovns", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false},
    {"cmovpe", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false},
    {"cmovpo", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false},
    {"cmovl", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false},
    {"cmovge", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false},
    {"cmovle", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false},
    {"cmovg", TWO_OPERANDS_INSTR, REG_OPER_OP_ORDER, false}};

//-------------------------------------------------
// DisassemblerX64 implementation.

static const char* kRegisterNames[] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
};

static_assert(ARRAYSIZE(kRegisterNames) == kNumRegisters,
              "register count mismatch");

static const char* kXmmRegisterNames[kNumXmmRegisters] = {
    "xmm0", "xmm1", "xmm2",  "xmm3",  "xmm4",  "xmm5",  "xmm6",  "xmm7",
    "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"};

static_assert(ARRAYSIZE(kXmmRegisterNames) == kNumXmmRegisters,
              "register count mismatch");

class DisassemblerX64 {
 public:
  DisassemblerX64(char* buffer, intptr_t buffer_size)
      : buffer_(buffer),
        buffer_size_(buffer_size),
        buffer_pos_(0),
        rex_(0),
        operand_size_(0),
        group_1_prefix_(0),
        byte_size_operand_(false) {
    buffer_[buffer_pos_] = '\0';
  }

  virtual ~DisassemblerX64() {}

  int instructionDecode(uword pc);

 private:
  enum OperandSize {
    BYTE_SIZE = 0,
    WORD_SIZE = 1,
    DOUBLEWORD_SIZE = 2,
    QUADWORD_SIZE = 3
  };

  void setRex(uint8_t rex) {
    DCHECK(0x40 == (rex & 0xF0), "");
    rex_ = rex;
  }

  bool rex() { return rex_ != 0; }

  bool rexB() { return (rex_ & 0x01) != 0; }

  // Actual number of base register given the low bits and the rex.b state.
  int baseReg(int low_bits) { return low_bits | ((rex_ & 0x01) << 3); }

  bool rexX() { return (rex_ & 0x02) != 0; }

  bool rexR() { return (rex_ & 0x04) != 0; }

  bool rexW() { return (rex_ & 0x08) != 0; }

  OperandSize operandSize() {
    if (byte_size_operand_) return BYTE_SIZE;
    if (rexW()) return QUADWORD_SIZE;
    if (operand_size_ != 0) return WORD_SIZE;
    return DOUBLEWORD_SIZE;
  }

  const char* operandSizeCode() { return &"b\0w\0l\0q\0"[2 * operandSize()]; }

  // Disassembler helper functions.
  void getModRM(uint8_t data, int* mod, int* regop, int* rm) {
    *mod = (data >> 6) & 3;
    *regop = ((data & 0x38) >> 3) | (rexR() ? 8 : 0);
    *rm = (data & 7) | (rexB() ? 8 : 0);
    DCHECK(*rm < kNumRegisters, "");
  }

  void getSIB(uint8_t data, int* scale, int* index, int* base) {
    *scale = (data >> 6) & 3;
    *index = ((data >> 3) & 7) | (rexX() ? 8 : 0);
    *base = (data & 7) | (rexB() ? 8 : 0);
    DCHECK(*base < kNumRegisters, "");
  }

  const char* nameOfCPURegister(int reg) const { return kRegisterNames[reg]; }

  const char* nameOfByteCPURegister(int reg) const {
    return nameOfCPURegister(reg);
  }

  // A way to get rax or eax's name.
  const char* rax() const { return nameOfCPURegister(0); }

  const char* nameOfXMMRegister(int reg) const {
    DCHECK((0 <= reg) && (reg < kNumXmmRegisters), "");
    return kXmmRegisterNames[reg];
  }

  void print(const char* format, ...);
  void printJump(uint8_t* pc, int32_t disp);
  void printAddress(uint8_t* addr);

  int printOperands(const char* mnem, OperandType op_order, uint8_t* data);

  typedef const char* (DisassemblerX64::*RegisterNameMapping)(int reg) const;

  int printRightOperandHelper(uint8_t* modrmp,
                              RegisterNameMapping register_name);
  int printRightOperand(uint8_t* modrmp);
  int printRightByteOperand(uint8_t* modrmp);
  int printRightXMMOperand(uint8_t* modrmp);
  void printDisp(int disp, const char* after);
  int printImmediate(uint8_t* data, OperandSize size, bool sign_extend = false);
  void printImmediateValue(int64_t value, bool signed_value = false,
                           int byte_count = -1);
  int printImmediateOp(uint8_t* data);
  const char* twoByteMnemonic(uint8_t opcode);
  int twoByteOpcodeInstruction(uint8_t* data);
  int print660F38Instruction(uint8_t* data);

  int f6F7Instruction(uint8_t* data);
  int shiftInstruction(uint8_t* data);
  int jumpShort(uint8_t* data);
  int jumpConditional(uint8_t* data);
  int jumpConditionalShort(uint8_t* data);
  int setCC(uint8_t* data);
  int fPUInstruction(uint8_t* data);
  int memoryFPUInstruction(int escape_opcode, int regop, uint8_t* modrm_start);
  int registerFPUInstruction(int escape_opcode, uint8_t modrm_byte);

  bool decodeInstructionType(uint8_t** data);

  void unimplementedInstruction() { UNREACHABLE("unimplemented instruction"); }

  char* buffer_;          // Decode instructions into this buffer.
  intptr_t buffer_size_;  // The size of the buffer_.
  intptr_t buffer_pos_;   // Current character position in the buffer_.

  // Prefixes parsed
  uint8_t rex_;
  uint8_t operand_size_;  // 0x66 or (if no group 3 prefix is present) 0x0.
  // 0xF2, 0xF3, or (if no group 1 prefix is present) 0.
  uint8_t group_1_prefix_;
  // Byte size operand override.
  bool byte_size_operand_;

  DISALLOW_COPY_AND_ASSIGN(DisassemblerX64);
};

// Append the str to the output buffer.
void DisassemblerX64::print(const char* format, ...) {
  intptr_t available = buffer_size_ - buffer_pos_;
  if (available <= 1) {
    DCHECK(buffer_[buffer_pos_] == '\0', "");
    return;
  }
  char* buf = buffer_ + buffer_pos_;
  va_list args;
  va_start(args, format);
  int length = std::vsnprintf(buf, available, format, args);
  va_end(args);
  buffer_pos_ =
      (length >= available) ? (buffer_size_ - 1) : (buffer_pos_ + length);
  DCHECK(buffer_pos_ < buffer_size_, "");
}

template <typename T>
static inline T LoadUnaligned(const T* ptr) {
  return *ptr;
}

int DisassemblerX64::printRightOperandHelper(
    uint8_t* modrmp, RegisterNameMapping direct_register_name) {
  int mod, regop, rm;
  getModRM(*modrmp, &mod, &regop, &rm);
  RegisterNameMapping register_name =
      (mod == 3) ? direct_register_name : &DisassemblerX64::nameOfCPURegister;
  switch (mod) {
    case 0:
      if ((rm & 7) == 5) {
        int32_t disp = Utils::readBytes<int32_t>(modrmp + 1);
        print("[rip");
        printDisp(disp, "]");
        return 5;
      }
      if ((rm & 7) == 4) {
        // Codes for SIB byte.
        uint8_t sib = *(modrmp + 1);
        int scale, index, base;
        getSIB(sib, &scale, &index, &base);
        if (index == 4 && (base & 7) == 4 && scale == 0 /*times_1*/) {
          // index == rsp means no index. Only use sib byte with no index for
          // rsp and r12 base.
          print("[%s]", nameOfCPURegister(base));
          return 2;
        }
        if (base == 5) {
          // base == rbp means no base register (when mod == 0).
          int32_t disp = LoadUnaligned(reinterpret_cast<int32_t*>(modrmp + 2));
          print("[%s*%d", nameOfCPURegister(index), 1 << scale);
          printDisp(disp, "]");
          return 6;
        }
        if (index != 4 && base != 5) {
          // [base+index*scale]
          print("[%s+%s*%d]", nameOfCPURegister(base), nameOfCPURegister(index),
                1 << scale);
          return 2;
        }
        unimplementedInstruction();
        return 1;
      }
      print("[%s]", nameOfCPURegister(rm));
      return 1;
      break;
    case 1:
      FALLTHROUGH;
    case 2:
      if ((rm & 7) == 4) {
        uint8_t sib = *(modrmp + 1);
        int scale, index, base;
        getSIB(sib, &scale, &index, &base);
        int disp = (mod == 2)
                       ? LoadUnaligned(reinterpret_cast<int32_t*>(modrmp + 2))
                       : *reinterpret_cast<int8_t*>(modrmp + 2);
        if (index == 4 && (base & 7) == 4 && scale == 0 /*times_1*/) {
          print("[%s", nameOfCPURegister(base));
          printDisp(disp, "]");
        } else {
          print("[%s+%s*%d", nameOfCPURegister(base), nameOfCPURegister(index),
                1 << scale);
          printDisp(disp, "]");
        }
        return mod == 2 ? 6 : 3;
      } else {
        // No sib.
        int disp = (mod == 2)
                       ? LoadUnaligned(reinterpret_cast<int32_t*>(modrmp + 1))
                       : *reinterpret_cast<int8_t*>(modrmp + 1);
        print("[%s", nameOfCPURegister(rm));
        printDisp(disp, "]");
        return (mod == 2) ? 5 : 2;
      }
      break;
    case 3:
      print("%s", (this->*register_name)(rm));
      return 1;
    default:
      unimplementedInstruction();
      return 1;
  }
  UNREACHABLE("unreachable");
}

int DisassemblerX64::printImmediate(uint8_t* data, OperandSize size,
                                    bool sign_extend) {
  int64_t value;
  int count;
  switch (size) {
    case BYTE_SIZE:
      if (sign_extend) {
        value = *reinterpret_cast<int8_t*>(data);
      } else {
        value = *data;
      }
      count = 1;
      break;
    case WORD_SIZE:
      if (sign_extend) {
        value = LoadUnaligned(reinterpret_cast<int16_t*>(data));
      } else {
        value = LoadUnaligned(reinterpret_cast<uint16_t*>(data));
      }
      count = 2;
      break;
    case DOUBLEWORD_SIZE:
    case QUADWORD_SIZE:
      if (sign_extend) {
        value = LoadUnaligned(reinterpret_cast<int32_t*>(data));
      } else {
        value = LoadUnaligned(reinterpret_cast<uint32_t*>(data));
      }
      count = 4;
      break;
    default:
      UNREACHABLE("unreachable");
      value = 0;  // Initialize variables on all paths to satisfy the compiler.
      count = 0;
  }
  printImmediateValue(value, sign_extend, count);
  return count;
}

void DisassemblerX64::printImmediateValue(int64_t value, bool signed_value,
                                          int byte_count) {
  if ((value >= 0) && (value <= 9)) {
    print("%" PRId64, value);
  } else if (signed_value && (value < 0) && (value >= -9)) {
    print("-%" PRId64, -value);
  } else {
    if (byte_count == 1) {
      int8_t v8 = static_cast<int8_t>(value);
      if (v8 < 0 && signed_value) {
        print("-%#" PRIX32, -static_cast<uint8_t>(v8));
      } else {
        print("%#" PRIX32, static_cast<uint8_t>(v8));
      }
    } else if (byte_count == 2) {
      int16_t v16 = static_cast<int16_t>(value);
      if (v16 < 0 && signed_value) {
        print("-%#" PRIX32, -static_cast<uint16_t>(v16));
      } else {
        print("%#" PRIX32, static_cast<uint16_t>(v16));
      }
    } else if (byte_count == 4) {
      int32_t v32 = static_cast<int32_t>(value);
      if (v32 < 0 && signed_value) {
        print("-%#010" PRIX32, -static_cast<uint32_t>(v32));
      } else {
        if (v32 > 0xffff) {
          print("%#010" PRIX32, v32);
        } else {
          print("%#" PRIX32, v32);
        }
      }
    } else if (byte_count == 8) {
      int64_t v64 = static_cast<int64_t>(value);
      if (v64 < 0 && signed_value) {
        print("-%#018" PRIX64, -static_cast<uint64_t>(v64));
      } else {
        if (v64 > 0xffffffffll) {
          print("%#018" PRIX64, v64);
        } else {
          print("%#" PRIX64, v64);
        }
      }
    } else {
      // Natural-sized immediates.
      if (value < 0 && signed_value) {
        print("-%#" PRIX64, -value);
      } else {
        print("%#" PRIX64, value);
      }
    }
  }
}

void DisassemblerX64::printDisp(int disp, const char* after) {
  if (-disp > 0) {
    print("-%#x", -disp);
  } else {
    print("+%#x", disp);
  }
  if (after != nullptr) print("%s", after);
}

// Returns number of bytes used by machine instruction, including *data byte.
// Writes immediate instructions to 'tmp_buffer_'.
int DisassemblerX64::printImmediateOp(uint8_t* data) {
  bool byte_size_immediate = (*data & 0x03) != 1;
  uint8_t modrm = *(data + 1);
  int mod, regop, rm;
  getModRM(modrm, &mod, &regop, &rm);
  const char* mnem = "Imm???";
  switch (regop) {
    case 0:
      mnem = "add";
      break;
    case 1:
      mnem = "or";
      break;
    case 2:
      mnem = "adc";
      break;
    case 3:
      mnem = "sbb";
      break;
    case 4:
      mnem = "and";
      break;
    case 5:
      mnem = "sub";
      break;
    case 6:
      mnem = "xor";
      break;
    case 7:
      mnem = "cmp";
      break;
    default:
      unimplementedInstruction();
  }
  print("%s%s ", mnem, operandSizeCode());
  int count = printRightOperand(data + 1);
  print(",");
  OperandSize immediate_size = byte_size_immediate ? BYTE_SIZE : operandSize();
  count +=
      printImmediate(data + 1 + count, immediate_size, byte_size_immediate);
  return 1 + count;
}

// Returns number of bytes used, including *data.
int DisassemblerX64::f6F7Instruction(uint8_t* data) {
  DCHECK(*data == 0xF7 || *data == 0xF6, "");
  uint8_t modrm = *(data + 1);
  int mod, regop, rm;
  getModRM(modrm, &mod, &regop, &rm);
  static const char* mnemonics[] = {"test", nullptr, "not", "neg",
                                    "mul",  "imul",  "div", "idiv"};
  const char* mnem = mnemonics[regop];
  if (mod == 3 && regop != 0) {
    if (regop > 3) {
      // These are instructions like idiv that implicitly use EAX and EDX as a
      // source and destination. We make this explicit in the disassembly.
      print("%s%s (%s,%s),%s", mnem, operandSizeCode(), rax(),
            nameOfCPURegister(2), nameOfCPURegister(rm));
    } else {
      print("%s%s %s", mnem, operandSizeCode(), nameOfCPURegister(rm));
    }
    return 2;
  }
  if (regop == 0) {
    print("test%s ", operandSizeCode());
    int count = printRightOperand(data + 1);  // Use name of 64-bit register.
    print(",");
    count += printImmediate(data + 1 + count, operandSize());
    return 1 + count;
  }
  if (regop >= 4) {
    print("%s%s (%s,%s),", mnem, operandSizeCode(), rax(),
          nameOfCPURegister(2));
    return 1 + printRightOperand(data + 1);
  }
  unimplementedInstruction();
  return 2;
}

int DisassemblerX64::shiftInstruction(uint8_t* data) {
  // C0/C1: Shift Imm8
  // D0/D1: Shift 1
  // D2/D3: Shift CL
  uint8_t op = *data & (~1);
  if (op != 0xD0 && op != 0xD2 && op != 0xC0) {
    unimplementedInstruction();
    return 1;
  }
  uint8_t* modrm = data + 1;
  int mod, regop, rm;
  getModRM(*modrm, &mod, &regop, &rm);
  regop &= 0x7;  // The REX.R bit does not affect the operation.
  int num_bytes = 1;
  const char* mnem = nullptr;
  switch (regop) {
    case 0:
      mnem = "rol";
      break;
    case 1:
      mnem = "ror";
      break;
    case 2:
      mnem = "rcl";
      break;
    case 3:
      mnem = "rcr";
      break;
    case 4:
      mnem = "shl";
      break;
    case 5:
      mnem = "shr";
      break;
    case 7:
      mnem = "sar";
      break;
    default:
      unimplementedInstruction();
      return num_bytes;
  }
  DCHECK(nullptr != mnem, "");
  print("%s%s ", mnem, operandSizeCode());
  if (byte_size_operand_) {
    num_bytes += printRightByteOperand(modrm);
  } else {
    num_bytes += printRightOperand(modrm);
  }

  if (op == 0xD0) {
    print(",1");
  } else if (op == 0xC0) {
    uint8_t imm8 = *(data + num_bytes);
    print(",%d", imm8);
    num_bytes++;
  } else {
    DCHECK(op == 0xD2, "");
    print(",cl");
  }
  return num_bytes;
}

int DisassemblerX64::printRightOperand(uint8_t* modrmp) {
  return printRightOperandHelper(modrmp, &DisassemblerX64::nameOfCPURegister);
}

int DisassemblerX64::printRightByteOperand(uint8_t* modrmp) {
  return printRightOperandHelper(modrmp,
                                 &DisassemblerX64::nameOfByteCPURegister);
}

int DisassemblerX64::printRightXMMOperand(uint8_t* modrmp) {
  return printRightOperandHelper(modrmp, &DisassemblerX64::nameOfXMMRegister);
}

// Returns number of bytes used including the current *data.
// Writes instruction's mnemonic, left and right operands to 'tmp_buffer_'.
int DisassemblerX64::printOperands(const char* mnem, OperandType op_order,
                                   uint8_t* data) {
  uint8_t modrm = *data;
  int mod, regop, rm;
  getModRM(modrm, &mod, &regop, &rm);
  int advance = 0;
  const char* register_name = byte_size_operand_ ? nameOfByteCPURegister(regop)
                                                 : nameOfCPURegister(regop);
  switch (op_order) {
    case REG_OPER_OP_ORDER: {
      print("%s%s %s,", mnem, operandSizeCode(), register_name);
      advance = byte_size_operand_ ? printRightByteOperand(data)
                                   : printRightOperand(data);
      break;
    }
    case OPER_REG_OP_ORDER: {
      print("%s%s ", mnem, operandSizeCode());
      advance = byte_size_operand_ ? printRightByteOperand(data)
                                   : printRightOperand(data);
      print(",%s", register_name);
      break;
    }
    default:
      UNREACHABLE("unreachable");
      break;
  }
  return advance;
}

void DisassemblerX64::printJump(uint8_t* pc, int32_t disp) {
  // TODO(emacs): Support relative disassembly flag.
  bool flag_disassemble_relative = false;
  if (flag_disassemble_relative) {
    print("%+d", disp);
  } else {
    printAddress(
        reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(pc) + disp));
  }
}

void DisassemblerX64::printAddress(uint8_t* addr_byte_ptr) {
  print("%#018" PRIX64 "", reinterpret_cast<uint64_t>(addr_byte_ptr));

  // TODO(emacs): Add support for mapping from address to stub name.
}

// Returns number of bytes used, including *data.
int DisassemblerX64::jumpShort(uint8_t* data) {
  DCHECK(0xEB == *data, "");
  uint8_t b = *(data + 1);
  int32_t disp = static_cast<int8_t>(b) + 2;
  print("jmp ");
  printJump(data, disp);
  return 2;
}

// Returns number of bytes used, including *data.
int DisassemblerX64::jumpConditional(uint8_t* data) {
  DCHECK(0x0F == *data, "");
  uint8_t cond = *(data + 1) & 0x0F;
  int32_t disp = LoadUnaligned(reinterpret_cast<int32_t*>(data + 2)) + 6;
  const char* mnem = kConditionalCodeSuffix[cond];
  print("j%s ", mnem);
  printJump(data, disp);
  return 6;  // includes 0x0F
}

// Returns number of bytes used, including *data.
int DisassemblerX64::jumpConditionalShort(uint8_t* data) {
  uint8_t cond = *data & 0x0F;
  uint8_t b = *(data + 1);
  int32_t disp = static_cast<int8_t>(b) + 2;
  const char* mnem = kConditionalCodeSuffix[cond];
  print("j%s ", mnem);
  printJump(data, disp);
  return 2;
}

// Returns number of bytes used, including *data.
int DisassemblerX64::setCC(uint8_t* data) {
  DCHECK(0x0F == *data, "");
  uint8_t cond = *(data + 1) & 0x0F;
  const char* mnem = kConditionalCodeSuffix[cond];
  print("set%s%s ", mnem, operandSizeCode());
  printRightByteOperand(data + 2);
  return 3;  // includes 0x0F
}

// Returns number of bytes used, including *data.
int DisassemblerX64::fPUInstruction(uint8_t* data) {
  uint8_t escape_opcode = *data;
  DCHECK(0xD8 == (escape_opcode & 0xF8), "");
  uint8_t modrm_byte = *(data + 1);

  if (modrm_byte >= 0xC0) {
    return registerFPUInstruction(escape_opcode, modrm_byte);
  }
  return memoryFPUInstruction(escape_opcode, modrm_byte, data + 1);
}

int DisassemblerX64::memoryFPUInstruction(int escape_opcode, int modrm_byte,
                                          uint8_t* modrm_start) {
  const char* mnem = "?";
  int regop = (modrm_byte >> 3) & 0x7;  // reg/op field of modrm byte.
  switch (escape_opcode) {
    case 0xD9:
      switch (regop) {
        case 0:
          mnem = "fld_s";
          break;
        case 3:
          mnem = "fstp_s";
          break;
        case 5:
          mnem = "fldcw";
          break;
        case 7:
          mnem = "fnstcw";
          break;
        default:
          unimplementedInstruction();
      }
      break;

    case 0xDB:
      switch (regop) {
        case 0:
          mnem = "fild_s";
          break;
        case 1:
          mnem = "fisttp_s";
          break;
        case 2:
          mnem = "fist_s";
          break;
        case 3:
          mnem = "fistp_s";
          break;
        default:
          unimplementedInstruction();
      }
      break;

    case 0xDD:
      switch (regop) {
        case 0:
          mnem = "fld_d";
          break;
        case 3:
          mnem = "fstp_d";
          break;
        default:
          unimplementedInstruction();
      }
      break;

    case 0xDF:
      switch (regop) {
        case 5:
          mnem = "fild_d";
          break;
        case 7:
          mnem = "fistp_d";
          break;
        default:
          unimplementedInstruction();
      }
      break;

    default:
      unimplementedInstruction();
  }
  print("%s ", mnem);
  int count = printRightOperand(modrm_start);
  return count + 1;
}

int DisassemblerX64::registerFPUInstruction(int escape_opcode,
                                            uint8_t modrm_byte) {
  bool has_register = false;  // Is the FPU register encoded in modrm_byte?
  const char* mnem = "?";

  switch (escape_opcode) {
    case 0xD8:
      unimplementedInstruction();
      break;

    case 0xD9:
      switch (modrm_byte & 0xF8) {
        case 0xC0:
          mnem = "fld";
          has_register = true;
          break;
        case 0xC8:
          mnem = "fxch";
          has_register = true;
          break;
        default:
          switch (modrm_byte) {
            case 0xE0:
              mnem = "fchs";
              break;
            case 0xE1:
              mnem = "fabs";
              break;
            case 0xE3:
              mnem = "fninit";
              break;
            case 0xE4:
              mnem = "ftst";
              break;
            case 0xE8:
              mnem = "fld1";
              break;
            case 0xEB:
              mnem = "fldpi";
              break;
            case 0xED:
              mnem = "fldln2";
              break;
            case 0xEE:
              mnem = "fldz";
              break;
            case 0xF0:
              mnem = "f2xm1";
              break;
            case 0xF1:
              mnem = "fyl2x";
              break;
            case 0xF2:
              mnem = "fptan";
              break;
            case 0xF5:
              mnem = "fprem1";
              break;
            case 0xF7:
              mnem = "fincstp";
              break;
            case 0xF8:
              mnem = "fprem";
              break;
            case 0xFB:
              mnem = "fsincos";
              break;
            case 0xFD:
              mnem = "fscale";
              break;
            case 0xFE:
              mnem = "fsin";
              break;
            case 0xFF:
              mnem = "fcos";
              break;
            default:
              unimplementedInstruction();
          }
      }
      break;

    case 0xDA:
      if (modrm_byte == 0xE9) {
        mnem = "fucompp";
      } else {
        unimplementedInstruction();
      }
      break;

    case 0xDB:
      if ((modrm_byte & 0xF8) == 0xE8) {
        mnem = "fucomi";
        has_register = true;
      } else if (modrm_byte == 0xE2) {
        mnem = "fclex";
      } else {
        unimplementedInstruction();
      }
      break;

    case 0xDC:
      has_register = true;
      switch (modrm_byte & 0xF8) {
        case 0xC0:
          mnem = "fadd";
          break;
        case 0xE8:
          mnem = "fsub";
          break;
        case 0xC8:
          mnem = "fmul";
          break;
        case 0xF8:
          mnem = "fdiv";
          break;
        default:
          unimplementedInstruction();
      }
      break;

    case 0xDD:
      has_register = true;
      switch (modrm_byte & 0xF8) {
        case 0xC0:
          mnem = "ffree";
          break;
        case 0xD8:
          mnem = "fstp";
          break;
        default:
          unimplementedInstruction();
      }
      break;

    case 0xDE:
      if (modrm_byte == 0xD9) {
        mnem = "fcompp";
      } else {
        has_register = true;
        switch (modrm_byte & 0xF8) {
          case 0xC0:
            mnem = "faddp";
            break;
          case 0xE8:
            mnem = "fsubp";
            break;
          case 0xC8:
            mnem = "fmulp";
            break;
          case 0xF8:
            mnem = "fdivp";
            break;
          default:
            unimplementedInstruction();
        }
      }
      break;

    case 0xDF:
      if (modrm_byte == 0xE0) {
        mnem = "fnstsw_ax";
      } else if ((modrm_byte & 0xF8) == 0xE8) {
        mnem = "fucomip";
        has_register = true;
      }
      break;

    default:
      unimplementedInstruction();
  }

  if (has_register) {
    print("%s st%d", mnem, modrm_byte & 0x7);
  } else {
    print("%s", mnem);
  }
  return 2;
}

// TODO(srdjan): Should we add a branch hint argument?
bool DisassemblerX64::decodeInstructionType(uint8_t** data) {
  uint8_t current;

  // Scan for prefixes.
  while (true) {
    current = **data;
    if (current == OPERAND_SIZE_OVERRIDE_PREFIX) {  // Group 3 prefix.
      operand_size_ = current;
    } else if ((current & 0xF0) == 0x40) {
      // REX prefix.
      setRex(current);
      // TODO(srdjan): Should we enable printing of REX.W?
      // if (rexW()) print("REX.W ");
    } else if ((current & 0xFE) == 0xF2) {  // Group 1 prefix (0xF2 or 0xF3).
      group_1_prefix_ = current;
    } else if (current == 0xF0) {
      print("lock ");
    } else {  // Not a prefix - an opcode.
      break;
    }
    (*data)++;
  }

  const InstructionDesc& idesc = instruction_table.get(current);
  byte_size_operand_ = idesc.byte_size_operation;

  switch (idesc.type) {
    case ZERO_OPERANDS_INSTR:
      if (current >= 0xA4 && current <= 0xA7) {
        // String move or compare operations.
        if (group_1_prefix_ == REP_PREFIX) {
          // REP.
          print("rep ");
        }
        if ((current & 0x01) == 0x01) {
          // Operation size: word, dword or qword
          switch (operandSize()) {
            case WORD_SIZE:
              print("%sw", idesc.mnem);
              break;
            case DOUBLEWORD_SIZE:
              print("%sl", idesc.mnem);
              break;
            case QUADWORD_SIZE:
              print("%sq", idesc.mnem);
              break;
            default:
              UNREACHABLE("bad operand size");
          }
        } else {
          // Operation size: byte
          print("%s", idesc.mnem);
        }
      } else if (current == 0x99 && rexW()) {
        print("cqo");  // Cdql is called cdq and cdqq is called cqo.
      } else {
        print("%s", idesc.mnem);
      }
      (*data)++;
      break;

    case TWO_OPERANDS_INSTR:
      (*data)++;
      (*data) += printOperands(idesc.mnem, idesc.op_order_, *data);
      break;

    case JUMP_CONDITIONAL_SHORT_INSTR:
      (*data) += jumpConditionalShort(*data);
      break;

    case REGISTER_INSTR:
      print("%s%s %s", idesc.mnem, operandSizeCode(),
            nameOfCPURegister(baseReg(current & 0x07)));
      (*data)++;
      break;
    case PUSHPOP_INSTR:
      print("%s %s", idesc.mnem, nameOfCPURegister(baseReg(current & 0x07)));
      (*data)++;
      break;
    case MOVE_REG_INSTR: {
      intptr_t addr = 0;
      int imm_bytes = 0;
      switch (operandSize()) {
        case WORD_SIZE:
          addr = LoadUnaligned(reinterpret_cast<int16_t*>(*data + 1));
          imm_bytes = 2;
          break;
        case DOUBLEWORD_SIZE:
          addr = LoadUnaligned(reinterpret_cast<int32_t*>(*data + 1));
          imm_bytes = 4;
          break;
        case QUADWORD_SIZE:
          addr = LoadUnaligned(reinterpret_cast<int64_t*>(*data + 1));
          imm_bytes = 8;
          break;
        default:
          UNREACHABLE("bad operand size");
      }
      (*data) += 1 + imm_bytes;
      print("mov%s %s,", operandSizeCode(),
            nameOfCPURegister(baseReg(current & 0x07)));
      printImmediateValue(addr, /*signed_value=*/false, imm_bytes);
      break;
    }

    case CALL_JUMP_INSTR: {
      int32_t disp = LoadUnaligned(reinterpret_cast<int32_t*>(*data + 1)) + 5;
      print("%s ", idesc.mnem);
      printJump(*data, disp);
      (*data) += 5;
      break;
    }

    case SHORT_IMMEDIATE_INSTR: {
      print("%s%s %s,", idesc.mnem, operandSizeCode(), rax());
      printImmediate(*data + 1, DOUBLEWORD_SIZE);
      (*data) += 5;
      break;
    }

    case NO_INSTR:
      return false;

    default:
      UNIMPLEMENTED("type not implemented");
  }
  return true;
}

int DisassemblerX64::print660F38Instruction(uint8_t* current) {
  int mod, regop, rm;
  if (*current == 0x25) {
    getModRM(*(current + 1), &mod, &regop, &rm);
    print("pmovsxdq %s,", nameOfXMMRegister(regop));
    return 1 + printRightXMMOperand(current + 1);
  }
  if (*current == 0x29) {
    getModRM(*(current + 1), &mod, &regop, &rm);
    print("pcmpeqq %s,", nameOfXMMRegister(regop));
    return 1 + printRightXMMOperand(current + 1);
  }
  unimplementedInstruction();
  return 1;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
// Handle all two-byte opcodes, which start with 0x0F.
// These instructions may be affected by an 0x66, 0xF2, or 0xF3 prefix.
// We do not use any three-byte opcodes, which start with 0x0F38 or 0x0F3A.
int DisassemblerX64::twoByteOpcodeInstruction(uint8_t* data) {
  uint8_t opcode = *(data + 1);
  uint8_t* current = data + 2;
  // At return, "current" points to the start of the next instruction.
  const char* mnemonic = twoByteMnemonic(opcode);
  if (operand_size_ == 0x66) {
    // 0x66 0x0F prefix.
    int mod, regop, rm;
    if (opcode == 0xC6) {
      int mod, regop, rm;
      getModRM(*current, &mod, &regop, &rm);
      print("shufpd %s, ", nameOfXMMRegister(regop));
      current += printRightXMMOperand(current);
      print(" [%x]", *current);
      current++;
    } else if (opcode == 0x3A) {
      uint8_t third_byte = *current;
      current = data + 3;
      if (third_byte == 0x16) {
        getModRM(*current, &mod, &regop, &rm);
        print("pextrd ");  // reg/m32, xmm, imm8
        current += printRightOperand(current);
        print(",%s,%d", nameOfXMMRegister(regop), (*current) & 7);
        current += 1;
      } else if (third_byte == 0x17) {
        getModRM(*current, &mod, &regop, &rm);
        print("extractps ");  // reg/m32, xmm, imm8
        current += printRightOperand(current);
        print(", %s, %d", nameOfCPURegister(regop), (*current) & 3);
        current += 1;
      } else if (third_byte == 0x0b) {
        getModRM(*current, &mod, &regop, &rm);
        // roundsd xmm, xmm/m64, imm8
        print("roundsd %s, ", nameOfCPURegister(regop));
        current += printRightOperand(current);
        print(", %d", (*current) & 3);
        current += 1;
      } else {
        unimplementedInstruction();
      }
    } else {
      getModRM(*current, &mod, &regop, &rm);
      if (opcode == 0x1f) {
        current++;
        if (rm == 4) {  // SIB byte present.
          current++;
        }
        if (mod == 1) {  // Byte displacement.
          current += 1;
        } else if (mod == 2) {  // 32-bit displacement.
          current += 4;
        }  // else no immediate displacement.
        print("nop");
      } else if (opcode == 0x28) {
        print("movapd %s, ", nameOfXMMRegister(regop));
        current += printRightXMMOperand(current);
      } else if (opcode == 0x29) {
        print("movapd ");
        current += printRightXMMOperand(current);
        print(", %s", nameOfXMMRegister(regop));
      } else if (opcode == 0x38) {
        current += print660F38Instruction(current);
      } else if (opcode == 0x6E) {
        print("mov%c %s,", rexW() ? 'q' : 'd', nameOfXMMRegister(regop));
        current += printRightOperand(current);
      } else if (opcode == 0x6F) {
        print("movdqa %s,", nameOfXMMRegister(regop));
        current += printRightXMMOperand(current);
      } else if (opcode == 0x7E) {
        print("mov%c ", rexW() ? 'q' : 'd');
        current += printRightOperand(current);
        print(",%s", nameOfXMMRegister(regop));
      } else if (opcode == 0x7F) {
        print("movdqa ");
        current += printRightXMMOperand(current);
        print(",%s", nameOfXMMRegister(regop));
      } else if (opcode == 0xD6) {
        print("movq ");
        current += printRightXMMOperand(current);
        print(",%s", nameOfXMMRegister(regop));
      } else if (opcode == 0x50) {
        print("movmskpd %s,", nameOfCPURegister(regop));
        current += printRightXMMOperand(current);
      } else if (opcode == 0xD7) {
        print("pmovmskb %s,", nameOfCPURegister(regop));
        current += printRightXMMOperand(current);
      } else {
        const char* mnemonic = "?";
        if (opcode == 0x5A) {
          mnemonic = "cvtpd2ps";
        } else if (0x51 <= opcode && opcode <= 0x5F) {
          mnemonic = kXmmInstructions[opcode & 0xF].pd_name;
        } else if (opcode == 0x14) {
          mnemonic = "unpcklpd";
        } else if (opcode == 0x15) {
          mnemonic = "unpckhpd";
        } else if (opcode == 0x2E) {
          mnemonic = "ucomisd";
        } else if (opcode == 0x2F) {
          mnemonic = "comisd";
        } else if (opcode == 0xFE) {
          mnemonic = "paddd";
        } else if (opcode == 0xFA) {
          mnemonic = "psubd";
        } else if (opcode == 0xEF) {
          mnemonic = "pxor";
        } else {
          unimplementedInstruction();
        }
        print("%s %s,", mnemonic, nameOfXMMRegister(regop));
        current += printRightXMMOperand(current);
      }
    }
  } else if (group_1_prefix_ == 0xF2) {
    // Beginning of instructions with prefix 0xF2.

    if (opcode == 0x11 || opcode == 0x10) {
      // MOVSD: Move scalar double-precision fp to/from/between XMM registers.
      print("movsd ");
      int mod, regop, rm;
      getModRM(*current, &mod, &regop, &rm);
      if (opcode == 0x11) {
        current += printRightXMMOperand(current);
        print(",%s", nameOfXMMRegister(regop));
      } else {
        print("%s,", nameOfXMMRegister(regop));
        current += printRightXMMOperand(current);
      }
    } else if (opcode == 0x2A) {
      // CVTSI2SD: integer to XMM double conversion.
      int mod, regop, rm;
      getModRM(*current, &mod, &regop, &rm);
      print("%sd %s,", mnemonic, nameOfXMMRegister(regop));
      current += printRightOperand(current);
    } else if (opcode == 0x2C) {
      // CVTTSD2SI:
      // Convert with truncation scalar double-precision FP to integer.
      int mod, regop, rm;
      getModRM(*current, &mod, &regop, &rm);
      print("cvttsd2si%s %s,", operandSizeCode(), nameOfCPURegister(regop));
      current += printRightXMMOperand(current);
    } else if (opcode == 0x2D) {
      // CVTSD2SI: Convert scalar double-precision FP to integer.
      int mod, regop, rm;
      getModRM(*current, &mod, &regop, &rm);
      print("cvtsd2si%s %s,", operandSizeCode(), nameOfCPURegister(regop));
      current += printRightXMMOperand(current);
    } else if (0x51 <= opcode && opcode <= 0x5F) {
      // XMM arithmetic. get the F2 0F prefix version of the mnemonic.
      int mod, regop, rm;
      getModRM(*current, &mod, &regop, &rm);
      const char* mnemonic =
          opcode == 0x5A ? "cvtsd2ss" : kXmmInstructions[opcode & 0xF].sd_name;
      print("%s %s,", mnemonic, nameOfXMMRegister(regop));
      current += printRightXMMOperand(current);
    } else {
      unimplementedInstruction();
    }
  } else if (group_1_prefix_ == 0xF3) {
    // Instructions with prefix 0xF3.
    if (opcode == 0x11 || opcode == 0x10) {
      // MOVSS: Move scalar double-precision fp to/from/between XMM registers.
      print("movss ");
      int mod, regop, rm;
      getModRM(*current, &mod, &regop, &rm);
      if (opcode == 0x11) {
        current += printRightOperand(current);
        print(",%s", nameOfXMMRegister(regop));
      } else {
        print("%s,", nameOfXMMRegister(regop));
        current += printRightOperand(current);
      }
    } else if (opcode == 0x2A) {
      // CVTSI2SS: integer to XMM single conversion.
      int mod, regop, rm;
      getModRM(*current, &mod, &regop, &rm);
      print("%ss %s,", mnemonic, nameOfXMMRegister(regop));
      current += printRightOperand(current);
    } else if (opcode == 0x2C || opcode == 0x2D) {
      bool truncating = (opcode & 1) == 0;
      // CVTTSS2SI/CVTSS2SI:
      // Convert (with truncation) scalar single-precision FP to dword integer.
      int mod, regop, rm;
      getModRM(*current, &mod, &regop, &rm);
      print("cvt%sss2si%s %s,", truncating ? "t" : "", operandSizeCode(),
            nameOfCPURegister(regop));
      current += printRightXMMOperand(current);
    } else if (0x51 <= opcode && opcode <= 0x5F) {
      int mod, regop, rm;
      getModRM(*current, &mod, &regop, &rm);
      const char* mnemonic =
          opcode == 0x5A ? "cvtss2sd" : kXmmInstructions[opcode & 0xF].ss_name;
      print("%s %s,", mnemonic, nameOfXMMRegister(regop));
      current += printRightXMMOperand(current);
    } else if (opcode == 0x7E) {
      int mod, regop, rm;
      getModRM(*current, &mod, &regop, &rm);
      print("movq %s, ", nameOfXMMRegister(regop));
      current += printRightXMMOperand(current);
    } else if (opcode == 0xE6) {
      int mod, regop, rm;
      getModRM(*current, &mod, &regop, &rm);
      print("cvtdq2pd %s,", nameOfXMMRegister(regop));
      current += printRightXMMOperand(current);
    } else if (opcode == 0xB8) {
      // POPCNT.
      current += printOperands(mnemonic, REG_OPER_OP_ORDER, current);
    } else if (opcode == 0xBD) {
      // LZCNT (rep BSR encoding).
      current += printOperands("lzcnt", REG_OPER_OP_ORDER, current);
    } else {
      unimplementedInstruction();
    }
  } else if (opcode == 0x1F) {
    // NOP
    int mod, regop, rm;
    getModRM(*current, &mod, &regop, &rm);
    current++;
    if (rm == 4) {  // SIB byte present.
      current++;
    }
    if (mod == 1) {  // Byte displacement.
      current += 1;
    } else if (mod == 2) {  // 32-bit displacement.
      current += 4;
    }  // else no immediate displacement.
    print("nop");

  } else if (opcode == 0x28 || opcode == 0x2f) {
    // ...s xmm, xmm/m128
    int mod, regop, rm;
    getModRM(*current, &mod, &regop, &rm);
    const char* mnemonic = opcode == 0x28 ? "movaps" : "comiss";
    print("%s %s,", mnemonic, nameOfXMMRegister(regop));
    current += printRightXMMOperand(current);
  } else if (opcode == 0x29) {
    // movaps xmm/m128, xmm
    int mod, regop, rm;
    getModRM(*current, &mod, &regop, &rm);
    print("movaps ");
    current += printRightXMMOperand(current);
    print(",%s", nameOfXMMRegister(regop));
  } else if (opcode == 0x11) {
    // movups xmm/m128, xmm
    int mod, regop, rm;
    getModRM(*current, &mod, &regop, &rm);
    print("movups ");
    current += printRightXMMOperand(current);
    print(",%s", nameOfXMMRegister(regop));
  } else if (opcode == 0x50) {
    int mod, regop, rm;
    getModRM(*current, &mod, &regop, &rm);
    print("movmskps %s,", nameOfCPURegister(regop));
    current += printRightXMMOperand(current);
  } else if (opcode == 0xA2 || opcode == 0x31) {
    // RDTSC or CPUID
    print("%s", mnemonic);
  } else if ((opcode & 0xF0) == 0x40) {
    // CMOVcc: conditional move.
    int condition = opcode & 0x0F;
    const InstructionDesc& idesc = cmov_instructions[condition];
    byte_size_operand_ = idesc.byte_size_operation;
    current += printOperands(idesc.mnem, idesc.op_order_, current);
  } else if (0x10 <= opcode && opcode <= 0x16) {
    // ...ps xmm, xmm/m128
    static const char* mnemonics[] = {"movups", nullptr,    "movhlps",
                                      nullptr,  "unpcklps", "unpckhps",
                                      "movlhps"};
    const char* mnemonic = mnemonics[opcode - 0x10];
    if (mnemonic == nullptr) {
      unimplementedInstruction();
      mnemonic = "???";
    }
    int mod, regop, rm;
    getModRM(*current, &mod, &regop, &rm);
    print("%s %s,", mnemonic, nameOfXMMRegister(regop));
    current += printRightXMMOperand(current);
  } else if (0x51 <= opcode && opcode <= 0x5F) {
    int mod, regop, rm;
    getModRM(*current, &mod, &regop, &rm);
    const char* mnemonic =
        opcode == 0x5A ? "cvtps2pd" : kXmmInstructions[opcode & 0xF].ps_name;
    print("%s %s,", mnemonic, nameOfXMMRegister(regop));
    current += printRightXMMOperand(current);
  } else if (opcode == 0xC2 || opcode == 0xC6) {
    int mod, regop, rm;
    getModRM(*current, &mod, &regop, &rm);
    if (opcode == 0xC2) {
      print("cmpps %s,", nameOfXMMRegister(regop));
      current += printRightXMMOperand(current);
      print(" [%s]", kXmmConditionalCodeSuffix[*current]);
    } else {
      DCHECK(opcode == 0xC6, "");
      print("shufps %s,", nameOfXMMRegister(regop));
      current += printRightXMMOperand(current);
      print(" [%x]", *current);
    }
    current++;
  } else if ((opcode & 0xF0) == 0x80) {
    // Jcc: Conditional jump (branch).
    current = data + jumpConditional(data);

  } else if (opcode == 0xBE || opcode == 0xBF || opcode == 0xB6 ||
             opcode == 0xB7 || opcode == 0xAF || opcode == 0xB0 ||
             opcode == 0xB1 || opcode == 0xBC || opcode == 0xBD) {
    // Size-extending moves, IMUL, cmpxchg, BSF, BSR.
    current += printOperands(mnemonic, REG_OPER_OP_ORDER, current);
  } else if ((opcode & 0xF0) == 0x90) {
    // SETcc: Set byte on condition. Needs pointer to beginning of instruction.
    current = data + setCC(data);
  } else if (((opcode & 0xFE) == 0xA4) || ((opcode & 0xFE) == 0xAC) ||
             (opcode == 0xAB) || (opcode == 0xA3)) {
    // SHLD, SHRD (double-prec. shift), BTS (bit test and set), BT (bit test).
    print("%s%s ", mnemonic, operandSizeCode());
    int mod, regop, rm;
    getModRM(*current, &mod, &regop, &rm);
    current += printRightOperand(current);
    print(",%s", nameOfCPURegister(regop));
    if ((opcode == 0xAB) || (opcode == 0xA3) || (opcode == 0xBD)) {
      // Done.
    } else if ((opcode == 0xA5) || (opcode == 0xAD)) {
      print(",cl");
    } else {
      print(",");
      current += printImmediate(current, BYTE_SIZE);
    }
  } else if (opcode == 0xBA && (*current & 0x60) == 0x60) {
    // bt? immediate instruction
    int r = (*current >> 3) & 7;
    static const char* const names[4] = {"bt", "bts", "btr", "btc"};
    print("%s ", names[r - 4]);
    current += printRightOperand(current);
    uint8_t bit = *current++;
    print(",%d", bit);
  } else if (opcode == 0x0B) {
    print("ud2");
  } else {
    unimplementedInstruction();
  }
  return static_cast<int>(current - data);
}
#pragma GCC diagnostic pop

// Mnemonics for two-byte opcode instructions starting with 0x0F.
// The argument is the second byte of the two-byte opcode.
// Returns nullptr if the instruction is not handled here.
const char* DisassemblerX64::twoByteMnemonic(uint8_t opcode) {
  if (opcode == 0x5A) {
    return "cvtps2pd";
  }
  if (0x51 <= opcode && opcode <= 0x5F) {
    return kXmmInstructions[opcode & 0xF].ps_name;
  }
  if (0xA2 <= opcode && opcode <= 0xBF) {
    static const char* mnemonics[] = {
        "cpuid", "bt",    "shld",    "shld",    nullptr,  nullptr,
        nullptr, nullptr, nullptr,   "bts",     "shrd",   "shrd",
        nullptr, "imul",  "cmpxchg", "cmpxchg", nullptr,  nullptr,
        nullptr, nullptr, "movzxb",  "movzxw",  "popcnt", nullptr,
        nullptr, nullptr, "bsf",     "bsr",     "movsxb", "movsxw"};
    return mnemonics[opcode - 0xA2];
  }
  switch (opcode) {
    case 0x12:
      return "movhlps";
    case 0x16:
      return "movlhps";
    case 0x1F:
      return "nop";
    case 0x2A:  // F2/F3 prefix.
      return "cvtsi2s";
    case 0x31:
      return "rdtsc";
    default:
      return nullptr;
  }
}

int DisassemblerX64::instructionDecode(uword pc) {
  uint8_t* data = reinterpret_cast<uint8_t*>(pc);

  const bool processed = decodeInstructionType(&data);

  if (!processed) {
    switch (*data) {
      case 0xC2:
        print("ret ");
        printImmediateValue(*reinterpret_cast<uint16_t*>(data + 1));
        data += 3;
        break;

      case 0xC8:
        print("enter %d, %d", *reinterpret_cast<uint16_t*>(data + 1), data[3]);
        data += 4;
        break;

      case 0x69:
        FALLTHROUGH;
      case 0x6B: {
        int mod, regop, rm;
        getModRM(*(data + 1), &mod, &regop, &rm);
        int32_t imm = *data == 0x6B
                          ? *(data + 2)
                          : LoadUnaligned(reinterpret_cast<int32_t*>(data + 2));
        print("imul%s %s,%s,", operandSizeCode(), nameOfCPURegister(regop),
              nameOfCPURegister(rm));
        printImmediateValue(imm);
        data += 2 + (*data == 0x6B ? 1 : 4);
        break;
      }

      case 0x81:
        FALLTHROUGH;
      case 0x83:  // 0x81 with sign extension bit set
        data += printImmediateOp(data);
        break;

      case 0x0F:
        data += twoByteOpcodeInstruction(data);
        break;

      case 0x8F: {
        data++;
        int mod, regop, rm;
        getModRM(*data, &mod, &regop, &rm);
        if (regop == 0) {
          print("pop ");
          data += printRightOperand(data);
        }
      } break;

      case 0xFF: {
        data++;
        int mod, regop, rm;
        getModRM(*data, &mod, &regop, &rm);
        const char* mnem = nullptr;
        switch (regop) {
          case 0:
            mnem = "inc";
            break;
          case 1:
            mnem = "dec";
            break;
          case 2:
            mnem = "call";
            break;
          case 4:
            mnem = "jmp";
            break;
          case 6:
            mnem = "push";
            break;
          default:
            mnem = "???";
        }
        if (regop <= 1) {
          print("%s%s ", mnem, operandSizeCode());
        } else {
          print("%s ", mnem);
        }
        data += printRightOperand(data);
      } break;

      case 0xC7:  // imm32, fall through
      case 0xC6:  // imm8
      {
        bool is_byte = *data == 0xC6;
        data++;
        if (is_byte) {
          print("movb ");
          data += printRightByteOperand(data);
          print(",");
          data += printImmediate(data, BYTE_SIZE);
        } else {
          print("mov%s ", operandSizeCode());
          data += printRightOperand(data);
          print(",");
          data += printImmediate(data, operandSize(), /* sign extend = */ true);
        }
      } break;

      case 0x80: {
        byte_size_operand_ = true;
        data += printImmediateOp(data);
      } break;

      case 0x88:  // 8bit, fall through
      case 0x89:  // 32bit
      {
        bool is_byte = *data == 0x88;
        int mod, regop, rm;
        data++;
        getModRM(*data, &mod, &regop, &rm);
        if (is_byte) {
          print("movb ");
          data += printRightByteOperand(data);
          print(",%s", nameOfByteCPURegister(regop));
        } else {
          print("mov%s ", operandSizeCode());
          data += printRightOperand(data);
          print(",%s", nameOfCPURegister(regop));
        }
      } break;

      case 0x90:
      case 0x91:
      case 0x92:
      case 0x93:
      case 0x94:
      case 0x95:
      case 0x96:
      case 0x97: {
        int reg = (*data & 0x7) | (rexB() ? 8 : 0);
        if (reg == 0) {
          print("nop");  // Common name for xchg rax,rax.
        } else {
          print("xchg%s %s, %s", operandSizeCode(), rax(),
                nameOfCPURegister(reg));
        }
        data++;
      } break;
      case 0xB0:
      case 0xB1:
      case 0xB2:
      case 0xB3:
      case 0xB4:
      case 0xB5:
      case 0xB6:
      case 0xB7:
      case 0xB8:
      case 0xB9:
      case 0xBA:
      case 0xBB:
      case 0xBC:
      case 0xBD:
      case 0xBE:
      case 0xBF: {
        // mov reg8,imm8 or mov reg32,imm32
        uint8_t opcode = *data;
        data++;
        const bool is_not_8bit = opcode >= 0xB8;
        int reg = (opcode & 0x7) | (rexB() ? 8 : 0);
        if (is_not_8bit) {
          print("mov%s %s,", operandSizeCode(), nameOfCPURegister(reg));
          data +=
              printImmediate(data, operandSize(), /* sign extend = */ false);
        } else {
          print("movb %s,", nameOfByteCPURegister(reg));
          data += printImmediate(data, BYTE_SIZE, /* sign extend = */ false);
        }
        break;
      }
      case 0xFE: {
        data++;
        int mod, regop, rm;
        getModRM(*data, &mod, &regop, &rm);
        if (regop == 1) {
          print("decb ");
          data += printRightByteOperand(data);
        } else {
          unimplementedInstruction();
        }
        break;
      }
      case 0x68:
        print("push ");
        printImmediateValue(
            LoadUnaligned(reinterpret_cast<int32_t*>(data + 1)));
        data += 5;
        break;

      case 0x6A:
        print("push ");
        printImmediateValue(*reinterpret_cast<int8_t*>(data + 1));
        data += 2;
        break;

      case 0xA1:
        FALLTHROUGH;
      case 0xA3:
        switch (operandSize()) {
          case DOUBLEWORD_SIZE: {
            printAddress(reinterpret_cast<uint8_t*>(
                *reinterpret_cast<int32_t*>(data + 1)));
            if (*data == 0xA1) {  // Opcode 0xA1
              print("movzxlq %s,(", rax());
              printAddress(reinterpret_cast<uint8_t*>(
                  *reinterpret_cast<int32_t*>(data + 1)));
              print(")");
            } else {  // Opcode 0xA3
              print("movzxlq (");
              printAddress(reinterpret_cast<uint8_t*>(
                  *reinterpret_cast<int32_t*>(data + 1)));
              print("),%s", rax());
            }
            data += 5;
            break;
          }
          case QUADWORD_SIZE: {
            // New x64 instruction mov rax,(imm_64).
            if (*data == 0xA1) {  // Opcode 0xA1
              print("movq %s,(", rax());
              printAddress(*reinterpret_cast<uint8_t**>(data + 1));
              print(")");
            } else {  // Opcode 0xA3
              print("movq (");
              printAddress(*reinterpret_cast<uint8_t**>(data + 1));
              print("),%s", rax());
            }
            data += 9;
            break;
          }
          default:
            unimplementedInstruction();
            data += 2;
        }
        break;

      case 0xA8:
        print("test al,");
        printImmediateValue(*reinterpret_cast<uint8_t*>(data + 1));
        data += 2;
        break;

      case 0xA9: {
        data++;
        print("test%s %s,", operandSizeCode(), rax());
        data += printImmediate(data, operandSize());
        break;
      }
      case 0xD1:
        FALLTHROUGH;
      case 0xD3:
        FALLTHROUGH;
      case 0xC1:
        data += shiftInstruction(data);
        break;
      case 0xD0:
        FALLTHROUGH;
      case 0xD2:
        FALLTHROUGH;
      case 0xC0:
        byte_size_operand_ = true;
        data += shiftInstruction(data);
        break;

      case 0xD9:
        FALLTHROUGH;
      case 0xDA:
        FALLTHROUGH;
      case 0xDB:
        FALLTHROUGH;
      case 0xDC:
        FALLTHROUGH;
      case 0xDD:
        FALLTHROUGH;
      case 0xDE:
        FALLTHROUGH;
      case 0xDF:
        data += fPUInstruction(data);
        break;

      case 0xEB:
        data += jumpShort(data);
        break;

      case 0xF6:
        byte_size_operand_ = true;
        FALLTHROUGH;
      case 0xF7:
        data += f6F7Instruction(data);
        break;

      case 0x0c:
      case 0x3c:
        data += printImmediateOp(data);
        break;

      // These encodings for inc and dec are IA32 only, but we don't get here
      // on X64 - the REX prefix recoginizer catches them earlier.
      case 0x40:
      case 0x41:
      case 0x42:
      case 0x43:
      case 0x44:
      case 0x45:
      case 0x46:
      case 0x47:
        print("inc %s", nameOfCPURegister(*data & 7));
        data += 1;
        break;

      case 0x48:
      case 0x49:
      case 0x4a:
      case 0x4b:
      case 0x4c:
      case 0x4d:
      case 0x4e:
      case 0x4f:
        print("dec %s", nameOfCPURegister(*data & 7));
        data += 1;
        break;

      default:
        unimplementedInstruction();
        data += 1;
    }
  }  // !processed

  DCHECK(buffer_[buffer_pos_] == '\0', "");

  int instr_len = data - reinterpret_cast<uint8_t*>(pc);
  DCHECK(instr_len > 0, "");  // Ensure progress.

  return instr_len;
}

}  // namespace x64

void Disassembler::decodeInstruction(char* hex_buffer, intptr_t hex_size,
                                     char* human_buffer, intptr_t human_size,
                                     int* out_instr_len, uword pc) {
  DCHECK(hex_size > 0, "");
  DCHECK(human_size > 0, "");
  x64::DisassemblerX64 decoder(human_buffer, human_size);
  int instruction_length = decoder.instructionDecode(pc);
  uint8_t* pc_ptr = reinterpret_cast<uint8_t*>(pc);
  int hex_index = 0;
  int remaining_size = hex_size - hex_index;
  for (int i = 0; (i < instruction_length) && (remaining_size > 2); ++i) {
    std::snprintf(&hex_buffer[hex_index], remaining_size, "%02x", pc_ptr[i]);
    hex_index += 2;
    remaining_size -= 2;
  }
  hex_buffer[hex_index] = '\0';
  if (out_instr_len != nullptr) {
    *out_instr_len = instruction_length;
  }
}

}  // namespace py
