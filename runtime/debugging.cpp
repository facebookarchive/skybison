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

namespace py {

static bool dumpSimple(std::ostream& os, RawObject value);

static const char* kOpNames[] = {
#define OPNAME(name, num, handler) #name,
    FOREACH_BYTECODE(OPNAME)};

static std::ostream& dumpBytecode(std::ostream& os, const Bytes& bytecode,
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

static std::ostream& dumpMutableBytecode(std::ostream& os,
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

static void dumpCodeFlags(std::ostream& os, word flags) {
  if (flags & Code::kOptimized) os << " optimized";
  if (flags & Code::kNewlocals) os << " newlocals";
  if (flags & Code::kVarargs) os << " varargs";
  if (flags & Code::kVarkeyargs) os << " varkeyargs";
  if (flags & Code::kNested) os << " nested";
  if (flags & Code::kGenerator) os << " generator";
  if (flags & Code::kNofree) os << " nofree";
  if (flags & Code::kCoroutine) os << " coroutine";
  if (flags & Code::kIterableCoroutine) os << " iterable_coroutine";
  if (flags & Code::kAsyncGenerator) os << " async_generator";
}

std::ostream& dumpExtendedCode(std::ostream& os, RawCode value,
                               const char* indent) {
  HandleScope scope;
  Code code(&scope, value);
  os << "code " << code.name() << ":\n" << indent << "  flags:";
  dumpCodeFlags(os, code.flags());
  os << '\n';
  os << indent << "  argcount: " << code.argcount() << '\n'
     << indent << "  posonlyargcount: " << code.posonlyargcount() << '\n'
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

static std::ostream& operator<<(std::ostream& os, SymbolId value) {
  os << (value == SymbolId::kInvalid
             ? "<invalid>"
             : Thread::current()->runtime()->symbols()->predefinedSymbolAt(
                   value));
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
     << "  intrinsic_id: " << static_cast<SymbolId>(function.intrinsicId())
     << '\n'
     << "  dict: " << function.dict() << '\n'
     << "  flags:";
  word flags = function.flags();
  dumpCodeFlags(os, flags);
  if (flags & Function::Flags::kSimpleCall) os << " simple_call";
  if (flags & Function::Flags::kInterpreted) os << " interpreted";
  os << '\n';

  os << "  code: ";
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

std::ostream& dumpExtendedInstance(std::ostream& os, RawInstance value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Instance instance(&scope, value);
  LayoutId layout_id = instance.layoutId();
  os << "heap object with layout " << static_cast<word>(layout_id);
  Object layout_obj(&scope, runtime->layoutAtSafe(layout_id));
  if (!layout_obj.isLayout()) {
    os << '\n';
    return os;
  }
  Layout layout(&scope, *layout_obj);
  if (!runtime->isInstanceOfType(layout.describedType())) {
    os << '\n';
    return os;
  }
  Type type(&scope, layout.describedType());
  os << " (" << type << "):\n";
  Tuple in_object(&scope, layout.inObjectAttributes());
  Tuple entry(&scope, runtime->emptyTuple());
  for (word i = 0, length = in_object.length(); i < length; i++) {
    entry = in_object.at(i);
    AttributeInfo info(entry.at(1));
    os << "  (in-object) " << entry.at(0) << " = "
       << instance.instanceVariableAt(info.offset()) << '\n';
  }
  if (layout.hasTupleOverflow()) {
    Tuple overflow_attributes(&scope, layout.overflowAttributes());
    Tuple overflow(&scope,
                   instance.instanceVariableAt(layout.overflowOffset()));
    for (word i = 0, length = overflow_attributes.length(); i < length; i++) {
      entry = overflow_attributes.at(i);
      AttributeInfo info(entry.at(1));
      os << "  (overflow)  " << entry.at(0) << " = "
         << overflow.at(info.offset()) << '\n';
    }
  } else if (layout.hasDictOverflow()) {
    word offset = layout.dictOverflowOffset();
    os << "  overflow dict: " << instance.instanceVariableAt(offset) << '\n';
  }
  return os;
}

std::ostream& dumpExtendedLayout(std::ostream& os, RawLayout value,
                                 const char* indent) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Layout layout(&scope, value);
  os << indent << "layout " << static_cast<word>(layout.id()) << ":\n";
  Object type(&scope, layout.describedType());
  os << indent << "  described type: " << type << '\n';
  os << indent
     << "  num in-object attributes: " << layout.numInObjectAttributes()
     << '\n';
  Tuple in_object(&scope, layout.inObjectAttributes());
  Runtime* runtime = thread->runtime();
  Tuple entry(&scope, runtime->emptyTuple());
  for (word i = 0, length = in_object.length(); i < length; i++) {
    entry = in_object.at(i);
    AttributeInfo info(entry.at(1));
    os << indent << "    " << entry.at(0) << " @ " << info.offset() << '\n';
  }
  if (layout.hasTupleOverflow()) {
    os << indent << "  overflow tuple:\n";
    Tuple overflow_attributes(&scope, layout.overflowAttributes());
    for (word i = 0, length = overflow_attributes.length(); i < length; i++) {
      entry = overflow_attributes.at(i);
      AttributeInfo info(entry.at(1));
      os << indent << "    " << entry.at(0) << " @ " << info.offset() << '\n';
    }
  } else if (layout.hasDictOverflow()) {
    os << indent << "  overflow dict @ " << layout.dictOverflowOffset() << '\n';
  } else if (layout.isSealed()) {
    os << indent << "  sealed\n";
  } else {
    os << indent << "  invalid overflow\n";
  }
  return os;
}

std::ostream& dumpExtendedType(std::ostream& os, RawType value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, value);

  os << "type " << type.name() << ":\n";
  os << "  bases: " << type.bases() << '\n';
  os << "  mro: " << type.mro() << '\n';
  os << "  flags:";
  if (type.hasFlag(Type::kIsAbstract)) os << " abstract";
  if (type.hasFlag(Type::kHasDunderDict)) os << " has_dunder_dict";
  if (type.hasFlag(Type::kIsNativeProxy)) os << " is_native_proxy";
  os << '\n';
  Object builtin_base_layout(
      &scope, thread->runtime()->layoutAtSafe(type.builtinBase()));
  os << "  builtin base: ";
  if (builtin_base_layout.isLayout()) {
    os << builtin_base_layout << '\n';
  } else {
    os << "invalid layout\n";
  }
  if (type.instanceLayout().isLayout()) {
    dumpExtendedLayout(os, Layout::cast(type.instanceLayout()), "  ");
  } else {
    // I don't think this case should occur during normal operation, but maybe
    // we dump a type that isn't completely initialized yet.
    os << "  layout: " << type.instanceLayout() << '\n';
  }
  return os;
}

// The functions in this file may be used during garbage collection, so this
// function is used to approximate a read barrier until we have a better
// solution.
static RawObject checkForward(std::ostream& os, RawObject value) {
  if (!value.isHeapObject()) return value;
  RawHeapObject heap_obj = HeapObject::cast(value);
  if (!heap_obj.isForwarding()) return value;
  os << "<Forward to> ";
  return heap_obj.forward();
}

static std::ostream& dumpObjectGeneric(std::ostream& os, RawObject object_raw) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, object_raw);
  LayoutId id = object.layoutId();
  Object layout(&scope, thread->runtime()->layoutAtSafe(id));
  if (layout.isLayout()) {
    Object type_obj(&scope, Layout::cast(*layout).describedType());
    if (thread->runtime()->isInstanceOfType(*type_obj)) {
      Type type(&scope, *type_obj);
      Object name(&scope, type.name());
      if (name.isStr()) {
        return os << '<' << name << " object>";
      }
    }
  }
  return os << "<object with LayoutId " << static_cast<word>(id) << '>';
}

std::ostream& dumpExtended(std::ostream& os, RawObject value) {
  value = checkForward(os, value);
  LayoutId layout = value.layoutId();
  switch (layout) {
    case LayoutId::kCode:
      return dumpExtendedCode(os, Code::cast(value), "");
    case LayoutId::kFunction:
      return dumpExtendedFunction(os, Function::cast(value));
    case LayoutId::kLayout:
      return dumpExtendedLayout(os, Layout::cast(value), "");
    case LayoutId::kType:
      return dumpExtendedType(os, Type::cast(value));
    default:
      if (dumpSimple(os, value)) {
        return os << '\n';
      }
      if (value.isInstance()) {
        return dumpExtendedInstance(os, Instance::cast(value));
      }
      dumpObjectGeneric(os, value);
      return os << '\n';
  }
}

std::ostream& operator<<(std::ostream& os, CastError value) {
  switch (value) {
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

std::ostream& operator<<(std::ostream& os, RawByteArray value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, value);
  Object repr_obj(&scope, byteArrayRepr(thread, self));
  if (repr_obj.isError()) return os << "<ERROR: An exception occurred.>";
  Str repr(&scope, *repr_obj);
  unique_c_ptr<char[]> data(repr.toCStr());
  return os.write(data.get(), repr.charLength());
}

std::ostream& operator<<(std::ostream& os, RawBytes value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Bytes self(&scope, value);
  Str repr(&scope, bytesReprSmartQuotes(thread, self));
  unique_c_ptr<char[]> data(repr.toCStr());
  return os.write(data.get(), repr.charLength());
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
  os.write(data.get(), str.charLength());
  return os << '"';
}

std::ostream& operator<<(std::ostream& os, RawLayout value) {
  Thread* thread = Thread::current();
  os << "<layout " << static_cast<word>(value.id());
  if (thread->runtime()->isInstanceOfType(value.describedType())) {
    HandleScope scope;
    Type type(&scope, value.describedType());
    os << " (" << type.name() << ')';
  }
  return os << '>';
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

std::ostream& operator<<(std::ostream& os, RawObject value) {
  value = checkForward(os, value);
  if (dumpSimple(os, value)) return os;
  return dumpObjectGeneric(os, value);
}

std::ostream& operator<<(std::ostream& os, RawSmallInt value) {
  return os << value.value();
}

std::ostream& operator<<(std::ostream& os, RawSmallStr value) {
  HandleScope scope;
  Str str(&scope, value);
  byte buffer[RawSmallStr::kMaxLength];
  word length = str.charLength();
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

std::ostream& operator<<(std::ostream& os, RawValueCell value) {
  os << "<value_cell ";
  if (value.isPlaceholder()) {
    os << "placeholder>";
  } else {
    os << '(' << value.value() << ")>";
  }
  return os;
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
  word num_locals = 0;
  if (frame->isSentinel()) {
    os << "- initial frame\n";
  } else if (!frame->function().isFunction()) {
    os << "- function: <invalid>\n";
  } else {
    Function function(&scope, frame->function());
    num_locals = frame->function().totalLocals();
    os << "- function: " << function << '\n';
    if (function.code().isCode()) {
      Code code(&scope, function.code());
      os << "  code: " << code.name() << '\n';
      if (code.isNative()) {
        os << "  pc: n/a (native)\n";
      } else {
        word pc = frame->virtualPC();
        os << "  pc: " << pc;

        // Print filename and line number, if possible.
        os << " (" << code.filename();
        if (code.lnotab().isBytes()) {
          os << ":" << code.offsetToLineNum(pc);
        }
        os << ")";
        os << '\n';
      }
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

static bool dumpSimple(std::ostream& os, RawObject value) {
  switch (value.layoutId()) {
    case LayoutId::kBool:
      os << Bool::cast(value);
      return true;
    case LayoutId::kBoundMethod:
      os << BoundMethod::cast(value);
      return true;
    case LayoutId::kByteArray:
      os << ByteArray::cast(value);
      return true;
    case LayoutId::kCode:
      os << Code::cast(value);
      return true;
    case LayoutId::kDict:
      os << Dict::cast(value);
      return true;
    case LayoutId::kError:
      os << Error::cast(value);
      return true;
    case LayoutId::kFloat:
      os << Float::cast(value);
      return true;
    case LayoutId::kFunction:
      os << Function::cast(value);
      return true;
    case LayoutId::kLargeBytes:
      os << Bytes::cast(value);
      return true;
    case LayoutId::kLargeInt:
      os << LargeInt::cast(value);
      return true;
    case LayoutId::kLargeStr:
      os << LargeStr::cast(value);
      return true;
    case LayoutId::kLayout:
      os << Layout::cast(value);
      return true;
    case LayoutId::kList:
      os << List::cast(value);
      return true;
    case LayoutId::kModule:
      os << Module::cast(value);
      return true;
    case LayoutId::kMutableBytes:
      os << Bytes::cast(value);
      return true;
    case LayoutId::kNoneType:
      os << NoneType::cast(value);
      return true;
    case LayoutId::kSmallBytes:
      os << Bytes::cast(value);
      return true;
    case LayoutId::kSmallInt:
      os << SmallInt::cast(value);
      return true;
    case LayoutId::kSmallStr:
      os << SmallStr::cast(value);
      return true;
    case LayoutId::kTuple:
      os << Tuple::cast(value);
      return true;
    case LayoutId::kType:
      os << Type::cast(value);
      return true;
    case LayoutId::kValueCell:
      os << ValueCell::cast(value);
      return true;
    default:
      return false;
  }
}

USED void dump(RawObject object) { dumpExtended(std::cerr, object); }

USED void dump(const Object& object) { dumpExtended(std::cerr, *object); }

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

}  // namespace py
