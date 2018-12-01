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
  Runtime runtime;
  HandleScope scope;

  SmallInt h1(&scope, RawObject{0xFEEDFACEL});

  Object h2(&scope, *h1);

  RememberingVisitor visitor;
  scope.handles()->visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 2);
  EXPECT_TRUE(visitor.hasVisited(*h1));
}

TEST(HandlesTest, DownCastTest) {
  Runtime runtime;
  HandleScope scope;

  RawObject i1{0xFEEDFACEL};
  Object h1(&scope, i1);

  SmallInt h2(&scope, *h1);

  RememberingVisitor visitor;
  scope.handles()->visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 2);
  EXPECT_TRUE(visitor.hasVisited(i1));
}

TEST(HandlesTest, IllegalCastRunTimeTest) {
  Runtime runtime;
  HandleScope scope;

  RawObject i1{0xFEEDFACEL};
  Object h1(&scope, i1);

  EXPECT_DEBUG_DEATH(Dict h2(&scope, *h1), "Invalid Handle construction");
}

TEST(HandlesTest, ThreadSubclassTest) {
  Runtime runtime;
  HandleScope scope;

  Layout layout(&scope, runtime.layoutAt(LayoutId::kStopIteration));
  BaseException exn(&scope, runtime.newInstance(layout));
  EXPECT_TRUE(exn.isStopIteration());
}

TEST(HandlesTest, IllegalCastWithThreadTest) {
  Runtime runtime;
  HandleScope scope;

  EXPECT_DEBUG_DEATH(BaseException(&scope, SmallInt::fromWord(123)),
                     "Invalid Handle construction");
}

TEST(HandlesTest, VisitNoScopes) {
  Handles handles;
  RememberingVisitor visitor;
  handles.visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 0);
}

TEST(HandlesTest, VisitEmptyScope) {
  Runtime runtime;
  HandleScope scope;
  RememberingVisitor visitor;
  scope.handles()->visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 0);
}

TEST(HandlesTest, VisitOneHandle) {
  Runtime runtime;
  HandleScope scope;
  RawObject object{0xFEEDFACEL};
  Object handle(&scope, object);
  RememberingVisitor visitor;
  scope.handles()->visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 1);
  EXPECT_TRUE(visitor.hasVisited(object));
}

TEST(HandlesTest, VisitTwoHandles) {
  Runtime runtime;
  HandleScope scope;
  RememberingVisitor visitor;
  RawObject o1{0xFEEDFACEL};
  Object h1(&scope, o1);
  RawObject o2{0xFACEFEEDL};
  Object h2(&scope, o2);
  scope.handles()->visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 2);
  EXPECT_TRUE(visitor.hasVisited(o1));
  EXPECT_TRUE(visitor.hasVisited(o2));
}

TEST(HandlesTest, VisitObjectInNestedScope) {
  Runtime runtime;
  Handles* handles = Thread::currentThread()->handles();

  RawObject object{0xFEEDFACEL};
  {
    HandleScope s1;
    {
      // No handles have been created so s1 should be empty.
      RememberingVisitor visitor;
      handles->visitPointers(&visitor);
      EXPECT_EQ(visitor.count(), 0);
      EXPECT_FALSE(visitor.hasVisited(object));
    }
    {
      HandleScope s2;
      Object handle(&s2, object);
      {
        // Check that one handle has been allocated (in the inner scope).
        RememberingVisitor visitor;
        handles->visitPointers(&visitor);
        EXPECT_EQ(visitor.count(), 1);
        EXPECT_TRUE(visitor.hasVisited(object));
      }
    }
    {
      // No handles should be present now the s2 has been popped.
      RememberingVisitor visitor;
      handles->visitPointers(&visitor);
      EXPECT_EQ(visitor.count(), 0);
      EXPECT_FALSE(visitor.hasVisited(object));
    }
  }
  {
    // All scopes are gone so there should still be no handles.
    RememberingVisitor visitor;
    handles->visitPointers(&visitor);
    EXPECT_EQ(visitor.count(), 0);
    EXPECT_FALSE(visitor.hasVisited(object));
  }
}

TEST(HandlesTest, NestedScopes) {
  Runtime runtime;
  Handles* handles = Thread::currentThread()->handles();

  RawObject o1{0xDECAF1L};
  RawObject o2{0xDECAF2L};
  RawObject o3{0xDECAF3L};

  {
    HandleScope s1;
    Object h1(&s1, o1);
    {
      // Check scope s1 for objects o1.
      RememberingVisitor visitor;
      handles->visitPointers(&visitor);
      EXPECT_EQ(visitor.count(), 1);
      EXPECT_TRUE(visitor.hasVisited(o1));
      EXPECT_FALSE(visitor.hasVisited(o2));
      EXPECT_FALSE(visitor.hasVisited(o3));
    }
    {
      // Push scope 2.
      HandleScope s2;
      Object h2(&s2, o2);
      {
        // Check s2 for o1 and o2.
        RememberingVisitor visitor;
        handles->visitPointers(&visitor);
        EXPECT_EQ(visitor.count(), 2);
        EXPECT_TRUE(visitor.hasVisited(o1));
        EXPECT_TRUE(visitor.hasVisited(o2));
        EXPECT_FALSE(visitor.hasVisited(o3));
      }
      // Verify abort if handle is created with s1.
      EXPECT_DEBUG_DEATH(Object h3(&s1, o3), "unexpected");
    }
    // (Scope 2 is now popped.)
    {
      // Push scope 3 (at the depth previously occupied by s2).
      HandleScope s3;
      Object h3(&s3, o3);
      {
        // Check s2 for o1 and o3 (but not o2).
        RememberingVisitor visitor;
        handles->visitPointers(&visitor);
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
      handles->visitPointers(&visitor);
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
    handles->visitPointers(&visitor);
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

  RawObject o1{0xFEEDFACEL};

  for (auto _ : state) {
    Object h1(&scope, o1);
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

  Object h1(&scope, RawObject{0xFEEDFACEL});
  Object h2(&scope, RawObject{0xFEEDFACFL});
  Object h3(&scope, RawObject{0xFEEDFAD0L});
  Object h4(&scope, RawObject{0xFEEDFAD1L});
  Object h5(&scope, RawObject{0xFEEDFAD2L});
  Object h6(&scope, RawObject{0xFEEDFAD3L});
  Object h7(&scope, RawObject{0xFEEDFAD4L});
  Object h8(&scope, RawObject{0xFEEDFAD5L});
  Object h9(&scope, RawObject{0xFEEDFAD6L});

  NothingVisitor visitor;
  for (auto _ : state) {
    handles.visitPointers(&visitor);
  }
}

}  // namespace python
