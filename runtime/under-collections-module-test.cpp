#include "under-collections-module.h"

#include "gtest/gtest.h"

#include "builtins-module.h"
#include "builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {

namespace testing {

using UnderCollectionsModuleTest = RuntimeFixture;

TEST_F(UnderCollectionsModuleTest, DunderNewConstructsDeque) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->typeAt(LayoutId::kDeque));
  Object result(&scope, runBuiltin(METH(deque, __new__), type));
  ASSERT_TRUE(result.isDeque());
  Deque deque(&scope, *result);
  EXPECT_EQ(deque.left(), 0);
  EXPECT_EQ(deque.numItems(), 0);
  EXPECT_EQ(deque.capacity(), 0);
  EXPECT_EQ(deque.items(), SmallInt::fromWord(0));
}

TEST_F(UnderCollectionsModuleTest, DequeAppendInsertsElementToEnd) {
  HandleScope scope(thread_);
  Deque self(&scope, runtime_->newDeque());
  // Test underlying array growth
  for (int i = 0; i < 30; i++) {
    Object value(&scope, SmallInt::fromWord(i));
    Object result(&scope, runBuiltin(METH(deque, append), self, value));
    EXPECT_EQ(*result, NoneType::object());
  }

  EXPECT_EQ(self.numItems(), 30);
  for (int i = 0; i < 30; i++) {
    EXPECT_TRUE(isIntEqualsWord(self.at(i), i)) << i;
  }
}

TEST_F(UnderCollectionsModuleTest, DequeAppendleftInsertsElementToFront) {
  HandleScope scope(thread_);
  Deque self(&scope, runtime_->newDeque());
  Object value(&scope, SmallInt::fromWord(1));
  Object value1(&scope, SmallInt::fromWord(2));
  runBuiltin(METH(deque, appendleft), self, value);
  Object result(&scope, runBuiltin(METH(deque, appendleft), self, value1));
  EXPECT_EQ(*result, NoneType::object());

  EXPECT_EQ(self.numItems(), 2);
  EXPECT_TRUE(isIntEqualsWord(self.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(self.at(self.capacity() - 1), 2));
}

TEST_F(UnderCollectionsModuleTest, EmptyDequeInvariants) {
  HandleScope scope(thread_);
  Deque deque(&scope, runtime_->newDeque());
  EXPECT_EQ(deque.left(), 0);
  EXPECT_EQ(deque.numItems(), 0);
  EXPECT_EQ(deque.capacity(), 0);
  EXPECT_EQ(deque.items(), SmallInt::fromWord(0));
}

}  // namespace testing

}  // namespace py
