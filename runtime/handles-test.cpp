#include "gtest/gtest.h"

#include <vector>

#include "handles.h"
#include "objects.h"
#include "visitor.h"

namespace python {

class RememberingVisitor : public PointerVisitor {
 public:
  virtual void visitPointer(Object** pointer) {
    pointers_.push_back(*pointer);
  }

  word count() {
    return pointers_.size();
  }

  bool hasVisited(Object* object) {
    for (auto elt : pointers_) {
      if (elt == object) {
        return true;
      }
    }
    return false;
  }

 private:
  std::vector<Object*> pointers_;
};

TEST(HandlesTest, VisitNoScopes) {
  Handles handles;
  RememberingVisitor visitor;
  handles.visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 0);
}

TEST(HandlesTest, VisitEmptyScope) {
  Handles handles;
  HandleScope scope(&handles);
  RememberingVisitor visitor;
  handles.visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 0);
}

TEST(HandlesTest, VisitOneHandle) {
  Handles handles;
  HandleScope scope(&handles);
  auto object = reinterpret_cast<Object*>(0xFEEDFACE);
  Handle<Object> handle(&scope, object);
  RememberingVisitor visitor;
  handles.visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 1);
  EXPECT_TRUE(visitor.hasVisited(object));
}

TEST(HandlesTest, VisitTwoHandles) {
  Handles handles;
  HandleScope scope(&handles);
  RememberingVisitor visitor;
  auto o1 = reinterpret_cast<Object*>(0xFEEDFACE);
  Handle<Object> h1(&scope, o1);
  auto o2 = reinterpret_cast<Object*>(0xFACEFEED);
  Handle<Object> h2(&scope, o2);
  handles.visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 2);
  EXPECT_TRUE(visitor.hasVisited(o1));
  EXPECT_TRUE(visitor.hasVisited(o2));
}

TEST(HandlesTest, VisitObjectInNestedScope) {
  Handles handles;
  auto object = reinterpret_cast<Object*>(0xFEEDFACE);
  {
    HandleScope s1(&handles);
    {
      // No handles have been created so s1 should be empty.
      RememberingVisitor visitor;
      handles.visitPointers(&visitor);
      EXPECT_EQ(visitor.count(), 0);
      EXPECT_FALSE(visitor.hasVisited(object));
    }
    {
      HandleScope s2(&handles);
      Handle<Object> handle(&s2, object);
      {
        // Check that one handle has been allocated (in the inner scope).
        RememberingVisitor visitor;
        handles.visitPointers(&visitor);
        EXPECT_EQ(visitor.count(), 1);
        EXPECT_TRUE(visitor.hasVisited(object));
      }
    }
    {
      // No handles should be present now the s2 has been popped.
      RememberingVisitor visitor;
      handles.visitPointers(&visitor);
      EXPECT_EQ(visitor.count(), 0);
      EXPECT_FALSE(visitor.hasVisited(object));
    }
  }
  {
    // All scopes are gone so there should still be no handles.
    RememberingVisitor visitor;
    handles.visitPointers(&visitor);
    EXPECT_EQ(visitor.count(), 0);
    EXPECT_FALSE(visitor.hasVisited(object));
  }
}

TEST(HandlesTest, NestedScopes) {
  Handles handles;
  auto o1 = reinterpret_cast<Object*>(0xDECAF1);
  auto o2 = reinterpret_cast<Object*>(0xDECAF2);
  auto o3 = reinterpret_cast<Object*>(0xDECAF3);

  {
    HandleScope s1(&handles);
    Handle<Object> h1(&s1, o1);
    {
      // Check scope s1 for objects o1.
      RememberingVisitor visitor;
      handles.visitPointers(&visitor);
      EXPECT_EQ(visitor.count(), 1);
      EXPECT_TRUE(visitor.hasVisited(o1));
      EXPECT_FALSE(visitor.hasVisited(o2));
      EXPECT_FALSE(visitor.hasVisited(o3));
    }
    {
      // Push scope 2.
      HandleScope s2(&handles);
      Handle<Object> h2(&s2, o2);
      {
        // Check s2 for o1 and o2.
        RememberingVisitor visitor;
        handles.visitPointers(&visitor);
        EXPECT_EQ(visitor.count(), 2);
        EXPECT_TRUE(visitor.hasVisited(o1));
        EXPECT_TRUE(visitor.hasVisited(o2));
        EXPECT_FALSE(visitor.hasVisited(o3));
      }
    }
    // (Scope 2 is now popped.)
    {
      // Push scope 3 (at the depth previously occupied by s2).
      HandleScope s3(&handles);
      Handle<Object> h3(&s3, o3);
      {
        // Check s2 for o1 and o3 (but not o2).
        RememberingVisitor visitor;
        handles.visitPointers(&visitor);
        EXPECT_EQ(visitor.count(), 2);
        EXPECT_TRUE(visitor.hasVisited(o1));
        EXPECT_FALSE(visitor.hasVisited(o2));
        EXPECT_TRUE(visitor.hasVisited(o3));
      }
    }
    // (Scope 3 is now popped.)
    {
      // Check scope s1 for o1.
      RememberingVisitor visitor;
      handles.visitPointers(&visitor);
      EXPECT_EQ(visitor.count(), 1);
      EXPECT_TRUE(visitor.hasVisited(o1));
      EXPECT_FALSE(visitor.hasVisited(o2));
      EXPECT_FALSE(visitor.hasVisited(o3));
    }
  }
  // (Scope 1 is now popped)
  {
    // All of the handles should be gone.
    RememberingVisitor visitor;
    handles.visitPointers(&visitor);
    EXPECT_EQ(visitor.count(), 0);
    EXPECT_FALSE(visitor.hasVisited(o1));
    EXPECT_FALSE(visitor.hasVisited(o2));
    EXPECT_FALSE(visitor.hasVisited(o3));
  }
}

} // namespace python
