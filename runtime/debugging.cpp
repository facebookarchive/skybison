#include "debugging.h"

#include <iomanip>
#include <iostream>

#include "bytearray-builtins.h"
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

std::ostream& dumpBytecode(std::ostream& os, const Bytes& bytecode,
                           const char* indent) {
  for (word i = 0, length = bytecode.length(); i + 1 < length; i += 2) {
    byte op = bytecode.byteAt(i);
    byte arg = bytecode.byteAt(i + 1);
    std::ios_base::fmtflags saved_flags = os.flags();
    os << indent << "  " << std::setw(4) << std::hex << i << ' ';
    os.flags(saved_flags);
    os << kOpNames[op] << " " << static_cast<unsigned>(arg) << '\n';
  }
  return os;
}

std::ostream& dumpMutableBytecode(std::ostream& os,
                                  const MutableBytes& bytecode,
                                  const char* indent) {
  for (word i = 0, length = bytecode.length(); i + 1 < length; i += 2) {
    byte op = bytecode.byteAt(i);
    byte arg = bytecode.byteAt(i + 1);
    std::ios_base::fmtflags saved_flags = os.flags();
    os << indent << "  " << std::setw(4) << std::hex << i << ' ';
    os.flags(saved_flags);
    os << kOpNames[op] << " " << static_cast<unsigned>(arg) << '\n';
  }
  return os;
}

std::ostream& dumpExtendedCode(std::ostream& os, RawCode value,
                               const char* indent) {
  HandleScope scope;
  Code code(&scope, value);
  os << "code " << code.name() << ":\n" << indent << "  flags:";
  word flags = code.flags();
  if (flags & Code::OPTIMIZED) os << " optimized";
  if (flags & Code::NEWLOCALS) os << " newlocals";
  if (flags & Code::VARARGS) os << " varargs";
  if (flags & Code::VARKEYARGS) os << " varkeyargs";
  if (flags & Code::NESTED) os << " nested";
  if (flags & Code::GENERATOR) os << " generator";
  if (flags & Code::NOFREE) os << " nofree";
  if (flags & Code::COROUTINE) os << " coroutine";
  if (flags & Code::ITERABLE_COROUTINE) os << " iterable_coroutine";
  if (flags & Code::ASYNC_GENERATOR) os << " async_generator";
  if (flags & Code::SIMPLE_CALL) os << " simple_call";
  os << '\n';
  os << indent << "  argcount: " << code.argcount() << '\n'
     << indent << "  kwonlyargcount: " << code.kwonlyargcount() << '\n'
     << indent << "  nlocals: " << code.nlocals() << '\n'
     << indent << "  stacksize: " << code.stacksize() << '\n'
     << indent << "  filename: " << code.filename() << '\n'
     << indent << "  consts: " << code.consts() << '\n'
     << indent << "  names: " << code.names() << '\n'
     << indent << "  cellvars: " << code.cellvars() << '\n'
     << indent << "  freevars: " << code.freevars() << '\n'
     << indent << "  varnames: " << code.varnames() << '\n';
  Object bytecode_obj(&scope, code.code());
  if (bytecode_obj.isBytes()) {
    Bytes bytecode(&scope, *bytecode_obj);
    dumpBytecode(os, bytecode, indent);
  }

  return os;
}

std::ostream& dumpExtendedFunction(std::ostream& os, RawFunction value) {
  HandleScope scope;
  Function function(&scope, value);
  os << "function " << function.name() << ":\n"
     << "  qualname: " << function.qualname() << '\n'
     << "  module: " << function.module() << '\n'
     << "  annotations: " << function.annotations() << '\n'
     << "  closure: " << function.closure() << '\n'
     << "  defaults: " << function.defaults() << '\n'
     << "  kwdefaults: " << function.kwDefaults() << '\n'
     << "  dict: " << function.dict() << '\n'
     << "  code: ";
  if (function.code().isCode()) {
    dumpExtendedCode(os, Code::cast(function.code()), "  ");
    if (function.rewrittenBytecode().isMutableBytes()) {
      MutableBytes bytecode(&scope, function.rewrittenBytecode());
      os << "  Rewritten bytecode:\n";
      dumpMutableBytecode(os, bytecode, "");
    }
  } else {
    os << function.code() << '\n';
  }
  return os;
}

std::ostream& dumpExtendedHeapObject(std::ostream& os, RawHeapObject value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  HeapObject heap_object(&scope, value);
  Layout layout(&scope, runtime->layoutAt(heap_object.layoutId()));
  Type type(&scope, layout.describedType());
  os << "heap object " << type << ":\n";
  Tuple in_object(&scope, layout.inObjectAttributes());
  Tuple entry(&scope, runtime->newTuple(0));
  for (word i = 0, length = in_object.length(); i < length; i++) {
    entry = in_object.at(i);
    AttributeInfo info(entry.at(1));
    os << "  (in-object) " << entry.at(0) << " = "
       << heap_object.instanceVariableAt(info.offset()) << '\n';
  }
  Object overflow_attributes_obj(&scope, layout.overflowAttributes());
  if (overflow_attributes_obj.isTuple()) {
    Tuple overflow_attributes(&scope, *overflow_attributes_obj);
    Tuple overflow(&scope,
                   heap_object.instanceVariableAt(layout.overflowOffset()));
    for (word i = 0, length = overflow_attributes.length(); i < length; i++) {
      entry = overflow_attributes.at(i);
      AttributeInfo info(entry.at(1));
      os << "  (overflow)  " << entry.at(0) << " = "
         << overflow.at(info.offset()) << '\n';
    }
  } else if (overflow_attributes_obj.isSmallInt()) {
    word offset = RawSmallInt::cast(*overflow_attributes_obj).value();
    os << "  overflow dict: " << heap_object.instanceVariableAt(offset) << '\n';
  }
  return os;
}

std::ostream& dumpExtended(std::ostream& os, RawObject value) {
  LayoutId layout = value.layoutId();
  switch (layout) {
    case LayoutId::kCode:
      return dumpExtendedCode(os, Code::cast(value), "");
    case LayoutId::kFunction:
      return dumpExtendedFunction(os, Function::cast(value));
    default:
      if (value.isInstance()) {
        return dumpExtendedHeapObject(os, RawHeapObject::cast(value));
      }
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

std::ostream& operator<<(std::ostream& os, RawBoundMethod value) {
  return os << "<bound_method " << Function::cast(value.function()).qualname()
            << ", " << value.self() << '>';
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
  Object iter_obj(&scope, runtime->newDictItemIterator(thread, dict));
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

std::ostream& operator<<(std::ostream& os, RawError value) {
  os << "Error";
  switch (value.kind()) {
    case ErrorKind::kNone:
      return os;
    case ErrorKind::kException:
      return os << "<Exception>";
    case ErrorKind::kNotFound:
      return os << "<NotFound>";
    case ErrorKind::kOutOfBounds:
      return os << "<OutOfBounds>";
    case ErrorKind::kOutOfMemory:
      return os << "<OutOfMemory>";
    case ErrorKind::kNoMoreItems:
      return os << "<NoMoreItems>";
  }
  return os << "<Invalid>";
}

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
  if (value.isSmallInt()) return os << SmallInt::cast(value);
  if (value.isBool()) return os << Bool::cast(value);
  return os << LargeInt::cast(value);
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
      return os << Bool::cast(value);
    case LayoutId::kBoundMethod:
      return os << BoundMethod::cast(value);
    case LayoutId::kCode:
      return os << Code::cast(value);
    case LayoutId::kDict:
      return os << Dict::cast(value);
    case LayoutId::kError:
      return os << Error::cast(value);
    case LayoutId::kFloat:
      return os << Float::cast(value);
    case LayoutId::kFunction:
      return os << Function::cast(value);
    case LayoutId::kLargeBytes:
      return os << Bytes::cast(value);
    case LayoutId::kLargeInt:
      return os << LargeInt::cast(value);
    case LayoutId::kLargeStr:
      return os << LargeStr::cast(value);
    case LayoutId::kList:
      return os << List::cast(value);
    case LayoutId::kModule:
      return os << Module::cast(value);
    case LayoutId::kNoneType:
      return os << NoneType::cast(value);
    case LayoutId::kSmallBytes:
      return os << Bytes::cast(value);
    case LayoutId::kSmallInt:
      return os << SmallInt::cast(value);
    case LayoutId::kSmallStr:
      return os << SmallStr::cast(value);
    case LayoutId::kTuple:
      return os << Tuple::cast(value);
    case LayoutId::kType:
      return os << Type::cast(value);
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
  if (value.isSmallStr()) return os << SmallStr::cast(value);
  return os << LargeStr::cast(value);
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
  if (const char* invalid = frame->isInvalid()) {
    os << "- invalid frame (" << invalid << ")\n";
    return;
  }

  HandleScope scope(thread);

  Tuple var_names(&scope, thread->runtime()->emptyTuple());
  Tuple freevar_names(&scope, thread->runtime()->emptyTuple());
  Tuple cellvar_names(&scope, thread->runtime()->emptyTuple());
  bool output_pc = true;
  if (frame->previousFrame() == nullptr) {
    os << "- initial frame\n";
  } else if (!frame->function().isFunction()) {
    os << "- function: <invalid>\n";
  } else {
    Function function(&scope, frame->function());
    os << "- function: " << function << '\n';
    if (function.code().isCode()) {
      Code code(&scope, function.code());
      os << "  code: " << code.name() << '\n';
      os << "  pc: " << frame->virtualPC();

      // Print filename and line number, if possible.
      os << " (" << code.filename();
      if (code.lnotab().isBytes()) {
        os << ":"
           << thread->runtime()->codeOffsetToLineNum(thread, code,
                                                     frame->virtualPC());
      }
      os << ")";
      os << '\n';
      output_pc = false;

      if (code.varnames().isTuple()) {
        var_names = code.varnames();
      }
      if (code.cellvars().isTuple()) {
        cellvar_names = code.cellvars();
      }
      if (code.freevars().isTuple()) {
        freevar_names = code.freevars();
      }
    }
  }
  if (output_pc) {
    os << "  pc: " << frame->virtualPC() << '\n';
  }

  // TODO(matthiasb): Also dump the block stack.
  word var_names_length = var_names.length();
  word cellvar_names_length = cellvar_names.length();
  word freevar_names_length = freevar_names.length();
  word num_locals = frame->numLocals();
  if (num_locals > 0) os << "  locals:\n";
  for (word l = 0; l < num_locals; l++) {
    os << "    " << l;
    if (l < var_names_length) {
      os << ' ' << var_names.at(l);
    } else if (l < var_names_length + freevar_names_length) {
      os << ' ' << freevar_names.at(l - var_names_length);
    } else if (l <
               var_names_length + freevar_names_length + cellvar_names_length) {
      os << ' '
         << cellvar_names.at(l - var_names_length - cellvar_names_length);
    }
    os << ": " << frame->local(l) << '\n';
  }

  word stack_size = frame->valueStackSize();
  if (stack_size > 0) os << "  stack:\n";
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

USED void dump(RawObject object) {
  dumpExtended(std::cerr, object);
  std::cerr << '\n';
}

USED void dump(const Object& object) {
  dumpExtended(std::cerr, *object);
  std::cerr << '\n';
}

USED void dump(Frame* frame) { std::cerr << frame; }

USED void dumpSingleFrame(Frame* frame) {
  dumpSingleFrame(Thread::current(), std::cerr, frame);
}

void initializeDebugging() {
  // This function must be called even though it is empty. If it is not called
  // then there is no reference from another file left and the linker will not
  // even look at the whole compilation unit and miss the `attribute((used))`
  // annotations on the dump functions.
}

}  // namespace python
