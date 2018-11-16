#include "heap.h"

#include <cstring>

#include "objects.h"
#include "runtime.h"
#include "visitor.h"

namespace python {

Heap::Heap(word size) { space_ = new Space(size); }

Heap::~Heap() { delete space_; }

Object* Heap::allocate(word size, word offset) {
  DCHECK(size >= HeapObject::kMinimumSize, "allocation %ld too small", size);
  DCHECK(Utils::isAligned(size, kPointerSize), "request %ld not aligned", size);
  // Try allocating.  If the allocation fails, invoke the garbage collector and
  // retry the allocation.
  for (word attempt = 0; attempt < 2 && size < space_->size(); attempt++) {
    uword address = space_->allocate(size);
    if (address != 0) {
      // Allocation succeeded return the address as an object.
      return HeapObject::fromAddress(address + offset);
    }
    if (attempt == 0) {
      // Allocation failed, garbage collect and retry the allocation.
      Thread::currentThread()->runtime()->collectGarbage();
    }
  }
  return Error::object();
}

bool Heap::contains(void* address) {
  return space_->contains(reinterpret_cast<uword>(address));
}

bool Heap::verify() {
  uword scan = space_->start();
  while (scan < space_->fill()) {
    if (!(*reinterpret_cast<Object**>(scan))->isHeader()) {
      // Skip immediate values for alignment padding or header overflow.
      scan += kPointerSize;
    } else {
      HeapObject* object = HeapObject::fromAddress(scan + Header::kSize);
      // Objects start before the start of the space they are allocated in.
      if (object->baseAddress() < space_->start()) {
        return false;
      }
      // Objects must have their instance data after their header.
      if (object->address() < object->baseAddress()) {
        return false;
      }
      // Objects cannot start after the end of the space they are allocated in.
      if (object->address() > space_->fill()) {
        return false;
      }
      // Objects cannot end after the end of the space they are allocated in.
      uword end = object->baseAddress() + object->size();
      if (end > space_->fill()) {
        return false;
      }
      // Scan pointers that follow the header word, if any.
      if (!object->isRoot()) {
        scan = end;
      } else {
        for (scan += Header::kSize; scan < end; scan += kPointerSize) {
          auto pointer = reinterpret_cast<Object**>(scan);
          if ((*pointer)->isHeapObject()) {
            if (!space_->isAllocated(HeapObject::cast(*pointer)->address())) {
              return false;
            }
          }
        }
      }
    }
  }
  return true;
}

Object* Heap::createBoundMethod() {
  word size = BoundMethod::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<BoundMethod*>(raw);
  result->setHeader(Header::from(BoundMethod::kSize / kPointerSize, 0,
                                 LayoutId::kBoundMethod,
                                 ObjectFormat::kObjectInstance));
  return BoundMethod::cast(result);
}

Object* Heap::createByteArray(word length) {
  word size = ByteArray::allocationSize(length);
  Object* raw = allocate(size, ByteArray::headerSize(length));
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<ByteArray*>(raw);
  result->setHeaderAndOverflow(length, 0, LayoutId::kByteArray,
                               ObjectFormat::kDataArray8);
  return ByteArray::cast(result);
}

Object* Heap::createClass(LayoutId metaclass_id) {
  Object* raw = allocate(Type::allocationSize(), Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Type*>(raw);
  result->setHeader(Header::from(Type::kSize / kPointerSize, 0, metaclass_id,
                                 ObjectFormat::kObjectInstance));
  result->initialize(Type::kSize, None::object());
  return Type::cast(result);
}

Object* Heap::createClassMethod() {
  Object* raw = allocate(ClassMethod::allocationSize(), Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<ClassMethod*>(raw);
  result->setHeader(Header::from(ClassMethod::kSize / kPointerSize, 0,
                                 LayoutId::kClassMethod,
                                 ObjectFormat::kObjectInstance));
  return ClassMethod::cast(result);
}

Object* Heap::createCode() {
  Object* raw = allocate(Code::allocationSize(), Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Code*>(raw);
  result->setHeader(Header::from(Code::kSize / kPointerSize, 0, LayoutId::kCode,
                                 ObjectFormat::kObjectInstance));
  result->initialize(Code::kSize, None::object());
  return Code::cast(result);
}

Object* Heap::createComplex(double real, double imag) {
  word size = Complex::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Complex*>(raw);
  result->setHeader(Header::from(Complex::kSize / kPointerSize, 0,
                                 LayoutId::kComplex,
                                 ObjectFormat::kDataInstance));
  result->initialize(real, imag);
  return Complex::cast(result);
}

Object* Heap::createDict() {
  word size = Dict::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Dict*>(raw);
  result->setHeader(Header::from(Dict::kSize / kPointerSize, 0, LayoutId::kDict,
                                 ObjectFormat::kObjectInstance));
  result->initialize(Dict::kSize, None::object());
  return Dict::cast(result);
}

Object* Heap::createFloat(double value) {
  word size = Float::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Float*>(raw);
  result->setHeader(Header::from(Float::kSize / kPointerSize, 0,
                                 LayoutId::kFloat,
                                 ObjectFormat::kDataInstance));
  result->initialize(value);
  return Float::cast(result);
}

Object* Heap::createEllipsis() {
  word size = Ellipsis::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Ellipsis*>(raw);
  result->setHeader(Header::from(Ellipsis::kSize / kPointerSize, 0,
                                 LayoutId::kEllipsis,
                                 ObjectFormat::kDataInstance));
  return Ellipsis::cast(result);
}

Object* Heap::createFunction() {
  word size = Function::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Function*>(raw);
  result->setHeader(Header::from(Function::kSize / kPointerSize, 0,
                                 LayoutId::kFunction,
                                 ObjectFormat::kObjectInstance));
  result->initialize(Function::kSize, None::object());
  return Function::cast(result);
}

Object* Heap::createInstance(LayoutId layout_id, word num_attributes) {
  word size = Instance::allocationSize(num_attributes);
  Object* raw = allocate(size, HeapObject::headerSize(num_attributes));
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Instance*>(raw);
  result->setHeader(Header::from(num_attributes, 0, layout_id,
                                 ObjectFormat::kObjectInstance));
  result->initialize(num_attributes * kPointerSize, None::object());
  return result;
}

Object* Heap::createLargeInt(word num_digits) {
  word size = LargeInt::allocationSize(num_digits);
  Object* raw = allocate(size, LargeInt::headerSize(num_digits));
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<LargeInt*>(raw);
  result->setHeader(Header::from(num_digits, 0, LayoutId::kLargeInt,
                                 ObjectFormat::kDataArray64));
  return LargeInt::cast(result);
}

Object* Heap::createLargeStr(word length) {
  DCHECK(length > SmallStr::kMaxLength,
         "string len %ld is too small to be a large string", length);
  word size = LargeStr::allocationSize(length);
  Object* raw = allocate(size, LargeStr::headerSize(length));
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<LargeStr*>(raw);
  result->setHeaderAndOverflow(length, 0, LayoutId::kLargeStr,
                               ObjectFormat::kDataArray8);
  return LargeStr::cast(result);
}

Object* Heap::createLayout(LayoutId layout_id) {
  Object* raw = allocate(Layout::allocationSize(), Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Layout*>(raw);
  result->setHeader(
      Header::from(Layout::kSize / kPointerSize, static_cast<word>(layout_id),
                   LayoutId::kLayout, ObjectFormat::kObjectInstance));
  result->initialize(Layout::kSize, None::object());
  return Layout::cast(result);
}

Object* Heap::createList() {
  word size = List::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<List*>(raw);
  result->setHeader(Header::from(List::kSize / kPointerSize, 0, LayoutId::kList,
                                 ObjectFormat::kObjectInstance));
  result->initialize(List::kSize, None::object());
  return List::cast(result);
}

Object* Heap::createListIterator() {
  word size = ListIterator::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  ListIterator* result = reinterpret_cast<ListIterator*>(raw);
  result->setHeader(Header::from(ListIterator::kSize / kPointerSize, 0,
                                 LayoutId::kListIterator,
                                 ObjectFormat::kObjectInstance));
  result->initialize(size, None::object());
  return result;
}

Object* Heap::createModule() {
  word size = Module::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Module*>(raw);
  result->setHeader(Header::from(Module::kSize / kPointerSize, 0,
                                 LayoutId::kModule,
                                 ObjectFormat::kObjectInstance));
  result->initialize(Module::kSize, None::object());
  return Module::cast(result);
}

Object* Heap::createNotImplemented() {
  word size = NotImplemented::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<NotImplemented*>(raw);
  result->setHeader(Header::from(NotImplemented::kSize / kPointerSize, 0,
                                 LayoutId::kNotImplemented,
                                 ObjectFormat::kDataInstance));
  return NotImplemented::cast(result);
}

Object* Heap::createObjectArray(word length, Object* value) {
  DCHECK(!value->isHeapObject(), "value must be an immediate object");
  word size = ObjectArray::allocationSize(length);
  Object* raw = allocate(size, HeapObject::headerSize(length));
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<ObjectArray*>(raw);
  result->setHeaderAndOverflow(length, 0, LayoutId::kObjectArray,
                               ObjectFormat::kObjectArray);
  result->initialize(size, value);
  return ObjectArray::cast(result);
}

Object* Heap::createProperty() {
  word size = Property::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Property*>(raw);
  result->setHeader(Header::from(Property::kSize / kPointerSize, 0,
                                 LayoutId::kProperty,
                                 ObjectFormat::kObjectInstance));
  result->initialize(size, None::object());
  return Property::cast(result);
}

Object* Heap::createRange() {
  word size = Range::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Range*>(raw);
  result->setHeader(Header::from(Range::kSize / kPointerSize, 0,
                                 LayoutId::kRange,
                                 ObjectFormat::kDataInstance));
  return Range::cast(result);
}

Object* Heap::createRangeIterator() {
  word size = RangeIterator::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<RangeIterator*>(raw);
  result->setHeader(Header::from(RangeIterator::kSize / kPointerSize, 0,
                                 LayoutId::kRangeIterator,
                                 ObjectFormat::kObjectInstance));
  return RangeIterator::cast(result);
}

Object* Heap::createSet() {
  word size = Set::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Set*>(raw);
  result->setHeader(Header::from(Set::kSize / kPointerSize, 0, LayoutId::kSet,
                                 ObjectFormat::kObjectInstance));
  result->initialize(Set::kSize, None::object());
  return Set::cast(result);
}

Object* Heap::createSlice() {
  word size = Slice::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Slice*>(raw);
  result->setHeader(Header::from(Slice::kSize / kPointerSize, 0,
                                 LayoutId::kSlice,
                                 ObjectFormat::kObjectInstance));
  result->initialize(Slice::kSize, None::object());
  return Slice::cast(result);
}

Object* Heap::createStaticMethod() {
  Object* raw = allocate(StaticMethod::allocationSize(), Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<StaticMethod*>(raw);
  result->setHeader(Header::from(StaticMethod::kSize / kPointerSize, 0,
                                 LayoutId::kStaticMethod,
                                 ObjectFormat::kObjectInstance));
  return StaticMethod::cast(result);
}

Object* Heap::createSetIterator() {
  Object* raw = allocate(SetIterator::allocationSize(), Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<SetIterator*>(raw);
  result->setHeader(Header::from(SetIterator::kSize / kPointerSize, 0,
                                 LayoutId::kSetIterator,
                                 ObjectFormat::kObjectInstance));
  return SetIterator::cast(result);
}

Object* Heap::createSuper() {
  word size = Super::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Super*>(raw);
  result->setHeader(Header::from(Super::kSize / kPointerSize, 0,
                                 LayoutId::kSuper,
                                 ObjectFormat::kObjectInstance));
  return Super::cast(result);
}

Object* Heap::createTupleIterator() {
  word size = TupleIterator::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<TupleIterator*>(raw);
  result->setHeader(Header::from(TupleIterator::kSize / kPointerSize, 0,
                                 LayoutId::kTupleIterator,
                                 ObjectFormat::kObjectInstance));
  return TupleIterator::cast(result);
}

Object* Heap::createValueCell() {
  word size = ValueCell::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<ValueCell*>(raw);
  result->setHeader(Header::from(ValueCell::kSize / kPointerSize, 0,
                                 LayoutId::kValueCell,
                                 ObjectFormat::kObjectInstance));
  return ValueCell::cast(result);
}

Object* Heap::createWeakRef() {
  word size = WeakRef::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<WeakRef*>(raw);
  result->setHeader(Header::from(WeakRef::kSize / kPointerSize, 0,
                                 LayoutId::kWeakRef,
                                 ObjectFormat::kObjectInstance));
  result->initialize(WeakRef::kSize, None::object());
  return WeakRef::cast(result);
}

}  // namespace python
