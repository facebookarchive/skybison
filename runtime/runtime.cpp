#include "runtime.h"

#include <cstring>

#include "globals.h"
#include "handles.h"
#include "heap.h"
#include "os.h"
#include "siphash.h"
#include "thread.h"
#include "trampolines.h"
#include "visitor.h"

namespace python {

Runtime::Runtime() : heap_(64 * MiB) {
  initializeRandom();
  initializeThreads();
  initializeClasses();
  initializeInstances();
  initializeModules();
}

Runtime::~Runtime() {
  for (Thread* thread = threads_; thread != nullptr; thread = thread->next()) {
    if (thread == Thread::currentThread()) {
      Thread::setCurrentThread(nullptr);
    } else {
      assert(0); // Not implemented.
    }
    delete thread;
  }
}

Object* Runtime::newByteArray(word length) {
  if (length == 0) {
    return empty_byte_array_;
  }
  return heap()->createByteArray(length);
}

Object* Runtime::newCode() {
  return heap()->createCode(empty_object_array_);
}

Object* Runtime::newByteArrayFromCString(const char* c_string, word length) {
  if (length == 0) {
    return empty_byte_array_;
  }
  Object* result = newByteArray(length);
  for (word i = 0; i < length; i++) {
    ByteArray::cast(result)->byteAtPut(
        i, *reinterpret_cast<const byte*>(c_string + i));
  }
  return result;
}

Object* Runtime::newDictionary() {
  Object* items = newObjectArray(Dictionary::kInitialItemsSize);
  assert(items != nullptr);
  return heap()->createDictionary(items);
}

Object* Runtime::newFunction() {
  Object* object = heap()->createFunction();
  assert(object != nullptr);
  auto function = Function::cast(object);
  auto tramp = trampolineToObject(unimplementedTrampoline);
  function->setEntry(tramp);
  function->setEntryKw(tramp);
  return function;
}

Object* Runtime::newList() {
  return heap()->createList(empty_object_array_);
}

Object* Runtime::newModule(Object* name) {
  Object* dict = newDictionary();
  assert(dict != nullptr);
  return heap()->createModule(name, dict);
}

Object* Runtime::newObjectArray(word length) {
  if (length == 0) {
    return empty_object_array_;
  }
  return heap()->createObjectArray(length, None::object());
}

Object* Runtime::newString(word length) {
  if (length == 0) {
    return empty_string_;
  }
  return heap()->createString(length);
}

Object* Runtime::newStringFromCString(const char* c_string) {
  word length = std::strlen(c_string);
  if (length == 0) {
    return empty_string_;
  }
  Object* result = newString(length);
  assert(result != nullptr);
  for (word i = 0; i < length; i++) {
    String::cast(result)->charAtPut(
        i, *reinterpret_cast<const byte*>(c_string + i));
  }
  return result;
}

Object* Runtime::hash(Object* object) {
  if (!object->isHeapObject()) {
    return immediateHash(object);
  }
  if (object->isByteArray() || object->isString()) {
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

Object* Runtime::valueHash(Object* object) {
  HeapObject* src = HeapObject::cast(object);
  Header* header = src->header();
  if (header->hashCode() == 0) {
    word code = 0;
    halfsiphash(
        reinterpret_cast<uint8_t*>(src->address()),
        src->size(),
        reinterpret_cast<uint8_t*>(hash_secret_),
        reinterpret_cast<uint8_t*>(&code),
        sizeof(code));
    header = header->withHashCode(code);
    src->setHeader(header);
  }
  return SmallInteger::fromWord(header->hashCode());
}

void Runtime::initializeClasses() {
  class_class_ = heap()->createClassClass();

  byte_array_class_ = heap()->createClass(ClassId::kByteArray, class_class_);
  code_class_ = heap()->createClass(ClassId::kCode, class_class_);
  dictionary_class_ = heap()->createClass(ClassId::kDictionary, class_class_);
  function_class_ = heap()->createClass(ClassId::kFunction, class_class_);
  list_class_ = heap()->createClass(ClassId::kList, class_class_);
  module_class_ = heap()->createClass(ClassId::kModule, class_class_);
  object_array_class_ =
      heap()->createClass(ClassId::kObjectArray, class_class_);
  string_class_ = heap()->createClass(ClassId::kString, class_class_);
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

void Runtime::initializeThreads() {
  auto main_thread = new Thread(Thread::kDefaultStackSize);
  threads_ = main_thread;
  main_thread->setRuntime(this);
  Thread::setCurrentThread(main_thread);
}

void Runtime::initializeInstances() {
  empty_byte_array_ = heap()->createByteArray(0);
  empty_object_array_ = heap()->createObjectArray(0, None::object());
  empty_string_ = heap()->createString(0);
}

void Runtime::initializeRandom() {
  Os::secureRandom(
      reinterpret_cast<byte*>(&random_state_), sizeof(random_state_));
  Os::secureRandom(
      reinterpret_cast<byte*>(&hash_secret_), sizeof(hash_secret_));
}

void Runtime::visitRoots(PointerVisitor* visitor) {
  visitRuntimeRoots(visitor);
  visitThreadRoots(visitor);
}

void Runtime::visitRuntimeRoots(PointerVisitor* visitor) {
  // Visit classes
  visitor->visitPointer(&byte_array_class_);
  visitor->visitPointer(&class_class_);
  visitor->visitPointer(&code_class_);
  visitor->visitPointer(&dictionary_class_);
  visitor->visitPointer(&function_class_);
  visitor->visitPointer(&list_class_);
  visitor->visitPointer(&module_class_);
  visitor->visitPointer(&object_array_class_);
  visitor->visitPointer(&string_class_);

  // Visit instances
  visitor->visitPointer(&empty_byte_array_);
  visitor->visitPointer(&empty_object_array_);
  visitor->visitPointer(&empty_string_);

  // Visit modules
  visitor->visitPointer(&modules_);
}

void Runtime::visitThreadRoots(PointerVisitor* visitor) {
  for (Thread* thread = threads_; thread != nullptr; thread = thread->next()) {
    thread->handles()->visitPointers(visitor);
  }
}

void Runtime::addModule(Object* module) {
  Object* name = Module::cast(module)->name();
  Dictionary::atPut(modules(), name, hash(name), module, this);
}

void Runtime::initializeModules() {
  modules_ = newDictionary();
  createBuiltinsModule();
}

void Runtime::createBuiltinsModule() {
  Object* name = newStringFromCString("builtins");
  assert(name != nullptr);
  Object* builtins = newModule(name);
  assert(builtins != nullptr);
  // Fill in builtins...
  addModule(builtins);
}

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

void Runtime::appendToList(
    const Handle<List>& list,
    const Handle<Object>& value) {
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

} // namespace python
