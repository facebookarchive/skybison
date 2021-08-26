/* Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com) */
#pragma once

#include <csignal>

#include "globals.h"
#include "handles.h"
#include "objects.h"

namespace py {

const RawSmallInt kDefaultHandler =
    SmallInt::fromWord(reinterpret_cast<word>(SIG_DFL));
const RawSmallInt kIgnoreHandler =
    SmallInt::fromWord(reinterpret_cast<word>(SIG_IGN));

void handleSignal(int signum);

}  // namespace py
