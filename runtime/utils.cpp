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
  void visit(Frame* frame) {
    std::stringstream line;

    if (frame->code().isInt()) {
      void* ptr = RawInt::cast(frame->code()).asCPtr();
      line << "  <native function at " << ptr << " (";

      Dl_info info = Dl_info();
      if (dladdr(ptr, &info) && info.dli_sname != nullptr) {
        line << info.dli_sname;
      } else {
        line << "no symbol found";
      }
      line << ")>";
      lines_.emplace_back(line.str());
      return;
    }
    if (!frame->code().isCode()) {
      lines_.emplace_back("  <unknown>");
      return;
    }

    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Code code(&scope, frame->code());

    // Extract filename
    if (code.filename().isStr()) {
      char* filename = RawStr::cast(code.filename()).toCStr();
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

    // Extract function
    if (code.name().isStr()) {
      char* name = RawStr::cast(code.name()).toCStr();
      line << "in " << name;
      std::free(name);
    } else {
      line << "in <unknown>";
    }

    lines_.emplace_back(line.str());
  };

  void print(FILE* fp) {
    fprintf(fp, "Traceback (most recent call last)\n");
    for (auto it = lines_.rbegin(); it != lines_.rend(); it++) {
      fprintf(fp, "%s\n", it->c_str());
    }
    fflush(fp);
  }

 private:
  std::vector<std::string> lines_;
};

void Utils::printTraceback() { printTraceback(stderr); }

void Utils::printTraceback(FILE* fp) {
  TracebackPrinter printer;
  Thread::current()->visitFrames(&printer);
  printer.print(fp);
}

}  // namespace python
