#include "version.h"

#include <cstdio>

#include "cpython-data.h"

namespace py {

static const char* const kCompiler = "\n[GCC " __VERSION__ "]";

const char* const kVersion = PY_VERSION;

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

const char* buildInfo() {
  static char buildinfo[8];
  // TODO(T60512304): Report the correct revision / branch information
  std::snprintf(buildinfo, sizeof(buildinfo), "%s", "default");
  return buildinfo;
}

const char* compilerInfo() { return kCompiler; }

const char* versionInfo() {
  static char version[256];
  std::snprintf(version, sizeof(version), "%.80s (%.80s) %.80s", kVersion,
                buildInfo(), compilerInfo());
  return version;
}

}  // namespace py
