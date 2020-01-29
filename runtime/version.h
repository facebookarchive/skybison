#pragma once

namespace py {

extern const char* const kVersion;

extern const int kVersionMajor;
extern const int kVersionMinor;
extern const int kVersionMicro;

extern const char* const kReleaseLevel;
extern const int kReleaseSerial;

const char* buildInfo();
const char* compilerInfo();
const char* versionInfo();

}  // namespace py
