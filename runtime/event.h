#pragma once

#if defined(OS_LINUX)

#include <sdt.h>

#define EVENT(probe_name) DTRACE_PROBE(python, probe_name)

#define EVENT_ID(probe_name, id)                                               \
  DTRACE_PROBE1(python, probe_name, static_cast<word>(id))

#elif defined(OS_OSX)

// Adding USDT probes is a bit more complicated on macOS than Linux and
// requires an intermediate build step. See
// https://www.unix.com/man-page/osx/1/dtrace/ for more details.

#define EVENT(probe_name)

#define EVENT_ID(probe_name, id)

#endif
