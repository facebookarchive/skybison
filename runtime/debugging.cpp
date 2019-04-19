#include "debugging.h"

#include <iomanip>
#include <iostream>

#include "bytecode.h"
#include "bytes-builtins.h"
#include "dict-builtins.h"
#include "frame.h"
#include "handles.h"
#include "runtime.h"
#include "vector.h"

namespace python {

static const char* kOpNames[] = {
#define OPNAME(name, num, handler) #name,
    FOREACH_BYTECODE(OPNAME)};

std::ostream& dumpExtendedCode(std::ostream& os, RawCode value) {
  HandleScope scope;
  Code code(&scope, value);
  os << "name: " << code.name() << " argcount: " << code.argcount()
     << " kwonlyargcount: " << code.kwonlyargcount()
     << " nlocals: " << code.nlocals() << " stacksize: " << code.stacksize()
     << '\n'
     << "filename: " << code.filename() << '\n'
     << "consts: " << code.consts() << '\n'
     << "names: " << code.names() << '\n'
     << "cellvars: " << code.cellvars() << '\n'
     << "freevars: " << code.freevars() << '\n'
     << "varnames: " << code.varnames() << '\n';
  Object bytecode_obj(&scope, code.code());
  if (bytecode_obj.isBytes()) {
    Bytes bytecode(&scope, *bytecode_obj);
    for (word i = 0, length = bytecode.length(); i + 1 < length; i += 2) {
      byte op = bytecode.byteAt(i);
      byte arg = bytecode.byteAt(i + 1);
      std::ios_base::fmtflags saved_flags = os.flags();
      os << std::setw(4) << std::hex << i << ' ';
      os.flags(saved_flags);
      os << kOpNames[op] << " " << static_cast<unsigned>(arg) << '\n';
    }
  }

  return os;
}

std::ostream& dumpExtendedFunction(std::ostream& os, RawFunction value) {
  HandleScope scope;
  Function function(&scope, value);
  os << "name: " << function.name() << '\n'
     << "qualname: " << function.qualname() << '\n'
     << "module: " << function.module() << '\n'
     << "annotations: " << function.annotations() << '\n'
     << "closure: " << function.closure() << '\n'
     << "defaults: " << function.defaults() << '\n'
     << "kwdefaults: " << function.kwDefaults() << '\n'
     << "code: ";
  if (function.code().isCode()) {
    dumpExtendedCode(os, RawCode::cast(function.code()));
  } else {
    os << function.code() << '\n';
  }
  return os;
}

std::ostream& dumpExtended(std::ostream& os, RawObject value) {
  LayoutId layout = value.layoutId();
  switch (layout) {
    case LayoutId::kCode:
      return dumpExtendedCode(os, RawCode::cast(value));
    case LayoutId::kFunction:
      return dumpExtendedFunction(os, RawFunction::cast(value));
    default:
      return os << value;
  }
}

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

std::ostream& operator<<(std::ostream& os, RawBytes value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Bytes self(&scope, value);
  Str repr(&scope, bytesReprSmartQuotes(thread, self));
  unique_c_ptr<char[]> data(repr.toCStr());
  return os.write(data.get(), repr.length());
}

std::ostream& operator<<(std::ostream& os, RawCode value) {
  return os << "<code " << value.name() << ">";
}

std::ostream& operator<<(std::ostream& os, RawDict value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Dict dict(&scope, value);
  Object iter_obj(&scope, runtime->newDictItemIterator(dict));
  if (!iter_obj.isDictItemIterator()) return os;
  os << '{';
  DictItemIterator iter(&scope, *iter_obj);
  const char* delimiter = "";
  for (Object key_value_obj(&scope, NoneType::object());
       !(key_value_obj = dictItemIteratorNext(thread, iter)).isError();) {
    Tuple key_value(&scope, *key_value_obj);
    os << delimiter << key_value.at(0) << ": " << key_value.at(1);
    delimiter = ", ";
  }
  return os << '}';
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

std::ostream& operator<<(std::ostream& os, RawModule value) {
  return os << "<module " << value.name() << ">";
}

std::ostream& operator<<(std::ostream& os, RawNoneType) { return os << "None"; }

static std::ostream& printObjectGeneric(std::ostream& os,
                                        RawObject object_raw) {
  Thread* thread = Thread::current();
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
    case LayoutId::kCode:
      return os << RawCode::cast(value);
    case LayoutId::kDict:
      return os << RawDict::cast(value);
    case LayoutId::kError:
      return os << RawError::cast(value);
    case LayoutId::kFloat:
      return os << RawFloat::cast(value);
    case LayoutId::kFunction:
      return os << RawFunction::cast(value);
    case LayoutId::kLargeBytes:
      return os << RawBytes::cast(value);
    case LayoutId::kLargeInt:
      return os << RawLargeInt::cast(value);
    case LayoutId::kLargeStr:
      return os << RawLargeStr::cast(value);
    case LayoutId::kList:
      return os << RawList::cast(value);
    case LayoutId::kModule:
      return os << RawModule::cast(value);
    case LayoutId::kNoneType:
      return os << RawNoneType::cast(value);
    case LayoutId::kSmallBytes:
      return os << RawBytes::cast(value);
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
  Object code_obj(&scope, frame->code());
  if (code_obj.isCode()) {
    Object names_raw(&scope, RawCode::cast(*code_obj).varnames());
    if (names_raw.isTuple()) {
      var_names = *names_raw;
    }
  }

  os << "- pc: " << frame->virtualPC();

  // Print filename and line number, if possible.
  if (code_obj.isCode()) {
    Code code(&scope, *code_obj);
    os << " (" << code.filename();
    if (code.lnotab().isBytes()) {
      os << ":"
         << thread->runtime()->codeOffsetToLineNum(thread, code,
                                                   frame->virtualPC());
    }
    os << ")";
  }
  os << '\n';

  Object function(&scope, frame->function());
  if (!function.isError()) {
    os << "  function: " << function << '\n';
  } else if (code_obj.isCode()) {
    Code code(&scope, *code_obj);
    os << "  code: " << code.name() << '\n';
  }

  // TODO(matthiasb): Also dump the block stack.
  word var_names_length = var_names.length();
  word num_locals = frame->numLocals();
  if (num_locals > 0) os << "  - locals:\n";
  for (word l = 0; l < num_locals; l++) {
    os << "    " << l;
    if (l < var_names_length) {
      os << ' ' << var_names.at(l);
    }
    os << ": " << frame->local(l) << '\n';
  }

  word stack_size = frame->valueStackSize();
  if (stack_size > 0) os << "  - stack:\n";
  for (word i = stack_size - 1; i >= 0; i--) {
    os << "    " << i << ": " << frame->peek(i) << '\n';
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

  Thread* thread = Thread::current();
  for (word i = frames.size() - 1; i >= 0; i--) {
    dumpSingleFrame(thread, os, frames[i]);
  }
  return os;
}

__attribute__((used)) void dump(RawObject object) {
  dumpExtended(std::cerr, object);
  std::cerr << '\n';
}

__attribute__((used)) void dump(const Object& object) {
  dumpExtended(std::cerr, *object);
  std::cerr << '\n';
}

__attribute__((used)) void dump(Frame* frame) { std::cerr << frame; }

void initializeDebugging() {
  // This function must be called even though it is empty. If it is not called
  // then there is no reference from another file left and the linker will not
  // even look at the whole compilation unit and miss the `attribute((used))`
  // annotations on the dump functions.
}

}  // namespace python
