#include "objects.h"
#include "runtime.h"

#include <cassert>

namespace python {

int List::allocationSize() {
  return Utils::roundUp(List::kSize, kPointerSize);
}

List* List::cast(Object* obj) {
  assert(obj->isList());
  return reinterpret_cast<List*>(obj);
}

void List::initialize() {
  // Lists start out empty and grow on the first append. This is how CPython
  // initializes lists; presumably they've done some analysis to show this is
  // better than starting with a small initial list (e.g. 1 or 2 elements).
  atPut(List::kElemsOffset, 0);
}

ObjectArray* List::elems() {
  Object* e = at(kElemsOffset);
  if (e == nullptr) {
    return nullptr;
  }
  return ObjectArray::cast(e);
}

void List::setElems(ObjectArray* elems) {
  atPut(kElemsOffset, elems);
}

word List::capacity() {
  ObjectArray* es = elems();
  if (es == nullptr) {
    return 0;
  }
  return es->length();
}

void List::set(int index, Object* value) {
  assert(index >= 0);
  assert(index < length());
  Object* elems = at(kElemsOffset);
  ObjectArray::cast(elems)->set(index, value);
}

Object* List::get(int index) {
  return elems()->get(index);
}

// TODO(mpage) - This needs handlizing
void List::appendAndGrow(List* list, Object* value, Runtime* runtime) {
  word len = list->length();
  word cap = list->capacity();
  if (len < cap) {
    list->setLength(len + 1);
    list->set(len, value);
    return;
  }
  intptr_t newCap = cap == 0 ? 4 : cap << 1;
  Object* rawNewElems = runtime->createObjectArray(newCap);
  ObjectArray* newElems = ObjectArray::cast(rawNewElems);
  ObjectArray* curElems = list->elems();
  if (curElems != nullptr) {
    curElems->copyTo(newElems);
  }

  list->setElems(newElems);
  list->setLength(len + 1);
  list->set(len, value);
}

} // namespace python
