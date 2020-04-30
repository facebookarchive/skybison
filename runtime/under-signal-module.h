#pragma once

#include <csignal>

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "runtime.h"
#include "thread.h"

namespace py {

const RawSmallInt kDefaultHandler =
    SmallInt::fromWord(reinterpret_cast<word>(SIG_DFL));
const RawSmallInt kIgnoreHandler =
    SmallInt::fromWord(reinterpret_cast<word>(SIG_IGN));

void handleSignal(int signum);

class UnderSignalModule {
 public:
  static void initialize(Thread* thread, const Module& module);
};

}  // namespace py
