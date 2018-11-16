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
  HandleScope scope;
  Handle<WeakRef> ref(&scope, runtime.newWeakRef());
  Handle<ObjectArray> array(&scope, runtime.newObjectArray(10));
  ref->setReferent(*array);
  runtime.collectGarbage();
  EXPECT_EQ(ref->referent(), *array);
}

TEST(ScavengerTest, PreserveWeakReferenceImmediateReferent) {
  Runtime runtime;
  HandleScope scope;
  Handle<WeakRef> ref(&scope, runtime.newWeakRef());
  ref->setReferent(SmallInt::fromWord(1234));
  runtime.collectGarbage();
  EXPECT_EQ(ref->referent(), SmallInt::fromWord(1234));
}

TEST(ScavengerTest, ClearWeakReference) {
  Runtime runtime;
  HandleScope scope;
  Handle<WeakRef> ref(&scope, runtime.newWeakRef());
  {
    Handle<ObjectArray> array(&scope, runtime.newObjectArray(10));
    ref->setReferent(*array);
    runtime.collectGarbage();
    EXPECT_EQ(ref->referent(), *array);
  }
  runtime.collectGarbage();
  EXPECT_EQ(ref->referent(), NoneType::object());
}

TEST(ScavengerTest, PreserveSomeClearSomeReferents) {
  Runtime runtime;
  HandleScope scope;

  // Create strongly referenced heap allocated objects.
  Handle<ObjectArray> strongrefs(&scope, runtime.newObjectArray(4));
  for (word i = 0; i < strongrefs->length(); i++) {
    Handle<Float> elt(&scope, runtime.newFloat(i));
    strongrefs->atPut(i, *elt);
  }

  // Create a parallel array of weak references with the strongly referenced
  // objects as referents.
  Handle<ObjectArray> weakrefs(&scope, runtime.newObjectArray(4));
  for (word i = 0; i < weakrefs->length(); i++) {
    Handle<WeakRef> elt(&scope, runtime.newWeakRef());
    elt->setReferent(strongrefs->at(i));
    weakrefs->atPut(i, *elt);
  }

  // Make sure the weak references point to the expected referents.
  EXPECT_EQ(strongrefs->at(0), WeakRef::cast(weakrefs->at(0))->referent());
  EXPECT_EQ(strongrefs->at(1), WeakRef::cast(weakrefs->at(1))->referent());
  EXPECT_EQ(strongrefs->at(2), WeakRef::cast(weakrefs->at(2))->referent());
  EXPECT_EQ(strongrefs->at(3), WeakRef::cast(weakrefs->at(3))->referent());

  // Now do a garbage collection.
  runtime.collectGarbage();

  // Make sure that the weak references still point to the expected referents.
  EXPECT_EQ(strongrefs->at(0), WeakRef::cast(weakrefs->at(0))->referent());
  EXPECT_EQ(strongrefs->at(1), WeakRef::cast(weakrefs->at(1))->referent());
  EXPECT_EQ(strongrefs->at(2), WeakRef::cast(weakrefs->at(2))->referent());
  EXPECT_EQ(strongrefs->at(3), WeakRef::cast(weakrefs->at(3))->referent());

  // Clear the odd indexed strong references.
  strongrefs->atPut(1, NoneType::object());
  strongrefs->atPut(3, NoneType::object());
  runtime.collectGarbage();

  // Now do another garbage collection.  This one should clear just the weak
  // references that point to objects that are no longer strongly referenced.

  // Check that the strongly referenced referents are preserved and the weakly
  // referenced referents are now cleared.
  EXPECT_EQ(strongrefs->at(0), WeakRef::cast(weakrefs->at(0))->referent());
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs->at(1))->referent());
  EXPECT_EQ(strongrefs->at(2), WeakRef::cast(weakrefs->at(2))->referent());
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs->at(3))->referent());

  // Clear the even indexed strong references.
  strongrefs->atPut(0, NoneType::object());
  strongrefs->atPut(2, NoneType::object());

  // Do a final garbage collection.  There are no more strongly referenced
  // objects so all of the weak references should be cleared.
  runtime.collectGarbage();

  // Check that all of the referents are cleared.
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs->at(0))->referent());
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs->at(1))->referent());
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs->at(2))->referent());
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs->at(3))->referent());
}

TEST(ScavengerTest, BaseCallback) {
  Runtime runtime;
  HandleScope scope;
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
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<WeakRef> ref1(&scope, runtime.newWeakRef());
  Handle<WeakRef> ref2(&scope, runtime.newWeakRef());
  {
    Handle<ObjectArray> array1(&scope, runtime.newObjectArray(10));
    Handle<Function> func_f(&scope, moduleAt(&runtime, main, "f"));
    ref1->setReferent(*array1);
    ref1->setCallback(*func_f);

    Handle<ObjectArray> array2(&scope, runtime.newObjectArray(10));
    Handle<Function> func_g(&scope, moduleAt(&runtime, main, "g"));
    ref2->setReferent(*array2);
    ref2->setCallback(*func_g);

    runtime.collectGarbage();

    EXPECT_EQ(ref1->referent(), *array1);
    EXPECT_EQ(ref2->referent(), *array2);
    Handle<SmallInt> a(&scope, moduleAt(&runtime, main, "a"));
    Handle<SmallInt> b(&scope, moduleAt(&runtime, main, "b"));
    EXPECT_EQ(a->value(), 1);
    EXPECT_EQ(b->value(), 2);
  }
  runtime.collectGarbage();

  EXPECT_EQ(ref1->referent(), NoneType::object());
  EXPECT_EQ(ref1->callback(), NoneType::object());
  EXPECT_EQ(ref2->referent(), NoneType::object());
  EXPECT_EQ(ref2->callback(), NoneType::object());
  Handle<SmallInt> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<SmallInt> b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(a->value(), 3);
  EXPECT_EQ(b->value(), 4);
}

TEST(ScavengerTest, MixCallback) {
  Runtime runtime;
  HandleScope scope;
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
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));

  Handle<WeakRef> ref1(&scope, runtime.newWeakRef());
  Handle<WeakRef> ref2(&scope, runtime.newWeakRef());
  Handle<ObjectArray> array1(&scope, runtime.newObjectArray(10));
  Handle<Function> func_f(&scope, moduleAt(&runtime, main, "f"));
  ref1->setReferent(*array1);
  ref1->setCallback(*func_f);
  {
    Handle<ObjectArray> array2(&scope, runtime.newObjectArray(10));
    Handle<Function> func_g(&scope, moduleAt(&runtime, main, "g"));
    ref2->setReferent(*array2);
    ref2->setCallback(*func_g);

    runtime.collectGarbage();

    EXPECT_EQ(ref1->referent(), *array1);
    EXPECT_EQ(ref2->referent(), *array2);
    Handle<SmallInt> a(&scope, moduleAt(&runtime, main, "a"));
    Handle<SmallInt> b(&scope, moduleAt(&runtime, main, "b"));
    EXPECT_EQ(a->value(), 1);
    EXPECT_EQ(b->value(), 2);
  }
  runtime.collectGarbage();

  EXPECT_EQ(ref1->referent(), *array1);
  EXPECT_EQ(ref1->callback(), *func_f);
  EXPECT_EQ(ref2->referent(), NoneType::object());
  EXPECT_EQ(ref2->callback(), NoneType::object());
  Handle<SmallInt> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<SmallInt> b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(a->value(), 1);
  EXPECT_EQ(b->value(), 4);
}

static RawObject doGarbageCollection(Thread* thread, Frame*, word) {
  thread->runtime()->collectGarbage();
  return NoneType::object();
}

TEST(ScavengerTest, CallbackInvokeGC) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
a = 1
def g(ref, b=2):
  global a
  a = b
)";
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<WeakRef> ref1(&scope, runtime.newWeakRef());
  Handle<WeakRef> ref2(&scope, runtime.newWeakRef());
  {
    Handle<ObjectArray> array1(&scope, runtime.newObjectArray(10));
    Handle<Function> collect(&scope, runtime.newFunction());
    collect->setEntry(nativeTrampoline<doGarbageCollection>);

    ref1->setReferent(*array1);
    ref1->setCallback(*collect);

    Handle<ObjectArray> array2(&scope, runtime.newObjectArray(10));
    Handle<Function> func_g(&scope, moduleAt(&runtime, main, "g"));
    ref2->setReferent(*array2);
    ref2->setCallback(*func_g);

    runtime.collectGarbage();

    EXPECT_EQ(ref1->referent(), *array1);
    EXPECT_EQ(ref2->referent(), *array2);
    Handle<SmallInt> a(&scope, moduleAt(&runtime, main, "a"));
    EXPECT_EQ(a->value(), 1);
  }
  runtime.collectGarbage();

  EXPECT_EQ(ref1->referent(), NoneType::object());
  EXPECT_EQ(ref1->callback(), NoneType::object());
  EXPECT_EQ(ref2->referent(), NoneType::object());
  EXPECT_EQ(ref2->callback(), NoneType::object());
  Handle<SmallInt> a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_EQ(a->value(), 2);
}

TEST(ScavengerTest, IgnoreCallbackException) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
a = 1
b = 2
def f():
  global a
  a = 3
def g(ref, c=4):
  global b
  b = c
)";
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<WeakRef> ref1(&scope, runtime.newWeakRef());
  Handle<WeakRef> ref2(&scope, runtime.newWeakRef());
  {
    Handle<ObjectArray> array1(&scope, runtime.newObjectArray(10));
    Handle<Function> func_f(&scope, moduleAt(&runtime, main, "f"));
    ref1->setReferent(*array1);
    ref1->setCallback(*func_f);

    Handle<ObjectArray> array2(&scope, runtime.newObjectArray(10));
    Handle<Function> func_g(&scope, moduleAt(&runtime, main, "g"));
    ref2->setReferent(*array2);
    ref2->setCallback(*func_g);

    runtime.collectGarbage();

    EXPECT_EQ(ref1->referent(), *array1);
    EXPECT_EQ(ref2->referent(), *array2);
    Handle<SmallInt> a(&scope, moduleAt(&runtime, main, "a"));
    Handle<SmallInt> b(&scope, moduleAt(&runtime, main, "b"));
    EXPECT_EQ(a->value(), 1);
    EXPECT_EQ(b->value(), 2);
  }

  std::stringstream tmp_stdout;
  std::ostream* saved_stdout = builtinStderr;
  builtinStderr = &tmp_stdout;
  runtime.collectGarbage();
  builtinStderr = saved_stdout;
  std::string exception = tmp_stdout.str();
  EXPECT_EQ(exception,
            "ignore pending exception: TypeError: too many arguments\n");

  EXPECT_EQ(ref1->referent(), NoneType::object());
  EXPECT_EQ(ref1->callback(), NoneType::object());
  EXPECT_EQ(ref2->referent(), NoneType::object());
  EXPECT_EQ(ref2->callback(), NoneType::object());
  Handle<SmallInt> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<SmallInt> b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(a->value(), 1);
  EXPECT_EQ(b->value(), 4);
}

}  // namespace python
