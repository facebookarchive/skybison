#pragma once

namespace py {

extern const char kBuildInfo[];
extern const char kCompilerVersion[];
extern const char kVersion[];
extern const char kVersionInfo[];

extern const int kVersionMajor;
extern const int kVersionMinor;
extern const int kVersionMicro;
extern const int kVersionHex;

extern const char* const kReleaseLevel;
extern const int kReleaseSerial;

const char* buildInfo();
const char* compilerInfo();
const char* versionInfo();

}  // namespace py
