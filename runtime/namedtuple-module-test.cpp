#include "gtest/gtest.h"

#include "namedtuple-module.h"
#include "runtime.h"
#include "test-utils.h"
#include "trampolines.h"

namespace python {

using namespace testing;

// These tests are not written in managed code and tested with unittest because
// unittest depends on namedtuple to run.

TEST(NamedtupleModuleTest,
     NamedtupleWithNonIdentifierTypeNameRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, R"(
from _namedtuple import namedtuple
namedtuple('5', ['a', 'b'])
)"),
      LayoutId::kValueError,
      "Type names and field names must be valid identifiers: '5'"));
}

TEST(NamedtupleModuleTest, NamedtupleWithKeywordRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, R"(
from _namedtuple import namedtuple
namedtuple('from', ['a', 'b'])
)"),
                    LayoutId::kValueError,
                    "Type names and field names cannot be a keyword: 'from'"));
}

TEST(NamedtupleModuleTest,
     NamedtupleWithFieldStartingWithUnderscoreRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, R"(
from _namedtuple import namedtuple
namedtuple('Foo', ['_a', 'b'])
)"),
                    LayoutId::kValueError,
                    "Field names cannot start with an underscore: '_a'"));
}

TEST(NamedtupleModuleTest, NamedtupleWithDuplicateFieldNameRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
from _namedtuple import namedtuple
namedtuple('Foo', ['a', 'a'])
)"),
                            LayoutId::kValueError,
                            "Encountered duplicate field name: 'a'"));
}

TEST(NamedtupleModuleTest, NamedtupleUnderMakeWithTooFewArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
from _namedtuple import namedtuple
Foo = namedtuple('Foo', ['a', 'b'])
Foo._make([1])
)"),
                            LayoutId::kTypeError,
                            "Expected 2 arguments, got 1"));
}

TEST(NamedtupleModuleTest, NamedtupleUnderMakeReturnsNewInstance) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
from _namedtuple import namedtuple
Foo = namedtuple('Foo', ['a', 'b'])
inst = Foo._make([1, 2])
result = (inst.a, inst.b)
)")
                   .isError());
  HandleScope scope;
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  EXPECT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
}

TEST(NamedtupleModuleTest,
     NamedtupleUnderReplaceWithNonExistentFieldNameRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
from _namedtuple import namedtuple
Foo = namedtuple('Foo', ['a', 'b'])
Foo(1, 2)._replace(x=4)
)"),
                            LayoutId::kValueError,
                            "Got unexpected field names: {'x': 4}"));
}

TEST(NamedtupleModuleTest, NamedtupleUnderReplaceReplacesValueAtName) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
from _namedtuple import namedtuple
Foo = namedtuple('Foo', ['a', 'b'])
inst = Foo(1, 2)._replace(b=3)
result = (inst.a, inst.b)
)")
                   .isError());
  HandleScope scope;
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  EXPECT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 3));
}

}  // namespace python
