// Copyright (c) 2013, the Dart project authors and Facebook, Inc. and its
// affiliates. Please see the AUTHORS-Dart file for details. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE-Dart file.

#include "disassembler.h"

#include <cinttypes>
#include <cstdarg>

#include "assembler-utils.h"

namespace py {

static const int kHexColumnWidth = 23;

void DisassembleToStdout::consumeInstruction(char* hex_buffer,
                                             intptr_t hex_size,
                                             char* human_buffer,
                                             intptr_t human_size, uword pc) {
  (void)hex_size;
  (void)human_size;
  std::fprintf(stdout, "0x%" PRIX64 "    %s", static_cast<uint64_t>(pc),
               hex_buffer);
  int hex_length = strlen(hex_buffer);
  if (hex_length < kHexColumnWidth) {
    for (int i = kHexColumnWidth - hex_length; i > 0; i--) {
      std::fprintf(stdout, " ");
    }
  }
  std::fprintf(stdout, "%s", human_buffer);
  std::fprintf(stdout, "\n");
}

void DisassembleToStdout::print(const char* format, ...) {
  va_list args;
  va_start(args, format);
  std::vprintf(format, args);
  va_end(args);
}

void DisassembleToMemory::consumeInstruction(char* hex_buffer,
                                             intptr_t hex_size,
                                             char* human_buffer,
                                             intptr_t human_size, uword pc) {
  (void)human_size;
  (void)hex_buffer;
  (void)hex_size;
  (void)pc;
  if (overflowed_) {
    return;
  }
  intptr_t len = strlen(human_buffer);
  if (remaining_ < len + 100) {
    *buffer_++ = '.';
    *buffer_++ = '.';
    *buffer_++ = '.';
    *buffer_++ = '\n';
    *buffer_++ = '\0';
    overflowed_ = true;
    return;
  }
  memmove(buffer_, human_buffer, len);
  buffer_ += len;
  remaining_ -= len;
  *buffer_++ = '\n';
  remaining_--;
  *buffer_ = '\0';
}

void DisassembleToMemory::print(const char* format, ...) {
  if (overflowed_) {
    return;
  }
  va_list args;
  va_start(args, format);
  intptr_t len = std::vsnprintf(nullptr, 0, format, args);
  va_end(args);
  if (remaining_ < len + 100) {
    *buffer_++ = '.';
    *buffer_++ = '.';
    *buffer_++ = '.';
    *buffer_++ = '\n';
    *buffer_++ = '\0';
    overflowed_ = true;
    return;
  }
  va_start(args, format);
  intptr_t len2 = std::vsnprintf(buffer_, len, format, args);
  va_end(args);
  DCHECK(len == len2, "");
  buffer_ += len;
  remaining_ -= len;
  *buffer_++ = '\n';
  remaining_--;
  *buffer_ = '\0';
}

void Disassembler::disassemble(uword start, uword end,
                               DisassemblyFormatter* formatter,
                               const CodeComments* comments) {
  DCHECK(formatter != nullptr, "");
  char hex_buffer[kHexadecimalBufferSize];  // Instruction in hexadecimal form.
  char human_buffer[kUserReadableBufferSize];  // Human-readable instruction.
  uword pc = start;
  intptr_t comment_finger = 0;
  while (pc < end) {
    const intptr_t offset = pc - start;
    while (comments && comment_finger < comments->length() &&
           comments->offsetAt(comment_finger) <= offset) {
      formatter->print("        ;; %s\n", comments->commentAt(comment_finger));
      comment_finger++;
    }
    int instruction_length;
    decodeInstruction(hex_buffer, sizeof(hex_buffer), human_buffer,
                      sizeof(human_buffer), &instruction_length, pc);
    // TODO(emacs): Add flag to disassemble relative? Then use offset instead
    // of pc.
    formatter->consumeInstruction(hex_buffer, sizeof(hex_buffer), human_buffer,
                                  sizeof(human_buffer), pc);
    pc += instruction_length;
  }
}

}  // namespace py
