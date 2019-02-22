#include "debugging.h"

#include <iomanip>
#include <iostream>

#include "frame.h"
#include "handles.h"
#include "runtime.h"
#include "vector.h"

namespace python {

std::ostream& operator<<(std::ostream& os, CastError err) {
  switch (err) {
    case CastError::None:
      return os << "None";
    case CastError::Underflow:
      return os << "Underflow";
    case CastError::Overflow:
      return os << "Overflow";
  }
  return os << "<invalid>";
}

std::ostream& operator<<(std::ostream& os, RawBool value) {
  return os << (value.value() ? "True" : "False");
}

std::ostream& operator<<(std::ostream& os, RawError) { return os << "Error"; }

std::ostream& operator<<(std::ostream& os, RawFloat value) {
  std::ios_base::fmtflags saved = os.flags();
  os << std::hexfloat << value.value();
  os.flags(saved);
  return os;
}

std::ostream& operator<<(std::ostream& os, RawFunction value) {
  return os << "<function " << value.qualname() << '>';
}

std::ostream& operator<<(std::ostream& os, RawInt value) {
  if (value.isSmallInt()) return os << RawSmallInt::cast(value);
  if (value.isBool()) return os << RawBool::cast(value);
  return os << RawLargeInt::cast(value);
}

std::ostream& operator<<(std::ostream& os, RawLargeInt value) {
  HandleScope scope;
  LargeInt large_int(&scope, value);

  os << "largeint([";
  for (word i = 0, num_digits = large_int.numDigits(); i < num_digits; i++) {
    uword digit = large_int.digitAt(i);
    if (i > 0) {
      os << ", ";
    }
    os << "0x";
    std::ios_base::fmtflags saved_flags = os.flags();
    char saved_fill = os.fill('0');
    os << std::setw(16) << std::hex << digit;
    os.fill(saved_fill);
    os.flags(saved_flags);
  }
  return os << "])";
}

std::ostream& operator<<(std::ostream& os, RawLargeStr value) {
  HandleScope scope;
  Str str(&scope, value);
  unique_c_ptr<char[]> data(str.toCStr());
  os << '"';
  os.write(data.get(), str.length());
  return os << '"';
}

std::ostream& operator<<(std::ostream& os, RawList value) {
  HandleScope scope;
  List list(&scope, value);
  os << '[';
  for (word i = 0, num_itesm = list.numItems(); i < num_itesm; i++) {
    if (i > 0) os << ", ";
    os << list.at(i);
  }
  return os << ']';
}

std::ostream& operator<<(std::ostream& os, RawNoneType) { return os << "None"; }

static std::ostream& printObjectGeneric(std::ostream& os,
                                        RawObject object_raw) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object object(&scope, object_raw);
  Object type_obj(&scope, thread->runtime()->typeOf(*object));
  if (thread->runtime()->isInstanceOfType(*type_obj)) {
    Type type(&scope, *type_obj);
    Object name(&scope, type.name());
    if (name.isStr()) {
      return os << '<' << name << " object>";
    }
  }
  return os << "<object with LayoutId " << static_cast<word>(object.layoutId())
            << '>';
}

std::ostream& operator<<(std::ostream& os, RawObject value) {
  LayoutId layout = value.layoutId();
  switch (layout) {
    case LayoutId::kBool:
      return os << RawBool::cast(value);
    case LayoutId::kError:
      return os << RawError::cast(value);
    case LayoutId::kFloat:
      return os << RawFloat::cast(value);
    case LayoutId::kFunction:
      return os << RawFunction::cast(value);
    case LayoutId::kLargeInt:
      return os << RawLargeInt::cast(value);
    case LayoutId::kLargeStr:
      return os << RawLargeStr::cast(value);
    case LayoutId::kList:
      return os << RawList::cast(value);
    case LayoutId::kNoneType:
      return os << RawNoneType::cast(value);
    case LayoutId::kSmallInt:
      return os << RawSmallInt::cast(value);
    case LayoutId::kSmallStr:
      return os << RawSmallStr::cast(value);
    case LayoutId::kTuple:
      return os << RawTuple::cast(value);
    case LayoutId::kType:
      return os << RawType::cast(value);
    default:
      return printObjectGeneric(os, value);
  }
}

std::ostream& operator<<(std::ostream& os, RawSmallInt value) {
  return os << value.value();
}

std::ostream& operator<<(std::ostream& os, RawSmallStr value) {
  HandleScope scope;
  Str str(&scope, value);
  byte buffer[RawSmallStr::kMaxLength];
  word length = str.length();
  DCHECK(static_cast<size_t>(length) <= sizeof(buffer), "Buffer too small");
  str.copyTo(buffer, length);
  os << '"';
  os.write(reinterpret_cast<const char*>(buffer), length);
  return os << '"';
}

std::ostream& operator<<(std::ostream& os, RawStr value) {
  if (value.isSmallStr()) return os << RawSmallStr::cast(value);
  return os << RawLargeStr::cast(value);
}

std::ostream& operator<<(std::ostream& os, RawTuple value) {
  HandleScope scope;
  Tuple tuple(&scope, value);
  os << '(';
  word length = tuple.length();
  for (word i = 0; i < length; i++) {
    if (i > 0) os << ", ";
    os << tuple.at(i);
  }
  if (length == 1) os << ',';
  return os << ')';
}

std::ostream& operator<<(std::ostream& os, RawType value) {
  return os << "<type " << value.name() << ">";
}

static void dumpSingleFrame(Thread* thread, std::ostream& os, Frame* frame) {
  HandleScope scope(thread);

  Tuple var_names(&scope, thread->runtime()->newTuple(0));
  Object code(&scope, frame->code());
  if (code.isCode()) {
    Object names_raw(&scope, RawCode::cast(*code).varnames());
    if (names_raw.isTuple()) {
      var_names = *names_raw;
    }
  }

  os << "- pc: " << frame->virtualPC() << '\n';
  if (frame->previousFrame() != nullptr) {
    os << "  function: " << frame->function() << '\n';
  }
  // TODO(matthiasb): Also dump the block stack.
  word var_names_length = var_names.length();
  for (word l = 0, num_locals = frame->numLocals(); l < num_locals; l++) {
    os << "  " << l;
    if (l < var_names_length) {
      os << ' ' << var_names.at(l);
    }
    os << ": " << frame->local(l) << '\n';
  }
}

std::ostream& operator<<(std::ostream& os, Frame* frame) {
  if (frame == nullptr) {
    return os << "<nullptr>";
  }

  Vector<Frame*> frames;
  for (Frame* f = frame; f != nullptr; f = f->previousFrame()) {
    frames.push_back(f);
  }

  Thread* thread = Thread::currentThread();
  for (word i = frames.size() - 1; i >= 0; i--) {
    dumpSingleFrame(thread, os, frames[i]);
  }
  return os;
}

__attribute__((used)) void dump(RawObject object) {
  std::cerr << object << '\n';
}

__attribute__((used)) void dump(const Object& object) {
  std::cerr << object << '\n';
}

__attribute__((used)) void dump(Frame* frame) { std::cerr << frame; }

void initializeDebugging() {
  // This function must be called even though it is empty. If it is not called
  // then there is no reference from another file left and the linker will not
  // even look at the whole compilation unit and miss the `attribute((used))`
  // annotations on the dump functions.
}

}  // namespace python
