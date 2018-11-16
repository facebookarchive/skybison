#include "runtime.h"

#include <cstring>
#include <fstream>
#include <memory>

#include "builtins.h"
#include "bytecode.h"
#include "callback.h"
#include "globals.h"
#include "handles.h"
#include "heap.h"
#include "marshal.h"
#include "os.h"
#include "siphash.h"
#include "thread.h"
#include "trampolines-inl.h"
#include "utils.h"
#include "visitor.h"

namespace python {

Runtime::Runtime() : heap_(64 * MiB), new_value_cell_callback_(this) {
  initializeRandom();
  initializeThreads();
  // This must be called before initializeClasses is called. Methods in
  // initializeClasses rely on instances that are created in this method.
  initializePrimitiveInstances();
  initializeClasses();
  initializeInterned();
  initializeSymbols();
  initializeModules();
}

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

static Object* functionDescriptorGet(
    Thread* thread,
    const Handle<Object>& self,
    const Handle<Object>& instance,
    const Handle<Object>& /* owner */) {
  if (instance->isNone()) {
    return *self;
  }
  return thread->runtime()->newBoundMethod(*self, *instance);
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

  HandleScope scope(thread->handles());
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

  HandleScope scope(thread->handles());
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
  HandleScope scope(thread->handles());
  Handle<Class> klass(&scope, classOf(*receiver));
  Handle<Object> klass_attr(&scope, lookupNameInMro(thread, klass, name));
  if (isDataDescriptor(thread, klass_attr)) {
    // TODO(T25692531): Call __get__ from klass_attr
    UNIMPLEMENTED("custom descriptors are unsupported");
  }

  // No data descriptor found on the class, look at the in-instance
  // attributes.
  //
  // TODO: Support overflow attributs
  Handle<Instance> instance(&scope, *receiver);
  Handle<ObjectArray> map(&scope, klass->instanceAttributeMap());
  for (word i = 0; i < map->length(); i++) {
    if (map->at(i) == *name) {
      return instance->attributeAt(i);
    }
  }

  // Nothing found in the instance, if we found a non-data descriptor via the
  // class search, use it.
  if (isNonDataDescriptor(thread, klass_attr)) {
    if (klass_attr->isFunction()) {
      Handle<Object> k(&scope, *klass);
      return functionDescriptorGet(thread, klass_attr, receiver, k);
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
  HandleScope scope(thread->handles());
  Handle<Class> klass(&scope, classOf(*receiver));
  Handle<Object> klass_attr(&scope, lookupNameInMro(thread, klass, name));
  if (isDataDescriptor(thread, klass_attr)) {
    // TODO(T25692531): Call __set__ from klass_attr
    UNIMPLEMENTED("custom descriptors are unsupported");
  }

  // No data descriptor found, store in the in-instance properties
  Handle<Instance> instance(&scope, *receiver);
  Handle<ObjectArray> map(&scope, klass->instanceAttributeMap());
  for (word i = 0; i < map->length(); i++) {
    if (map->at(i) == *name) {
      instance->attributeAtPut(i, *value);
      return None::object();
    }
  }

  // TODO: Support overflow attributes.
  UNIMPLEMENTED("unknown attribute '%s'", String::cast(*name)->toCString());
}

bool Runtime::isDataDescriptor(Thread* thread, const Handle<Object>& object) {
  if (object->isFunction() || object->isError()) {
    return false;
  }
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  HandleScope scope(thread->handles());
  Handle<Class> klass(&scope, classOf(*object));
  Handle<Object> dunder_set(&scope, symbols()->DunderSet());
  return !lookupNameInMro(thread, klass, dunder_set)->isError();
}

bool Runtime::isNonDataDescriptor(
    Thread* thread,
    const Handle<Object>& object) {
  if (object->isFunction()) {
    return true;
  } else if (object->isError()) {
    return false;
  }
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  HandleScope scope(thread->handles());
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

Object* Runtime::newList() {
  return heap()->createList(empty_object_array_);
}

Object* Runtime::newModule(const Handle<Object>& name) {
  HandleScope scope;
  Handle<Dictionary> dictionary(&scope, newDictionary());
  Handle<Object> key(&scope, symbols()->DunderName());
  dictionaryAtPut(dictionary, key, name);
  return heap()->createModule(*name, *dictionary);
}

Object* Runtime::newObjectArray(word length) {
  if (length == 0) {
    return empty_object_array_;
  }
  return heap()->createObjectArray(length, None::object());
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
    code = random();
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
  HandleScope scope;

  Handle<Class> object(&scope, newClassWithId(ClassId::kObject));
  object->setName(newStringFromCString("object"));
  const ClassId object_mro[] = {ClassId::kObject};
  object->setMro(createMro(object_mro, ARRAYSIZE(object_mro)));

  Handle<Class> type(&scope, newClassWithId(ClassId::kType));
  type->setName(newStringFromCString("type"));
  const ClassId type_mro[] = {ClassId::kType, ClassId::kObject};
  type->setMro(createMro(type_mro, ARRAYSIZE(type_mro)));

  Handle<Class> bound_method(&scope, newClassWithId(ClassId::kBoundMethod));
  bound_method->setName(newStringFromCString("method"));
  const ClassId bound_method_mro[] = {ClassId::kBoundMethod, ClassId::kObject};
  bound_method->setMro(
      createMro(bound_method_mro, ARRAYSIZE(bound_method_mro)));

  Handle<Class> byte_array(&scope, newClassWithId(ClassId::kByteArray));
  byte_array->setName(newStringFromCString("bytearray"));
  const ClassId byte_array_mro[] = {ClassId::kByteArray, ClassId::kObject};
  byte_array->setMro(createMro(byte_array_mro, ARRAYSIZE(byte_array_mro)));

  Handle<Class> code(&scope, newClassWithId(ClassId::kCode));
  code->setName(newStringFromCString("code"));
  const ClassId code_mro[] = {ClassId::kCode, ClassId::kObject};
  code->setMro(createMro(code_mro, ARRAYSIZE(code_mro)));

  Handle<Class> dictionary(&scope, newClassWithId(ClassId::kDictionary));
  dictionary->setName(newStringFromCString("dictionary"));
  const ClassId dictionary_mro[] = {ClassId::kDictionary, ClassId::kObject};
  dictionary->setMro(createMro(dictionary_mro, ARRAYSIZE(dictionary_mro)));

  Handle<Class> function(&scope, newClassWithId(ClassId::kFunction));
  function->setName(newStringFromCString("function"));
  const ClassId function_mro[] = {ClassId::kFunction, ClassId::kObject};
  function->setMro(createMro(function_mro, ARRAYSIZE(function_mro)));

  Handle<Class> integer(&scope, newClassWithId(ClassId::kInteger));
  integer->setName(newStringFromCString("integer"));
  const ClassId integer_mro[] = {ClassId::kInteger, ClassId::kObject};
  integer->setMro(createMro(integer_mro, ARRAYSIZE(integer_mro)));

  Handle<Class> list(&scope, newClassWithId(ClassId::kList));
  list->setName(newStringFromCString("list"));
  const ClassId list_mro[] = {ClassId::kList, ClassId::kObject};
  list->setMro(createMro(list_mro, ARRAYSIZE(list_mro)));

  Handle<Class> module(&scope, newClassWithId(ClassId::kModule));
  module->setName(newStringFromCString("module"));
  const ClassId module_mro[] = {ClassId::kModule, ClassId::kObject};
  module->setMro(createMro(module_mro, ARRAYSIZE(module_mro)));

  Handle<Class> object_array(&scope, newClassWithId(ClassId::kObjectArray));
  object_array->setName(newStringFromCString("objectarray"));
  const ClassId object_array_mro[] = {ClassId::kObjectArray, ClassId::kObject};
  object_array->setMro(
      createMro(object_array_mro, ARRAYSIZE(object_array_mro)));

  Handle<Class> string(&scope, newClassWithId(ClassId::kLargeString));
  string->setName(newStringFromCString("str"));
  const ClassId string_mro[] = {ClassId::kLargeString, ClassId::kObject};
  string->setMro(createMro(string_mro, ARRAYSIZE(string_mro)));

  Handle<Class> value_cell(&scope, newClassWithId(ClassId::kValueCell));
  value_cell->setName(newStringFromCString("valuecell"));
  const ClassId value_cell_mro[] = {ClassId::kValueCell, ClassId::kObject};
  value_cell->setMro(createMro(value_cell_mro, ARRAYSIZE(value_cell_mro)));

  Handle<Class> ellipsis(&scope, newClassWithId(ClassId::kEllipsis));
  ellipsis->setName(newStringFromCString("ellipsis"));
  const ClassId ellipsis_mro[] = {ClassId::kEllipsis, ClassId::kObject};
  ellipsis->setMro(createMro(ellipsis_mro, ARRAYSIZE(ellipsis_mro)));

  Handle<Class> range(&scope, newClassWithId(ClassId::kRange));
  range->setName(newStringFromCString("range"));
  const ClassId range_mro[] = {ClassId::kRange, ClassId::kObject};
  range->setMro(createMro(range_mro, ARRAYSIZE(range_mro)));

  Handle<Class> range_iterator(&scope, newClassWithId(ClassId::kRangeIterator));
  range_iterator->setName(newStringFromCString("range_iterator"));
  const ClassId range_iterator_mro[] = {ClassId::kRangeIterator,
                                        ClassId::kObject};
  range_iterator->setMro(
      createMro(range_iterator_mro, ARRAYSIZE(range_iterator_mro)));
}

void Runtime::initializeImmediateClasses() {
  HandleScope scope;

  Handle<Class> small_integer(&scope, newClassWithId(ClassId::kSmallInteger));
  small_integer->setName(newStringFromCString("smallint"));
  const ClassId small_integer_mro[] = {
      ClassId::kSmallInteger, ClassId::kInteger, ClassId::kObject};
  small_integer->setMro(
      createMro(small_integer_mro, ARRAYSIZE(small_integer_mro)));

  // We want to lookup the class of an immediate type by using the 5-bit tag
  // value as an index into the class table.  Replicate the class object for
  // SmallInteger to all locations that decode to a SmallInteger tag.
  for (word i = 1; i < 16; i++) {
    assert(List::cast(class_table_)->at(i << 1) == None::object());
    List::cast(class_table_)->atPut(i << 1, *small_integer);
  }

  Handle<Class> small_string(&scope, newClassWithId(ClassId::kSmallString));
  small_string->setName(newStringFromCString("smallstr"));
  const ClassId small_string_mro[] = {
      ClassId::kSmallString, ClassId::kInteger, ClassId::kObject};
  small_string->setMro(
      createMro(small_string_mro, ARRAYSIZE(small_string_mro)));

  Handle<Class> boolean(&scope, newClassWithId(ClassId::kBoolean));
  boolean->setName(newStringFromCString("bool"));
  const ClassId boolean_mro[] = {
      ClassId::kBoolean, ClassId::kInteger, ClassId::kObject};
  boolean->setMro(createMro(boolean_mro, ARRAYSIZE(boolean_mro)));

  Handle<Class> none(&scope, newClassWithId(ClassId::kNone));
  none->setName(newStringFromCString("NoneType"));
  const ClassId none_mro[] = {ClassId::kNone, ClassId::kObject};
  none->setMro(createMro(none_mro, ARRAYSIZE(none_mro)));
}

class ScavengeVisitor : public PointerVisitor {
 public:
  explicit ScavengeVisitor(Heap* heap) : heap_(heap) {}

  void visitPointer(Object** pointer) override {
    heap_->scavengePointer(pointer);
  }

 private:
  Heap* heap_;
};

void Runtime::collectGarbage() {
  heap()->flip();
  ScavengeVisitor visitor(heap());
  visitRoots(&visitor);
  heap()->scavenge();
}

Object* Runtime::run(const char* buffer) {
  HandleScope scope;

  Marshal::Reader reader(&scope, this, buffer);

  reader.readLong();
  reader.readLong();
  reader.readLong();

  auto code = reader.readObject();
  assert(code->isCode());
  assert(Code::cast(code)->argcount() == 0);

  Handle<Module> main(&scope, createMainModule());
  return Thread::currentThread()->runModuleFunction(*main, code);
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
  OS::secureRandom(
      reinterpret_cast<byte*>(&random_state_), sizeof(random_state_));
  OS::secureRandom(
      reinterpret_cast<byte*>(&hash_secret_), sizeof(hash_secret_));
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

Object* Runtime::findModule(const char* name) {
  HandleScope scope;
  Handle<Dictionary> dict(&scope, modules());
  Handle<Object> key(&scope, newStringFromCString(name));
  Handle<Object> value(&scope, None::object());
  dictionaryAt(dict, key, value.pointer());
  return *value;
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
      "isinstance",
      nativeTrampoline<builtinIsinstance>,
      nativeTrampoline<unimplementedTrampoline>);
  moduleAddBuiltinPrint(module);

  addModule(module);
}

void Runtime::createSysModule() {
  HandleScope scope;
  Handle<Object> name(&scope, newStringFromCString("sys"));
  Handle<Module> module(&scope, newModule(name));

  // Fill in sys...
  addModule(module);
}

Object* Runtime::createMainModule() {
  HandleScope scope;
  Handle<Object> name(&scope, symbols()->DunderMain());
  Handle<Module> module(&scope, newModule(name));

  // Fill in __main__...
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

Object*
Runtime::listReplicate(Thread* thread, const Handle<List>& list, word ntimes) {
  HandleScope scope(thread->handles());
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
  char* result = OS::readFile(pyc.c_str());
  system(cleanup.c_str());
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

bool Runtime::dictionaryAt(
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
    *value = Bucket(data, index).value();
  }
  return found;
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
  Object* value;
  if (!dictionaryAt(klass_dict, init, &value)) {
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
  HandleScope scope(thread->handles());
  Handle<ObjectArray> mro(&scope, klass->mro());
  for (word i = 0; i < mro->length(); i++) {
    Handle<Class> mro_klass(&scope, mro->at(i));
    Handle<Dictionary> dict(&scope, mro_klass->dictionary());
    Handle<Object> value_cell(&scope, None::object());
    if (dictionaryAt(dict, name, value_cell.pointer())) {
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
  } else if (receiver->isInstance()) {
    result = instanceGetAttr(thread, receiver, name);
  } else {
    result = thread->throwTypeErrorFromCString(
        "can only look up attributes from classes or instances");
  }
  return result;
}

Object* Runtime::attributeAtPut(
    Thread* thread,
    const Handle<Object>& receiver,
    const Handle<Object>& name,
    const Handle<Object>& value) {
  HandleScope scope(thread->handles());
  Handle<Object> interned_name(&scope, internString(name));
  // A minimal implementation of setattr needed to get richards running.
  Object* result;
  if (receiver->isClass()) {
    result = classSetAttr(thread, receiver, interned_name, value);
  } else if (receiver->isInstance()) {
    result = instanceSetAttr(thread, receiver, interned_name, value);
  } else {
    result = thread->throwTypeErrorFromCString(
        "can only set attributes on classes or instances");
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

} // namespace python
