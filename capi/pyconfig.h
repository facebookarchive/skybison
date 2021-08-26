/* Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com) */
#if defined(__linux__)

#include "pyconfig-linux.h"

#elif defined(__APPLE__)

#include "pyconfig-osx.h"

#else

#error "No pyconfig for the current platform"

#endif
