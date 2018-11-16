#if defined(OS_LINUX)

#include "pyconfig-linux.h"

#elif defined(OS_OSX)

#include "pyconfig-osx.h"

#else

#error "No pyconfig for the current platform"

#endif
