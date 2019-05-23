#include "handles.h"

#include "frame.h"
#include "objects.h"
#include "runtime.h"

namespace python {

// do_type_check<T> is a helper struct to perform an exact or subtype check,
// based on which handle list contains T.
namespace {
template <typename T>
struct do_type_check;
#define EXACT_TYPE_CHECK(ty)                                                   \
  template <>                                                                  \
  struct do_type_check<Raw##ty> {                                              \
    bool operator()(Thread*, RawObject obj) const { return obj.is##ty(); }     \
  };
HANDLE_TYPES(EXACT_TYPE_CHECK)
#undef EXACT_TYPE_CHECK

#define SUBTYPE_CHECK(ty)                                                      \
  template <>                                                                  \
  struct do_type_check<Raw##ty> {                                              \
    bool operator()(Thread* thread, RawObject obj) const {                     \
      return thread->runtime()->isInstanceOf##ty(obj);                         \
    }                                                                          \
  };
SUBTYPE_HANDLE_TYPES(SUBTYPE_CHECK)
#undef SUBTYPE_CHECK
}  // namespace

// This function needs Runtime's definition for the subtype case, which we
// can't use in handles.h due to a circular dependency. Define it in here for
// now, and manually instantiate Handle as needed.
template <typename T>
bool Handle<T>::isValidType() const {
  return do_type_check<T>()(Thread::current(), **this);
}

#define INSTANTIATE(ty) template class Handle<Raw##ty>;
HANDLE_TYPES(INSTANTIATE)
SUBTYPE_HANDLE_TYPES(INSTANTIATE)
#undef INSTANTIATE

}  // namespace python
