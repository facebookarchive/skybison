#include "api-handle.h"

#include "cpython-func.h"
#include "gtest/gtest.h"

#include "capi-state.h"
#include "capi.h"
#include "dict-builtins.h"
#include "int-builtins.h"
#include "object-builtins.h"
#include "runtime.h"
#include "test-utils.h"
#include "type-builtins.h"

namespace py {
namespace testing {

using CApiHandlesDeathTest = RuntimeFixture;
using ApiHandleTest = RuntimeFixture;

static RawObject initializeExtensionType(PyObject* extension_type) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Str name(&scope, runtime->newStrFromCStr("ExtType"));
  Object object_type(&scope, runtime->typeAt(LayoutId::kObject));
  Tuple bases(&scope, runtime->newTupleWith1(object_type));
  Dict dict(&scope, runtime->newDict());
  Type metaclass(&scope, runtime->typeAt(LayoutId::kType));
  Type type(&scope, typeNew(thread, metaclass, name, bases, dict,
                            Type::Flag::kHasNativeData,
                            /*inherit_slots=*/false,
                            /*add_instance_dict=*/false));

  extension_type->reference_ = type.raw();
  return *type;
}

static ApiHandle* findHandleMatching(Runtime* runtime,
                                     const char* str_contents) {
  class Visitor : public HandleVisitor {
   public:
    void visitHandle(void* handle, RawObject object) override {
      if (object.isStr() && Str::cast(object).equalsCStr(str_contents)) {
        found = handle;
      }
    }
    const char* str_contents;
    void* found = nullptr;
  };
  Visitor visitor;
  visitor.str_contents = str_contents;
  visitApiHandles(runtime, &visitor);
  return reinterpret_cast<ApiHandle*>(visitor.found);
}

TEST_F(ApiHandleTest, ManagedObjectHandleWithRefcountZeroIsDisposed) {
  HandleScope scope(thread_);
  Object object(&scope, runtime_->newStrFromCStr("hello world"));

  ApiHandle* handle = ApiHandle::newReference(runtime_, *object);
  ASSERT_FALSE(handle->isImmediate());
  ASSERT_FALSE(handle->isBorrowedNoImmediate());
  EXPECT_EQ(handle->refcnt(), 1);

  EXPECT_EQ(findHandleMatching(runtime_, "hello world"), handle);
  handle->decref();
  EXPECT_EQ(findHandleMatching(runtime_, "hello world"), nullptr);
}

TEST_F(ApiHandleTest, ManagedObjectHandleWithRefcountZeroAfterGCIsDisposed) {
  HandleScope scope(thread_);
  Object object(&scope, runtime_->newStrFromCStr("hello world"));

  ApiHandle* handle = ApiHandle::newReference(runtime_, *object);
  ASSERT_FALSE(handle->isImmediate());
  ASSERT_FALSE(handle->isBorrowedNoImmediate());
  EXPECT_EQ(handle->refcnt(), 1);

  EXPECT_EQ(findHandleMatching(runtime_, "hello world"), handle);
  runtime_->collectGarbage();
  EXPECT_EQ(findHandleMatching(runtime_, "hello world"), handle);
  handle->decref();
  EXPECT_EQ(findHandleMatching(runtime_, "hello world"), nullptr);
}

TEST_F(ApiHandleTest, ManagedObjectHandleBorrowedIsNotDisposed) {
  HandleScope scope(thread_);
  Object object(&scope, runtime_->newStrFromCStr("hello world"));

  ApiHandle* handle = ApiHandle::borrowedReference(runtime_, *object);
  ASSERT_FALSE(handle->isImmediate());
  ASSERT_TRUE(handle->isBorrowedNoImmediate());
  EXPECT_EQ(handle->refcnt(), 0);

  EXPECT_EQ(findHandleMatching(runtime_, "hello world"), handle);
  runtime_->collectGarbage();
  EXPECT_EQ(findHandleMatching(runtime_, "hello world"), handle);
}

TEST_F(ApiHandleTest, ManagedObjectHandleBorrowedIsDisposedByGC) {
  HandleScope scope(thread_);
  Object object(&scope, runtime_->newStrFromCStr("hello world"));

  ApiHandle* handle = ApiHandle::borrowedReference(runtime_, *object);
  ASSERT_FALSE(handle->isImmediate());
  ASSERT_TRUE(handle->isBorrowedNoImmediate());
  EXPECT_EQ(handle->refcnt(), 0);

  EXPECT_EQ(findHandleMatching(runtime_, "hello world"), handle);
  object = NoneType::object();
  runtime_->collectGarbage();
  EXPECT_EQ(findHandleMatching(runtime_, "hello world"), nullptr);
}

TEST_F(ApiHandleTest, ManagedObjectHandleCachedIsDisposedByGC) {
  HandleScope scope(thread_);
  Object object(&scope, runtime_->newStrFromCStr("hello world"));

  ApiHandle* handle = ApiHandle::newReference(runtime_, *object);
  ASSERT_FALSE(handle->isImmediate());
  ASSERT_FALSE(handle->isBorrowedNoImmediate());
  ASSERT_EQ(handle->refcnt(), 1);

  EXPECT_EQ(capiCaches(runtime_)->at(*object), nullptr);
  const char* as_utf8 = PyUnicode_AsUTF8(handle);
  EXPECT_EQ(capiCaches(runtime_)->at(*object), as_utf8);
  EXPECT_TRUE(handle->isBorrowedNoImmediate());
  handle->decref();
  ASSERT_EQ(handle->refcnt(), 0);
  EXPECT_EQ(findHandleMatching(runtime_, "hello world"), handle);

  // Check that handle is not disposed while still references by `object`.
  runtime_->collectGarbage();

  EXPECT_EQ(capiCaches(runtime_)->at(*object), as_utf8);
  EXPECT_TRUE(handle->isBorrowedNoImmediate());
  EXPECT_EQ(std::strcmp(as_utf8, "hello world"), 0);
  EXPECT_EQ(findHandleMatching(runtime_, "hello world"), handle);

  // Check that handle is disposed when last reference in `object` disappears.
  object = NoneType::object();
  runtime_->collectGarbage();
  EXPECT_EQ(findHandleMatching(runtime_, "hello world"), nullptr);
}

TEST_F(ApiHandleTest, BorrowedApiHandles) {
  HandleScope scope(thread_);

  // Create a new object and a new reference to that object.
  Object obj(&scope, runtime_->newList());
  ApiHandle* new_ref = ApiHandle::newReference(runtime_, *obj);
  word refcnt = new_ref->refcnt();

  // Create a borrowed reference to the same object.  This should not affect the
  // reference count of the handle.
  ApiHandle* borrowed_ref = ApiHandle::borrowedReference(runtime_, *obj);
  EXPECT_EQ(borrowed_ref, new_ref);
  EXPECT_EQ(borrowed_ref->refcnt(), refcnt);

  // Create another new reference.  This should increment the reference count
  // of the handle.
  ApiHandle* another_ref = ApiHandle::newReference(runtime_, *obj);
  EXPECT_EQ(another_ref, new_ref);
  EXPECT_EQ(another_ref->refcnt(), refcnt + 1);
}

TEST_F(ApiHandleTest, BuiltinHeapAllocatedIntObjectReturnsApiHandle) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->newInt(SmallInt::kMaxValue + 1));
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  EXPECT_NE(handle, nullptr);
  EXPECT_FALSE(handle->isImmediate());
  ApiHandleDict* dict = capiHandles(runtime_);
  EXPECT_EQ(dict->at(*obj), handle);
  handle->decref();
}

TEST_F(ApiHandleTest, BuiltinImmediateIntObjectReturnsImmediateApiHandle) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->newInt(1));
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  EXPECT_NE(handle, nullptr);
  EXPECT_TRUE(handle->isImmediate());
  ApiHandleDict* dict = capiHandles(runtime_);
  EXPECT_EQ(dict->at(*obj), nullptr);
  handle->decref();
}

TEST_F(ApiHandleTest, BuiltinImmediateTrueObjectReturnsImmediateApiHandle) {
  HandleScope scope(thread_);
  Object obj(&scope, Bool::trueObj());
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  EXPECT_NE(handle, nullptr);
  EXPECT_TRUE(handle->isImmediate());
  ApiHandleDict* dict = capiHandles(runtime_);
  EXPECT_EQ(dict->at(*obj), nullptr);
  handle->decref();
}

TEST_F(ApiHandleTest, BuiltinImmediateFalseObjectReturnsImmediateApiHandle) {
  HandleScope scope(thread_);
  Object obj(&scope, Bool::falseObj());
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  EXPECT_NE(handle, nullptr);
  EXPECT_TRUE(handle->isImmediate());
  ApiHandleDict* dict = capiHandles(runtime_);
  EXPECT_EQ(dict->at(*obj), nullptr);
  handle->decref();
}

TEST_F(ApiHandleTest,
       BuiltinImmediateNotImplementedObjectReturnsImmediateApiHandle) {
  HandleScope scope(thread_);
  Object obj(&scope, NotImplementedType::object());
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  EXPECT_NE(handle, nullptr);
  EXPECT_TRUE(handle->isImmediate());
  ApiHandleDict* dict = capiHandles(runtime_);
  EXPECT_EQ(dict->at(*obj), nullptr);
  handle->decref();
}

TEST_F(ApiHandleTest, BuiltinImmediateUnboundObjectReturnsImmediateApiHandle) {
  HandleScope scope(thread_);
  Object obj(&scope, Unbound::object());
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  EXPECT_NE(handle, nullptr);
  EXPECT_TRUE(handle->isImmediate());
  ApiHandleDict* dict = capiHandles(runtime_);
  EXPECT_EQ(dict->at(*obj), nullptr);
  handle->decref();
}

TEST_F(ApiHandleTest, ApiHandleReturnsBuiltinIntObject) {
  HandleScope scope(thread_);

  Object obj(&scope, runtime_->newInt(1));
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  Object handle_obj(&scope, handle->asObject());
  EXPECT_TRUE(isIntEqualsWord(*handle_obj, 1));
}

TEST_F(ApiHandleTest, BuiltinObjectReturnsApiHandle) {
  HandleScope scope(thread_);

  ApiHandleDict* dict = capiHandles(runtime_);
  Object obj(&scope, runtime_->newList());
  ASSERT_FALSE(dict->at(*obj) != nullptr);

  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  EXPECT_NE(handle, nullptr);

  EXPECT_TRUE(dict->at(*obj) != nullptr);
}

TEST_F(ApiHandleTest, BuiltinObjectReturnsSameApiHandle) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->newList());
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  ApiHandle* handle2 = ApiHandle::newReference(runtime_, *obj);
  EXPECT_EQ(handle, handle2);
}

TEST_F(ApiHandleTest, ApiHandleReturnsBuiltinObject) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->newList());
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  Object handle_obj(&scope, handle->asObject());
  EXPECT_TRUE(handle_obj.isList());
}

TEST_F(ApiHandleTest, ExtensionInstanceObjectReturnsPyObject) {
  HandleScope scope(thread_);

  // Create type
  PyObject extension_type;
  Type type(&scope, initializeExtensionType(&extension_type));
  Layout layout(&scope, type.instanceLayout());

  // Create instance
  NativeProxy proxy(&scope, runtime_->newInstance(layout));
  PyObject pyobj = {0, 1};
  proxy.setNative(runtime_->newIntFromCPtr(&pyobj));

  PyObject* result = ApiHandle::newReference(runtime_, *proxy);
  EXPECT_TRUE(result);
  EXPECT_EQ(result, &pyobj);
}

TEST_F(ApiHandleTest, RuntimeInstanceObjectReturnsPyObject) {
  HandleScope scope(thread_);

  // Create instance
  Layout layout(&scope, runtime_->layoutAt(LayoutId::kObject));
  Object instance(&scope, runtime_->newInstance(layout));
  PyObject* result = ApiHandle::newReference(runtime_, *instance);
  ASSERT_NE(result, nullptr);

  Object obj(&scope, ApiHandle::fromPyObject(result)->asObject());
  EXPECT_EQ(*obj, *instance);
}

TEST_F(ApiHandleTest,
       CheckFunctionResultNonNullptrWithoutPendingExceptionReturnsResult) {
  RawObject value = SmallInt::fromWord(1234);
  ApiHandle* handle = ApiHandle::newReference(runtime_, value);
  RawObject result = ApiHandle::checkFunctionResult(thread_, handle);
  EXPECT_EQ(result, value);
}

TEST_F(ApiHandleTest,
       CheckFunctionResultNullptrWithPendingExceptionReturnsError) {
  thread_->raiseBadArgument();  // TypeError
  RawObject result = ApiHandle::checkFunctionResult(thread_, nullptr);
  EXPECT_TRUE(result.isErrorException());
  EXPECT_TRUE(thread_->hasPendingException());
  EXPECT_TRUE(thread_->pendingExceptionMatches(LayoutId::kTypeError));
}

TEST_F(ApiHandleTest,
       CheckFunctionResultNullptrWithoutPendingExceptionRaisesSystemError) {
  EXPECT_FALSE(thread_->hasPendingException());
  RawObject result = ApiHandle::checkFunctionResult(thread_, nullptr);
  EXPECT_TRUE(result.isErrorException());
  EXPECT_TRUE(thread_->hasPendingException());
  EXPECT_TRUE(thread_->pendingExceptionMatches(LayoutId::kSystemError));
}

TEST_F(ApiHandleTest,
       CheckFunctionResultNonNullptrWithPendingExceptionRaisesSystemError) {
  thread_->raiseBadArgument();  // TypeError
  ApiHandle* handle =
      ApiHandle::newReference(runtime_, SmallInt::fromWord(1234));
  RawObject result = ApiHandle::checkFunctionResult(thread_, handle);
  EXPECT_TRUE(result.isErrorException());
  EXPECT_TRUE(thread_->hasPendingException());
  EXPECT_TRUE(thread_->pendingExceptionMatches(LayoutId::kSystemError));
}

TEST_F(ApiHandleTest, Cache) {
  HandleScope scope(thread_);

  auto handle1 = ApiHandle::newReference(runtime_, runtime_->newList());
  EXPECT_EQ(handle1->cache(runtime_), nullptr);

  Str str(&scope,
          runtime_->newStrFromCStr("this is too long for a RawSmallStr"));
  auto handle2 = ApiHandle::newReference(runtime_, *str);
  EXPECT_EQ(handle2->cache(runtime_), nullptr);

  void* buffer1 = std::malloc(16);
  handle1->setCache(runtime_, buffer1);
  EXPECT_EQ(handle1->cache(runtime_), buffer1);
  EXPECT_EQ(handle2->cache(runtime_), nullptr);

  void* buffer2 = std::malloc(16);
  handle2->setCache(runtime_, buffer2);
  EXPECT_EQ(handle2->cache(runtime_), buffer2);
  EXPECT_EQ(handle1->cache(runtime_), buffer1);

  handle1->setCache(runtime_, buffer2);
  handle2->setCache(runtime_, buffer1);
  EXPECT_EQ(handle1->cache(runtime_), buffer2);
  EXPECT_EQ(handle2->cache(runtime_), buffer1);

  Object key(&scope, handle1->asObject());
  handle1->disposeWithRuntime(runtime_);
  ApiHandleDict* caches = capiCaches(runtime_);
  EXPECT_FALSE(caches->at(*key) != nullptr);
  EXPECT_EQ(handle2->cache(runtime_), buffer1);
}

TEST_F(ApiHandleTest, VisitApiHandlesVisitsAllHandles) {
  HandleScope scope(thread_);

  Object obj0(&scope, runtime_->newDict());
  Object obj1(&scope, runtime_->newStrFromCStr("hello world"));
  ApiHandle* handle0 = ApiHandle::newReference(runtime_, *obj0);
  ApiHandle* handle1 = ApiHandle::borrowedReference(runtime_, *obj1);

  struct Visitor : public HandleVisitor {
    void visitHandle(void* handle, RawObject object) override {
      if (object == obj0) {
        EXPECT_EQ(obj0_handle, nullptr);
        obj0_handle = handle;
      }
      if (object == obj1) {
        EXPECT_EQ(obj1_handle, nullptr);
        obj1_handle = handle;
      }
    }

    RawObject obj0 = NoneType::object();
    RawObject obj1 = NoneType::object();
    void* obj0_handle = nullptr;
    void* obj1_handle = nullptr;
  };

  Visitor visitor;
  visitor.obj0 = *obj0;
  visitor.obj1 = *obj1;
  visitApiHandles(runtime_, &visitor);

  EXPECT_EQ(visitor.obj0_handle, handle0);
  EXPECT_EQ(visitor.obj1_handle, handle1);
  handle0->decref();
}

TEST_F(CApiHandlesDeathTest, CleanupApiHandlesOnExit) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->newStrFromCStr("hello"));
  ApiHandle::newReference(runtime_, *obj);
  ASSERT_EXIT(static_cast<void>(runFromCStr(runtime_, R"(
import sys
sys.exit()
)")),
              ::testing::ExitedWithCode(0), "");
}

}  // namespace testing
}  // namespace py
