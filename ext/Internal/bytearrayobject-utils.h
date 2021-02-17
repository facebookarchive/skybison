#pragma once

#include "capi-handles.h"
#include "handles-decl.h"
#include "runtime.h"

namespace py {

// Allocates a char* of the bytearray's contents and updates the ApiHandle's
// cache with the buffer, clears the cache's contents if a previous entry
// exists.
char* bytearrayAsString(Runtime* runtime, ApiHandle* handle,
                        const Bytearray& array);

}  // namespace py
