#include "register-state.h"

#include <cstring>

namespace py {
namespace x64 {

RegisterState::RegisterState() { memset(assignment_, 0, sizeof(assignment_)); }

void RegisterState::assign(VirtualRegister* vreg, Register reg) {
  VirtualRegister* previous_assignment = assignment_[reg];
  if (previous_assignment != nullptr) {
    previous_assignment->free();
  }
  assignment_[reg] = vreg;
  vreg->assign(reg);
}

void RegisterState::free(VirtualRegister* vreg) {
  Register reg = static_cast<Register>(*vreg);
  CHECK(reg != kNoRegister, "no register assigned");
  CHECK(assignment_[reg] == vreg, "inconsistent assignment");
  vreg->free();
  assignment_[reg] = nullptr;
}

void RegisterState::allocate(VirtualRegister* vreg, View<Register> candidates) {
  for (Register reg : candidates) {
    if (assignment_[reg] == nullptr) {
      assign(vreg, reg);
      return;
    }
  }
  UNREACHABLE("no free register left");
}

void RegisterState::clobber(View<Register> registers) {
  for (Register reg : registers) {
    VirtualRegister* vreg = assignment_[reg];
    if (vreg != nullptr) {
      vreg->free();
      assignment_[reg] = nullptr;
    }
  }
}

void RegisterState::reset() {
  for (word i = 0; i < kNumRegisters; i++) {
    VirtualRegister* vreg = assignment_[i];
    if (vreg != nullptr) {
      vreg->free();
      assignment_[i] = nullptr;
    }
  }
}

void RegisterState::resetTo(View<RegisterAssignment> assignment) {
  reset();
  for (word i = 0, length = assignment.length(); i < length; i++) {
    VirtualRegister* vreg = assignment.get(i).vreg;
    assign(vreg, assignment.get(i).reg);
  }
}

void RegisterState::check(View<RegisterAssignment> assignment) {
  for (word i = 0, length = assignment.length(); i < length; i++) {
    VirtualRegister* vreg = assignment.get(i).vreg;
    CHECK(static_cast<Register>(*vreg) == assignment.get(i).reg,
          "unexpected assignment");
  }
}

}  // namespace x64
}  // namespace py
