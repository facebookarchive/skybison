#include "capi-handles.h"

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
using CApiHandlesTest = RuntimeFixture;

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

TEST_F(CApiHandlesTest, BorrowedApiHandles) {
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

TEST_F(CApiHandlesTest, BuiltinHeapAllocatedIntObjectReturnsApiHandle) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->newInt(SmallInt::kMaxValue + 1));
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  EXPECT_NE(handle, nullptr);
  EXPECT_FALSE(ApiHandle::isImmediate(handle));
  IdentityDict* dict = capiHandles(runtime_);
  EXPECT_EQ(dict->at(*obj), handle);
  handle->decref();
}

TEST_F(CApiHandlesTest, BuiltinImmediateIntObjectReturnsImmediateApiHandle) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->newInt(1));
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  EXPECT_NE(handle, nullptr);
  EXPECT_TRUE(ApiHandle::isImmediate(handle));
  IdentityDict* dict = capiHandles(runtime_);
  EXPECT_EQ(dict->at(*obj), nullptr);
  handle->decref();
}

TEST_F(CApiHandlesTest, BuiltinImmediateTrueObjectReturnsImmediateApiHandle) {
  HandleScope scope(thread_);
  Object obj(&scope, Bool::trueObj());
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  EXPECT_NE(handle, nullptr);
  EXPECT_TRUE(ApiHandle::isImmediate(handle));
  IdentityDict* dict = capiHandles(runtime_);
  EXPECT_EQ(dict->at(*obj), nullptr);
  handle->decref();
}

TEST_F(CApiHandlesTest, BuiltinImmediateFalseObjectReturnsImmediateApiHandle) {
  HandleScope scope(thread_);
  Object obj(&scope, Bool::falseObj());
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  EXPECT_NE(handle, nullptr);
  EXPECT_TRUE(ApiHandle::isImmediate(handle));
  IdentityDict* dict = capiHandles(runtime_);
  EXPECT_EQ(dict->at(*obj), nullptr);
  handle->decref();
}

TEST_F(CApiHandlesTest,
       BuiltinImmediateNotImplementedObjectReturnsImmediateApiHandle) {
  HandleScope scope(thread_);
  Object obj(&scope, NotImplementedType::object());
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  EXPECT_NE(handle, nullptr);
  EXPECT_TRUE(ApiHandle::isImmediate(handle));
  IdentityDict* dict = capiHandles(runtime_);
  EXPECT_EQ(dict->at(*obj), nullptr);
  handle->decref();
}

TEST_F(CApiHandlesTest,
       BuiltinImmediateUnboundObjectReturnsImmediateApiHandle) {
  HandleScope scope(thread_);
  Object obj(&scope, Unbound::object());
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  EXPECT_NE(handle, nullptr);
  EXPECT_TRUE(ApiHandle::isImmediate(handle));
  IdentityDict* dict = capiHandles(runtime_);
  EXPECT_EQ(dict->at(*obj), nullptr);
  handle->decref();
}

TEST_F(CApiHandlesTest, ApiHandleReturnsBuiltinIntObject) {
  HandleScope scope(thread_);

  Object obj(&scope, runtime_->newInt(1));
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  Object handle_obj(&scope, handle->asObject());
  EXPECT_TRUE(isIntEqualsWord(*handle_obj, 1));
}

TEST_F(CApiHandlesTest, BuiltinObjectReturnsApiHandle) {
  HandleScope scope(thread_);

  IdentityDict* dict = capiHandles(runtime_);
  Object obj(&scope, runtime_->newList());
  ASSERT_FALSE(dict->includes(*obj));

  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  EXPECT_NE(handle, nullptr);

  EXPECT_TRUE(dict->includes(*obj));
}

TEST_F(CApiHandlesTest, BuiltinObjectReturnsSameApiHandle) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->newList());
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  ApiHandle* handle2 = ApiHandle::newReference(runtime_, *obj);
  EXPECT_EQ(handle, handle2);
}

TEST_F(CApiHandlesTest, ApiHandleReturnsBuiltinObject) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->newList());
  ApiHandle* handle = ApiHandle::newReference(runtime_, *obj);
  Object handle_obj(&scope, handle->asObject());
  EXPECT_TRUE(handle_obj.isList());
}

TEST_F(CApiHandlesTest, ExtensionInstanceObjectReturnsPyObject) {
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

TEST_F(CApiHandlesTest, RuntimeInstanceObjectReturnsPyObject) {
  HandleScope scope(thread_);

  // Create instance
  Layout layout(&scope, runtime_->layoutAt(LayoutId::kObject));
  Object instance(&scope, runtime_->newInstance(layout));
  PyObject* result = ApiHandle::newReference(runtime_, *instance);
  ASSERT_NE(result, nullptr);

  Object obj(&scope, ApiHandle::fromPyObject(result)->asObject());
  EXPECT_EQ(*obj, *instance);
}

TEST_F(CApiHandlesTest,
       CheckFunctionResultNonNullptrWithoutPendingExceptionReturnsResult) {
  RawObject value = SmallInt::fromWord(1234);
  ApiHandle* handle = ApiHandle::newReference(runtime_, value);
  RawObject result = ApiHandle::checkFunctionResult(thread_, handle);
  EXPECT_EQ(result, value);
}

TEST_F(CApiHandlesTest,
       CheckFunctionResultNullptrWithPendingExceptionReturnsError) {
  thread_->raiseBadArgument();  // TypeError
  RawObject result = ApiHandle::checkFunctionResult(thread_, nullptr);
  EXPECT_TRUE(result.isErrorException());
  EXPECT_TRUE(thread_->hasPendingException());
  EXPECT_TRUE(thread_->pendingExceptionMatches(LayoutId::kTypeError));
}

TEST_F(CApiHandlesTest,
       CheckFunctionResultNullptrWithoutPendingExceptionRaisesSystemError) {
  EXPECT_FALSE(thread_->hasPendingException());
  RawObject result = ApiHandle::checkFunctionResult(thread_, nullptr);
  EXPECT_TRUE(result.isErrorException());
  EXPECT_TRUE(thread_->hasPendingException());
  EXPECT_TRUE(thread_->pendingExceptionMatches(LayoutId::kSystemError));
}

TEST_F(CApiHandlesTest,
       CheckFunctionResultNonNullptrWithPendingExceptionRaisesSystemError) {
  thread_->raiseBadArgument();  // TypeError
  ApiHandle* handle =
      ApiHandle::newReference(runtime_, SmallInt::fromWord(1234));
  RawObject result = ApiHandle::checkFunctionResult(thread_, handle);
  EXPECT_TRUE(result.isErrorException());
  EXPECT_TRUE(thread_->hasPendingException());
  EXPECT_TRUE(thread_->pendingExceptionMatches(LayoutId::kSystemError));
}

TEST_F(CApiHandlesTest, Cache) {
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
  handle1->dispose(runtime_);
  IdentityDict* caches = capiCaches(runtime_);
  EXPECT_FALSE(caches->includes(*key));
  EXPECT_EQ(handle2->cache(runtime_), buffer1);
}

TEST_F(CApiHandlesTest, VisitReferences) {
  HandleScope scope(thread_);

  Object obj1(&scope, runtime_->newInt(123));
  Object obj2(&scope, runtime_->newStrFromCStr("hello"));
  ApiHandle::newReference(runtime_, *obj1);
  ApiHandle::newReference(runtime_, *obj2);

  RememberingVisitor visitor;
  ApiHandle::visitReferences(capiHandles(runtime_), &visitor);

  // We should've visited obj2, but not obj1 since it is a SmallInt.
  EXPECT_FALSE(visitor.hasVisited(*obj1));
  EXPECT_TRUE(visitor.hasVisited(*obj2));
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
