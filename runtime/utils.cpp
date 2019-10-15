#include "utils.h"

#include "debugging.h"
#include "frame.h"
#include "handles.h"
#include "runtime.h"
#include "thread.h"

#include <iostream>
#include <sstream>
#include <vector>

#include <dlfcn.h>

namespace python {

class TracebackPrinter : public FrameVisitor {
 public:
  bool visit(Frame* frame) {
    std::stringstream line;
    if (const char* invalid_frame = frame->isInvalid()) {
      line << "  Invalid frame (" << invalid_frame << ")";
      lines_.emplace_back(line.str());
      return false;
    }

    DCHECK(!frame->isSentinel(), "should not be called for sentinel");
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Function function(&scope, frame->function());
    Object code_obj(&scope, function.code());
    if (code_obj.isCode()) {
      Code code(&scope, *code_obj);

      // Extract filename
      if (code.filename().isStr()) {
        char* filename = Str::cast(code.filename()).toCStr();
        line << "  File \"" << filename << "\", ";
        std::free(filename);
      } else {
        line << "  File \"<unknown>\",  ";
      }

      // Extract line number unless it is a native functions.
      if (!code.isNative() && code.lnotab().isBytes()) {
        Runtime* runtime = thread->runtime();
        // virtualPC() points to the next PC. The currently executing PC
        // should be immediately before this when raising an exception which
        // should be the only relevant case for managed code. This value will
        // be off when we produce debug output in a failed `CHECK` or in lldb
        // immediately after a jump.
        word pc = Utils::maximum(frame->virtualPC() - kCodeUnitSize, word{0});
        word linenum = runtime->codeOffsetToLineNum(thread, code, pc);
        line << "line " << linenum << ", ";
      }
    }

    Object name(&scope, function.name());
    if (name.isStr()) {
      unique_c_ptr<char> name_cstr(Str::cast(*name).toCStr());
      line << "in " << name_cstr.get();
    } else {
      line << "in <invalid name>";
    }

    if (code_obj.isCode()) {
      Code code(&scope, *code_obj);
      if (code.isNative()) {
        void* fptr = Int::cast(code.code()).asCPtr();
        line << "  <native function at " << fptr << " (";

        Dl_info info = Dl_info();
        if (dladdr(fptr, &info) && info.dli_sname != nullptr) {
          line << info.dli_sname;
        } else {
          line << "no symbol found";
        }
        line << ")>";
      }
    }

    lines_.emplace_back(line.str());
    return true;
  }

  void print(std::ostream* os) {
    *os << "Traceback (most recent call last):\n";
    for (auto it = lines_.rbegin(); it != lines_.rend(); it++) {
      *os << *it << '\n';
    }
    os->flush();
  }

 private:
  std::vector<std::string> lines_;
};

word Utils::memoryFind(byte* haystack, word haystack_len, byte* needle,
                       word needle_len) {
  DCHECK(haystack != nullptr, "haystack cannot be null");
  DCHECK(needle != nullptr, "needle cannot be null");
  DCHECK(haystack_len >= 0, "haystack length must be nonnegative");
  DCHECK(needle_len >= 0, "needle length must be nonnegative");
  // We need something to compare
  if (haystack_len == 0 || needle_len == 0) return -1;
  // The needle is too big to be contained in haystack
  if (haystack_len < needle_len) return -1;
  void* result;
  if (needle_len == 1) {
    // Fast path: one character
    result = ::memchr(haystack, *needle, haystack_len);
  } else {
    result = ::memmem(haystack, haystack_len, needle, needle_len);
  }
  if (result == nullptr) return -1;
  return reinterpret_cast<byte*>(result) - haystack;
}

word Utils::memoryFindCharReverse(byte* haystack, byte needle, word length) {
  DCHECK(haystack != nullptr, "haystack cannot be null");
  DCHECK(length >= 0, "haystack length must be nonnegative");
  for (word i = length - 1; i >= 0; i--) {
    if (haystack[i] == needle) return i;
  }
  return -1;
}

word Utils::memoryFindReverse(byte* haystack, word haystack_len, byte* needle,
                              word needle_len) {
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
    return memoryFindCharReverse(haystack, needle_start, haystack_len);
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

void Utils::printTracebackToStderr() { printTraceback(&std::cerr); }

void Utils::printTraceback(std::ostream* os) {
  TracebackPrinter printer;
  Thread::current()->visitFrames(&printer);
  printer.print(os);
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
    printTracebackToStderr();
    if (thread->hasPendingException()) {
      std::cerr << "Pending exception\n  Type      : "
                << thread->pendingExceptionType()
                << "\n  Value     : " << thread->pendingExceptionValue()
                << "\n  Traceback : " << thread->pendingExceptionTraceback()
                << "\n";
    }
  }
  std::abort();
}

}  // namespace python
