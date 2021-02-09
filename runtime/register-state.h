#pragma once

#include "assembler-x64.h"
#include "view.h"

namespace py {
namespace x64 {

class WARN_UNUSED VirtualRegister {
 public:
  VirtualRegister(const char* name);

  operator Register();

  void assign(Register reg);
  void free();
  bool isAssigned();
  const char* name();

 private:
  Register assigned_ = kNoRegister;
  const char* name_;
};

struct RegisterAssignment {
  VirtualRegister* vreg;
  Register reg;
};

static constexpr View<RegisterAssignment> kNoRegisterAssignment =
    View<RegisterAssignment>(nullptr, 0);

/// Register state tracker.
/// Tracks assignment of registers to instances of the `VirtualRegister` class.
/// This helps catching problems in code generating machine code.
///
/// Straight line code example:
///     VirtualRegister r_value;
///     reg_state->assign(&r_value, RAX);
///     __ movl(r_value, Immediate(42));
///     __ pushq(r_value);
///     reg_state->clobber({..., RAX, ...});
///     __ pushq(r_value);  // Fails because r_value is no longer assigned.
///
/// Re-use example
///     VirtualRegister r_value;
///     reg_state->assign(&r_value, RAX);
///     __ popq(r_value)
///     // ...
///     VirtualRegister r_other;
///     reg_state->assign(&r_other, RAX);  // this resets `r_value` kNoRegister
///     __ movl(r_other, Immediate(5));
///     // ..
///     __ movl(..., r_value); // Fails because `r_value` is no longer assigned
///
/// Control Flow
/// When there are multiple jumps to a common label then there is usually also
/// an expectation to what registers values are available/unavailable in a
/// particular register at that label. Use `RegisterAssignment` lists for that.
///
///     // In env struct:
///     VirtualRegister arg0;
///     VirtualRegister arg1;
///     View<RegisterAssignment> function_begin_assignment;
///     Label function_begin;
///
///     RegisterAssignment function_begin_assignment[] = {
///       {&env->arg0, RDI},
///       {&env->arg1, RSI},
///     };
///     env->function_begin_assignment = function_begin_assignment;
///
///     // Reset state to expected assignment at common label.
///     // Revert all register assignment; then assign RDI to arg0, RSI to arg1.
///     reg_state->resetTo(env->function_begin_assignment);
///     __ bind(&env->function_begin);
///     __ pushq(env->arg0);
///     ...
///     // Check assignment before jumping to label. Fail if arg0 is not
///     // assigned to RDI or arg1 not assigned to RSI.
///     reg_state->check(&env->function_begin_assignment);
///     __ jmp(&env->function_begin);
class RegisterState {
 public:
  RegisterState();

  void assign(VirtualRegister* vreg, Register reg);
  void free(VirtualRegister* vreg);
  void allocate(VirtualRegister* vreg, View<Register> candidates);
  void clobber(View<Register> registers);

  void reset();
  void resetTo(View<RegisterAssignment> assignment);
  void check(View<RegisterAssignment> assignment);

 private:
  VirtualRegister* assignment_[kNumRegisters];
};

inline VirtualRegister::VirtualRegister(const char* name) : name_(name) {}

inline VirtualRegister::operator Register() {
  CHECK(isAssigned(), "no register assigned to '%s'", name_);
  return assigned_;
}

inline void VirtualRegister::assign(Register reg) { assigned_ = reg; }
inline void VirtualRegister::free() { assigned_ = kNoRegister; }
inline bool VirtualRegister::isAssigned() { return assigned_ != kNoRegister; }
inline const char* VirtualRegister::name() { return name_; }

}  // namespace x64
}  // namespace py
