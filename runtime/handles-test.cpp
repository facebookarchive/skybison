#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

#include <vector>

#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "visitor.h"

namespace python {

class RememberingVisitor : public PointerVisitor {
 public:
  virtual void visitPointer(RawObject* pointer) {
    pointers_.push_back(*pointer);
  }

  word count() { return pointers_.size(); }

  bool hasVisited(RawObject object) {
    for (auto elt : pointers_) {
      if (elt == object) {
        return true;
      }
    }
    return false;
  }

 private:
  std::vector<RawObject> pointers_;
};

TEST(HandlesTest, UpCastTest) {
  Handles handles;
  HandleScope scope(&handles);

  Handle<SmallInt> h1(&scope, bit_cast<RawSmallInt>(0xFEEDFACEL));

  Handle<Object> h2(h1);

  RememberingVisitor visitor;
  handles.visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 2);
  EXPECT_TRUE(visitor.hasVisited(*h1));
}

TEST(HandlesTest, DownCastTest) {
  Handles handles;
  HandleScope scope(&handles);

  auto i1 = bit_cast<RawSmallInt>(0xFEEDFACEL);
  Handle<Object> h1(&scope, i1);

  Handle<SmallInt> h2(h1);

  RememberingVisitor visitor;
  handles.visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 2);
  EXPECT_TRUE(visitor.hasVisited(i1));
}

TEST(HandlesTest, IllegalCastRunTimeTest) {
  Handles handles;
  HandleScope scope(&handles);

  auto i1 = bit_cast<RawSmallInt>(0xFEEDFACEL);
  Handle<Object> h1(&scope, i1);

  EXPECT_DEBUG_DEATH(Handle<Dict> h2(h1), "isDict");
}

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
  auto object = bit_cast<RawObject>(0xFEEDFACEL);
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
  auto o1 = bit_cast<RawObject>(0xFEEDFACEL);
  Handle<Object> h1(&scope, o1);
  auto o2 = bit_cast<RawObject>(0xFACEFEEDL);
  Handle<Object> h2(&scope, o2);
  handles.visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 2);
  EXPECT_TRUE(visitor.hasVisited(o1));
  EXPECT_TRUE(visitor.hasVisited(o2));
}

TEST(HandlesTest, VisitObjectInNestedScope) {
  Handles handles;
  auto object = bit_cast<RawObject>(0xFEEDFACEL);
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
  auto o1 = bit_cast<RawObject>(0xDECAF1L);
  auto o2 = bit_cast<RawObject>(0xDECAF2L);
  auto o3 = bit_cast<RawObject>(0xDECAF3L);

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
      // Verify abort if handle is created with s1.
      EXPECT_DEBUG_DEATH(Handle<Object> h3(&s1, o3), "unexpected");
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

// Benchmarks
class HandleBenchmark : public benchmark::Fixture {
 public:
  void SetUp(benchmark::State&) { runtime_ = new Runtime; }

  void TearDown(benchmark::State&) { delete runtime_; }

 private:
  Runtime* runtime_;
};

BENCHMARK_F(HandleBenchmark, CreationDestruction)(benchmark::State& state) {
  Handles handles;
  HandleScope scope(Thread::currentThread());

  auto o1 = bit_cast<RawObject>(0xFEEDFACEL);

  for (auto _ : state) {
    Handle<Object> h1(&scope, o1);
  }
}

class NothingVisitor : public PointerVisitor {
 public:
  virtual void visitPointer(RawObject*) { visit_count++; }

  int visit_count = 0;
};

BENCHMARK_F(HandleBenchmark, Visit)(benchmark::State& state) {
  Handles handles;
  HandleScope scope(Thread::currentThread());

  Handle<Object> h1(&scope, bit_cast<RawObject>(0xFEEDFACEL));
  Handle<Object> h2(&scope, bit_cast<RawObject>(0xFEEDFACFL));
  Handle<Object> h3(&scope, bit_cast<RawObject>(0xFEEDFAD0L));
  Handle<Object> h4(&scope, bit_cast<RawObject>(0xFEEDFAD1L));
  Handle<Object> h5(&scope, bit_cast<RawObject>(0xFEEDFAD2L));
  Handle<Object> h6(&scope, bit_cast<RawObject>(0xFEEDFAD3L));
  Handle<Object> h7(&scope, bit_cast<RawObject>(0xFEEDFAD4L));
  Handle<Object> h8(&scope, bit_cast<RawObject>(0xFEEDFAD5L));
  Handle<Object> h9(&scope, bit_cast<RawObject>(0xFEEDFAD6L));

  NothingVisitor visitor;
  for (auto _ : state) {
    handles.visitPointers(&visitor);
  }
}

}  // namespace python
