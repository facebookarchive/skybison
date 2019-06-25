#include "gtest/gtest.h"

#include "mro.h"
#include "test-utils.h"

namespace python {
using namespace testing;

using MroTest = RuntimeFixture;

TEST_F(MroTest, ComputeMroReturnsList) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A: pass
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "A"));
  Type a(&scope, *a_obj);
  Tuple parents(&scope, runtime_.emptyTuple());
  Object result_obj(&scope, computeMro(thread_, a, parents));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), a);
  EXPECT_EQ(result.at(1), runtime_.typeAt(LayoutId::kObject));
}

TEST_F(MroTest, ComputeMroWithTypeSubclassReturnsList) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Meta(type): pass
class A(metaclass=Meta): pass
class B(A): pass
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "A"));
  Object b_obj(&scope, moduleAt(&runtime_, "__main__", "B"));
  Type b(&scope, *b_obj);
  Tuple parents(&scope, runtime_.newTuple(1));
  parents.atPut(0, *a_obj);
  Object result_obj(&scope, computeMro(thread_, b, parents));
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 3);
  EXPECT_EQ(result.at(0), b);
  EXPECT_EQ(result.at(1), a_obj);
  EXPECT_EQ(result.at(2), runtime_.typeAt(LayoutId::kObject));
}

TEST_F(MroTest, ComputeMroWithMultipleTypeSubclassesReturnsList) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Meta(type): pass
class A(metaclass=Meta): pass
class B(metaclass=Meta): pass
class C(A, B): pass
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "A"));
  Object b_obj(&scope, moduleAt(&runtime_, "__main__", "B"));
  Object c_obj(&scope, moduleAt(&runtime_, "__main__", "C"));
  Type c(&scope, *c_obj);
  Tuple parents(&scope, runtime_.newTuple(2));
  parents.atPut(0, *a_obj);
  parents.atPut(1, *b_obj);
  Object result_obj(&scope, computeMro(thread_, c, parents));
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_EQ(result.at(0), c);
  EXPECT_EQ(result.at(1), a_obj);
  EXPECT_EQ(result.at(2), b_obj);
  EXPECT_EQ(result.at(3), runtime_.typeAt(LayoutId::kObject));
}

TEST_F(MroTest, ComputeMroWithIncompatibleBasesRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A: pass
class B(A): pass
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "A"));
  Object b_obj(&scope, moduleAt(&runtime_, "__main__", "B"));
  Type c(&scope, runtime_.newType());
  Tuple parents(&scope, runtime_.newTuple(2));
  parents.atPut(0, *a_obj);
  parents.atPut(1, *b_obj);
  EXPECT_TRUE(raisedWithStr(computeMro(thread_, c, parents),
                            LayoutId::kTypeError,
                            "Cannot create a consistent method resolution "
                            "order (MRO) for bases A, B"));
}

}  // namespace python
