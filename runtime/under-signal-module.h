#pragma once

#include <csignal>

#include "globals.h"
#include "handles.h"
#include "objects.h"
#include "thread.h"

namespace py {

const RawSmallInt kDefaultHandler =
    SmallInt::fromWord(reinterpret_cast<word>(SIG_DFL));
const RawSmallInt kIgnoreHandler =
    SmallInt::fromWord(reinterpret_cast<word>(SIG_IGN));

void handleSignal(int signum);

void initializeUnderSignalModule(Thread* thread, const Module& module);

}  // namespace py
