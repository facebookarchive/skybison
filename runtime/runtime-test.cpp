#include "gtest/gtest.h"

#include <memory>

#include "bytecode.h"
#include "frame.h"
#include "layout.h"
#include "runtime.h"
#include "symbols.h"
#include "test-utils.h"

namespace python {
using namespace testing;

TEST(RuntimeTest, CollectGarbage) {
  Runtime runtime;
  ASSERT_TRUE(runtime.heap()->verify());
  runtime.collectGarbage();
  ASSERT_TRUE(runtime.heap()->verify());
}

TEST(RuntimeTest, AllocateAndCollectGarbage) {
  const word heap_size = 32 * kMiB;
  const word array_length = 1024;
  const word allocation_size = Bytes::allocationSize(array_length);
  const word total_allocation_size = heap_size * 10;
  Runtime runtime(heap_size);
  ASSERT_TRUE(runtime.heap()->verify());
  for (word i = 0; i < total_allocation_size; i += allocation_size) {
    runtime.newBytes(array_length, 0);
  }
  ASSERT_TRUE(runtime.heap()->verify());
}

TEST(RuntimeTest, BuiltinsModuleExists) {
  Runtime runtime;
  HandleScope scope;

  Handle<Dict> modules(&scope, runtime.modules());
  Handle<Object> name(&scope, runtime.newStrFromCStr("builtins"));
  ASSERT_TRUE(runtime.dictAt(modules, name)->isModule());
}

// Return the raw name of a builtin LayoutId, or "<invalid>" for user-defined or
// invalid LayoutIds.
static const char* layoutIdName(LayoutId id) {
  switch (id) {
    case LayoutId::kError:
      // Special-case the one type that isn't really a class so we don't have to
      // have it in INTRINSIC_CLASS_NAMES.
      return "Error";

#define CASE(name)                                                             \
  case LayoutId::k##name:                                                      \
    return #name;
      INTRINSIC_CLASS_NAMES(CASE)
#undef CASE
  }
  return "<invalid>";
}

class BuiltinClassIdsTest : public ::testing::TestWithParam<LayoutId> {};

// Make sure that each built-in class has a class object.  Check that its class
// object points to a layout with the same layout ID as the built-in class.
TEST_P(BuiltinClassIdsTest, HasClassObject) {
  Runtime runtime;
  HandleScope scope;

  LayoutId id = GetParam();
  ASSERT_EQ(runtime.layoutAt(id)->layoutId(), LayoutId::kLayout)
      << "Bad Layout for " << layoutIdName(id);
  Handle<Object> elt(&scope, runtime.typeAt(id));
  ASSERT_TRUE(elt->isType());
  Handle<Type> cls(&scope, *elt);
  Handle<Layout> layout(&scope, cls->instanceLayout());
  EXPECT_EQ(layout->id(), GetParam());
}

static const LayoutId kBuiltinHeapClassIds[] = {
#define ENUM(x) LayoutId::k##x,
    INTRINSIC_HEAP_CLASS_NAMES(ENUM)
#undef ENUM
};

INSTANTIATE_TEST_CASE_P(BuiltinClassIdsParameters, BuiltinClassIdsTest,
                        ::testing::ValuesIn(kBuiltinHeapClassIds));

TEST(RuntimeDictTest, EmptyDictInvariants) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dict> dict(&scope, runtime.newDict());

  EXPECT_EQ(dict->numItems(), 0);
  ASSERT_TRUE(dict->data()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(dict->data())->length(), 0);
}

TEST(RuntimeDictTest, GetSet) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dict> dict(&scope, runtime.newDict());
  Handle<Object> key(&scope, SmallInt::fromWord(12345));

  // Looking up a key that doesn't exist should fail
  EXPECT_TRUE(runtime.dictAt(dict, key)->isError());

  // Store a value
  Handle<Object> stored(&scope, SmallInt::fromWord(67890));
  runtime.dictAtPut(dict, key, stored);
  EXPECT_EQ(dict->numItems(), 1);

  // Retrieve the stored value
  RawObject retrieved = runtime.dictAt(dict, key);
  ASSERT_TRUE(retrieved->isSmallInt());
  EXPECT_EQ(SmallInt::cast(retrieved)->value(),
            SmallInt::cast(*stored)->value());

  // Overwrite the stored value
  Handle<Object> new_value(&scope, SmallInt::fromWord(5555));
  runtime.dictAtPut(dict, key, new_value);
  EXPECT_EQ(dict->numItems(), 1);

  // Get the new value
  retrieved = runtime.dictAt(dict, key);
  ASSERT_TRUE(retrieved->isSmallInt());
  EXPECT_EQ(SmallInt::cast(retrieved)->value(),
            SmallInt::cast(*new_value)->value());
}

TEST(RuntimeDictTest, Remove) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dict> dict(&scope, runtime.newDict());
  Handle<Object> key(&scope, SmallInt::fromWord(12345));

  // Removing a key that doesn't exist should fail
  bool is_missing = runtime.dictRemove(dict, key)->isError();
  EXPECT_TRUE(is_missing);

  // Removing a key that exists should succeed and return the value that was
  // stored.
  Handle<Object> stored(&scope, SmallInt::fromWord(54321));

  runtime.dictAtPut(dict, key, stored);
  EXPECT_EQ(dict->numItems(), 1);

  RawObject retrieved = runtime.dictRemove(dict, key);
  ASSERT_FALSE(retrieved->isError());
  ASSERT_EQ(SmallInt::cast(retrieved)->value(),
            SmallInt::cast(*stored)->value());

  // Looking up a key that was deleted should fail
  EXPECT_TRUE(runtime.dictAt(dict, key)->isError());
  EXPECT_EQ(dict->numItems(), 0);
}

TEST(RuntimeDictTest, Length) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dict> dict(&scope, runtime.newDict());

  // Add 10 items and make sure length reflects it
  for (int i = 0; i < 10; i++) {
    Handle<Object> key(&scope, SmallInt::fromWord(i));
    runtime.dictAtPut(dict, key, key);
  }
  EXPECT_EQ(dict->numItems(), 10);

  // Remove half the items
  for (int i = 0; i < 5; i++) {
    Handle<Object> key(&scope, SmallInt::fromWord(i));
    ASSERT_FALSE(runtime.dictRemove(dict, key)->isError());
  }
  EXPECT_EQ(dict->numItems(), 5);
}

TEST(RuntimeDictTest, AtIfAbsentPutLength) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dict> dict(&scope, runtime.newDict());

  Handle<Object> k1(&scope, SmallInt::fromWord(1));
  Handle<Object> v1(&scope, SmallInt::fromWord(111));
  runtime.dictAtPut(dict, k1, v1);
  EXPECT_EQ(dict->numItems(), 1);

  class SmallIntCallback : public Callback<RawObject> {
   public:
    explicit SmallIntCallback(int i) : i_(i) {}
    RawObject call() override { return SmallInt::fromWord(i_); }

   private:
    int i_;
  };

  // Add new item
  Handle<Object> k2(&scope, SmallInt::fromWord(2));
  SmallIntCallback cb(222);
  Handle<Object> entry2(&scope, runtime.dictAtIfAbsentPut(dict, k2, &cb));
  EXPECT_EQ(dict->numItems(), 2);
  RawObject retrieved = runtime.dictAt(dict, k2);
  EXPECT_TRUE(retrieved->isSmallInt());
  EXPECT_EQ(retrieved, SmallInt::fromWord(222));

  // Don't overrwite existing item 1 -> v1
  Handle<Object> k3(&scope, SmallInt::fromWord(1));
  SmallIntCallback cb3(333);
  Handle<Object> entry3(&scope, runtime.dictAtIfAbsentPut(dict, k3, &cb3));
  EXPECT_EQ(dict->numItems(), 2);
  retrieved = runtime.dictAt(dict, k3);
  EXPECT_TRUE(retrieved->isSmallInt());
  EXPECT_EQ(retrieved, *v1);
}

TEST(RuntimeDictTest, GrowWhenFull) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dict> dict(&scope, runtime.newDict());

  // Fill up the dict - we insert an initial key to force the allocation of the
  // backing ObjectArray.
  Handle<Object> init_key(&scope, SmallInt::fromWord(0));
  runtime.dictAtPut(dict, init_key, init_key);
  ASSERT_TRUE(dict->data()->isObjectArray());
  word init_data_size = ObjectArray::cast(dict->data())->length();

  auto make_key = [&runtime](int i) {
    byte text[]{"0123456789abcdeghiklmn"};
    return runtime.newStrWithAll(View<byte>(text + i % 10, 10));
  };
  auto make_value = [](int i) { return SmallInt::fromWord(i); };

  // Fill in one fewer keys than would require growing the underlying object
  // array again
  word num_keys = Runtime::kInitialDictCapacity;
  for (int i = 1; i < num_keys; i++) {
    Handle<Object> key(&scope, make_key(i));
    Handle<Object> value(&scope, make_value(i));
    runtime.dictAtPut(dict, key, value);
  }

  // Add another key which should force us to double the capacity
  Handle<Object> straw(&scope, make_key(num_keys));
  Handle<Object> straw_value(&scope, make_value(num_keys));
  runtime.dictAtPut(dict, straw, straw_value);
  ASSERT_TRUE(dict->data()->isObjectArray());
  word new_data_size = ObjectArray::cast(dict->data())->length();
  EXPECT_EQ(new_data_size, Runtime::kDictGrowthFactor * init_data_size);

  // Make sure we can still read all the stored keys/values
  for (int i = 1; i <= num_keys; i++) {
    Handle<Object> key(&scope, make_key(i));
    RawObject value = runtime.dictAt(dict, key);
    ASSERT_FALSE(value->isError());
    EXPECT_TRUE(Object::equals(value, make_value(i)));
  }
}

TEST(RuntimeDictTest, CollidingKeys) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dict> dict(&scope, runtime.newDict());

  // Add two different keys with different values using the same hash
  Handle<Object> key1(&scope, SmallInt::fromWord(1));
  runtime.dictAtPut(dict, key1, key1);

  Handle<Object> key2(&scope, Bool::trueObj());
  runtime.dictAtPut(dict, key2, key2);

  // Make sure we get both back
  RawObject retrieved = runtime.dictAt(dict, key1);
  ASSERT_TRUE(retrieved->isSmallInt());
  EXPECT_EQ(SmallInt::cast(retrieved)->value(), SmallInt::cast(*key1)->value());

  retrieved = runtime.dictAt(dict, key2);
  ASSERT_TRUE(retrieved->isBool());
  EXPECT_EQ(Bool::cast(retrieved)->value(), Bool::cast(*key2)->value());
}

TEST(RuntimeDictTest, MixedKeys) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dict> dict(&scope, runtime.newDict());

  // Add keys of different type
  Handle<Object> int_key(&scope, SmallInt::fromWord(100));
  runtime.dictAtPut(dict, int_key, int_key);

  Handle<Object> str_key(&scope, runtime.newStrFromCStr("testing 123"));
  runtime.dictAtPut(dict, str_key, str_key);

  // Make sure we get the appropriate values back out
  RawObject retrieved = runtime.dictAt(dict, int_key);
  ASSERT_TRUE(retrieved->isSmallInt());
  EXPECT_EQ(SmallInt::cast(retrieved)->value(),
            SmallInt::cast(*int_key)->value());

  retrieved = runtime.dictAt(dict, str_key);
  ASSERT_TRUE(retrieved->isStr());
  EXPECT_TRUE(Object::equals(*str_key, retrieved));
}

TEST(RuntimeDictTest, GetKeys) {
  Runtime runtime;
  HandleScope scope;

  // Create keys
  Handle<ObjectArray> keys(&scope, runtime.newObjectArray(4));
  keys->atPut(0, SmallInt::fromWord(100));
  keys->atPut(1, runtime.newStrFromCStr("testing 123"));
  keys->atPut(2, Bool::trueObj());
  keys->atPut(3, NoneType::object());

  // Add keys to dict
  Handle<Dict> dict(&scope, runtime.newDict());
  for (word i = 0; i < keys->length(); i++) {
    Handle<Object> key(&scope, keys->at(i));
    runtime.dictAtPut(dict, key, key);
  }

  // Grab the keys and verify everything is there
  Handle<ObjectArray> retrieved(&scope, runtime.dictKeys(dict));
  ASSERT_EQ(retrieved->length(), keys->length());
  for (word i = 0; i < keys->length(); i++) {
    Handle<Object> key(&scope, keys->at(i));
    EXPECT_TRUE(objectArrayContains(retrieved, key)) << " missing key " << i;
  }
}

TEST(RuntimeListTest, ListGrowth) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  Handle<ObjectArray> array1(&scope, runtime.newObjectArray(1));
  list->setItems(*array1);
  EXPECT_EQ(array1->length(), 1);
  runtime.listEnsureCapacity(list, 2);
  Handle<ObjectArray> array2(&scope, list->items());
  EXPECT_NE(*array1, *array2);
  EXPECT_GT(array2->length(), 2);

  Handle<ObjectArray> array4(&scope, runtime.newObjectArray(4));
  EXPECT_EQ(array4->length(), 4);
  list->setItems(*array4);
  runtime.listEnsureCapacity(list, 5);
  Handle<ObjectArray> array8(&scope, list->items());
  EXPECT_NE(*array4, *array8);
  EXPECT_EQ(array8->length(), 8);
  list->setItems(*array8);
  runtime.listEnsureCapacity(list, 9);
  Handle<ObjectArray> array16(&scope, list->items());
  EXPECT_NE(*array8, *array16);
  EXPECT_EQ(array16->length(), 16);
}

TEST(RuntimeListTest, EmptyListInvariants) {
  Runtime runtime;
  RawList list = List::cast(runtime.newList());
  ASSERT_EQ(list->capacity(), 0);
  ASSERT_EQ(list->allocated(), 0);
}

TEST(RuntimeListTest, AppendToList) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());

  // Check that list capacity grows according to a doubling schedule
  word expected_capacity[] = {4,  4,  4,  4,  8,  8,  8,  8,
                              16, 16, 16, 16, 16, 16, 16, 16};
  for (int i = 0; i < 16; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(list, value);
    ASSERT_EQ(list->capacity(), expected_capacity[i]);
    ASSERT_EQ(list->allocated(), i + 1);
  }

  // Sanity check list contents
  for (int i = 0; i < 16; i++) {
    RawSmallInt elem = SmallInt::cast(list->at(i));
    ASSERT_EQ(elem->value(), i);
  }
}

TEST(RuntimeListTest, InsertToList) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());

  for (int i = 0; i < 9; i++) {
    if (i == 1 || i == 6) {
      continue;
    }
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(list, value);
  }
  ASSERT_NE(SmallInt::cast(list->at(1))->value(), 1);
  ASSERT_NE(SmallInt::cast(list->at(6))->value(), 6);

  Handle<Object> value2(&scope, SmallInt::fromWord(1));
  runtime.listInsert(list, value2, 1);
  Handle<Object> value12(&scope, SmallInt::fromWord(6));
  runtime.listInsert(list, value12, 6);

  EXPECT_EQ(SmallInt::cast(list->at(0))->value(), 0);
  EXPECT_EQ(SmallInt::cast(list->at(1))->value(), 1);
  EXPECT_EQ(SmallInt::cast(list->at(2))->value(), 2);
  EXPECT_EQ(SmallInt::cast(list->at(3))->value(), 3);
  EXPECT_EQ(SmallInt::cast(list->at(4))->value(), 4);
  EXPECT_EQ(SmallInt::cast(list->at(5))->value(), 5);
  EXPECT_EQ(SmallInt::cast(list->at(6))->value(), 6);
  EXPECT_EQ(SmallInt::cast(list->at(7))->value(), 7);
  EXPECT_EQ(SmallInt::cast(list->at(8))->value(), 8);
}

TEST(RuntimeListTest, InsertToListBounds) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  for (int i = 0; i < 10; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(list, value);
  }
  ASSERT_EQ(list->allocated(), 10);

  Handle<Object> value100(&scope, SmallInt::fromWord(100));
  runtime.listInsert(list, value100, 100);
  ASSERT_EQ(list->allocated(), 11);
  ASSERT_EQ(SmallInt::cast(list->at(10))->value(), 100);

  Handle<Object> value0(&scope, SmallInt::fromWord(400));
  runtime.listInsert(list, value0, 0);
  ASSERT_EQ(list->allocated(), 12);
  ASSERT_EQ(SmallInt::cast(list->at(0))->value(), 400);

  Handle<Object> value_n(&scope, SmallInt::fromWord(-10));
  runtime.listInsert(list, value_n, -10);
  ASSERT_EQ(list->allocated(), 13);
  ASSERT_EQ(SmallInt::cast(list->at(2))->value(), -10);
}

TEST(RuntimeListTest, PopList) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  for (int i = 0; i < 16; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(list, value);
  }
  ASSERT_EQ(list->allocated(), 16);

  // Pop from the end
  RawObject res1 = runtime.listPop(list, 15);
  ASSERT_EQ(list->allocated(), 15);
  ASSERT_EQ(SmallInt::cast(list->at(14))->value(), 14);
  ASSERT_EQ(SmallInt::cast(res1)->value(), 15);

  // Pop elements from 5 - 10
  for (int i = 0; i < 5; i++) {
    RawObject res5 = runtime.listPop(list, 5);
    ASSERT_EQ(SmallInt::cast(res5)->value(), i + 5);
  }
  ASSERT_EQ(list->allocated(), 10);
  for (int i = 0; i < 5; i++) {
    RawSmallInt elem = SmallInt::cast(list->at(i));
    ASSERT_EQ(elem->value(), i);
  }
  for (int i = 5; i < 10; i++) {
    RawSmallInt elem = SmallInt::cast(list->at(i));
    ASSERT_EQ(elem->value(), i + 5);
  }

  // Pop element 0
  RawObject res0 = runtime.listPop(list, 0);
  ASSERT_EQ(list->allocated(), 9);
  ASSERT_EQ(SmallInt::cast(list->at(0))->value(), 1);
  ASSERT_EQ(SmallInt::cast(res0)->value(), 0);
}

TEST(RuntimeListTest, ListExtendList) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  Handle<List> list1(&scope, runtime.newList());
  for (int i = 0; i < 4; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    Handle<Object> value1(&scope, SmallInt::fromWord(i + 4));
    runtime.listAdd(list, value);
    runtime.listAdd(list1, value1);
  }
  EXPECT_EQ(list->allocated(), 4);
  Handle<Object> list1_handle(&scope, *list1);
  runtime.listExtend(Thread::currentThread(), list, list1_handle);
  ASSERT_EQ(list->allocated(), 8);
  EXPECT_EQ(SmallInt::cast(list->at(0))->value(), 0);
  EXPECT_EQ(SmallInt::cast(list->at(1))->value(), 1);
  EXPECT_EQ(SmallInt::cast(list->at(2))->value(), 2);
  EXPECT_EQ(SmallInt::cast(list->at(3))->value(), 3);
  EXPECT_EQ(SmallInt::cast(list->at(4))->value(), 4);
  EXPECT_EQ(SmallInt::cast(list->at(5))->value(), 5);
  EXPECT_EQ(SmallInt::cast(list->at(6))->value(), 6);
  EXPECT_EQ(SmallInt::cast(list->at(7))->value(), 7);
}

TEST(RuntimeListTest, ListExtendListIterator) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  Handle<List> list1(&scope, runtime.newList());
  for (int i = 0; i < 4; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    Handle<Object> value1(&scope, SmallInt::fromWord(i + 4));
    runtime.listAdd(list, value);
    runtime.listAdd(list1, value1);
  }
  EXPECT_EQ(list->allocated(), 4);
  Handle<Object> list1_handle(&scope, *list1);
  Handle<Object> list1_iterator(&scope, runtime.newListIterator(list1_handle));
  runtime.listExtend(Thread::currentThread(), list, list1_iterator);
  ASSERT_EQ(list->allocated(), 8);
  EXPECT_EQ(SmallInt::cast(list->at(0))->value(), 0);
  EXPECT_EQ(SmallInt::cast(list->at(1))->value(), 1);
  EXPECT_EQ(SmallInt::cast(list->at(2))->value(), 2);
  EXPECT_EQ(SmallInt::cast(list->at(3))->value(), 3);
  EXPECT_EQ(SmallInt::cast(list->at(4))->value(), 4);
  EXPECT_EQ(SmallInt::cast(list->at(5))->value(), 5);
  EXPECT_EQ(SmallInt::cast(list->at(6))->value(), 6);
  EXPECT_EQ(SmallInt::cast(list->at(7))->value(), 7);
}

TEST(RuntimeListTest, ListExtendObjectArray) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  Handle<Object> object_array0(&scope, runtime.newObjectArray(0));
  Handle<ObjectArray> object_array1(&scope, runtime.newObjectArray(1));
  Handle<ObjectArray> object_array16(&scope, runtime.newObjectArray(16));

  for (int i = 0; i < 4; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(list, value);
  }
  runtime.listExtend(Thread::currentThread(), list, object_array0);
  EXPECT_EQ(list->allocated(), 4);

  Handle<Object> object_array1_handle(&scope, *object_array1);
  object_array1->atPut(0, NoneType::object());
  runtime.listExtend(Thread::currentThread(), list, object_array1_handle);
  ASSERT_GE(list->allocated(), 5);
  ASSERT_TRUE(list->at(4)->isNoneType());

  for (word i = 0; i < 4; i++) {
    object_array16->atPut(i, SmallInt::fromWord(i));
  }

  Handle<Object> object_array2_handle(&scope, *object_array16);
  runtime.listExtend(Thread::currentThread(), list, object_array2_handle);
  ASSERT_GE(list->allocated(), 4 + 1 + 4);
  EXPECT_EQ(list->at(5), SmallInt::fromWord(0));
  EXPECT_EQ(list->at(6), SmallInt::fromWord(1));
  EXPECT_EQ(list->at(7), SmallInt::fromWord(2));
  EXPECT_EQ(list->at(8), SmallInt::fromWord(3));
}

TEST(RuntimeListTest, ListExtendSet) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Object> value(&scope, NoneType::object());
  word sum = 0;

  for (word i = 0; i < 16; i++) {
    value = SmallInt::fromWord(i);
    runtime.setAdd(set, value);
    sum += i;
  }

  Handle<Object> set_obj(&scope, *set);
  runtime.listExtend(Thread::currentThread(), list,
                     Handle<Object>(&scope, *set_obj));
  EXPECT_EQ(list->allocated(), 16);

  for (word i = 0; i < 16; i++) {
    sum -= SmallInt::cast(list->at(i))->value();
  }
  ASSERT_EQ(sum, 0);
}

TEST(RuntimeListTest, ListExtendDict) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  Handle<Dict> dict(&scope, runtime.newDict());
  Handle<Object> value(&scope, NoneType::object());
  word sum = 0;

  for (word i = 0; i < 16; i++) {
    value = SmallInt::fromWord(i);
    runtime.dictAtPut(dict, value, value);
    sum += i;
  }

  Handle<Object> dict_obj(&scope, *dict);
  runtime.listExtend(Thread::currentThread(), list,
                     Handle<Object>(&scope, *dict_obj));
  EXPECT_EQ(list->allocated(), 16);

  for (word i = 0; i < 16; i++) {
    sum -= SmallInt::cast(list->at(i))->value();
  }
  ASSERT_EQ(sum, 0);
}

static RawObject iterableWithLengthHint(Runtime* runtime) {
  HandleScope scope;
  runtime->runFromCStr(R"(
class Iterator:
    def __init__(self):
        self.current = 0
        self.list = [1, 2, 3]

    def __iter__(self):
        return self

    def __next__(self):
        if self.current < len(self.list):
            value = self.list[self.current]
            self.current += 1
            return value
        raise StopIteration()

    def __length_hint__(self):
        return len(self.list) - self.current

iterator = Iterator()
)");
  Handle<Module> main(&scope, testing::findModule(runtime, "__main__"));
  Handle<Object> iterator(&scope, testing::moduleAt(runtime, main, "iterator"));
  return *iterator;
}

static RawObject iterableWithoutLengthHint(Runtime* runtime) {
  HandleScope scope;
  runtime->runFromCStr(R"(
class Iterator:
    def __init__(self):
        self.current = 0
        self.list = [1, 2, 3]

    def __iter__(self):
        return self

    def __next__(self):
        if self.current < len(self.list):
            value = self.list[self.current]
            self.current += 1
            return value
        raise StopIteration()

iterator = Iterator()
)");
  Handle<Module> main(&scope, testing::findModule(runtime, "__main__"));
  Handle<Object> iterator(&scope, testing::moduleAt(runtime, main, "iterator"));
  return *iterator;
}

TEST(RuntimeListTest, ListExtendIterator) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  Handle<Object> iterator(&scope, iterableWithLengthHint(&runtime));
  runtime.listExtend(Thread::currentThread(), list, iterator);

  EXPECT_EQ(list->allocated(), 3);

  ASSERT_TRUE(list->at(0)->isSmallInt());
  EXPECT_EQ(SmallInt::cast(list->at(0))->value(), 1);

  ASSERT_TRUE(list->at(1)->isSmallInt());
  EXPECT_EQ(SmallInt::cast(list->at(1))->value(), 2);

  ASSERT_TRUE(list->at(2)->isSmallInt());
  EXPECT_EQ(SmallInt::cast(list->at(2))->value(), 3);
}

TEST(RuntimeListTest, ListExtendIteratorWithoutDunderLengthHint) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  Handle<Object> iterator(&scope, iterableWithoutLengthHint(&runtime));
  runtime.listExtend(Thread::currentThread(), list, iterator);

  // An iterator with no __length_hint__ should not be consumed
  ASSERT_EQ(list->allocated(), 0);
}

TEST(RuntimeTest, NewBytes) {
  Runtime runtime;
  HandleScope scope;

  Handle<Bytes> len0(&scope, runtime.newBytes(0, 0));
  EXPECT_EQ(len0->length(), 0);

  Handle<Bytes> len3(&scope, runtime.newBytes(3, 9));
  EXPECT_EQ(len3->length(), 3);
  EXPECT_EQ(len3->byteAt(0), 9);
  EXPECT_EQ(len3->byteAt(1), 9);
  EXPECT_EQ(len3->byteAt(2), 9);

  Handle<Bytes> len254(&scope, runtime.newBytes(254, 0));
  EXPECT_EQ(len254->length(), 254);
  EXPECT_EQ(len254->size(), Utils::roundUp(kPointerSize + 254, kPointerSize));

  Handle<Bytes> len255(&scope, runtime.newBytes(255, 0));
  EXPECT_EQ(len255->length(), 255);
  EXPECT_EQ(len255->size(),
            Utils::roundUp(kPointerSize * 2 + 255, kPointerSize));
}

TEST(RuntimeTest, NewBytesWithAll) {
  Runtime runtime;
  HandleScope scope;

  Handle<Bytes> len0(&scope, runtime.newBytesWithAll(View<byte>(nullptr, 0)));
  EXPECT_EQ(len0->length(), 0);

  const byte src1[] = {0x42};
  Handle<Bytes> len1(&scope, runtime.newBytesWithAll(src1));
  EXPECT_EQ(len1->length(), 1);
  EXPECT_EQ(len1->size(), Utils::roundUp(kPointerSize + 1, kPointerSize));
  EXPECT_EQ(len1->byteAt(0), 0x42);

  const byte src3[] = {0xAA, 0xBB, 0xCC};
  Handle<Bytes> len3(&scope, runtime.newBytesWithAll(src3));
  EXPECT_EQ(len3->length(), 3);
  EXPECT_EQ(len3->size(), Utils::roundUp(kPointerSize + 3, kPointerSize));
  EXPECT_EQ(len3->byteAt(0), 0xAA);
  EXPECT_EQ(len3->byteAt(1), 0xBB);
  EXPECT_EQ(len3->byteAt(2), 0xCC);
}

TEST(RuntimeTest, NewCode) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  EXPECT_EQ(code->argcount(), 0);
  EXPECT_EQ(code->cell2arg(), 0);
  ASSERT_TRUE(code->cellvars()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->cellvars())->length(), 0);
  EXPECT_TRUE(code->code()->isNoneType());
  EXPECT_TRUE(code->consts()->isNoneType());
  EXPECT_TRUE(code->filename()->isNoneType());
  EXPECT_EQ(code->firstlineno(), 0);
  EXPECT_EQ(code->flags(), 0);
  ASSERT_TRUE(code->freevars()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->freevars())->length(), 0);
  EXPECT_EQ(code->kwonlyargcount(), 0);
  EXPECT_TRUE(code->lnotab()->isNoneType());
  EXPECT_TRUE(code->name()->isNoneType());
  EXPECT_EQ(code->nlocals(), 0);
  EXPECT_EQ(code->stacksize(), 0);
  EXPECT_TRUE(code->varnames()->isNoneType());
}

TEST(RuntimeTest, NewObjectArray) {
  Runtime runtime;
  HandleScope scope;

  Handle<ObjectArray> a0(&scope, runtime.newObjectArray(0));
  EXPECT_EQ(a0->length(), 0);

  Handle<ObjectArray> a1(&scope, runtime.newObjectArray(1));
  ASSERT_EQ(a1->length(), 1);
  EXPECT_EQ(a1->at(0), NoneType::object());
  a1->atPut(0, SmallInt::fromWord(42));
  EXPECT_EQ(a1->at(0), SmallInt::fromWord(42));

  Handle<ObjectArray> a300(&scope, runtime.newObjectArray(300));
  ASSERT_EQ(a300->length(), 300);
}

TEST(RuntimeTest, NewStr) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes[400]{0};
  Handle<Str> empty0(&scope, runtime.newStrWithAll(View<byte>(bytes, 0)));
  ASSERT_TRUE(empty0->isSmallStr());
  EXPECT_EQ(empty0->length(), 0);

  Handle<Str> empty1(&scope, runtime.newStrWithAll(View<byte>(bytes, 0)));
  ASSERT_TRUE(empty1->isSmallStr());
  EXPECT_EQ(*empty0, *empty1);

  Handle<Str> empty2(&scope, runtime.newStrFromCStr("\0"));
  ASSERT_TRUE(empty2->isSmallStr());
  EXPECT_EQ(*empty0, *empty2);

  Handle<Str> s1(&scope, runtime.newStrWithAll(View<byte>(bytes, 1)));
  ASSERT_TRUE(s1->isSmallStr());
  EXPECT_EQ(s1->length(), 1);

  Handle<Str> s254(&scope, runtime.newStrWithAll(View<byte>(bytes, 254)));
  EXPECT_EQ(s254->length(), 254);
  ASSERT_TRUE(s254->isLargeStr());
  EXPECT_EQ(HeapObject::cast(*s254)->size(),
            Utils::roundUp(kPointerSize + 254, kPointerSize));

  Handle<Str> s255(&scope, runtime.newStrWithAll(View<byte>(bytes, 255)));
  EXPECT_EQ(s255->length(), 255);
  ASSERT_TRUE(s255->isLargeStr());
  EXPECT_EQ(HeapObject::cast(*s255)->size(),
            Utils::roundUp(kPointerSize * 2 + 255, kPointerSize));

  Handle<Str> s300(&scope, runtime.newStrWithAll(View<byte>(bytes, 300)));
  ASSERT_EQ(s300->length(), 300);
}

TEST(RuntimeTest, NewStrFromFormatWithCStrArg) {
  Runtime runtime;
  HandleScope scope;

  const char input[] = "hello";
  Handle<Str> str(&scope, runtime.newStrFromFormat("%s", input));
  EXPECT_PYSTRING_EQ(*str, input);
}

TEST(RuntimeTest, NewStrWithAll) {
  Runtime runtime;
  HandleScope scope;

  Handle<Str> str0(&scope, runtime.newStrWithAll(View<byte>(nullptr, 0)));
  EXPECT_EQ(str0->length(), 0);
  EXPECT_TRUE(str0->equalsCStr(""));

  const byte bytes3[] = {'A', 'B', 'C'};
  Handle<Str> str3(&scope, runtime.newStrWithAll(bytes3));
  EXPECT_EQ(str3->length(), 3);
  EXPECT_TRUE(str3->equalsCStr("ABC"));

  const byte bytes10[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'};
  Handle<Str> str10(&scope, runtime.newStrWithAll(bytes10));
  EXPECT_EQ(str10->length(), 10);
  EXPECT_TRUE(str10->equalsCStr("ABCDEFGHIJ"));
}

TEST(RuntimeTest, HashBools) {
  Runtime runtime;

  // In CPython, False hashes to 0 and True hashes to 1.
  RawSmallInt hash0 = SmallInt::cast(runtime.hash(Bool::falseObj()));
  EXPECT_EQ(hash0->value(), 0);
  RawSmallInt hash1 = SmallInt::cast(runtime.hash(Bool::trueObj()));
  EXPECT_EQ(hash1->value(), 1);
}

TEST(RuntimeTest, HashBytes) {
  Runtime runtime;
  HandleScope scope;

  // Strings have their hash codes computed lazily.
  const byte src1[] = {0x1, 0x2, 0x3};
  Handle<Bytes> arr1(&scope, runtime.newBytesWithAll(src1));
  EXPECT_EQ(arr1->header()->hashCode(), 0);
  word hash1 = SmallInt::cast(runtime.hash(*arr1))->value();
  EXPECT_NE(arr1->header()->hashCode(), 0);
  EXPECT_EQ(arr1->header()->hashCode(), hash1);

  word code1 = runtime.siphash24(src1);
  EXPECT_EQ(code1 & Header::kHashCodeMask, static_cast<uword>(hash1));

  // Str with different values should (ideally) hash differently.
  const byte src2[] = {0x3, 0x2, 0x1};
  Handle<Bytes> arr2(&scope, runtime.newBytesWithAll(src2));
  word hash2 = SmallInt::cast(runtime.hash(*arr2))->value();
  EXPECT_NE(hash1, hash2);

  word code2 = runtime.siphash24(src2);
  EXPECT_EQ(code2 & Header::kHashCodeMask, static_cast<uword>(hash2));

  // Strings with the same value should hash the same.
  const byte src3[] = {0x1, 0x2, 0x3};
  Handle<Bytes> arr3(&scope, runtime.newBytesWithAll(src3));
  word hash3 = SmallInt::cast(runtime.hash(*arr3))->value();
  EXPECT_EQ(hash1, hash3);

  word code3 = runtime.siphash24(src3);
  EXPECT_EQ(code3 & Header::kHashCodeMask, static_cast<uword>(hash3));
}

TEST(RuntimeTest, HashSmallInts) {
  Runtime runtime;

  // In CPython, Ints hash to themselves.
  RawSmallInt hash123 = SmallInt::cast(runtime.hash(SmallInt::fromWord(123)));
  EXPECT_EQ(hash123->value(), 123);
  RawSmallInt hash456 = SmallInt::cast(runtime.hash(SmallInt::fromWord(456)));
  EXPECT_EQ(hash456->value(), 456);
}

TEST(RuntimeTest, HashSingletonImmediates) {
  Runtime runtime;

  // In CPython, these objects hash to arbitrary values.
  word none_value = NoneType::object().raw();
  RawSmallInt hash_none = SmallInt::cast(runtime.hash(NoneType::object()));
  EXPECT_EQ(hash_none->value(), none_value);

  word error_value = Error::object().raw();
  RawSmallInt hash_error = SmallInt::cast(runtime.hash(Error::object()));
  EXPECT_EQ(hash_error->value(), error_value);
}

TEST(RuntimeTest, HashStr) {
  Runtime runtime;
  HandleScope scope;

  // LargeStr instances have their hash codes computed lazily.
  Handle<Object> str1(&scope, runtime.newStrFromCStr("testing 123"));
  EXPECT_EQ(HeapObject::cast(*str1)->header()->hashCode(), 0);
  RawSmallInt hash1 = SmallInt::cast(runtime.hash(*str1));
  EXPECT_NE(HeapObject::cast(*str1)->header()->hashCode(), 0);
  EXPECT_EQ(HeapObject::cast(*str1)->header()->hashCode(), hash1->value());

  // Str with different values should (ideally) hash differently.
  Handle<Str> str2(&scope, runtime.newStrFromCStr("321 testing"));
  RawSmallInt hash2 = SmallInt::cast(runtime.hash(*str2));
  EXPECT_NE(hash1, hash2);

  // Strings with the same value should hash the same.
  Handle<Str> str3(&scope, runtime.newStrFromCStr("testing 123"));
  RawSmallInt hash3 = SmallInt::cast(runtime.hash(*str3));
  EXPECT_EQ(hash1, hash3);
}

TEST(RuntimeTest, Random) {
  Runtime runtime;
  uword r1 = runtime.random();
  uword r2 = runtime.random();
  EXPECT_NE(r1, r2);
  uword r3 = runtime.random();
  EXPECT_NE(r2, r3);
  uword r4 = runtime.random();
  EXPECT_NE(r3, r4);
}

TEST(RuntimeTest, HashCodeSizeCheck) {
  Runtime runtime;
  RawObject code = runtime.newCode();
  ASSERT_TRUE(code->isHeapObject());
  EXPECT_EQ(HeapObject::cast(code)->header()->hashCode(), 0);
  // Verify that large-magnitude random numbers are properly
  // truncated to somethat which fits in a SmallInt

  // Conspire based on knoledge of the random number genrated to
  // create a high-magnitude result from Runtime::random
  // which is truncated to 0 for storage in the header and
  // replaced with "1" so no hash code has value 0.
  uword high = static_cast<uword>(1) << (8 * sizeof(uword) - 1);
  uword state[2] = {0, high};
  uword secret[2] = {0, 0};
  runtime.seedRandom(state, secret);
  uword first = runtime.random();
  EXPECT_EQ(first, high);
  runtime.seedRandom(state, secret);
  word hash1 = SmallInt::cast(runtime.hash(code))->value();
  EXPECT_EQ(hash1, 1);
}

TEST(RuntimeTest, EnsureCapacity) {
  Runtime runtime;
  HandleScope scope;

  // Check that empty arrays expand
  Handle<List> list(&scope, runtime.newList());
  Handle<ObjectArray> empty(&scope, list->items());
  runtime.listEnsureCapacity(list, 0);
  Handle<ObjectArray> orig(&scope, list->items());
  ASSERT_NE(*empty, *orig);
  ASSERT_GT(orig->length(), 0);

  // We shouldn't grow the array if there is sufficient capacity
  runtime.listEnsureCapacity(list, orig->length() - 1);
  Handle<ObjectArray> ensured0(&scope, list->items());
  ASSERT_EQ(*orig, *ensured0);

  // We should double the array if there is insufficient capacity
  runtime.listEnsureCapacity(list, orig->length());
  Handle<ObjectArray> ensured1(&scope, list->items());
  ASSERT_EQ(ensured1->length(), orig->length() * 2);
}

TEST(RuntimeTest, InternLargeStr) {
  Runtime runtime;
  HandleScope scope;

  Handle<Set> interned(&scope, runtime.interned());

  // Creating an ordinary large string should not affect on the intern table.
  word num_interned = interned->numItems();
  Handle<Object> str1(&scope, runtime.newStrFromCStr("hello, world"));
  ASSERT_TRUE(str1->isLargeStr());
  EXPECT_EQ(num_interned, interned->numItems());
  EXPECT_FALSE(runtime.setIncludes(interned, str1));

  // Interning the string should add it to the intern table and increase the
  // size of the intern table by one.
  num_interned = interned->numItems();
  Handle<Object> sym1(&scope, runtime.internStr(str1));
  EXPECT_TRUE(runtime.setIncludes(interned, str1));
  EXPECT_EQ(*sym1, *str1);
  EXPECT_EQ(num_interned + 1, interned->numItems());

  Handle<Object> str2(&scope, runtime.newStrFromCStr("goodbye, world"));
  ASSERT_TRUE(str2->isLargeStr());
  EXPECT_NE(*str1, *str2);

  // Intern another string and make sure we get it back (as opposed to the
  // previously interned string).
  num_interned = interned->numItems();
  Handle<Object> sym2(&scope, runtime.internStr(str2));
  EXPECT_EQ(num_interned + 1, interned->numItems());
  EXPECT_TRUE(runtime.setIncludes(interned, str2));
  EXPECT_EQ(*sym2, *str2);
  EXPECT_NE(*sym1, *sym2);

  // Create a unique copy of a previously created string.
  Handle<Object> str3(&scope, runtime.newStrFromCStr("hello, world"));
  ASSERT_TRUE(str3->isLargeStr());
  EXPECT_NE(*str1, *str3);
  EXPECT_TRUE(runtime.setIncludes(interned, str3));

  // Interning a duplicate string should not affecct the intern table.
  num_interned = interned->numItems();
  Handle<Object> sym3(&scope, runtime.internStr(str3));
  EXPECT_EQ(num_interned, interned->numItems());
  EXPECT_NE(*sym3, *str3);
  EXPECT_EQ(*sym3, *sym1);
}

TEST(RuntimeTest, InternSmallStr) {
  Runtime runtime;
  HandleScope scope;

  Handle<Set> interned(&scope, runtime.interned());

  // Creating a small string should not affect the intern table.
  word num_interned = interned->numItems();
  Handle<Object> str(&scope, runtime.newStrFromCStr("a"));
  ASSERT_TRUE(str->isSmallStr());
  EXPECT_FALSE(runtime.setIncludes(interned, str));
  EXPECT_EQ(num_interned, interned->numItems());

  // Interning a small string should have no affect on the intern table.
  Handle<Object> sym(&scope, runtime.internStr(str));
  EXPECT_TRUE(sym->isSmallStr());
  EXPECT_FALSE(runtime.setIncludes(interned, str));
  EXPECT_EQ(num_interned, interned->numItems());
  EXPECT_EQ(*sym, *str);
}

TEST(RuntimeTest, InternCStr) {
  Runtime runtime;
  HandleScope scope;

  Handle<Set> interned(&scope, runtime.interned());

  word num_interned = interned->numItems();
  Handle<Object> sym(&scope, runtime.internStrFromCStr("hello, world"));
  EXPECT_TRUE(sym->isStr());
  EXPECT_TRUE(runtime.setIncludes(interned, sym));
  EXPECT_EQ(num_interned + 1, interned->numItems());
}

TEST(RuntimeTest, CollectAttributes) {
  Runtime runtime;
  HandleScope scope;

  Handle<Object> foo(&scope, runtime.newStrFromCStr("foo"));
  Handle<Object> bar(&scope, runtime.newStrFromCStr("bar"));
  Handle<Object> baz(&scope, runtime.newStrFromCStr("baz"));

  Handle<ObjectArray> names(&scope, runtime.newObjectArray(3));
  names->atPut(0, *foo);
  names->atPut(1, *bar);
  names->atPut(2, *baz);

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(4));
  consts->atPut(0, SmallInt::fromWord(100));
  consts->atPut(1, SmallInt::fromWord(200));
  consts->atPut(2, SmallInt::fromWord(300));
  consts->atPut(3, NoneType::object());

  Handle<Code> code(&scope, runtime.newCode());
  code->setNames(*names);
  // Bytecode for the snippet:
  //
  //   def __init__(self):
  //       self.foo = 100
  //       self.foo = 200
  //
  // The assignment to self.foo is intentionally duplicated to ensure that we
  // only record a single attribute name.
  const byte bc[] = {LOAD_CONST,   0, LOAD_FAST, 0, STORE_ATTR, 0,
                     LOAD_CONST,   1, LOAD_FAST, 0, STORE_ATTR, 0,
                     RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc));

  Handle<Dict> attributes(&scope, runtime.newDict());
  runtime.collectAttributes(code, attributes);

  // We should have collected a single attribute: 'foo'
  EXPECT_EQ(attributes->numItems(), 1);

  // Check that we collected 'foo'
  Handle<Object> result(&scope, runtime.dictAt(attributes, foo));
  ASSERT_TRUE(result->isStr());
  EXPECT_TRUE(Str::cast(*result)->equals(*foo));

  // Bytecode for the snippet:
  //
  //   def __init__(self):
  //       self.bar = 200
  //       self.baz = 300
  const byte bc2[] = {LOAD_CONST,   1, LOAD_FAST, 0, STORE_ATTR, 1,
                      LOAD_CONST,   2, LOAD_FAST, 0, STORE_ATTR, 2,
                      RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc2));
  runtime.collectAttributes(code, attributes);

  // We should have collected a two more attributes: 'bar' and 'baz'
  EXPECT_EQ(attributes->numItems(), 3);

  // Check that we collected 'bar'
  result = runtime.dictAt(attributes, bar);
  ASSERT_TRUE(result->isStr());
  EXPECT_TRUE(Str::cast(*result)->equals(*bar));

  // Check that we collected 'baz'
  result = runtime.dictAt(attributes, baz);
  ASSERT_TRUE(result->isStr());
  EXPECT_TRUE(Str::cast(*result)->equals(*baz));
}

TEST(RuntimeTest, CollectAttributesWithExtendedArg) {
  Runtime runtime;
  HandleScope scope;

  Handle<Object> foo(&scope, runtime.newStrFromCStr("foo"));
  Handle<Object> bar(&scope, runtime.newStrFromCStr("bar"));

  Handle<ObjectArray> names(&scope, runtime.newObjectArray(2));
  names->atPut(0, *foo);
  names->atPut(1, *bar);

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(1));
  consts->atPut(0, NoneType::object());

  Handle<Code> code(&scope, runtime.newCode());
  code->setNames(*names);
  // Bytecode for the snippet:
  //
  //   def __init__(self):
  //       self.foo = None
  //
  // There is an additional LOAD_FAST that is preceded by an EXTENDED_ARG
  // that must be skipped.
  const byte bc[] = {LOAD_CONST, 0, EXTENDED_ARG, 10, LOAD_FAST, 0,
                     STORE_ATTR, 1, LOAD_CONST,   0,  LOAD_FAST, 0,
                     STORE_ATTR, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bc));

  Handle<Dict> attributes(&scope, runtime.newDict());
  runtime.collectAttributes(code, attributes);

  // We should have collected a single attribute: 'foo'
  EXPECT_EQ(attributes->numItems(), 1);

  // Check that we collected 'foo'
  Handle<Object> result(&scope, runtime.dictAt(attributes, foo));
  ASSERT_TRUE(result->isStr());
  EXPECT_TRUE(Str::cast(*result)->equals(*foo));
}

TEST(RuntimeTest, GetClassConstructor) {
  Runtime runtime;
  HandleScope scope;
  Handle<Type> klass(&scope, runtime.newClass());
  Handle<Dict> klass_dict(&scope, runtime.newDict());
  klass->setDict(*klass_dict);

  EXPECT_EQ(runtime.classConstructor(klass), NoneType::object());

  Handle<Object> init(&scope, runtime.symbols()->DunderInit());
  Handle<Object> func(&scope, runtime.newFunction());
  runtime.dictAtPutInValueCell(klass_dict, init, func);

  EXPECT_EQ(runtime.classConstructor(klass), *func);
}

TEST(RuntimeTest, NewInstanceEmptyClass) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr("class MyEmptyClass: pass");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> type(&scope, moduleAt(&runtime, main, "MyEmptyClass"));
  Handle<Layout> layout(&scope, type->instanceLayout());
  EXPECT_EQ(layout->instanceSize(), 1);

  Handle<Type> cls(&scope, layout->describedClass());
  EXPECT_PYSTRING_EQ(Str::cast(cls->name()), "MyEmptyClass");

  Handle<Instance> instance(&scope, runtime.newInstance(layout));
  EXPECT_TRUE(instance->isInstance());
  EXPECT_EQ(instance->header()->layoutId(), layout->id());
}

TEST(RuntimeTest, NewInstanceManyAttributes) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class MyClassWithAttributes():
  def __init__(self):
    self.a = 1
    self.b = 2
    self.c = 3
)";
  runtime.runFromCStr(src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> type(&scope, moduleAt(&runtime, main, "MyClassWithAttributes"));
  Handle<Layout> layout(&scope, type->instanceLayout());
  ASSERT_EQ(layout->instanceSize(), 4);

  Handle<Type> cls(&scope, layout->describedClass());
  EXPECT_PYSTRING_EQ(Str::cast(cls->name()), "MyClassWithAttributes");

  Handle<Instance> instance(&scope, runtime.newInstance(layout));
  EXPECT_TRUE(instance->isInstance());
  EXPECT_EQ(instance->header()->layoutId(), layout->id());
}

TEST(RuntimeTest, VerifySymbols) {
  Runtime runtime;
  Symbols* symbols = runtime.symbols();
  for (int i = 0; i < static_cast<int>(SymbolId::kMaxId); i++) {
    SymbolId id = static_cast<SymbolId>(i);
    RawObject value = symbols->at(id);
    ASSERT_TRUE(value->isStr());
    const char* expected = symbols->literalAt(id);
    EXPECT_TRUE(Str::cast(value)->equalsCStr(expected))
        << "Incorrect symbol value for " << expected;
  }
}

static RawStr className(Runtime* runtime, RawObject o) {
  auto cls = Type::cast(runtime->typeOf(o));
  auto name = Str::cast(cls->name());
  return name;
}

TEST(RuntimeTest, ClassIds) {
  Runtime runtime;
  HandleScope scope;

  EXPECT_PYSTRING_EQ(className(&runtime, Bool::trueObj()), "bool");
  EXPECT_PYSTRING_EQ(className(&runtime, NoneType::object()), "NoneType");
  EXPECT_PYSTRING_EQ(className(&runtime, runtime.newStrFromCStr("abc")),
                     "smallstr");

  for (word i = 0; i < 16; i++) {
    auto small_int = SmallInt::fromWord(i);
    EXPECT_PYSTRING_EQ(className(&runtime, small_int), "smallint");
  }
}

TEST(RuntimeStrTest, StrConcat) {
  Runtime runtime;
  HandleScope scope;

  Handle<Str> str1(&scope, runtime.newStrFromCStr("abc"));
  Handle<Str> str2(&scope, runtime.newStrFromCStr("def"));

  // Large strings.
  Handle<Str> str3(&scope, runtime.newStrFromCStr("0123456789abcdef"));
  Handle<Str> str4(&scope, runtime.newStrFromCStr("fedbca9876543210"));

  Handle<Str> concat12(&scope, runtime.strConcat(str1, str2));
  Handle<Str> concat34(&scope, runtime.strConcat(str3, str4));

  Handle<Str> concat13(&scope, runtime.strConcat(str1, str3));
  Handle<Str> concat31(&scope, runtime.strConcat(str3, str1));

  // Test that we don't make large strings when small srings would suffice.
  EXPECT_PYSTRING_EQ(*concat12, "abcdef");
  EXPECT_PYSTRING_EQ(*concat34, "0123456789abcdeffedbca9876543210");
  EXPECT_PYSTRING_EQ(*concat13, "abc0123456789abcdef");
  EXPECT_PYSTRING_EQ(*concat31, "0123456789abcdefabc");

  EXPECT_TRUE(concat12->isSmallStr());
  EXPECT_TRUE(concat34->isLargeStr());
  EXPECT_TRUE(concat13->isLargeStr());
  EXPECT_TRUE(concat31->isLargeStr());
}

TEST(RuntimeStrTest, StrStripSpaceWithEmptyStrIsIdentity) {
  Runtime runtime;
  HandleScope scope;
  Handle<Str> empty_str(&scope, runtime.newStrFromCStr(""));
  Handle<Str> lstripped_empty_str(
      &scope, runtime.strStripSpace(empty_str, StrStripDirection::Left));
  EXPECT_EQ(*empty_str, *lstripped_empty_str);

  Handle<Str> rstripped_empty_str(
      &scope, runtime.strStripSpace(empty_str, StrStripDirection::Right));
  EXPECT_EQ(*empty_str, *rstripped_empty_str);

  Handle<Str> stripped_empty_str(
      &scope, runtime.strStripSpace(empty_str, StrStripDirection::Both));
  EXPECT_EQ(*empty_str, *stripped_empty_str);
}

TEST(RuntimeStrTest, StrStripSpaceWithUnstrippableStrIsIdentity) {
  Runtime runtime;
  HandleScope scope;
  Handle<Str> str(&scope, runtime.newStrFromCStr("Nothing to strip here"));
  ASSERT_TRUE(str->isLargeStr());
  Handle<Str> lstripped_str(
      &scope, runtime.strStripSpace(str, StrStripDirection::Left));
  EXPECT_EQ(*str, *lstripped_str);

  Handle<Str> rstripped_str(
      &scope, runtime.strStripSpace(str, StrStripDirection::Right));
  EXPECT_EQ(*str, *rstripped_str);

  Handle<Str> stripped_str(&scope,
                           runtime.strStripSpace(str, StrStripDirection::Both));
  EXPECT_EQ(*str, *stripped_str);
}

TEST(RuntimeStrTest, StrStripSpaceWithUnstrippableSmallStrIsIdentity) {
  Runtime runtime;
  HandleScope scope;
  Handle<Str> str(&scope, runtime.newStrFromCStr("nostrip"));
  ASSERT_TRUE(str->isSmallStr());
  Handle<Str> lstripped_str(
      &scope, runtime.strStripSpace(str, StrStripDirection::Left));
  EXPECT_EQ(*str, *lstripped_str);

  Handle<Str> rstripped_str(
      &scope, runtime.strStripSpace(str, StrStripDirection::Right));
  EXPECT_EQ(*str, *rstripped_str);

  Handle<Str> stripped_str(&scope,
                           runtime.strStripSpace(str, StrStripDirection::Both));
  EXPECT_EQ(*str, *stripped_str);
}

TEST(RuntimeStrTest, StrStripSpaceWithFullyStrippableStrReturnsEmptyStr) {
  Runtime runtime;
  HandleScope scope;
  Handle<Str> str(&scope, runtime.newStrFromCStr("\n\r\t\f         \n\t\r\f"));
  Handle<Str> lstripped_str(
      &scope, runtime.strStripSpace(str, StrStripDirection::Left));
  EXPECT_EQ(lstripped_str->length(), 0);

  Handle<Str> rstripped_str(
      &scope, runtime.strStripSpace(str, StrStripDirection::Right));
  EXPECT_EQ(rstripped_str->length(), 0);

  Handle<Str> stripped_str(&scope,
                           runtime.strStripSpace(str, StrStripDirection::Both));
  EXPECT_EQ(stripped_str->length(), 0);
}

TEST(RuntimeStrTest, StrStripSpaceLeft) {
  Runtime runtime;
  HandleScope scope;
  Handle<Str> str(&scope, runtime.newStrFromCStr(" strp "));
  ASSERT_TRUE(str->isSmallStr());
  Handle<Str> lstripped_str(
      &scope, runtime.strStripSpace(str, StrStripDirection::Left));
  ASSERT_TRUE(lstripped_str->isSmallStr());
  EXPECT_PYSTRING_EQ(*lstripped_str, "strp ");

  Handle<Str> str1(&scope,
                   runtime.newStrFromCStr("   \n \n\tLot of leading space  "));
  ASSERT_TRUE(str1->isLargeStr());
  Handle<Str> lstripped_str1(
      &scope, runtime.strStripSpace(str1, StrStripDirection::Left));
  EXPECT_PYSTRING_EQ(*lstripped_str1, "Lot of leading space  ");

  Handle<Str> str2(&scope,
                   runtime.newStrFromCStr("\n\n\n              \ntest"));
  ASSERT_TRUE(str2->isLargeStr());
  Handle<Str> lstripped_str2(
      &scope, runtime.strStripSpace(str2, StrStripDirection::Left));
  ASSERT_TRUE(lstripped_str2->isSmallStr());
  EXPECT_PYSTRING_EQ(*lstripped_str2, "test");
}

TEST(RuntimeStrTest, StrStripSpaceRight) {
  Runtime runtime;
  HandleScope scope;
  Handle<Str> str(&scope, runtime.newStrFromCStr(" strp "));
  ASSERT_TRUE(str->isSmallStr());
  Handle<Str> rstripped_str(
      &scope, runtime.strStripSpace(str, StrStripDirection::Right));
  ASSERT_TRUE(rstripped_str->isSmallStr());
  EXPECT_PYSTRING_EQ(*rstripped_str, " strp");

  Handle<Str> str1(
      &scope, runtime.newStrFromCStr("  Lot of trailing space\t\n \n    "));
  ASSERT_TRUE(str1->isLargeStr());
  Handle<Str> rstripped_str1(
      &scope, runtime.strStripSpace(str1, StrStripDirection::Right));
  EXPECT_PYSTRING_EQ(*rstripped_str1, "  Lot of trailing space");

  Handle<Str> str2(&scope, runtime.newStrFromCStr("test\n      \n\n\n"));
  ASSERT_TRUE(str2->isLargeStr());
  Handle<Str> rstripped_str2(
      &scope, runtime.strStripSpace(str2, StrStripDirection::Right));
  ASSERT_TRUE(rstripped_str2->isSmallStr());
  EXPECT_PYSTRING_EQ(*rstripped_str2, "test");
}

TEST(RuntimeStrTest, StrStripSpaceBoth) {
  Runtime runtime;
  HandleScope scope;
  Handle<Str> str(&scope, runtime.newStrFromCStr(" strp "));
  ASSERT_TRUE(str->isSmallStr());
  Handle<Str> stripped_str(&scope,
                           runtime.strStripSpace(str, StrStripDirection::Both));
  ASSERT_TRUE(stripped_str->isSmallStr());
  EXPECT_PYSTRING_EQ(*stripped_str, "strp");

  Handle<Str> str1(
      &scope, runtime.newStrFromCStr(
                  "\n \n    \n\tLot of leading and trailing space\n \n    "));
  ASSERT_TRUE(str1->isLargeStr());
  Handle<Str> stripped_str1(
      &scope, runtime.strStripSpace(str1, StrStripDirection::Both));
  EXPECT_PYSTRING_EQ(*stripped_str1, "Lot of leading and trailing space");

  Handle<Str> str2(&scope, runtime.newStrFromCStr("\n\ttest\t      \n\n\n"));
  ASSERT_TRUE(str2->isLargeStr());
  Handle<Str> stripped_str2(
      &scope, runtime.strStripSpace(str2, StrStripDirection::Both));
  ASSERT_TRUE(stripped_str2->isSmallStr());
  EXPECT_PYSTRING_EQ(*stripped_str2, "test");
}

TEST(RuntimeStrTest, StrStripWithEmptyStrIsIdentity) {
  Runtime runtime;
  HandleScope scope;
  Handle<Str> empty_str(&scope, runtime.newStrFromCStr(""));
  Handle<Str> chars(&scope, runtime.newStrFromCStr("abc"));
  Handle<Str> lstripped_empty_str(
      &scope, runtime.strStrip(empty_str, chars, StrStripDirection::Left));
  EXPECT_EQ(*empty_str, *lstripped_empty_str);

  Handle<Str> rstripped_empty_str(
      &scope, runtime.strStrip(empty_str, chars, StrStripDirection::Right));
  EXPECT_EQ(*empty_str, *rstripped_empty_str);

  Handle<Str> stripped_empty_str(
      &scope, runtime.strStrip(empty_str, chars, StrStripDirection::Both));
  EXPECT_EQ(*empty_str, *stripped_empty_str);
}

TEST(RuntimeStrTest, StrStripWithFullyStrippableStrReturnsEmptyStr) {
  Runtime runtime;
  HandleScope scope;
  Handle<Str> str(&scope, runtime.newStrFromCStr("bbbbaaaaccccdddd"));
  Handle<Str> chars(&scope, runtime.newStrFromCStr("abcd"));
  Handle<Str> lstripped_str(
      &scope, runtime.strStrip(str, chars, StrStripDirection::Left));
  EXPECT_EQ(lstripped_str->length(), 0);

  Handle<Str> rstripped_str(
      &scope, runtime.strStrip(str, chars, StrStripDirection::Right));
  EXPECT_EQ(rstripped_str->length(), 0);

  Handle<Str> stripped_str(
      &scope, runtime.strStrip(str, chars, StrStripDirection::Both));
  EXPECT_EQ(stripped_str->length(), 0);
}

TEST(RuntimeStrTest, StrStripWithEmptyCharsIsIdentity) {
  Runtime runtime;
  HandleScope scope;
  Handle<Str> str(&scope, runtime.newStrFromCStr(" Just another string "));
  Handle<Str> chars(&scope, runtime.newStrFromCStr(""));
  Handle<Str> lstripped_str(
      &scope, runtime.strStrip(str, chars, StrStripDirection::Left));
  EXPECT_EQ(*str, *lstripped_str);

  Handle<Str> rstripped_str(
      &scope, runtime.strStrip(str, chars, StrStripDirection::Right));
  EXPECT_EQ(*str, *rstripped_str);

  Handle<Str> stripped_str(
      &scope, runtime.strStrip(str, chars, StrStripDirection::Both));
  EXPECT_EQ(*str, *stripped_str);
}

TEST(RuntimeStrTest, StrStripBoth) {
  Runtime runtime;
  HandleScope scope;
  Handle<Str> str(&scope, runtime.newStrFromCStr("bcdHello Worldcab"));
  Handle<Str> chars(&scope, runtime.newStrFromCStr("abcd"));
  Handle<Str> stripped_str(
      &scope, runtime.strStrip(str, chars, StrStripDirection::Both));
  EXPECT_PYSTRING_EQ(*stripped_str, "Hello Worl");
}

TEST(RuntimeStrTest, StrStripLeft) {
  Runtime runtime;
  HandleScope scope;
  Handle<Str> str(&scope, runtime.newStrFromCStr("bcdHello Worldcab"));
  Handle<Str> chars(&scope, runtime.newStrFromCStr("abcd"));
  Handle<Str> lstripped_str(
      &scope, runtime.strStrip(str, chars, StrStripDirection::Left));
  EXPECT_PYSTRING_EQ(*lstripped_str, "Hello Worldcab");
}

TEST(RuntimeStrTest, StrStripRight) {
  Runtime runtime;
  HandleScope scope;
  Handle<Str> str(&scope, runtime.newStrFromCStr("bcdHello Worldcab"));
  Handle<Str> chars(&scope, runtime.newStrFromCStr("abcd"));
  Handle<Str> rstripped_str(
      &scope, runtime.strStrip(str, chars, StrStripDirection::Right));
  EXPECT_PYSTRING_EQ(*rstripped_str, "bcdHello Worl");
}

struct LookupNameInMroData {
  const char* test_name;
  const char* name;
  RawObject expected;
};

static std::string lookupNameInMroTestName(
    ::testing::TestParamInfo<LookupNameInMroData> info) {
  return info.param.test_name;
}

class LookupNameInMroTest
    : public ::testing::TestWithParam<LookupNameInMroData> {};

TEST_P(LookupNameInMroTest, Lookup) {
  Runtime runtime;
  HandleScope scope;

  auto create_class_with_attr = [&](const char* attr, word value) {
    Handle<Type> klass(&scope, runtime.newClass());
    Handle<Dict> dict(&scope, klass->dict());
    Handle<Object> key(&scope, runtime.newStrFromCStr(attr));
    Handle<Object> val(&scope, SmallInt::fromWord(value));
    runtime.dictAtPutInValueCell(dict, key, val);
    return *klass;
  };

  Handle<ObjectArray> mro(&scope, runtime.newObjectArray(3));
  mro->atPut(0, create_class_with_attr("foo", 2));
  mro->atPut(1, create_class_with_attr("bar", 4));
  mro->atPut(2, create_class_with_attr("baz", 8));

  Handle<Type> klass(&scope, mro->at(0));
  klass->setMro(*mro);

  auto param = GetParam();
  Handle<Object> key(&scope, runtime.newStrFromCStr(param.name));
  RawObject result =
      runtime.lookupNameInMro(Thread::currentThread(), klass, key);
  EXPECT_EQ(result, param.expected);
}

INSTANTIATE_TEST_CASE_P(
    LookupNameInMro, LookupNameInMroTest,
    ::testing::Values(
        LookupNameInMroData{"OnInstance", "foo", SmallInt::fromWord(2)},
        LookupNameInMroData{"OnParent", "bar", SmallInt::fromWord(4)},
        LookupNameInMroData{"OnGrandParent", "baz", SmallInt::fromWord(8)},
        LookupNameInMroData{"NonExistent", "xxx", Error::object()}),
    lookupNameInMroTestName);

TEST(RuntimeTypeCallTest, TypeCallNoInitMethod) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class MyClassWithNoInitMethod():
  def m(self):
    pass

c = MyClassWithNoInitMethod()
)";
  runtime.runFromCStr(src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> instance(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(instance->isInstance());
  LayoutId layout_id = instance->layoutId();
  Handle<Layout> layout(&scope, runtime.layoutAt(layout_id));
  EXPECT_EQ(layout->instanceSize(), 1);

  Handle<Type> cls(&scope, layout->describedClass());
  EXPECT_PYSTRING_EQ(Str::cast(cls->name()), "MyClassWithNoInitMethod");
}

TEST(RuntimeTypeCallTest, TypeCallEmptyInitMethod) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class MyClassWithEmptyInitMethod():
  def __init__(self):
    pass
  def m(self):
    pass

c = MyClassWithEmptyInitMethod()
)";
  runtime.runFromCStr(src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> instance(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(instance->isInstance());
  LayoutId layout_id = instance->layoutId();
  Handle<Layout> layout(&scope, runtime.layoutAt(layout_id));
  EXPECT_EQ(layout->instanceSize(), 1);

  Handle<Type> cls(&scope, layout->describedClass());
  EXPECT_PYSTRING_EQ(Str::cast(cls->name()), "MyClassWithEmptyInitMethod");
}

TEST(RuntimeTypeCallTest, TypeCallWithArguments) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class MyClassWithAttributes():
  def __init__(self, x):
    self.x = x
  def m(self):
    pass

c = MyClassWithAttributes(1)
)";
  runtime.runFromCStr(src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> type(&scope, moduleAt(&runtime, main, "MyClassWithAttributes"));
  Handle<Object> instance(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(instance->isInstance());
  LayoutId layout_id = instance->layoutId();
  // Since this class has extra attributes, its layout id should be greater than
  // the layout id from the type.
  ASSERT_GT(layout_id, Layout::cast(type->instanceLayout())->id());
  Handle<Layout> layout(&scope, runtime.layoutAt(layout_id));
  ASSERT_EQ(layout->instanceSize(), 2);

  Handle<Type> cls(&scope, layout->describedClass());
  EXPECT_PYSTRING_EQ(Str::cast(cls->name()), "MyClassWithAttributes");

  Handle<Object> name(&scope, runtime.newStrFromCStr("x"));
  Handle<Object> value(
      &scope, runtime.attributeAt(Thread::currentThread(), instance, name));
  EXPECT_FALSE(value->isError());
  EXPECT_EQ(*value, SmallInt::fromWord(1));
}

TEST(RuntimeTest, ComputeLineNumberForBytecodeOffset) {
  Runtime runtime;
  const char* src = R"(
def func():
  a = 1
  b = 2
  print(a, b)
)";
  runtime.runFromCStr(src);
  HandleScope scope;
  Handle<Object> dunder_main(&scope, runtime.symbols()->DunderMain());
  Handle<Module> main(&scope, runtime.findModule(dunder_main));

  // The bytecode for func is roughly:
  // LOAD_CONST     # a = 1
  // STORE_FAST
  //
  // LOAD_CONST     # b = 2
  // STORE_FAST
  //
  // LOAD_GLOBAL    # print(a, b)
  // LOAD_FAST
  // LOAD_FAST
  // CALL_FUNCTION

  Handle<Object> name(&scope, runtime.newStrFromCStr("func"));
  Handle<Function> func(&scope, runtime.moduleAt(main, name));
  Handle<Code> code(&scope, func->code());
  ASSERT_EQ(code->firstlineno(), 2);

  // a = 1
  Thread* thread = Thread::currentThread();
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 0), 3);
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 2), 3);

  // b = 2
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 4), 4);
  EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, 6), 4);

  // print(a, b)
  for (word i = 8; i < Bytes::cast(code->code())->length(); i++) {
    EXPECT_EQ(runtime.codeOffsetToLineNum(thread, code, i), 5);
  }
}

TEST(RuntimeObjectArrayTest, Create) {
  Runtime runtime;

  RawObject obj0 = runtime.newObjectArray(0);
  ASSERT_TRUE(obj0->isObjectArray());
  RawObjectArray array0 = ObjectArray::cast(obj0);
  EXPECT_EQ(array0->length(), 0);

  RawObject obj1 = runtime.newObjectArray(1);
  ASSERT_TRUE(obj1->isObjectArray());
  RawObjectArray array1 = ObjectArray::cast(obj1);
  EXPECT_EQ(array1->length(), 1);

  RawObject obj7 = runtime.newObjectArray(7);
  ASSERT_TRUE(obj7->isObjectArray());
  RawObjectArray array7 = ObjectArray::cast(obj7);
  EXPECT_EQ(array7->length(), 7);

  RawObject obj8 = runtime.newObjectArray(8);
  ASSERT_TRUE(obj8->isObjectArray());
  RawObjectArray array8 = ObjectArray::cast(obj8);
  EXPECT_EQ(array8->length(), 8);
}

TEST(RuntimeSetTest, EmptySetInvariants) {
  Runtime runtime;
  HandleScope scope;
  Handle<Set> set(&scope, runtime.newSet());

  EXPECT_EQ(set->numItems(), 0);
  ASSERT_TRUE(set->isSet());
  ASSERT_TRUE(set->data()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(set->data())->length(), 0);
}

TEST(RuntimeSetTest, Add) {
  Runtime runtime;
  HandleScope scope;
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Object> value(&scope, SmallInt::fromWord(12345));

  // Store a value
  runtime.setAdd(set, value);
  EXPECT_EQ(set->numItems(), 1);

  // Retrieve the stored value
  ASSERT_TRUE(runtime.setIncludes(set, value));

  // Add a new value
  Handle<Object> new_value(&scope, SmallInt::fromWord(5555));
  runtime.setAdd(set, new_value);
  EXPECT_EQ(set->numItems(), 2);

  // Get the new value
  ASSERT_TRUE(runtime.setIncludes(set, new_value));

  // Add a existing value
  Handle<Object> same_value(&scope, SmallInt::fromWord(12345));
  RawObject old_value = runtime.setAdd(set, same_value);
  EXPECT_EQ(set->numItems(), 2);
  EXPECT_EQ(old_value, *value);
}

TEST(RuntimeSetTest, Remove) {
  Runtime runtime;
  HandleScope scope;
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Object> value(&scope, SmallInt::fromWord(12345));

  // Removing a key that doesn't exist should fail
  EXPECT_FALSE(runtime.setRemove(set, value));

  runtime.setAdd(set, value);
  EXPECT_EQ(set->numItems(), 1);

  ASSERT_TRUE(runtime.setRemove(set, value));
  EXPECT_EQ(set->numItems(), 0);

  // Looking up a key that was deleted should fail
  ASSERT_FALSE(runtime.setIncludes(set, value));
}

TEST(RuntimeSetTest, Grow) {
  Runtime runtime;
  HandleScope scope;
  Handle<Set> set(&scope, runtime.newSet());

  // Fill up the dict - we insert an initial key to force the allocation of the
  // backing ObjectArray.
  Handle<Object> init_key(&scope, SmallInt::fromWord(0));
  runtime.setAdd(set, init_key);
  ASSERT_TRUE(set->data()->isObjectArray());
  word init_data_size = ObjectArray::cast(set->data())->length();

  auto make_key = [&runtime](int i) {
    byte text[]{"0123456789abcdeghiklmn"};
    return runtime.newStrWithAll(View<byte>(text + i % 10, 10));
  };

  // Fill in one fewer keys than would require growing the underlying object
  // array again
  word num_keys = Runtime::kInitialSetCapacity;
  for (int i = 1; i < num_keys; i++) {
    Handle<Object> key(&scope, make_key(i));
    runtime.setAdd(set, key);
  }

  // Add another key which should force us to double the capacity
  Handle<Object> straw(&scope, make_key(num_keys));
  runtime.setAdd(set, straw);
  ASSERT_TRUE(set->data()->isObjectArray());
  word new_data_size = ObjectArray::cast(set->data())->length();
  EXPECT_EQ(new_data_size, Runtime::kSetGrowthFactor * init_data_size);

  // Make sure we can still read all the stored keys
  for (int i = 1; i <= num_keys; i++) {
    Handle<Object> key(&scope, make_key(i));
    bool found = runtime.setIncludes(set, key);
    ASSERT_TRUE(found);
  }
}

TEST(RuntimeSetTest, UpdateSet) {
  Runtime runtime;
  HandleScope scope;
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Set> set1(&scope, runtime.newSet());
  Handle<Object> set1_handle(&scope, *set1);
  for (word i = 0; i < 8; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set, value);
  }
  runtime.setUpdate(Thread::currentThread(), set, set1_handle);
  ASSERT_EQ(set->numItems(), 8);
  for (word i = 4; i < 12; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set1, value);
  }
  runtime.setUpdate(Thread::currentThread(), set, set1_handle);
  ASSERT_EQ(set->numItems(), 12);
  runtime.setUpdate(Thread::currentThread(), set, set1_handle);
  ASSERT_EQ(set->numItems(), 12);
}

TEST(RuntimeSetTest, UpdateList) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  Handle<Set> set(&scope, runtime.newSet());
  for (word i = 0; i < 8; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(list, value);
  }
  for (word i = 4; i < 12; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set, value);
  }
  ASSERT_EQ(set->numItems(), 8);
  Handle<Object> list_handle(&scope, *list);
  runtime.setUpdate(Thread::currentThread(), set, list_handle);
  ASSERT_EQ(set->numItems(), 12);
  runtime.setUpdate(Thread::currentThread(), set, list_handle);
  ASSERT_EQ(set->numItems(), 12);
}

TEST(RuntimeSetTest, UpdateListIterator) {
  Runtime runtime;
  HandleScope scope;
  Handle<List> list(&scope, runtime.newList());
  Handle<Set> set(&scope, runtime.newSet());
  for (word i = 0; i < 8; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    runtime.listAdd(list, value);
  }
  for (word i = 4; i < 12; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set, value);
  }
  ASSERT_EQ(set->numItems(), 8);
  Handle<Object> list_handle(&scope, *list);
  Handle<Object> list_iterator(&scope, runtime.newListIterator(list_handle));
  runtime.setUpdate(Thread::currentThread(), set, list_iterator);
  ASSERT_EQ(set->numItems(), 12);
}

TEST(RuntimeSetTest, UpdateObjectArray) {
  Runtime runtime;
  HandleScope scope;
  Handle<ObjectArray> object_array(&scope, runtime.newObjectArray(8));
  Handle<Set> set(&scope, runtime.newSet());
  for (word i = 0; i < 8; i++) {
    object_array->atPut(i, SmallInt::fromWord(i));
  }
  for (word i = 4; i < 12; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set, value);
  }
  ASSERT_EQ(set->numItems(), 8);
  Handle<Object> object_array_handle(&scope, *object_array);
  runtime.setUpdate(Thread::currentThread(), set, object_array_handle);
  ASSERT_EQ(set->numItems(), 12);
}

TEST(RuntimeSetTest, UpdateIterator) {
  Runtime runtime;
  HandleScope scope;
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Object> iterator(&scope, iterableWithLengthHint(&runtime));
  runtime.setUpdate(Thread::currentThread(), set, iterator);

  ASSERT_EQ(set->numItems(), 3);
}

TEST(RuntimeSetTest, UpdateIteratorWithoutDunderLengthHint) {
  Runtime runtime;
  HandleScope scope;
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Object> iterator(&scope, iterableWithoutLengthHint(&runtime));
  runtime.setUpdate(Thread::currentThread(), set, iterator);

  // An iterator with no __length_hint__ should not be consumed
  ASSERT_EQ(set->numItems(), 0);
}

TEST(RuntimeSetTest, UpdateWithNonIterable) {
  Runtime runtime;
  HandleScope scope;
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Object> non_iterable(&scope, NoneType::object());
  Handle<Object> result(
      &scope, runtime.setUpdate(Thread::currentThread(), set, non_iterable));
  ASSERT_TRUE(result->isError());
}

TEST(RuntimeSetTest, EmptySetItersectionReturnsEmptySet) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Set> set1(&scope, runtime.newSet());

  // set() & set()
  Handle<Object> result(&scope, runtime.setIntersection(thread, set, set1));
  ASSERT_TRUE(result->isSet());
  EXPECT_EQ(Set::cast(*result)->numItems(), 0);
}

TEST(RuntimeSetTest, ItersectionWithEmptySetReturnsEmptySet) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Set> set1(&scope, runtime.newSet());

  for (word i = 0; i < 8; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set1, value);
  }

  // set() & {0, 1, 2, 3, 4, 5, 6, 7}
  Handle<Object> result(&scope, runtime.setIntersection(thread, set, set1));
  ASSERT_TRUE(result->isSet());
  EXPECT_EQ(Set::cast(*result)->numItems(), 0);

  // {0, 1, 2, 3, 4, 5, 6, 7} & set()
  Handle<Object> result1(&scope, runtime.setIntersection(thread, set1, set));
  ASSERT_TRUE(result1->isSet());
  EXPECT_EQ(Set::cast(*result1)->numItems(), 0);
}

TEST(RuntimeSetTest, IntersectionReturnsSetWithCommonElements) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Set> set1(&scope, runtime.newSet());
  Handle<Object> key(&scope, NoneType::object());

  for (word i = 0; i < 8; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set1, value);
  }

  for (word i = 0; i < 4; i++) {
    Handle<Object> value(&scope, SmallInt::fromWord(i));
    runtime.setAdd(set, value);
  }

  // {0, 1, 2, 3} & {0, 1, 2, 3, 4, 5, 6, 7}
  Handle<Set> result(&scope, runtime.setIntersection(thread, set, set1));
  EXPECT_EQ(Set::cast(*result)->numItems(), 4);
  key = SmallInt::fromWord(0);
  EXPECT_TRUE(runtime.setIncludes(result, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(result, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(runtime.setIncludes(result, key));
  key = SmallInt::fromWord(3);
  EXPECT_TRUE(runtime.setIncludes(result, key));

  // {0, 1, 2, 3, 4, 5, 6, 7} & {0, 1, 2, 3}
  Handle<Set> result1(&scope, runtime.setIntersection(thread, set, set1));
  EXPECT_EQ(Set::cast(*result1)->numItems(), 4);
  key = SmallInt::fromWord(0);
  EXPECT_TRUE(runtime.setIncludes(result1, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(result1, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(runtime.setIncludes(result1, key));
  key = SmallInt::fromWord(3);
  EXPECT_TRUE(runtime.setIncludes(result1, key));
}

TEST(RuntimeSetTest, IntersectIterator) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Object> iterator(&scope, iterableWithLengthHint(&runtime));
  Handle<Set> result(&scope, runtime.setIntersection(thread, set, iterator));
  EXPECT_EQ(result->numItems(), 0);

  Handle<Object> key(&scope, SmallInt::fromWord(1));
  runtime.setAdd(set, key);
  key = SmallInt::fromWord(2);
  runtime.setAdd(set, key);
  Handle<Object> iterator1(&scope, iterableWithLengthHint(&runtime));
  Handle<Set> result1(&scope, runtime.setIntersection(thread, set, iterator1));
  EXPECT_EQ(result1->numItems(), 2);
  EXPECT_TRUE(runtime.setIncludes(result1, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(result1, key));
}

TEST(RuntimeSetTest, IntersectIteratorWithoutDunderLengthHint) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Object> key(&scope, SmallInt::fromWord(0));
  runtime.setAdd(set, key);
  key = SmallInt::fromWord(1);
  runtime.setAdd(set, key);
  Handle<Object> iterator(&scope, iterableWithoutLengthHint(&runtime));
  Handle<Set> result(&scope, runtime.setIntersection(thread, set, iterator));

  // An iterator with no __length_hint__ should not be consumed
  ASSERT_EQ(result->numItems(), 0);
}

TEST(RuntimeSetTest, IntersectWithNonIterable) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Object> non_iterable(&scope, NoneType::object());

  Handle<Object> result(&scope,
                        runtime.setIntersection(thread, set, non_iterable));
  ASSERT_TRUE(result->isError());
}

TEST(RuntimeSetTest, SetCopy) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Object> set_copy(&scope, runtime.setCopy(set));
  ASSERT_TRUE(set_copy->isSet());
  EXPECT_EQ(Set::cast(*set_copy)->numItems(), 0);

  Handle<Object> key(&scope, SmallInt::fromWord(0));
  runtime.setAdd(set, key);
  key = SmallInt::fromWord(1);
  runtime.setAdd(set, key);
  key = SmallInt::fromWord(2);
  runtime.setAdd(set, key);

  Handle<Object> set_copy1(&scope, runtime.setCopy(set));
  ASSERT_TRUE(set_copy1->isSet());
  EXPECT_EQ(Set::cast(*set_copy1)->numItems(), 3);
  set = *set_copy1;
  key = SmallInt::fromWord(0);
  EXPECT_TRUE(runtime.setIncludes(set, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(set, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

// Attribute tests

static RawObject createClass(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> klass(&scope, runtime->newClass());
  Thread* thread = Thread::currentThread();
  Handle<Layout> layout(&scope, runtime->layoutCreateEmpty(thread));
  layout->setDescribedClass(*klass);
  klass->setInstanceLayout(*layout);
  Handle<ObjectArray> mro(&scope, runtime->newObjectArray(1));
  mro->atPut(0, *klass);
  klass->setMro(*mro);
  layout->setId(runtime->reserveLayoutId());
  runtime->layoutAtPut(layout->id(), *layout);
  return *klass;
}

static void setInClassDict(Runtime* runtime, const Handle<Object>& klass,
                           const Handle<Object>& attr,
                           const Handle<Object>& value) {
  HandleScope scope;
  Handle<Type> k(&scope, *klass);
  Handle<Dict> klass_dict(&scope, k->dict());
  runtime->dictAtPutInValueCell(klass_dict, attr, value);
}

static void setInMetaclass(Runtime* runtime, const Handle<Object>& klass,
                           const Handle<Object>& attr,
                           const Handle<Object>& value) {
  HandleScope scope;
  Handle<Object> meta_klass(&scope, runtime->typeOf(*klass));
  setInClassDict(runtime, meta_klass, attr, value);
}

// Get an attribute that corresponds to a function on the metaclass
TEST(ClassGetAttrTest, MetaClassFunction) {
  Runtime runtime;
  HandleScope scope;
  Handle<Object> klass(&scope, createClass(&runtime));

  // Store the function on the metaclass
  Handle<Object> attr(&scope, runtime.newStrFromCStr("test"));
  Handle<Object> value(&scope, runtime.newFunction());
  setInMetaclass(&runtime, klass, attr, value);

  // Fetch it from the class and ensure the bound method was created
  RawObject result = runtime.attributeAt(Thread::currentThread(), klass, attr);
  ASSERT_TRUE(result->isBoundMethod());
  Handle<BoundMethod> bm(&scope, result);
  EXPECT_TRUE(Object::equals(bm->function(), *value));
  EXPECT_TRUE(Object::equals(bm->self(), *klass));
}

// Get an attribute that resides on the metaclass
TEST(ClassGetAttrTest, MetaClassAttr) {
  Runtime runtime;
  HandleScope scope;
  Handle<Object> klass(&scope, createClass(&runtime));

  // Store the attribute on the metaclass
  Handle<Object> attr(&scope, runtime.newStrFromCStr("test"));
  Handle<Object> value(&scope, SmallInt::fromWord(100));
  setInMetaclass(&runtime, klass, attr, value);

  // Fetch it from the class
  RawObject result = runtime.attributeAt(Thread::currentThread(), klass, attr);
  EXPECT_TRUE(Object::equals(result, *value));
}

// Get an attribute that resides on the class and shadows an attribute on
// the metaclass
TEST(ClassGetAttrTest, ShadowingAttr) {
  Runtime runtime;
  HandleScope scope;
  Handle<Object> klass(&scope, createClass(&runtime));

  // Store the attribute on the metaclass
  Handle<Object> attr(&scope, runtime.newStrFromCStr("test"));
  Handle<Object> meta_klass_value(&scope, SmallInt::fromWord(100));
  setInMetaclass(&runtime, klass, attr, meta_klass_value);

  // Store the attribute on the class so that it shadows the attr
  // on the metaclass
  Handle<Object> klass_value(&scope, SmallInt::fromWord(200));
  setInClassDict(&runtime, klass, attr, klass_value);

  // Fetch it from the class
  RawObject result = runtime.attributeAt(Thread::currentThread(), klass, attr);
  EXPECT_TRUE(Object::equals(result, *klass_value));
}

struct IntrinsicClassSetAttrTestData {
  LayoutId layout_id;
  const char* name;
};

IntrinsicClassSetAttrTestData kIntrinsicClassSetAttrTests[] = {
// clang-format off
#define DEFINE_TEST(class_name) {LayoutId::k##class_name, #class_name},
  INTRINSIC_CLASS_NAMES(DEFINE_TEST)
#undef DEFINE_TEST
    // clang-format on
};

static std::string intrinsicClassName(
    ::testing::TestParamInfo<IntrinsicClassSetAttrTestData> info) {
  return info.param.name;
}

class IntrinsicClassSetAttrTest
    : public ::testing::TestWithParam<IntrinsicClassSetAttrTestData> {};

TEST_P(IntrinsicClassSetAttrTest, SetAttr) {
  Runtime runtime;
  HandleScope scope;
  Handle<Object> klass(&scope, runtime.typeAt(GetParam().layout_id));
  Handle<Object> attr(&scope, runtime.newStrFromCStr("test"));
  Handle<Object> value(&scope, SmallInt::fromWord(100));
  Thread* thread = Thread::currentThread();

  RawObject result = runtime.attributeAtPut(thread, klass, attr, value);

  EXPECT_TRUE(result->isError());
  ASSERT_TRUE(thread->exceptionValue()->isStr());
  EXPECT_PYSTRING_EQ(Str::cast(thread->exceptionValue()),
                     "can't set attributes of built-in/extension type");
}

INSTANTIATE_TEST_CASE_P(IntrinsicClasses, IntrinsicClassSetAttrTest,
                        ::testing::ValuesIn(kIntrinsicClassSetAttrTests),
                        intrinsicClassName);

// Set an attribute directly on the class
TEST(ClassAttributeTest, SetAttrOnClass) {
  Runtime runtime;
  HandleScope scope;

  Handle<Object> klass(&scope, createClass(&runtime));
  Handle<Object> attr(&scope, runtime.newStrFromCStr("test"));
  Handle<Object> value(&scope, SmallInt::fromWord(100));

  RawObject result =
      runtime.attributeAtPut(Thread::currentThread(), klass, attr, value);
  ASSERT_FALSE(result->isError());

  Handle<Dict> klass_dict(&scope, Type::cast(*klass)->dict());
  Handle<Object> value_cell(&scope, runtime.dictAt(klass_dict, attr));
  ASSERT_TRUE(value_cell->isValueCell());
  EXPECT_EQ(ValueCell::cast(*value_cell)->value(), SmallInt::fromWord(100));
}

TEST(ClassAttributeTest, Simple) {
  Runtime runtime;
  const char* src = R"(
class A:
  foo = 'hello'
print(A.foo)
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello\n");
}

TEST(ClassAttributeTest, SingleInheritance) {
  Runtime runtime;
  const char* src = R"(
class A:
  foo = 'hello'
class B(A): pass
class C(B): pass
print(A.foo, B.foo, C.foo)
B.foo = 123
print(A.foo, B.foo, C.foo)
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello hello hello\nhello 123 123\n");
}

TEST(ClassAttributeTest, MultipleInheritance) {
  Runtime runtime;
  const char* src = R"(
class A:
  foo = 'hello'
class B:
  bar = 'there'
class C(B, A): pass
print(C.foo, C.bar)
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello there\n");
}

TEST(ClassAttributeDeathTest, GetMissingAttribute) {
  Runtime runtime;
  const char* src = R"(
class A: pass
print(A.foo)
)";
  ASSERT_DEATH(runtime.runFromCStr(src),
               "aborting due to pending exception: missing attribute");
}

TEST(ClassAttributeTest, GetFunction) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  def bar(self):
    print(self)
Foo.bar('testing 123')
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "testing 123\n");
}

TEST(ClassAttributeDeathTest, GetDataDescriptorOnMetaClass) {
  Runtime runtime;

  // Create the data descriptor class
  const char* src = R"(
class DataDescriptor:
  def __set__(self, instance, value):
    pass

  def __get__(self, instance, owner):
    pass
)";
  runtime.runFromCStr(src);
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> descr_klass(&scope, moduleAt(&runtime, main, "DataDescriptor"));

  // Create the class
  Handle<Object> klass(&scope, createClass(&runtime));

  // Create an instance of the descriptor and store it on the metaclass
  Handle<Object> attr(&scope, runtime.newStrFromCStr("test"));
  Handle<Layout> layout(&scope, descr_klass->instanceLayout());
  Handle<Object> descr(&scope, runtime.newInstance(layout));
  setInMetaclass(&runtime, klass, attr, descr);

  ASSERT_DEATH(runtime.attributeAt(Thread::currentThread(), klass, attr),
               "custom descriptors are unsupported");
}

TEST(ClassAttributeTest, GetNonDataDescriptorOnMetaClass) {
  Runtime runtime;

  // Create the non-data descriptor class
  const char* src = R"(
class DataDescriptor:
  def __get__(self, instance, owner):
    return (self, instance, owner)
)";
  runtime.runFromCStr(src);
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> descr_klass(&scope, moduleAt(&runtime, main, "DataDescriptor"));

  // Create the class
  Handle<Object> klass(&scope, createClass(&runtime));

  // Create an instance of the descriptor and store it on the metaclass
  Handle<Object> attr(&scope, runtime.newStrFromCStr("test"));
  Handle<Layout> layout(&scope, descr_klass->instanceLayout());
  Handle<Object> descr(&scope, runtime.newInstance(layout));
  setInMetaclass(&runtime, klass, attr, descr);

  RawObject result = runtime.attributeAt(Thread::currentThread(), klass, attr);
  ASSERT_EQ(ObjectArray::cast(result)->length(), 3);
  EXPECT_EQ(runtime.typeOf(ObjectArray::cast(result)->at(0)), *descr_klass);
  EXPECT_EQ(ObjectArray::cast(result)->at(1), *klass);
  EXPECT_EQ(ObjectArray::cast(result)->at(2), runtime.typeOf(*klass));
}

TEST(ClassAttributeTest, GetNonDataDescriptorOnClass) {
  Runtime runtime;

  // Create the non-data descriptor class
  const char* src = R"(
class DataDescriptor:
  def __get__(self, instance, owner):
    return (self, instance, owner)
)";
  runtime.runFromCStr(src);
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> descr_klass(&scope, moduleAt(&runtime, main, "DataDescriptor"));

  // Create the class
  Handle<Object> klass(&scope, createClass(&runtime));

  // Create an instance of the descriptor and store it on the metaclass
  Handle<Object> attr(&scope, runtime.newStrFromCStr("test"));
  Handle<Layout> layout(&scope, descr_klass->instanceLayout());
  Handle<Object> descr(&scope, runtime.newInstance(layout));
  setInClassDict(&runtime, klass, attr, descr);

  RawObject result = runtime.attributeAt(Thread::currentThread(), klass, attr);
  ASSERT_EQ(ObjectArray::cast(result)->length(), 3);
  EXPECT_EQ(runtime.typeOf(ObjectArray::cast(result)->at(0)), *descr_klass);
  EXPECT_EQ(ObjectArray::cast(result)->at(1), NoneType::object());
  EXPECT_EQ(ObjectArray::cast(result)->at(2), *klass);
}

TEST(GetClassAttributeTest, GetMetaclassAttribute) {
  Runtime runtime;
  const char* src = R"(
class MyMeta(type):
    attr = 'foo'

class Foo(metaclass=MyMeta):
    pass
)";
  runtime.runFromCStr(src);
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> foo(&scope, moduleAt(&runtime, main, "Foo"));
  Handle<Object> attr(&scope, runtime.newStrFromCStr("attr"));
  Handle<Object> result(
      &scope, runtime.attributeAt(Thread::currentThread(), foo, attr));
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(Str::cast(*result), "foo");
}

// Fetch an unknown attribute
TEST(InstanceAttributeDeathTest, GetMissing) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  pass

def test(x):
  print(x.foo)
)";
  runtime.runFromCStr(src);
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Function> test(&scope, moduleAt(&runtime, main, "test"));
  Handle<Type> klass(&scope, moduleAt(&runtime, main, "Foo"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  Handle<Layout> layout(&scope, klass->instanceLayout());
  args->atPut(0, runtime.newInstance(layout));

  ASSERT_DEATH(callFunctionToString(test, args), "missing attribute");
}

// Fetch an attribute defined on the class
TEST(InstanceAttributeTest, GetClassAttribute) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  attr = 'testing 123'

def test(x):
  print(x.attr)
)";
  runtime.runFromCStr(src);

  // Create the instance
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Function> test(&scope, moduleAt(&runtime, main, "test"));
  Handle<Type> klass(&scope, moduleAt(&runtime, main, "Foo"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  Handle<Layout> layout(&scope, klass->instanceLayout());
  args->atPut(0, runtime.newInstance(layout));

  EXPECT_EQ(callFunctionToString(test, args), "testing 123\n");
}

// Fetch an attribute defined in __init__
TEST(InstanceAttributeTest, GetInstanceAttribute) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  def __init__(self):
    self.attr = 'testing 123'

def test(x):
  Foo.__init__(x)
  print(x.attr)
)";
  runtime.runFromCStr(src);

  // Create the instance
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> klass(&scope, moduleAt(&runtime, main, "Foo"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  Handle<Layout> layout(&scope, klass->instanceLayout());
  args->atPut(0, runtime.newInstance(layout));

  // Run __init__
  Handle<Function> test(&scope, moduleAt(&runtime, main, "test"));
  EXPECT_EQ(callFunctionToString(test, args), "testing 123\n");
}

// Set an attribute defined in __init__
TEST(InstanceAttributeTest, SetInstanceAttribute) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  def __init__(self):
    self.attr = 'testing 123'

def test(x):
  Foo.__init__(x)
  print(x.attr)
  x.attr = '321 testing'
  print(x.attr)
)";
  runtime.runFromCStr(src);

  // Create the instance
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> klass(&scope, moduleAt(&runtime, main, "Foo"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  Handle<Layout> layout(&scope, klass->instanceLayout());
  args->atPut(0, runtime.newInstance(layout));

  // Run __init__ then RMW the attribute
  Handle<Function> test(&scope, moduleAt(&runtime, main, "test"));
  EXPECT_EQ(callFunctionToString(test, args), "testing 123\n321 testing\n");
}

TEST(InstanceAttributeTest, AddOverflowAttributes) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  pass

def test(x):
  x.foo = 100
  x.bar = 200
  x.baz = 'hello'
  print(x.foo, x.bar, x.baz)

  x.foo = 'aaa'
  x.bar = 'bbb'
  x.baz = 'ccc'
  print(x.foo, x.bar, x.baz)
)";
  runtime.runFromCStr(src);

  // Create an instance of Foo
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> klass(&scope, moduleAt(&runtime, main, "Foo"));
  Handle<Layout> layout(&scope, klass->instanceLayout());
  Handle<Instance> foo1(&scope, runtime.newInstance(layout));
  LayoutId original_layout_id = layout->id();

  // Add overflow attributes that should force layout transitions
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  args->atPut(0, *foo1);
  Handle<Function> test(&scope, moduleAt(&runtime, main, "test"));
  EXPECT_EQ(callFunctionToString(test, args), "100 200 hello\naaa bbb ccc\n");
  EXPECT_NE(foo1->layoutId(), original_layout_id);

  // Add the same set of attributes to a new instance, should arrive at the
  // same layout
  Handle<Instance> foo2(&scope, runtime.newInstance(layout));
  args->atPut(0, *foo2);
  EXPECT_EQ(callFunctionToString(test, args), "100 200 hello\naaa bbb ccc\n");
}

// This is the real deal
TEST(InstanceAttributeTest, CallInstanceMethod) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  def __init__(self):
    self.attr = 'testing 123'

  def doit(self):
    print(self.attr)
    self.attr = '321 testing'
    print(self.attr)

def test(x):
  Foo.__init__(x)
  x.doit()
)";
  runtime.runFromCStr(src);

  // Create the instance
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> klass(&scope, moduleAt(&runtime, main, "Foo"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  Handle<Layout> layout(&scope, klass->instanceLayout());
  args->atPut(0, runtime.newInstance(layout));

  // Run __init__ then call the method
  Handle<Function> test(&scope, moduleAt(&runtime, main, "test"));
  EXPECT_EQ(callFunctionToString(test, args), "testing 123\n321 testing\n");
}

TEST(InstanceAttributeTest, GetDataDescriptor) {
  Runtime runtime;
  const char* src = R"(
class DataDescr:
  def __set__(self, instance, value):
    pass

  def __get__(self, instance, owner):
    return (self, instance, owner)

class Foo:
  pass
)";
  runtime.runFromCStr(src);

  // Create an instance of the descriptor and store it on the class
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> descr_klass(&scope, moduleAt(&runtime, main, "DataDescr"));
  Handle<Object> klass(&scope, moduleAt(&runtime, main, "Foo"));
  Handle<Object> attr(&scope, runtime.newStrFromCStr("attr"));
  Handle<Layout> descr_layout(&scope, descr_klass->instanceLayout());
  Handle<Object> descr(&scope, runtime.newInstance(descr_layout));
  setInClassDict(&runtime, klass, attr, descr);

  // Fetch it from the instance
  Handle<Layout> instance_layout(&scope, Type::cast(*klass)->instanceLayout());
  Handle<Object> instance(&scope, runtime.newInstance(instance_layout));
  Handle<ObjectArray> result(
      &scope, runtime.attributeAt(Thread::currentThread(), instance, attr));
  ASSERT_EQ(result->length(), 3);
  EXPECT_EQ(runtime.typeOf(result->at(0)), *descr_klass);
  EXPECT_EQ(result->at(1), *instance);
  EXPECT_EQ(result->at(2), *klass);
}

TEST(InstanceAttributeTest, GetNonDataDescriptor) {
  Runtime runtime;
  const char* src = R"(
class Descr:
  def __get__(self, instance, owner):
    return (self, instance, owner)

class Foo:
  pass
)";
  runtime.runFromCStr(src);

  // Create an instance of the descriptor and store it on the class
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> descr_klass(&scope, moduleAt(&runtime, main, "Descr"));
  Handle<Object> klass(&scope, moduleAt(&runtime, main, "Foo"));
  Handle<Object> attr(&scope, runtime.newStrFromCStr("attr"));
  Handle<Layout> descr_layout(&scope, descr_klass->instanceLayout());
  Handle<Object> descr(&scope, runtime.newInstance(descr_layout));
  setInClassDict(&runtime, klass, attr, descr);

  // Fetch it from the instance
  Handle<Layout> instance_layout(&scope, Type::cast(*klass)->instanceLayout());
  Handle<Object> instance(&scope, runtime.newInstance(instance_layout));

  RawObject result =
      runtime.attributeAt(Thread::currentThread(), instance, attr);
  ASSERT_EQ(ObjectArray::cast(result)->length(), 3);
  EXPECT_EQ(runtime.typeOf(ObjectArray::cast(result)->at(0)), *descr_klass);
  EXPECT_EQ(ObjectArray::cast(result)->at(1), *instance);
  EXPECT_EQ(ObjectArray::cast(result)->at(2), *klass);
}

TEST(InstanceAttributeTest, ManipulateMultipleAttributes) {
  Runtime runtime;
  const char* src = R"(
class Foo:
  def __init__(self):
    self.foo = 'foo'
    self.bar = 'bar'
    self.baz = 'baz'

def test(x):
  Foo.__init__(x)
  print(x.foo, x.bar, x.baz)
  x.foo = 'aaa'
  x.bar = 'bbb'
  x.baz = 'ccc'
  print(x.foo, x.bar, x.baz)
)";
  runtime.runFromCStr(src);

  // Create the instance
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> klass(&scope, moduleAt(&runtime, main, "Foo"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  Handle<Layout> layout(&scope, klass->instanceLayout());
  args->atPut(0, runtime.newInstance(layout));

  // Run the test
  Handle<Function> test(&scope, moduleAt(&runtime, main, "test"));
  EXPECT_EQ(callFunctionToString(test, args), "foo bar baz\naaa bbb ccc\n");
}

TEST(InstanceAttributeDeathTest, FetchConditionalInstanceAttribute) {
  Runtime runtime;
  const char* src = R"(
def false():
  return False

class Foo:
  def __init__(self):
    self.foo = 'foo'
    if false():
      self.bar = 'bar'

foo = Foo()
print(foo.bar)
)";
  ASSERT_DEATH(runtime.runFromCStr(src),
               "aborting due to pending exception: missing attribute");
}

TEST(InstanceAttributeTest, DunderClass) {
  Runtime runtime;
  const char* src = R"(
class Foo: pass
class Bar(Foo): pass
class Hello(Bar, list): pass
print(list().__class__ is list)
print(Foo().__class__ is Foo)
print(Bar().__class__ is Bar)
print(Hello().__class__ is Hello)
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "True\nTrue\nTrue\nTrue\n");
}

TEST(InstanceAttributeTest, DunderNew) {
  Runtime runtime;
  const char* src = R"(
class Foo:
    def __new__(self):
        print("New")
    def __init__(self):
        print("Init")
a = Foo()
)";
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "New\nInit\n");
}

TEST(InstanceAttributeTest, NoInstanceDictReturnsClassAttribute) {
  Runtime runtime;
  HandleScope scope;
  Handle<Object> immediate(&scope, SmallInt::fromWord(-1));
  Handle<Object> name(&scope, runtime.symbols()->DunderNeg());
  RawObject attr =
      runtime.attributeAt(Thread::currentThread(), immediate, name);
  ASSERT_TRUE(attr->isBoundMethod());
}

TEST(InstanceAttributeDeletionTest, DeleteKnownAttribute) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
class Foo:
    def __init__(self):
      self.foo = 'foo'
      self.bar = 'bar'

def test():
    foo = Foo()
    del foo.bar
)";
  compileAndRunToString(&runtime, src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Function> test(&scope, moduleAt(&runtime, main, "test"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(0));
  Handle<Object> result(&scope, callFunction(test, args));
  EXPECT_EQ(*result, NoneType::object());
}

TEST(InstanceAttributeDeletionTest, DeleteDescriptor) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
result = None

class DeleteDescriptor:
    def __delete__(self, instance):
        global result
        result = self, instance
descr = DeleteDescriptor()

class Foo:
    bar = descr

foo = Foo()
del foo.bar
)";
  compileAndRunToString(&runtime, src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> data(&scope, moduleAt(&runtime, main, "result"));
  ASSERT_TRUE(data->isObjectArray());

  Handle<ObjectArray> result(&scope, *data);
  ASSERT_EQ(result->length(), 2);

  Handle<Object> descr(&scope, moduleAt(&runtime, main, "descr"));
  EXPECT_EQ(result->at(0), *descr);

  Handle<Object> foo(&scope, moduleAt(&runtime, main, "foo"));
  EXPECT_EQ(result->at(1), *foo);
}

TEST(InstanceAttributeDeletionDeathTest, DeleteUnknownAttribute) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
class Foo:
    pass

foo = Foo()
del foo.bar
)";
  EXPECT_DEATH(compileAndRunToString(&runtime, src), "missing attribute");
}

TEST(InstanceAttributeDeletionTest, DeleteAttributeWithDunderDelattr) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
result = None

class Foo:
    def __delattr__(self, name):
        global result
        result = self, name

foo = Foo()
del foo.bar
)";
  compileAndRunToString(&runtime, src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> data(&scope, moduleAt(&runtime, main, "result"));
  ASSERT_TRUE(data->isObjectArray());

  Handle<ObjectArray> result(&scope, *data);
  ASSERT_EQ(result->length(), 2);

  Handle<Object> foo(&scope, moduleAt(&runtime, main, "foo"));
  EXPECT_EQ(result->at(0), *foo);
  ASSERT_TRUE(result->at(1)->isStr());
  EXPECT_PYSTRING_EQ(Str::cast(result->at(1)), "bar");
}

TEST(InstanceAttributeDeletionTest,
     DeleteAttributeWithDunderDelattrOnSuperclass) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
result = None

class Foo:
    def __delattr__(self, name):
        global result
        result = self, name

class Bar(Foo):
    pass

bar = Bar()
del bar.baz
)";
  compileAndRunToString(&runtime, src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> data(&scope, moduleAt(&runtime, main, "result"));
  ASSERT_TRUE(data->isObjectArray());

  Handle<ObjectArray> result(&scope, *data);
  ASSERT_EQ(result->length(), 2);

  Handle<Object> bar(&scope, moduleAt(&runtime, main, "bar"));
  EXPECT_EQ(result->at(0), *bar);
  ASSERT_TRUE(result->at(1)->isStr());
  EXPECT_PYSTRING_EQ(Str::cast(result->at(1)), "baz");
}

TEST(ClassAttributeDeletionTest, DeleteKnownAttribute) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
class Foo:
    foo = 'foo'
    bar = 'bar'

def test():
    del Foo.bar
)";
  compileAndRunToString(&runtime, src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Function> test(&scope, moduleAt(&runtime, main, "test"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(0));
  Handle<Object> result(&scope, callFunction(test, args));
  EXPECT_EQ(*result, NoneType::object());
}

TEST(ClassAttributeDeletionTest, DeleteDescriptorOnMetaclass) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
args = None

class DeleteDescriptor:
    def __delete__(self, instance):
        global args
        args = (self, instance)

descr = DeleteDescriptor()

class FooMeta(type):
    attr = descr

class Foo(metaclass=FooMeta):
    pass

del Foo.attr
)";
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> data(&scope, moduleAt(&runtime, main, "args"));
  ASSERT_TRUE(data->isObjectArray());

  Handle<ObjectArray> args(&scope, *data);
  ASSERT_EQ(args->length(), 2);

  Handle<Object> descr(&scope, moduleAt(&runtime, main, "descr"));
  EXPECT_EQ(args->at(0), *descr);

  Handle<Object> foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_EQ(args->at(1), *foo);
}

TEST(ClassAttributeDeletionDeathTest, DeleteUnknownAttribute) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
class Foo:
    pass

del Foo.bar
)";
  EXPECT_DEATH(compileAndRunToString(&runtime, src), "missing attribute");
}

TEST(ClassAttributeDeletionTest, DeleteAttributeWithDunderDelattrOnMetaclass) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
args = None

class FooMeta(type):
    def __delattr__(self, name):
        global args
        args = self, name

class Foo(metaclass=FooMeta):
    pass

del Foo.bar
)";
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> data(&scope, moduleAt(&runtime, main, "args"));
  ASSERT_TRUE(data->isObjectArray());

  Handle<ObjectArray> args(&scope, *data);
  ASSERT_EQ(args->length(), 2);

  Handle<Object> foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_EQ(args->at(0), *foo);

  Handle<Object> attr(&scope, runtime.internStrFromCStr("bar"));
  EXPECT_EQ(args->at(1), *attr);
}

TEST(ModuleAttributeDeletionDeathTest, DeleteUnknownAttribute) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
def test(module):
    del module.foo
)";
  compileAndRunToString(&runtime, src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Function> test(&scope, moduleAt(&runtime, main, "test"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  args->atPut(0, *main);
  EXPECT_DEATH(callFunction(test, args), "missing attribute");
}

TEST(ModuleAttributeDeletionTest, DeleteKnownAttribute) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
foo = 'testing 123'

def test(module):
    del module.foo
    return 123
)";
  compileAndRunToString(&runtime, src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Function> test(&scope, moduleAt(&runtime, main, "test"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  args->atPut(0, *main);
  EXPECT_EQ(callFunction(test, args), SmallInt::fromWord(123));

  Handle<Object> attr(&scope, runtime.newStrFromCStr("foo"));
  Handle<Object> module(&scope, *main);
  EXPECT_EQ(runtime.attributeAt(Thread::currentThread(), module, attr),
            Error::object());
}

TEST(RuntimeIntTest, NewSmallIntWithDigits) {
  Runtime runtime;
  HandleScope scope;

  Handle<Int> zero(&scope, runtime.newIntWithDigits(View<uword>(nullptr, 0)));
  ASSERT_TRUE(zero->isSmallInt());
  EXPECT_EQ(zero->asWord(), 0);

  uword digit = 1;
  RawObject one = runtime.newIntWithDigits(View<uword>(&digit, 1));
  ASSERT_TRUE(one->isSmallInt());
  EXPECT_EQ(SmallInt::cast(one)->value(), 1);

  digit = kMaxUword;
  RawObject negative_one = runtime.newIntWithDigits(View<uword>(&digit, 1));
  ASSERT_TRUE(negative_one->isSmallInt());
  EXPECT_EQ(SmallInt::cast(negative_one)->value(), -1);

  word min_small_int = SmallInt::kMaxValue;
  digit = static_cast<uword>(min_small_int);
  Handle<Int> min_smallint(&scope,
                           runtime.newIntWithDigits(View<uword>(&digit, 1)));
  ASSERT_TRUE(min_smallint->isSmallInt());
  EXPECT_EQ(min_smallint->asWord(), min_small_int);

  word max_small_int = SmallInt::kMaxValue;
  digit = static_cast<uword>(max_small_int);
  Handle<Int> max_smallint(&scope,
                           runtime.newIntWithDigits(View<uword>(&digit, 1)));
  ASSERT_TRUE(max_smallint->isSmallInt());
  EXPECT_EQ(max_smallint->asWord(), max_small_int);
}

TEST(RuntimeIntTest, NewLargeIntWithDigits) {
  Runtime runtime;
  HandleScope scope;

  word negative_large_int = SmallInt::kMinValue - 1;
  uword digit = static_cast<uword>(negative_large_int);
  Handle<Int> negative_largeint(
      &scope, runtime.newIntWithDigits(View<uword>(&digit, 1)));
  ASSERT_TRUE(negative_largeint->isLargeInt());
  EXPECT_EQ(negative_largeint->asWord(), negative_large_int);

  word positive_large_int = SmallInt::kMaxValue + 1;
  digit = static_cast<uword>(positive_large_int);
  Handle<Int> positive_largeint(
      &scope, runtime.newIntWithDigits(View<uword>(&digit, 1)));
  ASSERT_TRUE(positive_largeint->isLargeInt());
  EXPECT_EQ(positive_largeint->asWord(), positive_large_int);
}

TEST(InstanceDelTest, DeleteUnknownAttribute) {
  const char* src = R"(
class Foo:
    pass
)";
  Runtime runtime;
  compileAndRunToString(&runtime, src);

  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> klass(&scope, moduleAt(&runtime, main, "Foo"));
  Handle<Layout> layout(&scope, klass->instanceLayout());
  Handle<HeapObject> instance(&scope, runtime.newInstance(layout));
  Handle<Object> attr(&scope, runtime.newStrFromCStr("unknown"));
  EXPECT_EQ(runtime.instanceDel(Thread::currentThread(), instance, attr),
            Error::object());
}

TEST(InstanceDelTest, DeleteInObjectAttribute) {
  const char* src = R"(
class Foo:
    def __init__(self):
        self.bar = 'bar'
        self.baz = 'baz'

def new_foo():
    return Foo()
)";
  Runtime runtime;
  compileAndRunToString(&runtime, src);

  // Create an instance of Foo
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Function> new_foo(&scope, moduleAt(&runtime, main, "new_foo"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(0));
  Handle<HeapObject> instance(&scope, callFunction(new_foo, args));

  // Verify that 'bar' is an in-object property
  Handle<Layout> layout(&scope,
                        runtime.layoutAt(instance->header()->layoutId()));
  Handle<Object> attr(&scope, runtime.internStrFromCStr("bar"));
  AttributeInfo info;
  Thread* thread = Thread::currentThread();
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout, attr, &info));
  ASSERT_TRUE(info.isInObject());

  // After successful deletion, the instance should have a new layout and should
  // no longer reference the previous value
  EXPECT_EQ(runtime.instanceDel(thread, instance, attr), NoneType::object());
  Handle<Layout> new_layout(&scope,
                            runtime.layoutAt(instance->header()->layoutId()));
  EXPECT_NE(*layout, *new_layout);
  EXPECT_FALSE(runtime.layoutFindAttribute(thread, new_layout, attr, &info));
}

TEST(InstanceDelTest, DeleteOverflowAttribute) {
  const char* src = R"(
class Foo:
    pass

def new_foo():
    foo = Foo()
    foo.bar = 'bar'
    return foo
)";
  Runtime runtime;
  compileAndRunToString(&runtime, src);

  // Create an instance of Foo
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Function> new_foo(&scope, moduleAt(&runtime, main, "new_foo"));
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(0));
  Handle<HeapObject> instance(&scope, callFunction(new_foo, args));

  // Verify that 'bar' is an overflow property
  Handle<Layout> layout(&scope,
                        runtime.layoutAt(instance->header()->layoutId()));
  Handle<Object> attr(&scope, runtime.internStrFromCStr("bar"));
  AttributeInfo info;
  Thread* thread = Thread::currentThread();
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout, attr, &info));
  ASSERT_TRUE(info.isOverflow());

  // After successful deletion, the instance should have a new layout and should
  // no longer reference the previous value
  EXPECT_EQ(runtime.instanceDel(thread, instance, attr), NoneType::object());
  Handle<Layout> new_layout(&scope,
                            runtime.layoutAt(instance->header()->layoutId()));
  EXPECT_NE(*layout, *new_layout);
  EXPECT_FALSE(runtime.layoutFindAttribute(thread, new_layout, attr, &info));
}

TEST(MetaclassTest, ClassWithTypeMetaclassIsConcreteClass) {
  const char* src = R"(
# This is equivalent to `class Foo(type)`
class Foo(type, metaclass=type):
    pass

class Bar(Foo):
    pass
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));

  Handle<Object> foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_TRUE(foo->isType());

  Handle<Object> bar(&scope, moduleAt(&runtime, main, "Bar"));
  EXPECT_TRUE(bar->isType());
}

TEST(MetaclassTest, ClassWithCustomMetaclassIsntConcreteClass) {
  const char* src = R"(
class MyMeta(type):
    pass

class Foo(type, metaclass=MyMeta):
    pass
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));

  Handle<Object> foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_FALSE(foo->isType());
}

TEST(MetaclassTest, ClassWithTypeMetaclassIsInstanceOfClass) {
  const char* src = R"(
class Foo(type):
    pass

class Bar(Foo):
    pass
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));

  Handle<Object> foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_TRUE(runtime.isInstanceOfClass(*foo));

  Handle<Object> bar(&scope, moduleAt(&runtime, main, "Bar"));
  EXPECT_TRUE(runtime.isInstanceOfClass(*bar));
}

TEST(MetaclassTest, ClassWithCustomMetaclassIsInstanceOfClass) {
  const char* src = R"(
class MyMeta(type):
    pass

class Foo(type, metaclass=MyMeta):
    pass
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_TRUE(runtime.isInstanceOfClass(*foo));
}

TEST(MetaclassTest, VerifyMetaclassHierarchy) {
  const char* src = R"(
class GrandMeta(type):
    pass

class ParentMeta(type, metaclass=GrandMeta):
    pass

class ChildMeta(type, metaclass=ParentMeta):
    pass
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> type(&scope, runtime.typeAt(LayoutId::kType));

  Handle<Object> grand_meta(&scope, moduleAt(&runtime, main, "GrandMeta"));
  EXPECT_EQ(runtime.typeOf(*grand_meta), *type);

  Handle<Object> parent_meta(&scope, moduleAt(&runtime, main, "ParentMeta"));
  EXPECT_EQ(runtime.typeOf(*parent_meta), *grand_meta);

  Handle<Object> child_meta(&scope, moduleAt(&runtime, main, "ChildMeta"));
  EXPECT_EQ(runtime.typeOf(*child_meta), *parent_meta);
}

TEST(MetaclassTest, CallMetaclass) {
  const char* src = R"(
class MyMeta(type):
    pass

Foo = MyMeta('Foo', (), {})
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> mymeta(&scope, moduleAt(&runtime, main, "MyMeta"));
  Handle<Object> foo(&scope, moduleAt(&runtime, main, "Foo"));
  EXPECT_EQ(runtime.typeOf(*foo), *mymeta);
  EXPECT_FALSE(foo->isType());
  EXPECT_TRUE(runtime.isInstanceOfClass(*foo));
}

TEST(ImportlibTest, SysMetaPathIsList) {
  const char* src = R"(
import sys

meta_path = sys.meta_path
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> meta_path(&scope, moduleAt(&runtime, main, "meta_path"));
  ASSERT_TRUE(meta_path->isList());
}

TEST(SubclassingTest, SubclassBuiltinSubclass) {
  const char* src = R"(
class Test(Exception):
  pass
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> value(&scope, moduleAt(&runtime, main, "Test"));
  ASSERT_TRUE(value->isType());

  Handle<Type> type(&scope, *value);
  ASSERT_TRUE(type->mro()->isObjectArray());

  Handle<ObjectArray> mro(&scope, type->mro());
  ASSERT_EQ(mro->length(), 4);
  EXPECT_EQ(mro->at(0), *type);
  EXPECT_EQ(mro->at(1), runtime.typeAt(LayoutId::kException));
  EXPECT_EQ(mro->at(2), runtime.typeAt(LayoutId::kBaseException));
  EXPECT_EQ(mro->at(3), runtime.typeAt(LayoutId::kObject));
}

TEST(ModuleImportTest, ModuleImportsAllPublicSymbols) {
  Runtime runtime;
  HandleScope scope;

  // Create Module
  Handle<Object> name(&scope, runtime.newStrFromCStr("foo"));
  Handle<Module> module(&scope, runtime.newModule(name));

  // Add symbols
  Handle<Dict> module_dict(&scope, module->dict());
  Handle<Object> symbol_str1(&scope, runtime.newStrFromCStr("public_symbol"));
  Handle<Object> symbol_str2(&scope, runtime.newStrFromCStr("_private_symbol"));
  runtime.dictAtPutInValueCell(module_dict, symbol_str1, symbol_str1);
  runtime.dictAtPutInValueCell(module_dict, symbol_str2, symbol_str2);

  // Import public symbols to dictionary
  Handle<Dict> symbols_dict(&scope, runtime.newDict());
  runtime.moduleImportAllFrom(symbols_dict, module);
  EXPECT_EQ(symbols_dict->numItems(), 1);

  Handle<ValueCell> result(&scope, runtime.dictAt(symbols_dict, symbol_str1));
  EXPECT_PYSTRING_EQ(Str::cast(result->value()), "public_symbol");
}

TEST(HeapFrameTest, Create) {
  const char* src = R"(
def gen():
  yield 12
)";

  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Handle<Object> gen(&scope, moduleAt(&runtime, "__main__", "gen"));
  ASSERT_TRUE(gen->isFunction());
  Handle<Code> code(&scope, Function::cast(*gen)->code());
  Handle<Object> frame_obj(&scope, runtime.newHeapFrame(code));
  ASSERT_TRUE(frame_obj->isHeapFrame());
  Handle<HeapFrame> heap_frame(frame_obj);
  EXPECT_EQ(heap_frame->maxStackSize(), code->stacksize());
}

}  // namespace python
