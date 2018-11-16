#include "objects.h"
#include "runtime.h"
#include "thread.h"

#include <cstring>
namespace python {

// SmallStr

Object* SmallStr::fromCStr(const char* value) {
  word len = strlen(value);
  return fromBytes(View<byte>(reinterpret_cast<const byte*>(value), len));
}

Object* SmallStr::fromBytes(View<byte> data) {
  word length = data.length();
  DCHECK_BOUND(length, kMaxLength);
  word result = 0;
  for (word i = length; i > 0;) {
    i -= 1;
    result = (result << kBitsPerByte) | data.get(i);
  }
  result = (result << kBitsPerByte) | (length << kTagSize) | kTag;
  return reinterpret_cast<SmallStr*>(result);
}

char* SmallStr::toCStr() {
  word length = this->length();
  byte* result = static_cast<byte*>(std::malloc(length + 1));
  CHECK(result != nullptr, "out of memory");
  copyTo(result, length);
  result[length] = '\0';
  return reinterpret_cast<char*>(result);
}

// Str

bool LargeStr::equals(Object* that) {
  if (this == that) {
    return true;
  }
  if (!that->isLargeStr()) {
    return false;
  }
  auto* that_str = LargeStr::cast(that);
  if (length() != that_str->length()) {
    return false;
  }
  auto s1 = reinterpret_cast<void*>(address());
  auto s2 = reinterpret_cast<void*>(that_str->address());
  return std::memcmp(s1, s2, length()) == 0;
}

void LargeStr::copyTo(byte* dst, word length) {
  DCHECK_BOUND(length, this->length());
  std::memcpy(dst, reinterpret_cast<const byte*>(address()), length);
}

char* LargeStr::toCStr() {
  word length = this->length();
  byte* result = static_cast<byte*>(std::malloc(length + 1));
  CHECK(result != nullptr, "out of memory");
  copyTo(result, length);
  result[length] = '\0';
  return reinterpret_cast<char*>(result);
}

void Slice::unpack(word* start, word* stop, word* step) {
  if (this->step()->isNone()) {
    *step = 1;
  } else {
    // TODO(T27897506): CPython uses _PyEval_SliceIndex to convert any
    //       integer to eval any object into a valid index. For now, it'll
    //       assume that all indices are SmallInts.
    *step = SmallInt::cast(this->step())->value();
    if (*step == 0) {
      UNIMPLEMENTED("Throw ValueError. slice step cannot be zero");
    }
    // Here *step might be -SmallInt::kMaxValue-1; in this case we replace
    // it with -SmallInt::kMaxValue.  This doesn't affect the semantics,
    // and it guards against later undefined behaviour resulting from code that
    // does "step = -step" as part of a slice reversal.
    if (*step < -SmallInt::kMaxValue) {
      *step = -SmallInt::kMaxValue;
    }
  }

  if (this->start()->isNone()) {
    *start = (*step < 0) ? SmallInt::kMaxValue : 0;
  } else {
    *start = SmallInt::cast(this->start())->value();
  }

  if (this->stop()->isNone()) {
    *stop = (*step < 0) ? SmallInt::kMinValue : SmallInt::kMaxValue;
  } else {
    *stop = SmallInt::cast(this->stop())->value();
  }
}

word Slice::adjustIndices(word length, word* start, word* stop, word step) {
  DCHECK(step != 0, "Step should be non zero");

  if (*start < 0) {
    *start += length;
    if (*start < 0) {
      *start = (step < 0) ? -1 : 0;
    }
  } else if (*start >= length) {
    *start = (step < 0) ? length - 1 : length;
  }

  if (*stop < 0) {
    *stop += length;
    if (*stop < 0) {
      *stop = (step < 0) ? -1 : 0;
    }
  } else if (*stop >= length) {
    *stop = (step < 0) ? length - 1 : length;
  }

  if (step < 0) {
    if (*stop < *start) {
      return (*start - *stop - 1) / (-step) + 1;
    }
  } else {
    if (*start < *stop) {
      return (*stop - *start - 1) / step + 1;
    }
  }
  return 0;
}

void WeakRef::enqueueReference(Object* reference, Object** tail) {
  if (*tail == None::object()) {
    WeakRef::cast(reference)->setLink(reference);
  } else {
    Object* head = WeakRef::cast(*tail)->link();
    WeakRef::cast(*tail)->setLink(reference);
    WeakRef::cast(reference)->setLink(head);
  }
  *tail = reference;
}

Object* WeakRef::dequeueReference(Object** tail) {
  DCHECK(*tail != None::object(), "empty queue");
  Object* head = WeakRef::cast(*tail)->link();
  if (head == *tail) {
    *tail = None::object();
  } else {
    Object* next = WeakRef::cast(head)->link();
    WeakRef::cast(*tail)->setLink(next);
  }
  WeakRef::cast(head)->setLink(None::object());
  return head;
}

// Append tail2 to tail1 and return the new tail.
Object* WeakRef::spliceQueue(Object* tail1, Object* tail2) {
  if (tail1 == None::object() && tail2 == None::object()) {
    return None::object();
  }
  if (tail1 == None::object()) {
    return tail2;
  }
  if (tail2 == None::object()) {
    return tail1;
  }
  // merge two list, tail1 -> head2 -> ... -> tail2 -> head1
  Object* head1 = WeakRef::cast(tail1)->link();
  Object* head2 = WeakRef::cast(tail2)->link();
  WeakRef::cast(tail1)->setLink(head2);
  WeakRef::cast(tail2)->setLink(head1);
  return tail2;
}

Type* Type::cast(Object* object) {
  DCHECK(object->isType() ||
             Thread::currentThread()->runtime()->isInstanceOfClass(object),
         "invalid cast, expected class");
  return reinterpret_cast<Type*>(object);
}

List* List::cast(Object* object) {
  DCHECK(object->isList() ||
             Thread::currentThread()->runtime()->isInstanceOfList(object),
         "invalid cast, expected list");
  return reinterpret_cast<List*>(object);
}

}  // namespace python
