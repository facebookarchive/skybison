#include "gtest/gtest.h"

#include "scavenger.h"

namespace python {

TEST(ScavengerTest, PreserveWeakReferenceHeapReferent) {
  Runtime runtime;
  HandleScope scope;
  Handle<WeakRef> ref(&scope, runtime.newWeakRef());
  Handle<ObjectArray> array(&scope, runtime.newObjectArray(10));
  ref->setReferent(*array);
  Scavenger(&runtime).scavenge();
  EXPECT_EQ(ref->referent(), *array);
}

TEST(ScavengerTest, PreserveWeakReferenceImmediateReferent) {
  Runtime runtime;
  HandleScope scope;
  Handle<WeakRef> ref(&scope, runtime.newWeakRef());
  ref->setReferent(SmallInteger::fromWord(1234));
  Scavenger(&runtime).scavenge();
  EXPECT_EQ(ref->referent(), SmallInteger::fromWord(1234));
}

TEST(ScavengerTest, ClearWeakReference) {
  Runtime runtime;
  HandleScope scope;
  Handle<WeakRef> ref(&scope, runtime.newWeakRef());
  {
    Handle<ObjectArray> array(&scope, runtime.newObjectArray(10));
    ref->setReferent(*array);
    Scavenger(&runtime).scavenge();
    EXPECT_EQ(ref->referent(), *array);
  }
  Scavenger(&runtime).scavenge();
  EXPECT_EQ(ref->referent(), None::object());
}

TEST(ScavengerTest, PreserveSomeClearSomeReferents) {
  Runtime runtime;
  HandleScope scope;

  // Create strongly referenced heap allocated objects.
  Handle<ObjectArray> strongrefs(&scope, runtime.newObjectArray(4));
  for (word i = 0; i < strongrefs->length(); i++) {
    Handle<Double> elt(&scope, runtime.newDouble(i));
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
  Scavenger(&runtime).scavenge();

  // Make sure that the weak references still point to the expected referents.
  EXPECT_EQ(strongrefs->at(0), WeakRef::cast(weakrefs->at(0))->referent());
  EXPECT_EQ(strongrefs->at(1), WeakRef::cast(weakrefs->at(1))->referent());
  EXPECT_EQ(strongrefs->at(2), WeakRef::cast(weakrefs->at(2))->referent());
  EXPECT_EQ(strongrefs->at(3), WeakRef::cast(weakrefs->at(3))->referent());

  // Clear the odd indexed strong references.
  strongrefs->atPut(1, None::object());
  strongrefs->atPut(3, None::object());

  // Now do another garbage collection.  This one should clear just the weak
  // references that point to objects that are no longer strongly referenced.
  Scavenger(&runtime).scavenge();

  // Check that the strongly referenced referents are preserved and the weakly
  // referenced referents are now cleared.
  EXPECT_EQ(strongrefs->at(0), WeakRef::cast(weakrefs->at(0))->referent());
  EXPECT_EQ(None::object(), WeakRef::cast(weakrefs->at(1))->referent());
  EXPECT_EQ(strongrefs->at(2), WeakRef::cast(weakrefs->at(2))->referent());
  EXPECT_EQ(None::object(), WeakRef::cast(weakrefs->at(3))->referent());

  // Clear the even indexed strong references.
  strongrefs->atPut(0, None::object());
  strongrefs->atPut(2, None::object());

  // Do a final garbage collection.  There are no more strongly referenced
  // objects so all of the weak references should be cleared.
  Scavenger(&runtime).scavenge();

  // Check that all of the referents are cleared.
  EXPECT_EQ(None::object(), WeakRef::cast(weakrefs->at(0))->referent());
  EXPECT_EQ(None::object(), WeakRef::cast(weakrefs->at(1))->referent());
  EXPECT_EQ(None::object(), WeakRef::cast(weakrefs->at(2))->referent());
  EXPECT_EQ(None::object(), WeakRef::cast(weakrefs->at(3))->referent());
}

} // namespace python
