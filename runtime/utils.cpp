#include "utils.h"

#include <dlfcn.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

#include "debugging.h"
#include "file.h"
#include "frame.h"
#include "handles.h"
#include "runtime.h"
#include "thread.h"
#include "traceback-builtins.h"

namespace py {

const byte Utils::kHexDigits[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

word Utils::memoryFind(const byte* haystack, word haystack_len,
                       const byte* needle, word needle_len) {
  DCHECK(haystack != nullptr, "haystack cannot be null");
  DCHECK(needle != nullptr, "needle cannot be null");
  DCHECK(haystack_len >= 0, "haystack length must be nonnegative");
  DCHECK(needle_len >= 0, "needle length must be nonnegative");
  // We need something to compare
  if (haystack_len == 0 || needle_len == 0) return -1;
  // The needle is too big to be contained in haystack
  if (haystack_len < needle_len) return -1;
  const void* result;
  if (needle_len == 1) {
    // Fast path: one character
    result = std::memchr(haystack, *needle, haystack_len);
  } else {
    result = ::memmem(haystack, haystack_len, needle, needle_len);
  }
  if (result == nullptr) return -1;
  return static_cast<const byte*>(result) - haystack;
}

word Utils::memoryFindChar(const byte* haystack, word haystack_len,
                           byte needle) {
  DCHECK(haystack != nullptr, "haystack cannot be null");
  DCHECK(haystack_len >= 0, "haystack length must be nonnegative");
  const void* result = std::memchr(haystack, needle, haystack_len);
  if (result == nullptr) return -1;
  return static_cast<const byte*>(result) - haystack;
}

word Utils::memoryFindCharReverse(const byte* haystack, word haystack_len,
                                  byte needle) {
  DCHECK(haystack != nullptr, "haystack cannot be null");
  DCHECK(haystack_len >= 0, "haystack length must be nonnegative");
  for (word i = haystack_len - 1; i >= 0; i--) {
    if (haystack[i] == needle) return i;
  }
  return -1;
}

word Utils::memoryFindReverse(const byte* haystack, word haystack_len,
                              const byte* needle, word needle_len) {
  DCHECK(haystack != nullptr, "haystack cannot be null");
  DCHECK(needle != nullptr, "needle cannot be null");
  DCHECK(haystack_len >= 0, "haystack length must be nonnegative");
  DCHECK(needle_len >= 0, "needle length must be nonnegative");
  // We need something to compare
  if (haystack_len == 0 || needle_len == 0) return -1;
  // The needle is too big to be contained in haystack
  if (haystack_len < needle_len) return -1;
  byte needle_start = *needle;
  if (needle_len == 1) {
    // Fast path: one character
    return memoryFindCharReverse(haystack, haystack_len, needle_start);
  }
  // The last position where its possible to find needle in haystack
  word last_offset = haystack_len - needle_len;
  for (word i = last_offset; i >= 0; i--) {
    if (haystack[i] == needle_start &&
        std::memcmp(haystack + i, needle, needle_len) == 0) {
      return i;
    }
  }
  return -1;
}

void Utils::printDebugInfoAndAbort() {
  static thread_local bool aborting = false;
  if (aborting) {
    std::cerr << "Attempting to abort while already aborting. Not printing "
                 "another traceback.\n";
    std::abort();
  }
  aborting = true;

  Thread* thread = Thread::current();
  if (thread != nullptr) {
    Runtime* runtime = thread->runtime();
    runtime->printTraceback(thread, File::kStderr);
    if (thread->hasPendingException()) {
      HandleScope scope(thread);
      Object type(&scope, thread->pendingExceptionType());
      Object value(&scope, thread->pendingExceptionValue());
      Traceback traceback(&scope, thread->pendingExceptionTraceback());
      thread->clearPendingException();

      std::cerr << "Pending exception\n  Type          : " << type
                << "\n  Value         : " << value;
      if (runtime->isInstanceOfBaseException(*value)) {
        BaseException exception(&scope, *value);
        std::cerr << "\n  Exception Args: " << exception.args();
      }
      std::cerr << "\n  Traceback     : " << traceback << '\n';

      ValueCell stderr_cell(&scope, runtime->sysStderr());
      if (!stderr_cell.isUnbound()) {
        Object stderr(&scope, stderr_cell.value());
        CHECK(!tracebackWrite(thread, traceback, stderr).isErrorException(),
              "failed to print traceback");
      }
    }
  }
  std::abort();
}

}  // namespace py
