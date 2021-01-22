#include "builtins.h"
#include "bytecode.h"
#include "dict-builtins.h"
#include "module-builtins.h"
#include "objects.h"
#include "runtime.h"

namespace py {

void FUNC(_bytecode_utils, __init_module__)(Thread* thread,
                                            const Module& module,
                                            View<byte> bytecode) {
  HandleScope scope(thread);

  // Add module variables
  {
    Object co_optimized(&scope, SmallInt::fromWord(Code::Flags::kOptimized));
    moduleAtPutByCStr(thread, module, "CO_OPTIMIZED", co_optimized);

    Object co_newlocals(&scope, SmallInt::fromWord(Code::Flags::kNewlocals));
    moduleAtPutByCStr(thread, module, "CO_NEWLOCALS", co_newlocals);

    Object co_nofree(&scope, SmallInt::fromWord(Code::Flags::kNofree));
    moduleAtPutByCStr(thread, module, "CO_NOFREE", co_nofree);

    Dict opmap(&scope, thread->runtime()->newDict());
    moduleAtPutByCStr(thread, module, "opmap", opmap);

    Object opname(&scope, Str::empty());
    Object opnum(&scope, Str::empty());
    const char prefix[] = "UNUSED_";
    word prefix_len = std::strlen(prefix);
    for (word i = 0; i < kNumBytecodes; i++) {
      const char* opname_cstr = kBytecodeNames[i];
      if (std::strncmp(prefix, opname_cstr, prefix_len) == 0) {
        // Do not add opcodes that begin with UNUSED_
        continue;
      }
      opname = Runtime::internStrFromCStr(thread, opname_cstr);
      opnum = SmallInt::fromWord(i);
      dictAtPutByStr(thread, opmap, opname, opnum);
    }
  }

  executeFrozenModule(thread, module, bytecode);
}
}  // namespace py
