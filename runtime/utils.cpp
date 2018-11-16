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

    if (frame->code()->isInt()) {
      void* ptr = Int::cast(frame->code())->asCPtr();
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
    if (!frame->code()->isCode()) {
      lines_.emplace_back("  <unknown>");
      return;
    }

    Thread* thread = Thread::currentThread();
    HandleScope scope(thread);
    Handle<Code> code(&scope, frame->code());

    // Extract filename
    if (code->filename()->isStr()) {
      char* filename = Str::cast(code->filename())->toCStr();
      line << "  File '" << filename << "', ";
      std::free(filename);
    } else {
      line << "  File '<unknown>',  ";
    }

    // Extract line number
    if (code->lnotab()->isBytes()) {
      Runtime* runtime = thread->runtime();
      word linenum =
          runtime->codeOffsetToLineNum(thread, code, frame->virtualPC());
      line << "line " << linenum << ", ";
    }

    // Extract function
    if (code->name()->isStr()) {
      char* name = Str::cast(code->name())->toCStr();
      line << "in " << name;
      std::free(name);
    } else {
      line << "in <unknown>";
    }

    lines_.emplace_back(line.str());
  };

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

void Utils::printTraceback() { printTraceback(&std::cerr); }

void Utils::printTraceback(std::ostream* os) {
  TracebackPrinter printer;
  Thread::currentThread()->visitFrames(&printer);
  printer.print(os);
}

}  // namespace python
