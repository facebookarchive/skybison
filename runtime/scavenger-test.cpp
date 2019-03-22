#include "gtest/gtest.h"

#include "builtins-module.h"
#include "scavenger.h"
#include "test-utils.h"
#include "trampolines-inl.h"
#include "trampolines.h"

namespace python {
using namespace testing;

TEST(ScavengerTest, PreserveWeakReferenceHeapReferent) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple array(&scope, runtime.newTuple(10));
  Object none(&scope, NoneType::object());
  WeakRef ref(&scope, runtime.newWeakRef(thread, array, none));
  runtime.collectGarbage();
  EXPECT_EQ(ref.referent(), *array);
}

TEST(ScavengerTest, PreserveWeakReferenceImmediateReferent) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Int obj(&scope, SmallInt::fromWord(1234));
  Object none(&scope, NoneType::object());
  WeakRef ref(&scope, runtime.newWeakRef(thread, obj, none));
  runtime.collectGarbage();
  EXPECT_EQ(ref.referent(), SmallInt::fromWord(1234));
}

TEST(ScavengerTest, ClearWeakReference) {
  Runtime runtime;
  HandleScope scope;
  Object none(&scope, NoneType::object());
  Object ref(&scope, *none);
  {
    Tuple array(&scope, runtime.newTuple(10));
    WeakRef ref_inner(&scope,
                      runtime.newWeakRef(Thread::current(), array, none));
    ref = *ref_inner;
    runtime.collectGarbage();
    EXPECT_EQ(ref_inner.referent(), *array);
  }
  runtime.collectGarbage();
  EXPECT_EQ(RawWeakRef::cast(*ref).referent(), NoneType::object());
}

TEST(ScavengerTest, PreserveSomeClearSomeReferents) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  // Create strongly referenced heap allocated objects.
  Tuple strongrefs(&scope, runtime.newTuple(4));
  for (word i = 0; i < strongrefs.length(); i++) {
    Float elt(&scope, runtime.newFloat(i));
    strongrefs.atPut(i, *elt);
  }

  // Create a parallel array of weak references with the strongly referenced
  // objects as referents.
  Tuple weakrefs(&scope, runtime.newTuple(4));
  for (word i = 0; i < weakrefs.length(); i++) {
    Object obj(&scope, strongrefs.at(i));
    Object none(&scope, NoneType::object());
    WeakRef elt(&scope, runtime.newWeakRef(thread, obj, none));
    weakrefs.atPut(i, *elt);
  }

  // Make sure the weak references point to the expected referents.
  EXPECT_EQ(strongrefs.at(0), RawWeakRef::cast(weakrefs.at(0)).referent());
  EXPECT_EQ(strongrefs.at(1), RawWeakRef::cast(weakrefs.at(1)).referent());
  EXPECT_EQ(strongrefs.at(2), RawWeakRef::cast(weakrefs.at(2)).referent());
  EXPECT_EQ(strongrefs.at(3), RawWeakRef::cast(weakrefs.at(3)).referent());

  // Now do a garbage collection.
  runtime.collectGarbage();

  // Make sure that the weak references still point to the expected referents.
  EXPECT_EQ(strongrefs.at(0), RawWeakRef::cast(weakrefs.at(0)).referent());
  EXPECT_EQ(strongrefs.at(1), RawWeakRef::cast(weakrefs.at(1)).referent());
  EXPECT_EQ(strongrefs.at(2), RawWeakRef::cast(weakrefs.at(2)).referent());
  EXPECT_EQ(strongrefs.at(3), RawWeakRef::cast(weakrefs.at(3)).referent());

  // Clear the odd indexed strong references.
  strongrefs.atPut(1, NoneType::object());
  strongrefs.atPut(3, NoneType::object());
  runtime.collectGarbage();

  // Now do another garbage collection.  This one should clear just the weak
  // references that point to objects that are no longer strongly referenced.

  // Check that the strongly referenced referents are preserved and the weakly
  // referenced referents are now cleared.
  EXPECT_EQ(strongrefs.at(0), RawWeakRef::cast(weakrefs.at(0)).referent());
  EXPECT_EQ(NoneType::object(), RawWeakRef::cast(weakrefs.at(1)).referent());
  EXPECT_EQ(strongrefs.at(2), RawWeakRef::cast(weakrefs.at(2)).referent());
  EXPECT_EQ(NoneType::object(), RawWeakRef::cast(weakrefs.at(3)).referent());

  // Clear the even indexed strong references.
  strongrefs.atPut(0, NoneType::object());
  strongrefs.atPut(2, NoneType::object());

  // Do a final garbage collection.  There are no more strongly referenced
  // objects so all of the weak references should be cleared.
  runtime.collectGarbage();

  // Check that all of the referents are cleared.
  EXPECT_EQ(NoneType::object(), RawWeakRef::cast(weakrefs.at(0)).referent());
  EXPECT_EQ(NoneType::object(), RawWeakRef::cast(weakrefs.at(1)).referent());
  EXPECT_EQ(NoneType::object(), RawWeakRef::cast(weakrefs.at(2)).referent());
  EXPECT_EQ(NoneType::object(), RawWeakRef::cast(weakrefs.at(3)).referent());
}

TEST(ScavengerTest, BaseCallback) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  const char* src = R"(
a = 1
b = 2
def f(ref):
  global a
  a = 3
def g(ref, c=4):
  global b
  b = c
)";
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object none(&scope, NoneType::object());
  Object ref1(&scope, *none);
  Object ref2(&scope, *none);
  {
    Tuple array1(&scope, runtime.newTuple(10));
    Function func_f(&scope, moduleAt(&runtime, main, "f"));
    WeakRef ref1_inner(&scope, runtime.newWeakRef(thread, array1, func_f));
    ref1 = *ref1_inner;

    Tuple array2(&scope, runtime.newTuple(10));
    Function func_g(&scope, moduleAt(&runtime, main, "g"));
    WeakRef ref2_inner(&scope, runtime.newWeakRef(thread, array2, func_g));
    ref2 = *ref2_inner;

    runtime.collectGarbage();

    EXPECT_EQ(ref1_inner.referent(), *array1);
    EXPECT_EQ(ref2_inner.referent(), *array2);
    SmallInt a(&scope, moduleAt(&runtime, main, "a"));
    SmallInt b(&scope, moduleAt(&runtime, main, "b"));
    EXPECT_EQ(a.value(), 1);
    EXPECT_EQ(b.value(), 2);
  }
  runtime.collectGarbage();

  EXPECT_EQ(RawWeakRef::cast(*ref1).referent(), NoneType::object());
  EXPECT_EQ(RawWeakRef::cast(*ref1).callback(), NoneType::object());
  EXPECT_EQ(RawWeakRef::cast(*ref2).referent(), NoneType::object());
  EXPECT_EQ(RawWeakRef::cast(*ref2).callback(), NoneType::object());
  SmallInt a(&scope, moduleAt(&runtime, main, "a"));
  SmallInt b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(a.value(), 3);
  EXPECT_EQ(b.value(), 4);
}

TEST(ScavengerTest, MixCallback) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  const char* src = R"(
a = 1
b = 2
def f(ref):
  global a
  a = 3
def g(ref, c=4):
  global b
  b = c
)";
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));

  Tuple array1(&scope, runtime.newTuple(10));
  Function func_f(&scope, moduleAt(&runtime, main, "f"));
  WeakRef ref1(&scope, runtime.newWeakRef(thread, array1, func_f));
  Object ref2(&scope, NoneType::object());
  {
    Tuple array2(&scope, runtime.newTuple(10));
    Function func_g(&scope, moduleAt(&runtime, main, "g"));
    WeakRef ref2_inner(&scope, runtime.newWeakRef(thread, array2, func_g));
    ref2 = *ref2_inner;

    runtime.collectGarbage();

    EXPECT_EQ(ref1.referent(), *array1);
    EXPECT_EQ(ref2_inner.referent(), *array2);
    SmallInt a(&scope, moduleAt(&runtime, main, "a"));
    SmallInt b(&scope, moduleAt(&runtime, main, "b"));
    EXPECT_EQ(a.value(), 1);
    EXPECT_EQ(b.value(), 2);
  }
  runtime.collectGarbage();

  EXPECT_EQ(ref1.referent(), *array1);
  EXPECT_EQ(ref1.callback(), *func_f);
  EXPECT_EQ(RawWeakRef::cast(*ref2).referent(), NoneType::object());
  EXPECT_EQ(RawWeakRef::cast(*ref2).callback(), NoneType::object());
  SmallInt a(&scope, moduleAt(&runtime, main, "a"));
  SmallInt b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(a.value(), 1);
  EXPECT_EQ(b.value(), 4);
}

static RawObject doGarbageCollection(Thread* thread, Frame*, word) {
  thread->runtime()->collectGarbage();
  return NoneType::object();
}

TEST(ScavengerTest, CallbackInvokeGC) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  const char* src = R"(
a = 1
def g(ref, b=2):
  global a
  a = b
)";
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object ref1(&scope, NoneType::object());
  Object ref2(&scope, NoneType::object());
  {
    Tuple array1(&scope, runtime.newTuple(10));
    Function collect(&scope, runtime.newFunction());
    collect.setEntry(nativeTrampoline<doGarbageCollection>);

    WeakRef ref1_inner(&scope, runtime.newWeakRef(thread, array1, collect));
    ref1 = *ref1_inner;

    Tuple array2(&scope, runtime.newTuple(10));
    Function func_g(&scope, moduleAt(&runtime, main, "g"));
    WeakRef ref2_inner(&scope, runtime.newWeakRef(thread, array2, func_g));
    ref2 = *ref2_inner;

    runtime.collectGarbage();

    EXPECT_EQ(ref1_inner.referent(), *array1);
    EXPECT_EQ(ref2_inner.referent(), *array2);
    SmallInt a(&scope, moduleAt(&runtime, main, "a"));
    EXPECT_EQ(a.value(), 1);
  }
  runtime.collectGarbage();

  EXPECT_EQ(RawWeakRef::cast(*ref1).referent(), NoneType::object());
  EXPECT_EQ(RawWeakRef::cast(*ref1).callback(), NoneType::object());
  EXPECT_EQ(RawWeakRef::cast(*ref2).referent(), NoneType::object());
  EXPECT_EQ(RawWeakRef::cast(*ref2).callback(), NoneType::object());
  SmallInt a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_EQ(a.value(), 2);
}

TEST(ScavengerTest, IgnoreCallbackException) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  const char* src = R"(
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
)";
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object ref1(&scope, NoneType::object());
  Object ref2(&scope, NoneType::object());
  {
    Tuple array1(&scope, runtime.newTuple(10));
    Function func_f(&scope, moduleAt(&runtime, main, "f"));
    WeakRef ref1_inner(&scope, runtime.newWeakRef(thread, array1, func_f));
    ref1 = *ref1_inner;

    Tuple array2(&scope, runtime.newTuple(10));
    Function func_g(&scope, moduleAt(&runtime, main, "g"));
    WeakRef ref2_inner(&scope, runtime.newWeakRef(thread, array2, func_g));
    ref2 = *ref2_inner;

    runtime.collectGarbage();

    EXPECT_EQ(ref1_inner.referent(), *array1);
    EXPECT_EQ(ref2_inner.referent(), *array2);
    SmallInt a(&scope, moduleAt(&runtime, main, "a"));
    SmallInt b(&scope, moduleAt(&runtime, main, "b"));
    EXPECT_EQ(a.value(), 1);
    EXPECT_EQ(b.value(), 2);
  }

  runtime.collectGarbage();
  EXPECT_FALSE(Thread::current()->hasPendingException());
  EXPECT_EQ(moduleAt(&runtime, main, "callback_ran"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, main, "callback_returned"), Bool::falseObj());

  EXPECT_EQ(RawWeakRef::cast(*ref1).referent(), NoneType::object());
  EXPECT_EQ(RawWeakRef::cast(*ref1).callback(), NoneType::object());
  EXPECT_EQ(RawWeakRef::cast(*ref2).referent(), NoneType::object());
  EXPECT_EQ(RawWeakRef::cast(*ref2).callback(), NoneType::object());
  SmallInt a(&scope, moduleAt(&runtime, main, "a"));
  SmallInt b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(a.value(), 1);
  EXPECT_EQ(b.value(), 4);
}

}  // namespace python
