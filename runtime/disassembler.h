// Copyright (c) 2013, the Dart project authors and Facebook, Inc. and its
// affiliates. Please see the AUTHORS-Dart file for details. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE-Dart file.

#pragma once

#include "globals.h"
#include "handles-decl.h"

namespace py {

// Forward declaration.
class CodeComments;

// Disassembly formatter interface, which consumes the
// disassembled instructions in any desired form.
class DisassemblyFormatter {
 public:
  DisassemblyFormatter() {}
  virtual ~DisassemblyFormatter() {}

  // Consume the decoded instruction at the given pc.
  virtual void consumeInstruction(char* hex_buffer, intptr_t hex_size,
                                  char* human_buffer, intptr_t human_size,
                                  uword pc) = 0;

  // print a formatted message.
  virtual void print(const char* format, ...) = 0;
};

// Basic disassembly formatter that outputs the disassembled instruction
// to stdout.
class DisassembleToStdout : public DisassemblyFormatter {
 public:
  DisassembleToStdout() : DisassemblyFormatter() {}
  ~DisassembleToStdout() {}

  virtual void consumeInstruction(char* hex_buffer, intptr_t hex_size,
                                  char* human_buffer, intptr_t human_size,
                                  uword pc);

  virtual void print(const char* format, ...);

 private:
  DISALLOW_HEAP_ALLOCATION();
  DISALLOW_COPY_AND_ASSIGN(DisassembleToStdout);
};

// Basic disassembly formatter that outputs the disassembled instruction
// to a memory buffer. This is only intended for test writing.
class DisassembleToMemory : public DisassemblyFormatter {
 public:
  DisassembleToMemory(char* buffer, uintptr_t length)
      : DisassemblyFormatter(),
        buffer_(buffer),
        remaining_(length),
        overflowed_(false) {}
  ~DisassembleToMemory() {}

  virtual void consumeInstruction(char* hex_buffer, intptr_t hex_size,
                                  char* human_buffer, intptr_t human_size,
                                  uword pc);

  virtual void print(const char* format, ...);

 private:
  char* buffer_;
  int remaining_;
  bool overflowed_;
  DISALLOW_HEAP_ALLOCATION();
  DISALLOW_COPY_AND_ASSIGN(DisassembleToMemory);
};

// Disassemble instructions.
class Disassembler {
 public:
  // disassemble instructions between start and end.
  // (The assumption is that start is at a valid instruction).
  // Return true if all instructions were successfully decoded, false otherwise.
  static void disassemble(uword start, uword end,
                          DisassemblyFormatter* formatter,
                          const CodeComments* comments = nullptr);

  static void disassemble(uword start, uword end) {
    DisassembleToStdout stdout_formatter;
    disassemble(start, end, &stdout_formatter);
  }

  static void disassemble(uword start, uword end, char* buffer,
                          uintptr_t buffer_size) {
    DisassembleToMemory memory_formatter(buffer, buffer_size);
    disassemble(start, end, &memory_formatter);
  }

  // Decodes one instruction.
  // Writes a hexadecimal representation into the hex_buffer and a
  // human-readable representation into the human_buffer.
  // Writes the length of the decoded instruction in bytes in out_instr_len.
  static void decodeInstruction(char* hex_buffer, intptr_t hex_size,
                                char* human_buffer, intptr_t human_size,
                                int* out_instr_len, uword pc);

 private:
  static const int kHexadecimalBufferSize = 32;
  static const int kUserReadableBufferSize = 256;
};

}  // namespace py
