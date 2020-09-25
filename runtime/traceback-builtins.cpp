#include "traceback-builtins.h"

#include "builtins.h"
#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kTracebackAttributes[] = {
    {ID(_traceback__next), RawTraceback::kNextOffset, AttributeFlags::kHidden},
    {ID(_traceback__function), RawTraceback::kFunctionOffset,
     AttributeFlags::kHidden},
    {ID(tb_lasti), RawTraceback::kLastiOffset, AttributeFlags::kReadOnly},
    {ID(_traceback__lineno), RawTraceback::kLinenoOffset,
     AttributeFlags::kHidden},
    {ID(_traceback__frame), RawTraceback::kFrameOffset,
     AttributeFlags::kHidden},
};

void initializeTracebackType(Thread* thread) {
  addBuiltinType(thread, ID(traceback), LayoutId::kTraceback,
                 /*superclass_id=*/LayoutId::kObject, kTracebackAttributes);
}

}  // namespace py
