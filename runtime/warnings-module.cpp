#include "warnings-module.h"

#include "frame.h"
#include "handles.h"
#include "runtime.h"
#include "thread.h"

namespace python {

namespace {
RawObject getCategory(Thread* thread, const Object& message,
                      const Object& category) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Type result(&scope, runtime->typeOf(*message));
  Type warning(&scope, runtime->typeAt(LayoutId::kWarning));
  // TODO(bsimmers): Use our equivalent of PyObject_IsInstance once we have it.
  if (!runtime->isSubclass(result, warning)) {
    if (category.isNoneType()) {
      result = *warning;
    } else if (runtime->isInstanceOfType(*category)) {
      result = *category;
    }
    if (!runtime->isSubclass(result, warning)) {
      return thread->raiseTypeErrorWithCStr(
          "category must be a Warning subclass");
    }
  }

  return *result;
}
}  // namespace

RawObject warningsWarn(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Arguments args(frame, nargs);
  Object message(&scope, args.get(0));
  Object category(&scope, args.get(1));
  Object stacklevel(&scope, args.get(2));

  if (!runtime->isInstanceOfInt(*stacklevel)) {
    return thread->raiseTypeErrorWithCStr("integer argument expected");
  }
  auto result = Int(&scope, *stacklevel).asInt<word>();
  if (result.error != CastError::None) {
    return thread->raiseOverflowErrorWithCStr(
        "Python int too large to convert to C ssize_t");
  }

  Object real_category(&scope, getCategory(thread, message, category));
  if (real_category.isError()) return *real_category;

  // TODO(T39431178): Implement proper filtering/escalation.
  return NoneType::object();
}

}  // namespace python
