#include "utils.h"

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
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    DCHECK(!frame->isSentinelFrame(), "should not be called for sentinel");
    Object function_obj(&scope, frame->function());
    if (!function_obj.isFunction()) {
      lines_.emplace_back("  <invalid function>\n");
      return true;
    }

    Function function(&scope, *function_obj);
    Object code_obj(&scope, function.code());
    if (code_obj.isCode()) {
      Code code(&scope, *code_obj);

      // Extract filename
      if (code.filename().isStr()) {
        char* filename = Str::cast(code.filename()).toCStr();
        line << "  File '" << filename << "', ";
        std::free(filename);
      } else {
        line << "  File '<unknown>',  ";
      }

      // Extract line number
      if (code.lnotab().isBytes()) {
        Runtime* runtime = thread->runtime();
        word linenum =
            runtime->codeOffsetToLineNum(thread, code, frame->virtualPC());
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
      if (code.code().isInt()) {
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
    *os << "Traceback (most recent call last)\n";
    for (auto it = lines_.rbegin(); it != lines_.rend(); it++) {
      *os << *it << '\n';
    }
    os->flush();
  }

 private:
  std::vector<std::string> lines_;
};

void Utils::printTracebackToStderr() { printTraceback(&std::cerr); }

void Utils::printTraceback(std::ostream* os) {
  TracebackPrinter printer;
  Thread::current()->visitFrames(&printer);
  printer.print(os);
}

}  // namespace python
