#include "handles.h"

#include <vector>

#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

#include "objects.h"
#include "runtime.h"
#include "test-utils.h"
#include "visitor.h"

// Put `USE()` around expression or variables to avoid unused variable warnings.
#define USE(x) static_cast<void>(x)

namespace py {
namespace testing {

using HandlesTest = RuntimeFixture;

TEST_F(HandlesTest, UpCastTest) {
  HandleScope scope(thread_);

  SmallInt h1(&scope, RawObject{0xFEEDFACEL});

  Object h2(&scope, *h1);
  USE(h2);

  RememberingVisitor visitor;
  scope.handles()->visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 2);
  EXPECT_TRUE(visitor.hasVisited(*h1));
}

TEST_F(HandlesTest, DownCastTest) {
  HandleScope scope(thread_);

  RawObject i1{0xFEEDFACEL};
  Object h1(&scope, i1);

  SmallInt h2(&scope, *h1);
  USE(h2);

  RememberingVisitor visitor;
  scope.handles()->visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 2);
  EXPECT_TRUE(visitor.hasVisited(i1));
}

TEST_F(HandlesTest, IllegalCastRunTimeTest) {
  HandleScope scope(thread_);

  RawObject i1{0xFEEDFACEL};
  Object h1(&scope, i1);

  EXPECT_DEBUG_DEATH(USE(Dict(&scope, *h1)), "Invalid Handle construction");
}

TEST_F(HandlesTest, ThreadSubclassTest) {
  HandleScope scope(thread_);

  Layout layout(&scope, runtime_->layoutAt(LayoutId::kStopIteration));
  BaseException exn(&scope, runtime_->newInstance(layout));
  EXPECT_TRUE(exn.isStopIteration());
}

TEST_F(HandlesTest, IllegalCastWithThreadTest) {
  HandleScope scope(thread_);

  EXPECT_DEBUG_DEATH(USE(BaseException(&scope, SmallInt::fromWord(123))),
                     "Invalid Handle construction");
}

TEST_F(HandlesTest, VisitNoScopes) {
  Handles handles;
  RememberingVisitor visitor;
  handles.visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 0);
}

TEST_F(HandlesTest, VisitEmptyScope) {
  HandleScope scope(thread_);
  RememberingVisitor visitor;
  scope.handles()->visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 0);
}

TEST_F(HandlesTest, VisitOneHandle) {
  HandleScope scope(thread_);
  RawObject object{0xFEEDFACEL};
  Object handle(&scope, object);
  USE(handle);
  RememberingVisitor visitor;
  scope.handles()->visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 1);
  EXPECT_TRUE(visitor.hasVisited(object));
}

TEST_F(HandlesTest, VisitTwoHandles) {
  HandleScope scope(thread_);
  RememberingVisitor visitor;
  RawObject o1{0xFEEDFACEL};
  Object h1(&scope, o1);
  USE(h1);
  RawObject o2{0xFACEFEEDL};
  Object h2(&scope, o2);
  USE(h2);
  scope.handles()->visitPointers(&visitor);
  EXPECT_EQ(visitor.count(), 2);
  EXPECT_TRUE(visitor.hasVisited(o1));
  EXPECT_TRUE(visitor.hasVisited(o2));
}

TEST_F(HandlesTest, VisitObjectInNestedScope) {
  Handles* handles = Thread::current()->handles();

  RawObject object{0xFEEDFACEL};
  {
    HandleScope s1;
    USE(s1);
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
      USE(handle);
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

TEST_F(HandlesTest, NestedScopes) {
  Handles* handles = Thread::current()->handles();

  RawObject o1{0xDECAF1L};
  RawObject o2{0xDECAF2L};
  RawObject o3{0xDECAF3L};

  {
    HandleScope s1;
    Object h1(&s1, o1);
    USE(h1);
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
      USE(h2);
      {
        // Check s2 for o1 and o2.
        RememberingVisitor visitor;
        handles->visitPointers(&visitor);
        EXPECT_EQ(visitor.count(), 2);
        EXPECT_TRUE(visitor.hasVisited(o1));
        EXPECT_TRUE(visitor.hasVisited(o2));
        EXPECT_FALSE(visitor.hasVisited(o3));
      }
    }
    // (Scope 2 is now popped.)
    {
      // Push scope 3 (at the depth previously occupied by s2).
      HandleScope s3;
      Object h3(&s3, o3);
      USE(h3);
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
  HandleScope scope(Thread::current());

  RawObject o1{0xFEEDFACEL};

  for (auto _ : state) {
    Object h1(&scope, o1);
    USE(h1);
  }
}

class NothingVisitor : public PointerVisitor {
 public:
  virtual void visitPointer(RawObject*) { visit_count++; }

  int visit_count = 0;
};

BENCHMARK_F(HandleBenchmark, Visit)(benchmark::State& state) {
  Handles handles;
  HandleScope scope(Thread::current());

  Object h1(&scope, RawObject{0xFEEDFACEL});
  Object h2(&scope, RawObject{0xFEEDFACFL});
  Object h3(&scope, RawObject{0xFEEDFAD0L});
  Object h4(&scope, RawObject{0xFEEDFAD1L});
  Object h5(&scope, RawObject{0xFEEDFAD2L});
  Object h6(&scope, RawObject{0xFEEDFAD3L});
  Object h7(&scope, RawObject{0xFEEDFAD4L});
  Object h8(&scope, RawObject{0xFEEDFAD5L});
  Object h9(&scope, RawObject{0xFEEDFAD6L});
  USE(h1);
  USE(h2);
  USE(h3);
  USE(h4);
  USE(h5);
  USE(h6);
  USE(h7);
  USE(h8);
  USE(h9);

  NothingVisitor visitor;
  for (auto _ : state) {
    handles.visitPointers(&visitor);
  }
}

}  // namespace testing
}  // namespace py
