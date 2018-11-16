#include "runtime.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <memory>

#include "builtins.h"
#include "bytecode.h"
#include "callback.h"
#include "globals.h"
#include "handles.h"
#include "heap.h"
#include "interpreter.h"
#include "marshal.h"
#include "os.h"
#include "scavenger.h"
#include "siphash.h"
#include "thread.h"
#include "trampolines-inl.h"
#include "utils.h"
#include "visitor.h"

namespace python {

Runtime::Runtime(word heap_size)
    : heap_(heap_size), new_value_cell_callback_(this) {
  initializeRandom();
  initializeThreads();
  // This must be called before initializeClasses is called. Methods in
  // initializeClasses rely on instances that are created in this method.
  initializePrimitiveInstances();
  initializeInterned();
  initializeSymbols();
  initializeClasses();
  initializeModules();
  Interpreter::initOpTable();
}

Runtime::Runtime() : Runtime(64 * MiB) {}

Runtime::~Runtime() {
  for (Thread* thread = threads_; thread != nullptr;) {
    if (thread == Thread::currentThread()) {
      Thread::setCurrentThread(nullptr);
    } else {
      assert(0); // Not implemented.
    }
    auto prev = thread;
    thread = thread->next();
    delete prev;
  }
  delete symbols_;
}

Object* Runtime::newBoundMethod(Object* function, Object* self) {
  HandleScope scope;
  Handle<BoundMethod> bound_method(&scope, heap()->createBoundMethod());
  bound_method->setFunction(function);
  bound_method->setSelf(self);
  return *bound_method;
}

Object* Runtime::newByteArray(word length, byte fill) {
  assert(length >= 0);
  if (length == 0) {
    return empty_byte_array_;
  }
  Object* result = heap()->createByteArray(length);
  byte* dst = reinterpret_cast<byte*>(ByteArray::cast(result)->address());
  std::memset(dst, fill, length);
  return result;
}

Object* Runtime::newByteArrayWithAll(View<byte> array) {
  if (array.length() == 0) {
    return empty_byte_array_;
  }
  Object* result = heap()->createByteArray(array.length());
  byte* dst = reinterpret_cast<byte*>(ByteArray::cast(result)->address());
  std::memcpy(dst, array.data(), array.length());
  return result;
}

Object* Runtime::newClass() {
  return newClassWithId(newClassId());
}

Object* Runtime::classGetAttr(
    Thread* thread,
    const Handle<Object>& receiver,
    const Handle<Object>& name) {
  if (!name->isString()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->throwTypeErrorFromCString("attribute name must be a string");
  }

  HandleScope scope(thread);
  Handle<Class> klass(&scope, *receiver);
  Handle<Class> meta_klass(&scope, classOf(*receiver));

  // Look for the attribute in the meta class
  Handle<Object> meta_attr(&scope, lookupNameInMro(thread, meta_klass, name));
  if (isDataDescriptor(thread, meta_attr)) {
    // TODO(T25692531): Call __get__ from meta_attr
    CHECK(false, "custom descriptors are unsupported");
  }

  // No data descriptor found on the meta class, look in the mro of the klass
  Handle<Object> attr(&scope, lookupNameInMro(thread, klass, name));
  if (!attr->isError()) {
    if (attr->isFunction()) {
      Handle<Object> none(&scope, None::object());
      return functionDescriptorGet(thread, attr, none, receiver);
    } else if (attr->isClassMethod()) {
      Handle<Object> none(&scope, None::object());
      return classmethodDescriptorGet(thread, attr, none, receiver);
    } else if (isNonDataDescriptor(thread, attr)) {
      // TODO(T25692531): Call __get__ from meta_attr
      CHECK(false, "custom descriptors are unsupported");
    }
    return *attr;
  }

  // No attr found in klass or its mro, use the non-data descriptor found in
  // the metaclass (if any).
  if (isNonDataDescriptor(thread, meta_attr)) {
    if (meta_attr->isFunction()) {
      Handle<Object> mk(&scope, *meta_klass);
      return functionDescriptorGet(thread, meta_attr, receiver, mk);
    } else if (meta_attr->isClassMethod()) {
      Handle<Object> mk(&scope, *meta_klass);
      return classmethodDescriptorGet(thread, meta_attr, receiver, mk);
    } else {
      // TODO(T25692531): Call __get__ from meta_attr
      CHECK(false, "custom descriptors are unsupported");
    }
  }

  // If a regular attribute was found in the metaclass, return it
  if (!meta_attr->isError()) {
    return *meta_attr;
  }

  // TODO(T25140871): Refactor this into something like:
  //     thread->throwMissingAttributeError(name)
  return thread->throwAttributeErrorFromCString("missing attribute");
}

Object* Runtime::classSetAttr(
    Thread* thread,
    const Handle<Object>& receiver,
    const Handle<Object>& name,
    const Handle<Object>& value) {
  if (!name->isString()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->throwTypeErrorFromCString("attribute name must be a string");
  }

  HandleScope scope(thread);
  Handle<Class> klass(&scope, *receiver);
  if (klass->isIntrinsicOrExtension()) {
    // TODO(T25140871): Refactor this into something that includes the type name
    // like:
    //     thread->throwImmutableTypeManipulationError(klass)
    return thread->throwTypeErrorFromCString(
        "can't set attributes of built-in/extension type");
  }

  // Check for a data descriptor
  Handle<Class> metaklass(&scope, classOf(*receiver));
  Handle<Object> meta_attr(&scope, lookupNameInMro(thread, metaklass, name));
  if (isDataDescriptor(thread, meta_attr)) {
    // TODO(T25692531): Call __set__ from meta_attr
    CHECK(false, "custom descriptors are unsupported");
  }

  // No data descriptor found, store the attribute in the klass dictionary
  Handle<Dictionary> klass_dict(&scope, klass->dictionary());
  dictionaryAtPutInValueCell(klass_dict, name, value);

  return None::object();
}

// Generic attribute lookup code used for instance objects
Object* Runtime::instanceGetAttr(
    Thread* thread,
    const Handle<Object>& receiver,
    const Handle<Object>& name) {
  if (!name->isString()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->throwTypeErrorFromCString("attribute name must be a string");
  }

  // Look for the attribute in the class
  HandleScope scope(thread);
  Handle<Class> klass(&scope, classOf(*receiver));
  Handle<Object> klass_attr(&scope, lookupNameInMro(thread, klass, name));
  if (isDataDescriptor(thread, klass_attr)) {
    // TODO(T25692531): Call __get__ from klass_attr
    UNIMPLEMENTED("custom descriptors are unsupported");
  }

  // No data descriptor found on the class, look at the in-instance
  // attributes.
  //
  // TODO: Support overflow attributes
  Handle<HeapObject> instance(&scope, *receiver);
  Handle<ObjectArray> map(&scope, klass->instanceAttributeMap());
  for (word i = 0; i < map->length(); i++) {
    if (map->at(i) == *name) {
      return instance->instanceVariableAt(i * kPointerSize);
    }
  }

  // Nothing found in the instance, if we found a non-data descriptor via the
  // class search, use it.
  if (isNonDataDescriptor(thread, klass_attr)) {
    if (klass_attr->isFunction()) {
      Handle<Object> k(&scope, *klass);
      return functionDescriptorGet(thread, klass_attr, receiver, k);
    } else if (klass_attr->isClassMethod()) {
      Handle<Object> k(&scope, *klass);
      return classmethodDescriptorGet(thread, klass_attr, receiver, k);
    }
    // TODO(T25692531): Call __get__ from klass_attr
    UNIMPLEMENTED("custom descriptors are unsupported");
  }

  // If a regular attribute was found in the class, return it
  if (!klass_attr->isError()) {
    return *klass_attr;
  }

  // TODO(T25140871): Refactor this into something like:
  //     thread->throwMissingAttributeError(name)
  return thread->throwAttributeErrorFromCString("missing attribute");
}

Object* Runtime::instanceSetAttr(
    Thread* thread,
    const Handle<Object>& receiver,
    const Handle<Object>& name,
    const Handle<Object>& value) {
  if (!name->isString()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->throwTypeErrorFromCString("attribute name must be a string");
  }

  // Check for a data descriptor
  HandleScope scope(thread);
  Handle<Class> klass(&scope, classOf(*receiver));
  Handle<Object> klass_attr(&scope, lookupNameInMro(thread, klass, name));
  if (isDataDescriptor(thread, klass_attr)) {
    // TODO(T25692531): Call __set__ from klass_attr
    UNIMPLEMENTED("custom descriptors are unsupported");
  }

  // No data descriptor found, store in the in-instance properties
  Handle<HeapObject> instance(&scope, *receiver);
  Handle<ObjectArray> map(&scope, klass->instanceAttributeMap());
  for (word i = 0; i < map->length(); i++) {
    if (map->at(i) == *name) {
      instance->instanceVariableAtPut(i * kPointerSize, *value);
      return None::object();
    }
  }

  // TODO: Support overflow attributes.
  UNIMPLEMENTED("unknown attribute '%s'", String::cast(*name)->toCString());
}

// Note that PEP 562 adds support for data descriptors in module objects.
// We are targeting python 3.6 for now, so we won't worry about that.
Object* Runtime::moduleGetAttr(
    Thread* thread,
    const Handle<Object>& receiver,
    const Handle<Object>& name) {
  if (!name->isString()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->throwTypeErrorFromCString("attribute name must be a string");
  }

  HandleScope scope(thread);
  Handle<Module> mod(&scope, *receiver);
  Handle<Object> ret(&scope, moduleAt(mod, name));

  if (!ret->isError()) {
    return *ret;
  } else {
    // TODO(T25140871): Refactor this into something like:
    //     thread->throwMissingAttributeError(name)
    return thread->throwAttributeErrorFromCString("missing attribute");
  }
}

Object* Runtime::moduleSetAttr(
    Thread* thread,
    const Handle<Object>& receiver,
    const Handle<Object>& name,
    const Handle<Object>& value) {
  if (!name->isString()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->throwTypeErrorFromCString("attribute name must be a string");
  }

  HandleScope scope(thread);
  Handle<Module> mod(&scope, *receiver);
  moduleAtPut(mod, name, value);
  return None::object();
}

bool Runtime::isDataDescriptor(Thread* thread, const Handle<Object>& object) {
  if (object->isFunction() || object->isClassMethod() || object->isError()) {
    return false;
  }
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  HandleScope scope(thread);
  Handle<Class> klass(&scope, classOf(*object));
  Handle<Object> dunder_set(&scope, symbols()->DunderSet());
  return !lookupNameInMro(thread, klass, dunder_set)->isError();
}

bool Runtime::isNonDataDescriptor(
    Thread* thread,
    const Handle<Object>& object) {
  if (object->isFunction() || object->isClassMethod()) {
    return true;
  } else if (object->isError()) {
    return false;
  }
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  HandleScope scope(thread);
  Handle<Class> klass(&scope, classOf(*object));
  Handle<Object> dunder_get(&scope, symbols()->DunderGet());
  return !lookupNameInMro(thread, klass, dunder_get)->isError();
}

Object* Runtime::newClassWithId(ClassId class_id) {
  auto index = static_cast<word>(class_id);
  // kSmallInteger must be the only even immediate class id.
  assert(
      index >= static_cast<word>(ClassId::kObject) ||
      class_id == ClassId::kSmallInteger || (index & 1) == 1);
  assert(index < List::cast(class_table_)->allocated());
  HandleScope scope;
  Handle<Class> klass(&scope, heap()->createClass(class_id));
  List::cast(class_table_)->atPut(index, *klass);
  klass->initialize(newDictionary(), empty_object_array_);
  return *klass;
}

Object* Runtime::newCode() {
  return heap()->createCode(empty_object_array_);
}

Object* Runtime::newBuiltinFunction(
    Function::Entry entry,
    Function::Entry entryKw) {
  Object* result = heap()->createFunction();
  assert(result != nullptr);
  auto function = Function::cast(result);
  function->setEntry(entry);
  function->setEntryKw(entryKw);
  return result;
}

Object* Runtime::newFunction() {
  Object* object = heap()->createFunction();
  assert(object != nullptr);
  auto function = Function::cast(object);
  function->setEntry(unimplementedTrampoline);
  function->setEntryKw(unimplementedTrampoline);
  return function;
}

Object* Runtime::newInstance(ClassId class_id) {
  Object* cls = classAt(class_id);
  assert(!cls->isNone());
  return heap()->createInstance(class_id, Class::cast(cls)->instanceSize());
}

void Runtime::initializeListClass() {
  HandleScope scope;
  Handle<Class> list(&scope, newClassWithId(ClassId::kList));
  list->setName(symbols()->List());
  const ClassId list_mro[] = {ClassId::kList, ClassId::kObject};
  list->setMro(createMro(list_mro, ARRAYSIZE(list_mro)));
  Handle<Dictionary> dict(&scope, newDictionary());
  list->setDictionary(*dict);

  classAddBuiltinFunction(
      list,
      symbols()->Append(),
      nativeTrampoline<builtinListAppend>,
      unimplementedTrampoline);

  Handle<Function> dunder_new(
      &scope,
      newBuiltinFunction(
          nativeTrampoline<builtinListNew>, unimplementedTrampoline));
  list->setDunderNew(*dunder_new);
}

void Runtime::initializeSmallIntClass() {
  HandleScope scope;
  Handle<Class> small_integer(&scope, newClassWithId(ClassId::kSmallInteger));
  small_integer->setName(newStringFromCString("smallint"));
  const ClassId small_integer_mro[] = {
      ClassId::kSmallInteger, ClassId::kLargeInteger, ClassId::kObject};
  small_integer->setMro(
      createMro(small_integer_mro, ARRAYSIZE(small_integer_mro)));

  // We want to lookup the class of an immediate type by using the 5-bit tag
  // value as an index into the class table.  Replicate the class object for
  // SmallInteger to all locations that decode to a SmallInteger tag.
  for (word i = 1; i < 16; i++) {
    assert(List::cast(class_table_)->at(i << 1) == None::object());
    List::cast(class_table_)->atPut(i << 1, *small_integer);
  }
}

void Runtime::classAddBuiltinFunction(
    const Handle<Class>& klass,
    Object* name,
    Function::Entry entry,
    Function::Entry entryKw) {
  HandleScope scope;
  Handle<Object> key(&scope, name);
  Handle<Object> value(&scope, newBuiltinFunction(entry, entryKw));
  Handle<Dictionary> dict(&scope, klass->dictionary());
  dictionaryAtPutInValueCell(dict, key, value);
}

Object* Runtime::newList() {
  return heap()->createList(empty_object_array_);
}

Object* Runtime::newModule(const Handle<Object>& name) {
  HandleScope scope;
  Handle<Dictionary> dictionary(&scope, newDictionary());
  Handle<Object> key(&scope, symbols()->DunderName());
  dictionaryAtPutInValueCell(dictionary, key, name);
  return heap()->createModule(*name, *dictionary);
}

Object* Runtime::newIntegerFromCPointer(void* ptr) {
  return newInteger(reinterpret_cast<word>(ptr));
}

Object* Runtime::newObjectArray(word length) {
  if (length == 0) {
    return empty_object_array_;
  }
  return heap()->createObjectArray(length, None::object());
}

Object* Runtime::newInteger(word value) {
  if (SmallInteger::isValid(value)) {
    return SmallInteger::fromWord(value);
  }
  return LargeInteger::cast(heap()->createLargeInteger(value));
}

Object* Runtime::newDouble(double value) {
  return Double::cast(heap()->createDouble(value));
}

Object* Runtime::newRange(word start, word stop, word step) {
  auto range = Range::cast(heap()->createRange());
  range->setStart(start);
  range->setStop(stop);
  range->setStep(step);
  return range;
}

Object* Runtime::newRangeIterator(Object* o) {
  HandleScope scope;
  Handle<Range> range(&scope, o);
  Handle<RangeIterator> range_iterator(&scope, heap()->createRangeIterator());
  range_iterator->setRange(*range);
  return *range_iterator;
}

Object* Runtime::newStringFromCString(const char* c_string) {
  word length = std::strlen(c_string);
  auto data = reinterpret_cast<const byte*>(c_string);
  return newStringWithAll(View<byte>(data, length));
}

Object* Runtime::newStringWithAll(View<byte> code_units) {
  word length = code_units.length();
  if (length <= SmallString::kMaxLength) {
    return SmallString::fromBytes(code_units);
  }
  Object* result = heap()->createLargeString(length);
  assert(result != nullptr);
  byte* dst = reinterpret_cast<byte*>(LargeString::cast(result)->address());
  const byte* src = code_units.data();
  memcpy(dst, src, length);
  return result;
}

Object* Runtime::internString(const Handle<Object>& string) {
  HandleScope scope;
  Handle<Set> set(&scope, interned());
  Handle<Object> key(&scope, *string);
  assert(string->isString());
  if (string->isSmallString()) {
    return *string;
  }
  return setAdd(set, key);
}

Object* Runtime::hash(Object* object) {
  if (!object->isHeapObject()) {
    return immediateHash(object);
  }
  if (object->isByteArray() || object->isLargeString()) {
    return valueHash(object);
  }
  return identityHash(object);
}

Object* Runtime::immediateHash(Object* object) {
  if (object->isSmallInteger()) {
    return object;
  }
  if (object->isBoolean()) {
    return SmallInteger::fromWord(Boolean::cast(object)->value() ? 1 : 0);
  }
  if (object->isSmallString()) {
    return SmallInteger::fromWord(
        reinterpret_cast<uword>(object) >> SmallString::kTagSize);
  }
  return SmallInteger::fromWord(reinterpret_cast<uword>(object));
}

// Xoroshiro128+
// http://xoroshiro.di.unimi.it/
uword Runtime::random() {
  const uint64_t s0 = random_state_[0];
  uint64_t s1 = random_state_[1];
  const uint64_t result = s0 + s1;
  s1 ^= s0;
  random_state_[0] = Utils::rotateLeft(s0, 55) ^ s1 ^ (s1 << 14);
  random_state_[1] = Utils::rotateLeft(s1, 36);
  return result;
}

Object* Runtime::identityHash(Object* object) {
  HeapObject* src = HeapObject::cast(object);
  word code = src->header()->hashCode();
  if (code == 0) {
    code = random() & Header::kHashCodeMask;
    code = (code == 0) ? 1 : code;
    src->setHeader(src->header()->withHashCode(code));
  }
  return SmallInteger::fromWord(code);
}

word Runtime::siphash24(View<byte> array) {
  word result = 0;
  ::halfsiphash(
      array.data(),
      array.length(),
      reinterpret_cast<const uint8_t*>(hash_secret_),
      reinterpret_cast<uint8_t*>(&result),
      sizeof(result));
  return result;
}

Object* Runtime::valueHash(Object* object) {
  HeapObject* src = HeapObject::cast(object);
  Header* header = src->header();
  word code = header->hashCode();
  if (code == 0) {
    word size = src->headerCountOrOverflow();
    code = siphash24(View<byte>(reinterpret_cast<byte*>(src->address()), size));
    code &= Header::kHashCodeMask;
    code = (code == 0) ? 1 : code;
    src->setHeader(header->withHashCode(code));
    assert(code == src->header()->hashCode());
  }
  return SmallInteger::fromWord(code);
}

void Runtime::initializeClasses() {
  HandleScope scope;

  Handle<ObjectArray> array(&scope, newObjectArray(256));
  Handle<List> list(&scope, newList());
  list->setItems(*array);
  const word allocated = static_cast<word>(ClassId::kLastId + 1);
  assert(allocated < array->length());
  list->setAllocated(allocated);
  class_table_ = *list;
  initializeHeapClasses();
  initializeImmediateClasses();
}

Object* Runtime::createMro(const ClassId* superclasses, word length) {
  HandleScope scope;
  Handle<ObjectArray> result(&scope, newObjectArray(length));
  for (word i = 0; i < length; i++) {
    auto index = static_cast<word>(superclasses[i]);
    assert(List::cast(class_table_)->at(index) != None::object());
    result->atPut(i, List::cast(class_table_)->at(index));
  }
  return *result;
}

void Runtime::initializeHeapClasses() {
  initializeHeapClass("object");
  initializeHeapClass("type", ClassId::kType);
  initializeHeapClass("type", ClassId::kType);
  initializeHeapClass("method", ClassId::kBoundMethod);
  initializeHeapClass("byteArray", ClassId::kByteArray);
  initializeHeapClass("code", ClassId::kCode);
  initializeHeapClass("dictionary", ClassId::kDictionary);
  initializeHeapClass("double", ClassId::kDouble);
  initializeHeapClass("function", ClassId::kFunction);
  initializeHeapClass("integer", ClassId::kLargeInteger);
  initializeHeapClass("module", ClassId::kModule);
  initializeHeapClass("objectarray", ClassId::kObjectArray);
  initializeHeapClass("str", ClassId::kLargeString);
  initializeHeapClass("valuecell", ClassId::kValueCell);
  initializeHeapClass("ellipsis", ClassId::kEllipsis);
  initializeHeapClass("range", ClassId::kRange);
  initializeHeapClass("range_iterator", ClassId::kRangeIterator);
  initializeHeapClass("weakref", ClassId::kWeakRef);
  initializeListClass();
  initializeClassMethodClass();
}

void Runtime::initializeImmediateClasses() {
  initializeHeapClass("bool", ClassId::kBoolean, ClassId::kLargeInteger);
  initializeHeapClass("NoneType", ClassId::kNone);
  initializeHeapClass(
      "smallstr", ClassId::kSmallString, ClassId::kLargeInteger);
  initializeSmallIntClass();
}

void Runtime::collectGarbage() {
  Scavenger(this).scavenge();
}

Object* Runtime::run(const char* buffer) {
  HandleScope scope;

  Handle<Module> main(&scope, createMainModule());
  return executeModule(buffer, main);
}

Object* Runtime::executeModule(
    const char* buffer,
    const Handle<Module>& module) {
  HandleScope scope;
  Marshal::Reader reader(&scope, this, buffer);

  reader.readLong();
  reader.readLong();
  reader.readLong();

  Handle<Code> code(&scope, reader.readObject());
  assert(code->argcount() == 0);

  return Thread::currentThread()->runModuleFunction(*module, *code);
}

Object* Runtime::importModule(const Handle<Object>& name) {
  HandleScope scope;
  Handle<Object> cached_module(&scope, findModule(name));
  if (!cached_module->isNone()) {
    return *cached_module;
  }

  return Thread::currentThread()->throwRuntimeErrorFromCString(
      "importModule is unimplemented!");
}

// TODO: support fromlist and level. Ideally, we'll never implement that
// functionality in c++, instead using the pure-python importlib
// implementation that ships with cpython.
Object* Runtime::importModuleFromBuffer(
    const char* buffer,
    const Handle<Object>& name) {
  HandleScope scope;
  Handle<Object> cached_module(&scope, findModule(name));
  if (!cached_module->isNone()) {
    return *cached_module;
  }

  Handle<Module> module(&scope, newModule(name));
  addModule(module);
  executeModule(buffer, module);
  return *module;
}

void Runtime::initializeThreads() {
  auto main_thread = new Thread(Thread::kDefaultStackSize);
  threads_ = main_thread;
  main_thread->setRuntime(this);
  Thread::setCurrentThread(main_thread);
}

void Runtime::initializePrimitiveInstances() {
  empty_object_array_ = heap()->createObjectArray(0, None::object());
  empty_byte_array_ = heap()->createByteArray(0);
  ellipsis_ = heap()->createEllipsis();
}

void Runtime::initializeInterned() {
  interned_ = newSet();
}

void Runtime::initializeRandom() {
  uword random_state[2];
  uword hash_secret[2];
  OS::secureRandom(
      reinterpret_cast<byte*>(&random_state), sizeof(random_state));
  OS::secureRandom(reinterpret_cast<byte*>(&hash_secret), sizeof(hash_secret));
  seedRandom(random_state, hash_secret);
}

void Runtime::initializeSymbols() {
  HandleScope scope;
  symbols_ = new Symbols(this);
  for (word i = 0; i < Symbols::kMaxSymbolId; i++) {
    Handle<Object> symbol(
        &scope, symbols_->at(static_cast<Symbols::SymbolId>(i)));
    internString(symbol);
  }
}

void Runtime::visitRoots(PointerVisitor* visitor) {
  visitRuntimeRoots(visitor);
  visitThreadRoots(visitor);
}

void Runtime::visitRuntimeRoots(PointerVisitor* visitor) {
  // Visit classes
  visitor->visitPointer(&class_table_);

  // Visit instances
  visitor->visitPointer(&empty_byte_array_);
  visitor->visitPointer(&empty_object_array_);
  visitor->visitPointer(&ellipsis_);
  visitor->visitPointer(&build_class_);
  visitor->visitPointer(&print_default_end_);

  // Visit interned strings.
  visitor->visitPointer(&interned_);

  // Visit modules
  visitor->visitPointer(&modules_);

  // Visit symbols
  symbols_->visit(visitor);
}

void Runtime::visitThreadRoots(PointerVisitor* visitor) {
  for (Thread* thread = threads_; thread != nullptr; thread = thread->next()) {
    thread->visitRoots(visitor);
  }
}

void Runtime::addModule(const Handle<Module>& module) {
  HandleScope scope;
  Handle<Dictionary> dict(&scope, modules());
  Handle<Object> key(&scope, module->name());
  Handle<Object> value(&scope, *module);
  dictionaryAtPut(dict, key, value);
}

Object* Runtime::findModule(const Handle<Object>& name) {
  assert(name->isString());

  HandleScope scope;
  Handle<Dictionary> dict(&scope, modules());
  Object* value = dictionaryAt(dict, name);
  if (value->isError()) {
    return None::object();
  }
  return value;
}

Object* Runtime::moduleAt(
    const Handle<Module>& module,
    const Handle<Object>& key) {
  HandleScope scope;
  Handle<Dictionary> dict(&scope, module->dictionary());
  Handle<Object> value_cell(&scope, dictionaryAt(dict, key));
  if (value_cell->isError()) {
    return Error::object();
  }
  return ValueCell::cast(*value_cell)->value();
}

void Runtime::moduleAtPut(
    const Handle<Module>& module,
    const Handle<Object>& key,
    const Handle<Object>& value) {
  HandleScope scope;
  Handle<Dictionary> dict(&scope, module->dictionary());
  dictionaryAtPutInValueCell(dict, key, value);
}

void Runtime::initializeModules() {
  modules_ = newDictionary();
  createBuiltinsModule();
  createSysModule();
}

Object* Runtime::classAt(ClassId class_id) {
  return List::cast(class_table_)->at(static_cast<word>(class_id));
}

Object* Runtime::classOf(Object* object) {
  return classAt(object->classId());
}

ClassId Runtime::newClassId() {
  HandleScope scope;
  Handle<List> list(&scope, class_table_);
  Handle<Object> value(&scope, None::object());
  auto result = static_cast<ClassId>(list->allocated());
  listAdd(list, value);
  return result;
}

void Runtime::moduleAddGlobal(
    const Handle<Module>& module,
    const Handle<Object>& key,
    const Handle<Object>& value) {
  HandleScope scope;
  Handle<Dictionary> dictionary(&scope, module->dictionary());
  dictionaryAtPutInValueCell(dictionary, key, value);
}

Object* Runtime::moduleAddBuiltinFunction(
    const Handle<Module>& module,
    const char* name,
    const Function::Entry entry,
    const Function::Entry entryKw) {
  HandleScope scope;
  Handle<Object> key(&scope, newStringFromCString(name));
  Handle<Dictionary> dictionary(&scope, module->dictionary());
  Handle<Object> value(&scope, newBuiltinFunction(entry, entryKw));
  return dictionaryAtPutInValueCell(dictionary, key, value);
}

void Runtime::moduleAddBuiltinPrint(const Handle<Module>& module) {
  HandleScope scope;
  Handle<Function> print(
      &scope,
      newBuiltinFunction(
          nativeTrampoline<builtinPrint>, nativeTrampoline<builtinPrintKw>));

  // Name
  Handle<Object> name(&scope, newStringFromCString("print"));
  print->setName(*name);

  Handle<Object> val(&scope, *print);
  moduleAddGlobal(module, name, val);
}

void Runtime::createBuiltinsModule() {
  HandleScope scope;
  Handle<Object> name(&scope, newStringFromCString("builtins"));
  Handle<Module> module(&scope, newModule(name));

  // Fill in builtins...
  build_class_ = moduleAddBuiltinFunction(
      module,
      "__build_class__",
      nativeTrampoline<builtinBuildClass>,
      nativeTrampoline<unimplementedTrampoline>);
  moduleAddBuiltinPrint(module);
  moduleAddBuiltinFunction(
      module,
      "ord",
      nativeTrampoline<builtinOrd>,
      nativeTrampoline<unimplementedTrampoline>);
  moduleAddBuiltinFunction(
      module,
      "chr",
      nativeTrampoline<builtinChr>,
      nativeTrampoline<unimplementedTrampoline>);
  moduleAddBuiltinFunction(
      module,
      "range",
      nativeTrampoline<builtinRange>,
      nativeTrampoline<unimplementedTrampoline>);
  moduleAddBuiltinFunction(
      module,
      "isinstance",
      nativeTrampoline<builtinIsinstance>,
      nativeTrampoline<unimplementedTrampoline>);

  // Add 'object'
  Handle<Object> object_name(&scope, symbols()->ObjectClassname());
  Handle<Object> object_value(&scope, classAt(ClassId::kObject));
  moduleAddGlobal(module, object_name, object_value);

  Handle<Object> list_name(&scope, symbols()->List());
  Handle<Object> list_value(&scope, classAt(ClassId::kList));
  moduleAddGlobal(module, list_name, list_value);

  Handle<Object> classmethod_name(&scope, symbols()->Classmethod());
  Handle<Object> classmethod_value(&scope, classAt(ClassId::kClassMethod));
  moduleAddGlobal(module, classmethod_name, classmethod_value);

  addModule(module);
}

void Runtime::createSysModule() {
  HandleScope scope;
  Handle<Object> name(&scope, newStringFromCString("sys"));
  Handle<Module> module(&scope, newModule(name));

  Handle<Object> modules_id(&scope, newStringFromCString("modules"));
  Handle<Object> modules(&scope, modules_);
  moduleAddGlobal(module, modules_id, modules);

  // Fill in sys...
  addModule(module);
}

Object* Runtime::createMainModule() {
  HandleScope scope;
  Handle<Object> name(&scope, symbols()->DunderMain());
  Handle<Module> module(&scope, newModule(name));

  // Fill in __main__...

  addModule(module);

  return *module;
}

Object* Runtime::getIter(Object* o) {
  // TODO: Support other forms of iteration.
  return newRangeIterator(o);
}

// List

ObjectArray* Runtime::ensureCapacity(
    const Handle<ObjectArray>& array,
    word index) {
  HandleScope scope;
  word capacity = array->length();
  if (index < capacity) {
    return *array;
  }
  word newCapacity = (capacity == 0) ? kInitialEnsuredCapacity : capacity << 1;
  Handle<ObjectArray> newArray(&scope, newObjectArray(newCapacity));
  array->copyTo(*newArray);
  return *newArray;
}

void Runtime::listAdd(const Handle<List>& list, const Handle<Object>& value) {
  HandleScope scope;
  word index = list->allocated();
  Handle<ObjectArray> items(&scope, list->items());
  Handle<ObjectArray> newItems(&scope, ensureCapacity(items, index));
  if (*items != *newItems) {
    list->setItems(*newItems);
  }
  list->setAllocated(index + 1);
  list->atPut(index, *value);
}

void Runtime::listInsert(
    const Handle<List>& list,
    const Handle<Object>& value,
    word index) {
  // TODO: Add insert(-x) where it inserts at pos: len(list) - x
  listAdd(list, value);
  word last_index = list->allocated() - 1;
  index = std::max(static_cast<word>(0), std::min(last_index, index));
  for (word i = last_index; i > index; i--) {
    list->atPut(i, list->at(i - 1));
  }
  list->atPut(index, *value);
}

Object*
Runtime::listReplicate(Thread* thread, const Handle<List>& list, word ntimes) {
  HandleScope scope(thread);
  word len = list->allocated();
  Handle<ObjectArray> items(&scope, newObjectArray(ntimes * len));
  for (word i = 0; i < ntimes; i++) {
    for (word j = 0; j < len; j++) {
      items->atPut((i * len) + j, list->at(j));
    }
  }
  Handle<List> result(&scope, newList());
  result->setItems(*items);
  result->setAllocated(items->length());
  return *result;
}

char* Runtime::compile(const char* src) {
  // increment this if you change the caching code, to invalidate existing
  // cache entries.
  uint64_t seed[2] = {0, 1};
  word hash = 0;

  // Hash the input.
  ::siphash(
      reinterpret_cast<const uint8_t*>(src),
      strlen(src),
      reinterpret_cast<const uint8_t*>(seed),
      reinterpret_cast<uint8_t*>(&hash),
      sizeof(hash));

  const char* cache_env = OS::getenv("PYRO_CACHE_DIR");
  std::string cache_dir;
  if (cache_env != nullptr) {
    cache_dir = cache_env;
  } else {
    const char* home_env = OS::getenv("HOME");
    if (home_env != nullptr) {
      cache_dir = home_env;
      cache_dir += "/.pyro-compile-cache";
    }
  }

  char filename_buf[512] = {};
  snprintf(filename_buf, 512, "%s/%016zx", cache_dir.c_str(), hash);

  // Read compiled code from the cache
  if (!cache_dir.empty() && OS::fileExists(filename_buf)) {
    return OS::readFile(filename_buf);
  }

  // Cache miss, must run the compiler.
  std::unique_ptr<char[]> tmpDir(OS::temporaryDirectory("python-tests"));
  const std::string dir(tmpDir.get());
  const std::string py = dir + "/foo.py";
  const std::string pyc = dir + "/foo.pyc";
  const std::string cleanup = "rm -rf " + dir;
  std::ofstream output(py);
  output << src;
  output.close();
  const std::string command =
      "/usr/local/fbcode/gcc-5-glibc-2.23/bin/python3.6 -m compileall -q -b " +
      py;
  system(command.c_str());
  word len;
  char* result = OS::readFile(pyc.c_str(), &len);
  system(cleanup.c_str());

  // Cache the output if possible.
  if (!cache_dir.empty() && OS::dirExists(cache_dir.c_str())) {
    OS::writeFileExcl(filename_buf, result, len);
  }

  return result;
}

// Dictionary

// Helper class for manipulating buckets in the ObjectArray that backs the
// dictionary
class Bucket {
 public:
  Bucket(const Handle<ObjectArray>& data, word index)
      : data_(data), index_(index) {}

  inline Object* hash() {
    return data_->at(index_ + kHashOffset);
  }

  inline Object* key() {
    return data_->at(index_ + kKeyOffset);
  }

  inline Object* value() {
    return data_->at(index_ + kValueOffset);
  }

  inline void set(Object* hash, Object* key, Object* value) {
    data_->atPut(index_ + kHashOffset, hash);
    data_->atPut(index_ + kKeyOffset, key);
    data_->atPut(index_ + kValueOffset, value);
  }

  inline bool hasKey(Object* thatKey) {
    return !hash()->isNone() && Object::equals(key(), thatKey);
  }

  inline bool isTombstone() {
    return hash()->isNone() && !key()->isNone();
  }

  inline void setTombstone() {
    set(None::object(), Error::object(), None::object());
  }

  inline bool isEmpty() {
    return hash()->isNone() && key()->isNone();
  }

  bool isFilled() {
    return !(isEmpty() || isTombstone());
  }

  static inline word getIndex(Object* data, Object* hash) {
    word nbuckets = ObjectArray::cast(data)->length() / kNumPointers;
    assert(Utils::isPowerOfTwo(nbuckets));
    word value = SmallInteger::cast(hash)->value();
    return (value & (nbuckets - 1)) * kNumPointers;
  }

  static const word kHashOffset = 0;
  static const word kKeyOffset = kHashOffset + 1;
  static const word kValueOffset = kKeyOffset + 1;
  static const word kNumPointers = kValueOffset + 1;

 private:
  const Handle<ObjectArray>& data_;
  word index_;

  DISALLOW_HEAP_ALLOCATION();
};

// Set

// Helper class for manipulating buckets in the ObjectArray that backs the
// Set, it has one less slot than Bucket.
class SetBucket {
 public:
  SetBucket(const Handle<ObjectArray>& data, word index)
      : data_(data), index_(index) {}

  inline Object* hash() {
    return data_->at(index_ + kHashOffset);
  }

  inline Object* key() {
    return data_->at(index_ + kKeyOffset);
  }

  inline void set(Object* hash, Object* key) {
    data_->atPut(index_ + kHashOffset, hash);
    data_->atPut(index_ + kKeyOffset, key);
  }

  inline bool hasKey(Object* thatKey) {
    return !hash()->isNone() && Object::equals(key(), thatKey);
  }

  inline bool isTombstone() {
    return hash()->isNone() && !key()->isNone();
  }

  inline void setTombstone() {
    set(None::object(), Error::object());
  }

  inline bool isEmpty() {
    return hash()->isNone() && key()->isNone();
  }

  static inline word getIndex(Object* data, Object* hash) {
    word nbuckets = ObjectArray::cast(data)->length() / kNumPointers;
    assert(Utils::isPowerOfTwo(nbuckets));
    word value = SmallInteger::cast(hash)->value();
    return (value & (nbuckets - 1)) * kNumPointers;
  }

  static const word kHashOffset = 0;
  static const word kKeyOffset = kHashOffset + 1;
  static const word kNumPointers = kKeyOffset + 1;

 private:
  const Handle<ObjectArray>& data_;
  word index_;

  DISALLOW_HEAP_ALLOCATION();
};

Object* Runtime::newDictionary() {
  return heap()->createDictionary(empty_object_array_);
}

Object* Runtime::newDictionary(word initialSize) {
  HandleScope scope;
  // TODO: initialSize should be scaled up by a load factor.
  auto initialCapacity = Utils::nextPowerOfTwo(initialSize);
  Handle<ObjectArray> array(
      &scope,
      newObjectArray(
          std::max(
              static_cast<word>(kInitialDictionaryCapacity), initialCapacity) *
          Bucket::kNumPointers));
  return heap()->createDictionary(*array);
}

void Runtime::dictionaryAtPut(
    const Handle<Dictionary>& dict,
    const Handle<Object>& key,
    const Handle<Object>& value) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  word index = -1;
  Handle<Object> key_hash(&scope, hash(*key));
  bool found = dictionaryLookup(data, key, key_hash, &index);
  if (index == -1) {
    // TODO(mpage): Grow at a predetermined load factor, rather than when full
    Handle<ObjectArray> newData(&scope, dictionaryGrow(data));
    dictionaryLookup(newData, key, key_hash, &index);
    assert(index != -1);
    dict->setData(*newData);
    Bucket bucket(newData, index);
    bucket.set(*key_hash, *key, *value);
  } else {
    Bucket bucket(data, index);
    bucket.set(*key_hash, *key, *value);
  }
  if (!found) {
    dict->setNumItems(dict->numItems() + 1);
  }
}

ObjectArray* Runtime::dictionaryGrow(const Handle<ObjectArray>& data) {
  HandleScope scope;
  word newLength = data->length() * kDictionaryGrowthFactor;
  if (newLength == 0) {
    newLength = kInitialDictionaryCapacity * Bucket::kNumPointers;
  }
  Handle<ObjectArray> newData(&scope, newObjectArray(newLength));
  // Re-insert items
  for (word i = 0; i < data->length(); i += Bucket::kNumPointers) {
    Bucket oldBucket(data, i);
    if (oldBucket.isEmpty() || oldBucket.isTombstone()) {
      continue;
    }
    Handle<Object> key(&scope, oldBucket.key());
    Handle<Object> hash(&scope, oldBucket.hash());
    word index = -1;
    dictionaryLookup(newData, key, hash, &index);
    assert(index != -1);
    Bucket newBucket(newData, index);
    newBucket.set(*hash, *key, oldBucket.value());
  }
  return *newData;
}

Object* Runtime::dictionaryAt(
    const Handle<Dictionary>& dict,
    const Handle<Object>& key) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  word index = -1;
  Handle<Object> key_hash(&scope, hash(*key));
  bool found = dictionaryLookup(data, key, key_hash, &index);
  if (found) {
    assert(index != -1);
    return Bucket(data, index).value();
  }
  return Error::object();
}

Object* Runtime::dictionaryAtIfAbsentPut(
    const Handle<Dictionary>& dict,
    const Handle<Object>& key,
    Callback<Object*>* thunk) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  word index = -1;
  Handle<Object> key_hash(&scope, hash(*key));
  bool found = dictionaryLookup(data, key, key_hash, &index);
  if (found) {
    assert(index != -1);
    return Bucket(data, index).value();
  }
  Handle<Object> value(&scope, thunk->call());
  if (index == -1) {
    // TODO(mpage): Grow at a predetermined load factor, rather than when full
    Handle<ObjectArray> new_data(&scope, dictionaryGrow(data));
    dictionaryLookup(new_data, key, key_hash, &index);
    assert(index != -1);
    dict->setData(*new_data);
    Bucket bucket(new_data, index);
    bucket.set(*key_hash, *key, *value);
  } else {
    Bucket bucket(data, index);
    bucket.set(*key_hash, *key, *value);
  }
  dict->setNumItems(dict->numItems() + 1);
  return *value;
}

Object* Runtime::dictionaryAtPutInValueCell(
    const Handle<Dictionary>& dict,
    const Handle<Object>& key,
    const Handle<Object>& value) {
  Object* result = dictionaryAtIfAbsentPut(dict, key, newValueCellCallback());
  ValueCell::cast(result)->setValue(*value);
  return result;
}

bool Runtime::dictionaryIncludes(
    const Handle<Dictionary>& dict,
    const Handle<Object>& key) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  Handle<Object> key_hash(&scope, hash(*key));
  word ignore;
  return dictionaryLookup(data, key, key_hash, &ignore);
}

bool Runtime::dictionaryRemove(
    const Handle<Dictionary>& dict,
    const Handle<Object>& key,
    Object** value) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  word index = -1;
  Handle<Object> key_hash(&scope, hash(*key));
  bool found = dictionaryLookup(data, key, key_hash, &index);
  if (found) {
    assert(index != -1);
    Bucket bucket(data, index);
    *value = bucket.value();
    bucket.setTombstone();
    dict->setNumItems(dict->numItems() - 1);
  }
  return found;
}

bool Runtime::dictionaryLookup(
    const Handle<ObjectArray>& data,
    const Handle<Object>& key,
    const Handle<Object>& key_hash,
    word* index) {
  word start = Bucket::getIndex(*data, *key_hash);
  word current = start;
  word nextFreeIndex = -1;

  // TODO(mpage) - Quadratic probing?
  word length = data->length();
  if (length == 0) {
    *index = -1;
    return false;
  }

  do {
    Bucket bucket(data, current);
    if (bucket.hasKey(*key)) {
      *index = current;
      return true;
    } else if (nextFreeIndex == -1 && bucket.isTombstone()) {
      nextFreeIndex = current;
    } else if (bucket.isEmpty()) {
      if (nextFreeIndex == -1) {
        nextFreeIndex = current;
      }
      break;
    }
    current = (current + Bucket::kNumPointers) % length;
  } while (current != start);

  *index = nextFreeIndex;

  return false;
}

ObjectArray* Runtime::dictionaryKeys(const Handle<Dictionary>& dict) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  Handle<ObjectArray> keys(&scope, newObjectArray(dict->numItems()));
  word numKeys = 0;
  for (word i = 0; i < data->length(); i += Bucket::kNumPointers) {
    Bucket bucket(data, i);
    if (bucket.isFilled()) {
      assert(numKeys < keys->length());
      keys->atPut(numKeys, bucket.key());
      numKeys++;
    }
  }
  assert(numKeys == keys->length());
  return *keys;
}

Object* Runtime::newSet() {
  return heap()->createSet(empty_object_array_);
}

bool Runtime::setLookup(
    const Handle<ObjectArray>& data,
    const Handle<Object>& key,
    const Handle<Object>& key_hash,
    word* index) {
  word start = SetBucket::getIndex(*data, *key_hash);
  word current = start;
  word nextFreeIndex = -1;

  // TODO(mpage) - Quadratic probing?
  word length = data->length();
  if (length == 0) {
    *index = -1;
    return false;
  }

  do {
    SetBucket bucket(data, current);
    if (bucket.hasKey(*key)) {
      *index = current;
      return true;
    } else if (nextFreeIndex == -1 && bucket.isTombstone()) {
      nextFreeIndex = current;
    } else if (bucket.isEmpty()) {
      if (nextFreeIndex == -1) {
        nextFreeIndex = current;
      }
      break;
    }
    current = (current + SetBucket::kNumPointers) % length;
  } while (current != start);

  *index = nextFreeIndex;

  return false;
}

ObjectArray* Runtime::setGrow(const Handle<ObjectArray>& data) {
  HandleScope scope;
  word newLength = data->length() * kSetGrowthFactor;
  if (newLength == 0) {
    newLength = kInitialSetCapacity * SetBucket::kNumPointers;
  }
  Handle<ObjectArray> newData(&scope, newObjectArray(newLength));
  // Re-insert items
  for (word i = 0; i < data->length(); i += SetBucket::kNumPointers) {
    SetBucket oldBucket(data, i);
    if (oldBucket.isEmpty() || oldBucket.isTombstone()) {
      continue;
    }
    Handle<Object> key(&scope, oldBucket.key());
    Handle<Object> hash(&scope, oldBucket.hash());
    word index = -1;
    setLookup(newData, key, hash, &index);
    assert(index != -1);
    SetBucket newBucket(newData, index);
    newBucket.set(*hash, *key);
  }
  return *newData;
}

Object* Runtime::setAdd(const Handle<Set>& set, const Handle<Object>& value) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, set->data());
  word index = -1;
  Handle<Object> key_hash(&scope, hash(*value));
  bool found = setLookup(data, value, key_hash, &index);
  if (found) {
    assert(index != -1);
    return SetBucket(data, index).key();
  }
  if (index == -1) {
    // TODO(mpage): Grow at a predetermined load factor, rather than when full
    Handle<ObjectArray> newData(&scope, setGrow(data));
    setLookup(newData, value, key_hash, &index);
    assert(index != -1);
    set->setData(*newData);
    SetBucket bucket(newData, index);
    bucket.set(*key_hash, *value);
  } else {
    SetBucket bucket(data, index);
    bucket.set(*key_hash, *value);
  }
  set->setNumItems(set->numItems() + 1);
  return *value;
}

bool Runtime::setIncludes(const Handle<Set>& set, const Handle<Object>& value) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, set->data());
  Handle<Object> key_hash(&scope, hash(*value));
  word ignore;
  return setLookup(data, value, key_hash, &ignore);
}

bool Runtime::setRemove(const Handle<Set>& set, const Handle<Object>& value) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, set->data());
  Handle<Object> key_hash(&scope, hash(*value));
  word index = -1;
  bool found = setLookup(data, value, key_hash, &index);
  if (found) {
    assert(index != -1);
    SetBucket bucket(data, index);
    bucket.setTombstone();
    set->setNumItems(set->numItems() - 1);
  }
  return found;
}

Object* Runtime::newValueCell() {
  return heap()->createValueCell();
}

Object* Runtime::newWeakRef() {
  return heap()->createWeakRef();
}

void Runtime::collectAttributes(
    const Handle<Code>& code,
    const Handle<Dictionary>& attributes) {
  HandleScope scope;
  Handle<ByteArray> bc(&scope, code->code());
  Handle<ObjectArray> names(&scope, code->names());

  word len = bc->length();
  for (word i = 0; i < len - 3; i += 2) {
    // If the current instruction is EXTENDED_ARG we must skip it and the next
    // instruction.
    if (bc->byteAt(i) == Bytecode::EXTENDED_ARG) {
      i += 2;
      continue;
    }
    // Check for LOAD_FAST 0 (self)
    if (bc->byteAt(i) != Bytecode::LOAD_FAST || bc->byteAt(i + 1) != 0) {
      continue;
    }
    // Followed by a STORE_ATTR
    if (bc->byteAt(i + 2) != Bytecode::STORE_ATTR) {
      continue;
    }
    word name_index = bc->byteAt(i + 3);
    Handle<Object> name(&scope, names->at(name_index));
    dictionaryAtPut(attributes, name, name);
  }
}

Object* Runtime::classConstructor(const Handle<Class>& klass) {
  HandleScope scope;
  Handle<Dictionary> klass_dict(&scope, klass->dictionary());
  Handle<Object> init(&scope, symbols()->DunderInit());
  Object* value = dictionaryAt(klass_dict, init);
  if (value->isError()) {
    return None::object();
  }
  return ValueCell::cast(value)->value();
}

ObjectArray* Runtime::computeInstanceAttributeMap(const Handle<Class>& klass) {
  HandleScope scope;
  Handle<ObjectArray> mro(&scope, klass->mro());
  Handle<Dictionary> attrs(&scope, newDictionary());

  for (word i = 0; i < mro->length(); i++) {
    Handle<Class> mro_klass(&scope, mro->at(i));
    Handle<Object> maybe_init(&scope, classConstructor(mro_klass));
    if (!maybe_init->isFunction()) {
      continue;
    }
    Handle<Function> init(maybe_init);
    Object* maybe_code = init->code();
    if (!maybe_code->isCode()) {
      continue;
    }
    Handle<Code> code(&scope, maybe_code);
    collectAttributes(code, attrs);
  }

  return dictionaryKeys(attrs);
}

Object* Runtime::lookupNameInMro(
    Thread* thread,
    const Handle<Class>& klass,
    const Handle<Object>& name) {
  HandleScope scope(thread);
  Handle<ObjectArray> mro(&scope, klass->mro());
  for (word i = 0; i < mro->length(); i++) {
    Handle<Class> mro_klass(&scope, mro->at(i));
    Handle<Dictionary> dict(&scope, mro_klass->dictionary());
    Handle<Object> value_cell(&scope, dictionaryAt(dict, name));
    if (!value_cell->isError()) {
      return ValueCell::cast(*value_cell)->value();
    }
  }
  return Error::object();
}

Object* Runtime::attributeAt(
    Thread* thread,
    const Handle<Object>& receiver,
    const Handle<Object>& name) {
  // A minimal implementation of getattr needed to get richards running.
  Object* result;
  if (receiver->isClass()) {
    result = classGetAttr(thread, receiver, name);
  } else if (receiver->isModule()) {
    result = moduleGetAttr(thread, receiver, name);
  } else {
    // everything else should fallback to instance
    result = instanceGetAttr(thread, receiver, name);
  }
  return result;
}

Object* Runtime::attributeAtPut(
    Thread* thread,
    const Handle<Object>& receiver,
    const Handle<Object>& name,
    const Handle<Object>& value) {
  HandleScope scope(thread);
  Handle<Object> interned_name(&scope, internString(name));
  // A minimal implementation of setattr needed to get richards running.
  Object* result;
  if (receiver->isClass()) {
    result = classSetAttr(thread, receiver, interned_name, value);
  } else if (receiver->isModule()) {
    result = moduleSetAttr(thread, receiver, interned_name, value);
  } else {
    // everything else should fallback to instance
    result = instanceSetAttr(thread, receiver, interned_name, value);
  }
  return result;
}

bool Runtime::isTruthy(Object* object) {
  if (object->isBoolean()) {
    return Boolean::cast(object)->value();
  }
  // TODO(mpage): Raise an exception
  std::abort();
}

Object* Runtime::stringConcat(
    const Handle<String>& left,
    const Handle<String>& right) {
  HandleScope scope;

  const word llen = left->length();
  const word rlen = right->length();
  const word new_len = llen + rlen;

  if (new_len <= SmallString::kMaxLength) {
    byte buffer[SmallString::kMaxLength];
    left->copyTo(buffer, llen);
    right->copyTo(buffer + llen, rlen);
    return SmallString::fromBytes(View<byte>(buffer, new_len));
  }

  Handle<String> result(
      &scope, LargeString::cast(heap()->createLargeString(new_len)));
  assert(result->isLargeString());
  const word address = HeapObject::cast(*result)->address();

  left->copyTo(reinterpret_cast<byte*>(address), llen);
  right->copyTo(reinterpret_cast<byte*>(address) + llen, rlen);
  return *result;
}

Object* Runtime::computeFastGlobals(
    const Handle<Code>& code,
    const Handle<Dictionary>& globals,
    const Handle<Dictionary>& builtins) {
  HandleScope scope;
  Handle<ByteArray> bytes(&scope, code->code());
  Handle<ObjectArray> names(&scope, code->names());
  Handle<ObjectArray> fast_globals(&scope, newObjectArray(names->length()));
  for (word i = 0; i < bytes->length(); i += 2) {
    Bytecode bc = static_cast<Bytecode>(bytes->byteAt(i));
    word arg = bytes->byteAt(i + 1);
    while (bc == EXTENDED_ARG) {
      i += 2;
      bc = static_cast<Bytecode>(bytes->byteAt(i));
      arg = (arg << 8) | bytes->byteAt(i + 1);
    }
    if (bc != LOAD_GLOBAL && bc != STORE_GLOBAL && bc != DELETE_GLOBAL &&
        bc != LOAD_NAME) {
      continue;
    }
    Handle<Object> key(&scope, names->at(arg));
    Object* value = dictionaryAt(globals, key);
    if (value->isError()) {
      value = dictionaryAt(builtins, key);
      if (value->isError()) {
        // insert a place holder to allow {STORE|DELETE}_GLOBAL
        Handle<Object> handle(&scope, value);
        value = dictionaryAtPutInValueCell(builtins, key, handle);
        ValueCell::cast(value)->makeUnbound();
      }
      Handle<Object> handle(&scope, value);
      value = dictionaryAtPutInValueCell(globals, key, handle);
    }
    assert(value->isValueCell());
    fast_globals->atPut(arg, value);
  }
  return *fast_globals;
}

// See https://github.com/python/cpython/blob/master/Objects/lnotab_notes.txt
// for details about the line number table format
word Runtime::codeOffsetToLineNum(
    Thread* thread,
    const Handle<Code>& code,
    word offset) {
  HandleScope scope(thread);
  Handle<ByteArray> table(&scope, code->lnotab());
  word line = code->firstlineno();
  word cur_offset = 0;
  for (word i = 0; i < table->length(); i += 2) {
    cur_offset += table->byteAt(i);
    if (cur_offset > offset) {
      break;
    }
    line += static_cast<sbyte>(table->byteAt(i + 1));
  }
  return line;
}

Object* Runtime::isSubClass(
    const Handle<Class>& subclass,
    const Handle<Class>& superclass) {
  HandleScope scope;
  Handle<ObjectArray> mro(&scope, subclass->mro());
  for (word i = 0; i < mro->length(); i++) {
    if (mro->at(i) == *superclass) {
      return Boolean::fromBool(true);
    }
  }
  return Boolean::fromBool(false);
}

Object* Runtime::isInstance(
    const Handle<Object>& obj,
    const Handle<Class>& klass) {
  HandleScope scope;
  Handle<Class> obj_class(&scope, classOf(*obj));
  return isSubClass(obj_class, klass);
}

Object* Runtime::newClassMethod() {
  return heap()->createClassMethod();
}

void Runtime::initializeClassMethodClass() {
  HandleScope scope;
  Handle<Class> classmethod(&scope, newClassWithId(ClassId::kClassMethod));
  classmethod->setName(symbols()->Classmethod());
  const ClassId classmethod_mro[] = {ClassId::kClassMethod, ClassId::kObject};
  classmethod->setMro(createMro(classmethod_mro, ARRAYSIZE(classmethod_mro)));
  Handle<Dictionary> dict(&scope, newDictionary());
  classmethod->setDictionary(*dict);

  classAddBuiltinFunction(
      classmethod,
      symbols()->DunderInit(),
      nativeTrampoline<builtinClassMethodInit>,
      unimplementedTrampoline);

  Handle<Function> dunder_new(
      &scope,
      newBuiltinFunction(
          nativeTrampoline<builtinClassMethodNew>, unimplementedTrampoline));
  classmethod->setDunderNew(*dunder_new);
}

} // namespace python
