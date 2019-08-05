#include "gtest/gtest.h"

#include "namedtuple-module.h"
#include "runtime.h"
#include "test-utils.h"
#include "trampolines.h"

namespace python {

using namespace testing;

using NamedtupleModuleTest = RuntimeFixture;

// These tests are not written in managed code and tested with unittest because
// unittest depends on namedtuple to run.

TEST_F(NamedtupleModuleTest,
       NamedtupleWithNonIdentifierTypeNameRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, R"(
from _namedtuple import namedtuple
namedtuple('5', ['a', 'b'])
)"),
      LayoutId::kValueError,
      "Type names and field names must be valid identifiers: '5'"));
}

TEST_F(NamedtupleModuleTest, NamedtupleWithKeywordRaisesValueError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, R"(
from _namedtuple import namedtuple
namedtuple('from', ['a', 'b'])
)"),
                    LayoutId::kValueError,
                    "Type names and field names cannot be a keyword: 'from'"));
}

TEST_F(NamedtupleModuleTest,
       NamedtupleWithFieldStartingWithUnderscoreRaisesValueError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, R"(
from _namedtuple import namedtuple
namedtuple('Foo', ['_a', 'b'])
)"),
                    LayoutId::kValueError,
                    "Field names cannot start with an underscore: '_a'"));
}

TEST_F(NamedtupleModuleTest, NamedtupleWithDuplicateFieldNameRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
from _namedtuple import namedtuple
namedtuple('Foo', ['a', 'a'])
)"),
                            LayoutId::kValueError,
                            "Encountered duplicate field name: 'a'"));
}

TEST_F(NamedtupleModuleTest, NamedtupleUnderMakeWithTooFewArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
from _namedtuple import namedtuple
Foo = namedtuple('Foo', ['a', 'b'])
Foo._make([1])
)"),
                            LayoutId::kTypeError,
                            "Expected 2 arguments, got 1"));
}

TEST_F(NamedtupleModuleTest, NamedtupleUnderMakeReturnsNewInstance) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
from _namedtuple import namedtuple
Foo = namedtuple('Foo', ['a', 'b'])
inst = Foo._make([1, 2])
result = (inst.a, inst.b)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result_obj(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  EXPECT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
}

TEST_F(NamedtupleModuleTest,
       NamedtupleUnderReplaceWithNonExistentFieldNameRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
from _namedtuple import namedtuple
Foo = namedtuple('Foo', ['a', 'b'])
Foo(1, 2)._replace(x=4)
)"),
                            LayoutId::kValueError,
                            "Got unexpected field names: {'x': 4}"));
}

TEST_F(NamedtupleModuleTest, NamedtupleUnderReplaceReplacesValueAtName) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
from _namedtuple import namedtuple
Foo = namedtuple('Foo', ['a', 'b'])
inst = Foo(1, 2)._replace(b=3)
result = (inst.a, inst.b)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result_obj(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  EXPECT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 3));
}

}  // namespace python
