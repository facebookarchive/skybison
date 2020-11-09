#pragma once

#include "capi-handles.h"
#include "capi.h"

namespace py {

struct CAPIState {
  // Some API functions promise to cache their return value and return the same
  // value for repeated invocations on a specific PyObject. Those value are
  // cached here.
  IdentityDict caches_;

  // C-API object handles
  IdentityDict handles_;
};

static_assert(sizeof(CAPIState) < kCAPIStateSize, "kCAPIStateSize too small");

}  // namespace py
