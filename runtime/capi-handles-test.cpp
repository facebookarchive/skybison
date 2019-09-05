#include "gtest/gtest.h"

#include "capi-handles.h"

#include "object-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

using CApiHandlesDeathTest = RuntimeFixture;
using CApiHandlesTest = RuntimeFixture;

static RawObject initializeExtensionType(PyObject* extension_type) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // Initialize Type
  PyObject* pytype_type =
      ApiHandle::newReference(thread, runtime->layoutAt(LayoutId::kType));
  PyObject* pyobj = reinterpret_cast<PyObject*>(extension_type);
  pyobj->ob_type = reinterpret_cast<PyTypeObject*>(pytype_type);
  Type type(&scope, runtime->newType());

  // Compute MRO
  Tuple mro(&scope, runtime->emptyTuple());
  type.setMro(*mro);

  // Initialize instance Layout
  Layout layout_init(&scope, runtime->layoutCreateEmpty(thread));
  Object attr_name(&scope, runtime->symbols()->ExtensionPtr());
  Layout layout(&scope,
                runtime->layoutAddAttribute(thread, layout_init, attr_name, 0));
  layout.setDescribedType(*type);
  type.setInstanceLayout(*layout);

  pyobj->reference_ = type.raw();
  return *type;
}

TEST_F(CApiHandlesTest, BorrowedApiHandles) {
  HandleScope scope(thread_);

  // Create a new object and a new reference to that object.
  Object obj(&scope, runtime_.newTuple(10));
  ApiHandle* new_ref = ApiHandle::newReference(thread_, *obj);
  word refcnt = new_ref->refcnt();

  // Create a borrowed reference to the same object.  This should not affect the
  // reference count of the handle.
  ApiHandle* borrowed_ref = ApiHandle::borrowedReference(thread_, *obj);
  EXPECT_EQ(borrowed_ref, new_ref);
  EXPECT_EQ(borrowed_ref->refcnt(), refcnt);

  // Create another new reference.  This should increment the reference count
  // of the handle.
  ApiHandle* another_ref = ApiHandle::newReference(thread_, *obj);
  EXPECT_EQ(another_ref, new_ref);
  EXPECT_EQ(another_ref->refcnt(), refcnt + 1);
}

TEST_F(CApiHandlesTest, BuiltinIntObjectReturnsApiHandle) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.apiHandles());
  Object obj(&scope, runtime_.newInt(1));
  ApiHandle* handle = ApiHandle::newReference(thread_, *obj);
  EXPECT_NE(handle, nullptr);
  EXPECT_TRUE(runtime_.dictIncludes(thread_, dict, obj));
}

TEST_F(CApiHandlesTest, ApiHandleReturnsBuiltinIntObject) {
  HandleScope scope(thread_);

  Object obj(&scope, runtime_.newInt(1));
  ApiHandle* handle = ApiHandle::newReference(thread_, *obj);
  Object handle_obj(&scope, handle->asObject());
  EXPECT_TRUE(isIntEqualsWord(*handle_obj, 1));
}

TEST_F(CApiHandlesTest, BuiltinObjectReturnsApiHandle) {
  HandleScope scope(thread_);

  Dict dict(&scope, runtime_.apiHandles());
  Object obj(&scope, runtime_.newList());
  ASSERT_FALSE(runtime_.dictIncludes(thread_, dict, obj));

  ApiHandle* handle = ApiHandle::newReference(thread_, *obj);
  EXPECT_NE(handle, nullptr);

  EXPECT_TRUE(runtime_.dictIncludes(thread_, dict, obj));
}

TEST_F(CApiHandlesTest, BuiltinObjectReturnsSameApiHandle) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_.newList());
  ApiHandle* handle = ApiHandle::newReference(thread_, *obj);
  ApiHandle* handle2 = ApiHandle::newReference(thread_, *obj);
  EXPECT_EQ(handle, handle2);
}

TEST_F(CApiHandlesTest, ApiHandleReturnsBuiltinObject) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_.newList());
  ApiHandle* handle = ApiHandle::newReference(thread_, *obj);
  Object handle_obj(&scope, handle->asObject());
  EXPECT_TRUE(handle_obj.isList());
}

TEST_F(CApiHandlesTest, ExtensionInstanceObjectReturnsPyObject) {
  HandleScope scope(thread_);

  // Create type
  PyObject extension_type;
  Type type(&scope, initializeExtensionType(&extension_type));

  // Create instance
  Layout layout(&scope, type.instanceLayout());
  Object attr_name(&scope, runtime_.symbols()->ExtensionPtr());
  HeapObject instance(&scope, runtime_.newInstance(layout));

  PyObject* type_handle = ApiHandle::newReference(thread_, *type);
  PyObject pyobj = {0, 1, reinterpret_cast<PyTypeObject*>(type_handle)};
  Object object_ptr(&scope,
                    runtime_.newIntFromCPtr(static_cast<void*>(&pyobj)));
  instanceSetAttr(thread_, instance, attr_name, object_ptr);

  PyObject* result = ApiHandle::newReference(thread_, *instance);
  EXPECT_TRUE(result);
  EXPECT_EQ(result, &pyobj);
}

TEST_F(CApiHandlesTest, Runtime_InstanceObjectReturnsPyObject) {
  HandleScope scope(thread_);

  // Create type
  PyObject extension_type;
  Type type(&scope, initializeExtensionType(&extension_type));

  // Initialize instance Layout
  Layout layout(&scope, runtime_.layoutCreateEmpty(thread_));
  layout.setDescribedType(*type);
  type.setInstanceLayout(*layout);

  // Create instance
  HeapObject instance(&scope, runtime_.newInstance(layout));
  PyObject* result = ApiHandle::newReference(thread_, *instance);
  ASSERT_NE(result, nullptr);

  Object obj(&scope, ApiHandle::fromPyObject(result)->asObject());
  EXPECT_EQ(*obj, *instance);
}

TEST_F(CApiHandlesTest, PyObjectReturnsExtensionInstance) {
  HandleScope scope(thread_);

  // Create type
  PyObject extension_type;
  Type type(&scope, initializeExtensionType(&extension_type));
  PyObject* extension_type_ref = ApiHandle::newReference(thread_, *type);

  PyObject* pyobj = static_cast<PyObject*>(std::malloc(sizeof(PyObject)));
  pyobj->reference_ = 0;
  pyobj->ob_refcnt = 1;
  pyobj->ob_type = reinterpret_cast<PyTypeObject*>(extension_type_ref);
  Object handle_obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  EXPECT_TRUE(handle_obj.isInstance());
}

TEST_F(CApiHandlesTest, Cache) {
  HandleScope scope(thread_);

  auto handle1 = ApiHandle::newReference(thread_, SmallInt::fromWord(5));
  EXPECT_EQ(handle1->cache(), nullptr);

  Str str(&scope,
          runtime_.newStrFromCStr("this is too long for a RawSmallStr"));
  auto handle2 = ApiHandle::newReference(thread_, *str);
  EXPECT_EQ(handle2->cache(), nullptr);

  void* buffer1 = std::malloc(16);
  handle1->setCache(buffer1);
  EXPECT_EQ(handle1->cache(), buffer1);
  EXPECT_EQ(handle2->cache(), nullptr);

  void* buffer2 = std::malloc(16);
  handle2->setCache(buffer2);
  EXPECT_EQ(handle2->cache(), buffer2);
  EXPECT_EQ(handle1->cache(), buffer1);

  handle1->setCache(buffer2);
  handle2->setCache(buffer1);
  EXPECT_EQ(handle1->cache(), buffer2);
  EXPECT_EQ(handle2->cache(), buffer1);

  Object key(&scope, handle1->asObject());
  handle1->dispose();
  Dict caches(&scope, runtime_.apiCaches());
  EXPECT_TRUE(runtime_.dictAt(thread_, caches, key).isError());
  EXPECT_EQ(handle2->cache(), buffer1);
}

TEST_F(CApiHandlesTest, VisitReferences) {
  HandleScope scope(thread_);

  Object obj1(&scope, runtime_.newInt(123));
  Object obj2(&scope, runtime_.newStrFromCStr("hello"));
  ApiHandle::newReference(thread_, *obj1);
  ApiHandle::newReference(thread_, *obj2);

  RememberingVisitor visitor;
  ApiHandle::visitReferences(runtime_.apiHandles(), &visitor);

  // We should've visited obj1, obj2, their types, and Type.
  EXPECT_TRUE(visitor.hasVisited(*obj1));
  EXPECT_TRUE(visitor.hasVisited(runtime_.typeAt(obj1.layoutId())));
  EXPECT_TRUE(visitor.hasVisited(*obj2));
  EXPECT_TRUE(visitor.hasVisited(runtime_.typeAt(obj2.layoutId())));
  EXPECT_TRUE(visitor.hasVisited(runtime_.typeAt(LayoutId::kType)));
}

TEST_F(CApiHandlesDeathTest, CleanupApiHandlesOnExit) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_.newStrFromCStr("hello"));
  ApiHandle::newReference(thread_, *obj);
  ASSERT_EXIT(static_cast<void>(runFromCStr(&runtime_, R"(
import sys
sys.exit()
)")),
              ::testing::ExitedWithCode(0), "");
}

}  // namespace python
