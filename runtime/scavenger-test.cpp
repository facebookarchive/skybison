#include "scavenger.h"

#include "gtest/gtest.h"

#include "builtins-module.h"
#include "test-utils.h"
#include "trampolines.h"

namespace py {
namespace testing {

using ScavengerTest = RuntimeFixture;

TEST_F(ScavengerTest, PreserveWeakReferenceHeapReferent) {
  HandleScope scope(thread_);
  Tuple array(&scope, newTupleWithNone(10));
  WeakRef ref(&scope, runtime_->newWeakRef(thread_, array));
  runtime_->collectGarbage();
  EXPECT_EQ(ref.referent(), *array);
}

TEST_F(ScavengerTest, ClearWeakReference) {
  HandleScope scope(Thread::current());
  Object none(&scope, NoneType::object());
  Object ref(&scope, *none);
  {
    Tuple array(&scope, newTupleWithNone(10));
    WeakRef ref_inner(&scope, runtime_->newWeakRef(Thread::current(), array));
    ref = *ref_inner;
    runtime_->collectGarbage();
    EXPECT_EQ(ref_inner.referent(), *array);
  }
  runtime_->collectGarbage();
  EXPECT_EQ(WeakRef::cast(*ref).referent(), NoneType::object());
}

TEST_F(ScavengerTest, ClearWeakLinkReferences) {
  HandleScope scope(thread_);
  Object none(&scope, NoneType::object());
  Object link0(&scope, *none);
  Object link1(&scope, *none);
  Object link2(&scope, *none);
  Tuple referent1(&scope, newTupleWithNone(11));
  {
    Tuple referent0(&scope, newTupleWithNone(10));
    Tuple referent2(&scope, newTupleWithNone(11));
    WeakLink link0_inner(&scope,
                         runtime_->newWeakLink(thread_, referent0, none, none));
    WeakLink link1_inner(
        &scope, runtime_->newWeakLink(thread_, referent1, link0_inner, none));
    WeakLink link2_inner(
        &scope, runtime_->newWeakLink(thread_, referent2, link1_inner, none));
    link0_inner.setNext(*link1_inner);
    link1_inner.setNext(*link2_inner);

    link0 = *link0_inner;
    link1 = *link1_inner;
    link2 = *link2_inner;

    runtime_->collectGarbage();
    EXPECT_EQ(link0_inner.referent(), *referent0);
    EXPECT_EQ(link1_inner.referent(), *referent1);
    EXPECT_EQ(link2_inner.referent(), *referent2);
  }
  runtime_->collectGarbage();
  EXPECT_EQ(WeakRef::cast(*link0).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*link1).referent(), *referent1);
  EXPECT_EQ(WeakRef::cast(*link2).referent(), NoneType::object());
}

TEST_F(ScavengerTest, PreserveSomeClearSomeReferents) {
  HandleScope scope(thread_);

  // Create strongly referenced heap allocated objects.
  MutableTuple strongrefs(&scope, runtime_->newMutableTuple(4));
  for (word i = 0; i < strongrefs.length(); i++) {
    Float elt(&scope, runtime_->newFloat(i));
    strongrefs.atPut(i, *elt);
  }

  // Create a parallel array of weak references with the strongly referenced
  // objects as referents.
  MutableTuple weakrefs(&scope, runtime_->newMutableTuple(4));
  for (word i = 0; i < weakrefs.length(); i++) {
    Object obj(&scope, strongrefs.at(i));
    WeakRef elt(&scope, runtime_->newWeakRef(thread_, obj));
    weakrefs.atPut(i, *elt);
  }

  // Make sure the weak references point to the expected referents.
  EXPECT_EQ(strongrefs.at(0), WeakRef::cast(weakrefs.at(0)).referent());
  EXPECT_EQ(strongrefs.at(1), WeakRef::cast(weakrefs.at(1)).referent());
  EXPECT_EQ(strongrefs.at(2), WeakRef::cast(weakrefs.at(2)).referent());
  EXPECT_EQ(strongrefs.at(3), WeakRef::cast(weakrefs.at(3)).referent());

  // Now do a garbage collection.
  runtime_->collectGarbage();

  // Make sure that the weak references still point to the expected referents.
  EXPECT_EQ(strongrefs.at(0), WeakRef::cast(weakrefs.at(0)).referent());
  EXPECT_EQ(strongrefs.at(1), WeakRef::cast(weakrefs.at(1)).referent());
  EXPECT_EQ(strongrefs.at(2), WeakRef::cast(weakrefs.at(2)).referent());
  EXPECT_EQ(strongrefs.at(3), WeakRef::cast(weakrefs.at(3)).referent());

  // Clear the odd indexed strong references.
  strongrefs.atPut(1, NoneType::object());
  strongrefs.atPut(3, NoneType::object());
  runtime_->collectGarbage();

  // Now do another garbage collection.  This one should clear just the weak
  // references that point to objects that are no longer strongly referenced.

  // Check that the strongly referenced referents are preserved and the weakly
  // referenced referents are now cleared.
  EXPECT_EQ(strongrefs.at(0), WeakRef::cast(weakrefs.at(0)).referent());
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs.at(1)).referent());
  EXPECT_EQ(strongrefs.at(2), WeakRef::cast(weakrefs.at(2)).referent());
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs.at(3)).referent());

  // Clear the even indexed strong references.
  strongrefs.atPut(0, NoneType::object());
  strongrefs.atPut(2, NoneType::object());

  // Do a final garbage collection.  There are no more strongly referenced
  // objects so all of the weak references should be cleared.
  runtime_->collectGarbage();

  // Check that all of the referents are cleared.
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs.at(0)).referent());
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs.at(1)).referent());
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs.at(2)).referent());
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs.at(3)).referent());
}

TEST_F(ScavengerTest, BaseCallback) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = 1
b = 2
def f(ref):
  global a
  a = 3
def g(ref, c=4):
  global b
  b = c
)")
                   .isError());
  Object none(&scope, NoneType::object());
  Object ref1(&scope, *none);
  Object ref2(&scope, *none);
  {
    Tuple array1(&scope, newTupleWithNone(10));
    Function func_f(&scope, mainModuleAt(runtime_, "f"));
    WeakRef ref1_inner(
        &scope, newWeakRefWithCallback(runtime_, thread_, array1, func_f));
    ref1 = *ref1_inner;

    Tuple array2(&scope, newTupleWithNone(10));
    Function func_g(&scope, mainModuleAt(runtime_, "g"));
    WeakRef ref2_inner(
        &scope, newWeakRefWithCallback(runtime_, thread_, array2, func_g));
    ref2 = *ref2_inner;

    runtime_->collectGarbage();

    EXPECT_EQ(ref1_inner.referent(), *array1);
    EXPECT_EQ(ref2_inner.referent(), *array2);
    SmallInt a(&scope, mainModuleAt(runtime_, "a"));
    SmallInt b(&scope, mainModuleAt(runtime_, "b"));
    EXPECT_EQ(a.value(), 1);
    EXPECT_EQ(b.value(), 2);
  }
  runtime_->collectGarbage();

  EXPECT_EQ(WeakRef::cast(*ref1).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref1).callback(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref2).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref2).callback(), NoneType::object());
  SmallInt a(&scope, mainModuleAt(runtime_, "a"));
  SmallInt b(&scope, mainModuleAt(runtime_, "b"));
  EXPECT_EQ(a.value(), 3);
  EXPECT_EQ(b.value(), 4);
}

TEST_F(ScavengerTest, MixCallback) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = 1
b = 2
def f(ref):
  global a
  a = 3
def g(ref, c=4):
  global b
  b = c
)")
                   .isError());

  Tuple array1(&scope, newTupleWithNone(10));
  Function func_f(&scope, mainModuleAt(runtime_, "f"));
  WeakRef ref1(&scope,
               newWeakRefWithCallback(runtime_, thread_, array1, func_f));
  Object ref2(&scope, NoneType::object());
  {
    Tuple array2(&scope, newTupleWithNone(10));
    Function func_g(&scope, mainModuleAt(runtime_, "g"));
    WeakRef ref2_inner(
        &scope, newWeakRefWithCallback(runtime_, thread_, array2, func_g));
    ref2 = *ref2_inner;

    runtime_->collectGarbage();

    EXPECT_EQ(ref1.referent(), *array1);
    EXPECT_EQ(ref2_inner.referent(), *array2);
    SmallInt a(&scope, mainModuleAt(runtime_, "a"));
    SmallInt b(&scope, mainModuleAt(runtime_, "b"));
    EXPECT_EQ(a.value(), 1);
    EXPECT_EQ(b.value(), 2);
  }
  runtime_->collectGarbage();

  EXPECT_EQ(ref1.referent(), *array1);
  EXPECT_EQ(BoundMethod::cast(ref1.callback()).function(), *func_f);
  EXPECT_EQ(WeakRef::cast(*ref2).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref2).callback(), NoneType::object());
  SmallInt a(&scope, mainModuleAt(runtime_, "a"));
  SmallInt b(&scope, mainModuleAt(runtime_, "b"));
  EXPECT_EQ(a.value(), 1);
  EXPECT_EQ(b.value(), 4);
}

static ALIGN_16 RawObject doGarbageCollection(Thread* thread, Arguments) {
  thread->runtime()->collectGarbage();
  return NoneType::object();
}

TEST_F(ScavengerTest, CallbackInvokeGC) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = 1
def g(ref, b=2):
  global a
  a = b
)")
                   .isError());
  Object ref1(&scope, NoneType::object());
  Object ref2(&scope, NoneType::object());
  {
    Tuple array1(&scope, newTupleWithNone(10));
    Str name(&scope, runtime_->newStrFromCStr("collect"));
    Object empty_tuple(&scope, runtime_->emptyTuple());
    Code code(&scope,
              runtime_->newBuiltinCode(
                  /*argcount=*/0, /*posonlyargcount=*/0,
                  /*kwonlyargcount=*/0, /*flags=*/0, doGarbageCollection,
                  /*parameter_names=*/empty_tuple, name));
    Module module(&scope, findMainModule(runtime_));
    Function collect(
        &scope, runtime_->newFunctionWithCode(thread_, name, code, module));

    WeakRef ref1_inner(
        &scope, newWeakRefWithCallback(runtime_, thread_, array1, collect));
    ref1 = *ref1_inner;

    Tuple array2(&scope, newTupleWithNone(10));
    Function func_g(&scope, mainModuleAt(runtime_, "g"));
    WeakRef ref2_inner(
        &scope, newWeakRefWithCallback(runtime_, thread_, array2, func_g));
    ref2 = *ref2_inner;

    runtime_->collectGarbage();

    EXPECT_EQ(ref1_inner.referent(), *array1);
    EXPECT_EQ(ref2_inner.referent(), *array2);
    SmallInt a(&scope, mainModuleAt(runtime_, "a"));
    EXPECT_EQ(a.value(), 1);
  }
  runtime_->collectGarbage();

  EXPECT_EQ(WeakRef::cast(*ref1).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref1).callback(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref2).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref2).callback(), NoneType::object());
  SmallInt a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_EQ(a.value(), 2);
}

TEST_F(ScavengerTest, IgnoreCallbackException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = 1
b = 2
callback_ran = False
callback_returned = False
def f(ref):
  global callback_ran
  callback_ran = True
  raise AttributeError("aloha")
  global callback_returned
  callback_returned = True
def g(ref, c=4):
  global b
  b = c
)")
                   .isError());
  Object ref1(&scope, NoneType::object());
  Object ref2(&scope, NoneType::object());
  {
    Tuple array1(&scope, newTupleWithNone(10));
    Function func_f(&scope, mainModuleAt(runtime_, "f"));
    WeakRef ref1_inner(
        &scope, newWeakRefWithCallback(runtime_, thread_, array1, func_f));
    ref1 = *ref1_inner;

    Tuple array2(&scope, newTupleWithNone(10));
    Function func_g(&scope, mainModuleAt(runtime_, "g"));
    WeakRef ref2_inner(
        &scope, newWeakRefWithCallback(runtime_, thread_, array2, func_g));
    ref2 = *ref2_inner;

    runtime_->collectGarbage();

    EXPECT_EQ(ref1_inner.referent(), *array1);
    EXPECT_EQ(ref2_inner.referent(), *array2);
    SmallInt a(&scope, mainModuleAt(runtime_, "a"));
    SmallInt b(&scope, mainModuleAt(runtime_, "b"));
    EXPECT_EQ(a.value(), 1);
    EXPECT_EQ(b.value(), 2);
  }

  runtime_->collectGarbage();
  EXPECT_FALSE(Thread::current()->hasPendingException());
  EXPECT_EQ(mainModuleAt(runtime_, "callback_ran"), Bool::trueObj());
  EXPECT_EQ(mainModuleAt(runtime_, "callback_returned"), Bool::falseObj());

  EXPECT_EQ(WeakRef::cast(*ref1).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref1).callback(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref2).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref2).callback(), NoneType::object());
  SmallInt a(&scope, mainModuleAt(runtime_, "a"));
  SmallInt b(&scope, mainModuleAt(runtime_, "b"));
  EXPECT_EQ(a.value(), 1);
  EXPECT_EQ(b.value(), 4);
}

TEST_F(ScavengerTest, CollectGarbagePreservesPendingException) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
did_run = False
def callback(ref):
  global did_run
  did_run = ref
  raise TabError()
class C:
  pass
)")
                   .isError());
  HandleScope scope(thread_);
  ASSERT_EQ(mainModuleAt(runtime_, "did_run"), Bool::falseObj());
  Object callback(&scope, mainModuleAt(runtime_, "callback"));
  Type c(&scope, mainModuleAt(runtime_, "C"));

  // Create an instance C and a weak reference with callback to it.
  Layout layout(&scope, c.instanceLayout());
  Object instance(&scope, runtime_->newInstance(layout));
  WeakRef ref(&scope,
              newWeakRefWithCallback(runtime_, thread_, instance, callback));
  instance = NoneType::object();

  thread_->raise(LayoutId::kUserWarning, runtime_->newInt(99));
  runtime_->collectGarbage();

  EXPECT_EQ(ref.referent(), NoneType::object());
  EXPECT_EQ(mainModuleAt(runtime_, "did_run"), ref);
  ASSERT_TRUE(thread_->hasPendingException());
  EXPECT_TRUE(thread_->pendingExceptionMatches(LayoutId::kUserWarning));
  EXPECT_TRUE(isIntEqualsWord(thread_->pendingExceptionValue(), 99));
}

TEST_F(ScavengerTest, CollectGarbageCollectsDeadLayout) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  pass
)")
                   .isError());

  HandleScope scope(thread_);
  LayoutId c_layout_id;
  {
    Type c(&scope, mainModuleAt(runtime_, "C"));
    Layout c_layout(&scope, c.instanceLayout());
    c_layout_id = c_layout.id();
    ASSERT_EQ(runtime_->layoutAt(c_layout_id), *c_layout);
  }

  ASSERT_FALSE(runFromCStr(runtime_, "del C").isError());
  // This collection should kill C.
  runtime_->collectGarbage();
  EXPECT_TRUE(runtime_->layoutAt(c_layout_id) == SmallInt::fromWord(0));
}

TEST_F(ScavengerTest, CollectGarbagePreservesTypeWithoutNormalReferences) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  pass
c = C()
del C
)")
                   .isError());

  runtime_->collectGarbage();
  HandleScope scope(thread_);
  Object c(&scope, mainModuleAt(runtime_, "c"));
  EXPECT_TRUE(runtime_->typeOf(*c).isType());
}

TEST_F(ScavengerTest, CollectGarbagePreservesLayoutWithNoLiveObjects) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  pass
)")
                   .isError());

  HandleScope scope(thread_);
  Type c(&scope, mainModuleAt(runtime_, "C"));
  LayoutId c_layout_id;
  {
    Layout c_layout(&scope, c.instanceLayout());
    c_layout_id = c_layout.id();
    ASSERT_EQ(runtime_->layoutAt(c_layout_id), *c_layout);
  }

  runtime_->collectGarbage();
  EXPECT_EQ(c.instanceLayout(), runtime_->layoutAt(c_layout_id));
}

}  // namespace testing
}  // namespace py
