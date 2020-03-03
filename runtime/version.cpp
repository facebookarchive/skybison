#include "version.h"

#include <cstdio>

#include "cpython-data.h"

namespace py {

#define BUILD_INFO "default"
#define COMPILER_VERSION "\n[GCC " __VERSION__ "]"

const char kBuildInfo[] = BUILD_INFO;
const char kCompilerVersion[] = COMPILER_VERSION;
const char kVersion[] = PY_VERSION;
const char kVersionInfo[] = PY_VERSION " (" BUILD_INFO ") " COMPILER_VERSION;

const int kVersionMajor = PY_MAJOR_VERSION;
const int kVersionMinor = PY_MINOR_VERSION;
const int kVersionMicro = PY_MICRO_VERSION;
const int kVersionHex = PY_VERSION_HEX;

const char* const kReleaseLevel =
    PY_RELEASE_LEVEL == PY_RELEASE_LEVEL_ALPHA
        ? "alpha"
        : (PY_RELEASE_LEVEL == PY_RELEASE_LEVEL_BETA
               ? "beta"
               : (PY_RELEASE_LEVEL == PY_RELEASE_LEVEL_GAMMA
                      ? "candidate"
                      : (PY_RELEASE_LEVEL == PY_RELEASE_LEVEL_FINAL
                             ? "final"
                             : "<bad>")));
const int kReleaseSerial = PY_RELEASE_SERIAL;

}  // namespace py
