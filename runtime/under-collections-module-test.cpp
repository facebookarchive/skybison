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
}

}  // namespace testing

}  // namespace py
