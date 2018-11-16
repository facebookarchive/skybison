#include "utils.h"

#include "frame.h"
#include "handles.h"
#include "runtime.h"
#include "thread.h"

#include <iostream>
#include <sstream>
#include <vector>

namespace python {

class TracebackPrinter : public FrameVisitor {
 public:
  void visit(Frame* frame) {
    if (!frame->code()->isCode()) {
      lines_.emplace_back("  <unknown>");
      return;
    }

    Thread* thread = Thread::currentThread();
    HandleScope scope(thread);
    Handle<Code> code(&scope, frame->code());
    std::stringstream line;

    // Extract filename
    if (code->filename()->isString()) {
      char* filename = String::cast(code->filename())->toCString();
      line << "  File '" << filename << "', ";
      std::free(filename);
    } else {
      line << "  File '<unknown>',  ";
    }

    // Extract line number
    Runtime* runtime = thread->runtime();
    word linenum =
        runtime->codeOffsetToLineNum(thread, code, frame->virtualPC());
    line << "line " << linenum << ", ";

    // Extract function
    if (code->name()->isString()) {
      char* name = String::cast(code->name())->toCString();
      line << "in " << name;
      std::free(name);
    } else {
      line << "in <unknown>";
    }

    lines_.emplace_back(line.str());
  };

  void print() {
    std::cerr << "Traceback (most recent call last)" << std::endl;
    for (auto it = lines_.rbegin(); it != lines_.rend(); it++) {
      std::cerr << *it << std::endl;
    }
  }

 private:
  std::vector<std::string> lines_;
};

void Utils::printTraceback() {
  TracebackPrinter printer;
  Thread::currentThread()->visitFrames(&printer);
  printer.print();
}

} // namespace python
