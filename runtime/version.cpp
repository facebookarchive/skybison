#include "version.h"

#include <cstdio>

namespace py {

static const char* const kCompiler = "\n[GCC " __VERSION__ "]";

const char* const kVersion = "3.6.8+";

const int kVersionMajor = 3;
const int kVersionMinor = 6;
const int kVersionMicro = 8;

const char* const kReleaseLevel = "final";
const int kReleaseSerial = 0;

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
