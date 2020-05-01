#include "gtest/gtest.h"

#include "builtins.h"
#include "test-utils.h"

namespace py {
namespace testing {

using CodeBuiltinsTest = RuntimeFixture;

static RawObject makeTestCode() {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  const byte bytes_array[] = {100, 0, 83, 0};
  Bytes bytes(&scope, runtime->newBytesWithAll(bytes_array));
  Object const0(&scope, runtime->newStrFromCStr("const0"));
  Tuple consts(&scope, runtime->newTupleWith1(const0));
  Object name0(&scope, runtime->newStrFromCStr("name0"));
  Tuple names(&scope, runtime->newTupleWith1(name0));
  Object var0(&scope, runtime->newStrFromCStr("var0"));
  Object var1(&scope, runtime->newStrFromCStr("var1"));
  Tuple varnames(&scope, runtime->newTupleWith2(var0, var1));
  Object freevar0(&scope, runtime->newStrFromCStr("freevar0"));
  Tuple freevars(&scope, runtime->newTupleWith1(freevar0));
  Object cellvar0(&scope, runtime->newStrFromCStr("cellvar0"));
  Tuple cellvars(&scope, runtime->newTupleWith1(cellvar0));
  Object filename(&scope, runtime->newStrFromCStr("filename0"));
  Object name(&scope, runtime->newStrFromCStr("name0"));
  byte lnotab_array[] = {'l', 'n', 'o', 't', 'a', 'b'};
  Object lnotab(&scope, runtime->newBytesWithAll(lnotab_array));
  word argcount = 0;
  word posonlyargcount = 0;
  word kwonlyargcount = 1;
  word nlocals = 2;
  word stacksize = 3;
  word flags = Code::Flags::kNested | Code::Flags::kGenerator;
  word firstlineno = 5;
  return runtime->newCode(argcount, posonlyargcount, kwonlyargcount, nlocals,
                          stacksize, flags, bytes, consts, names, varnames,
                          freevars, cellvars, filename, name, firstlineno,
                          lnotab);
}

TEST_F(CodeBuiltinsTest, CoArgcountReturnsArgcount) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "co_argcount"));
  Object result(&scope, runtime_->attributeAt(thread_, code, name));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(CodeBuiltinsTest, CoPosonlyargcountReturnsPosonlyargcount) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object name(&scope,
              Runtime::internStrFromCStr(thread_, "co_posonlyargcount"));
  Object result(&scope, runtime_->attributeAt(thread_, code, name));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(CodeBuiltinsTest, CoKwonlyargcountReturnsKwonlyargcount) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "co_kwonlyargcount"));
  Object result(&scope, runtime_->attributeAt(thread_, code, name));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST_F(CodeBuiltinsTest, CoNlocalsReturnsNLocals) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "co_nlocals"));
  Object result(&scope, runtime_->attributeAt(thread_, code, name));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(CodeBuiltinsTest, CoStacksizeReturnsStacksize) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "co_stacksize"));
  Object result(&scope, runtime_->attributeAt(thread_, code, name));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
}

TEST_F(CodeBuiltinsTest, CoFlagsReturnsFlags) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "co_flags"));
  Object result(&scope, runtime_->attributeAt(thread_, code, name));
  EXPECT_TRUE(
      isIntEqualsWord(*result, Code::Flags::kNested | Code::Flags::kGenerator));
}

TEST_F(CodeBuiltinsTest, CoCodeReturnsCode) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "co_code"));
  Object result(&scope, runtime_->attributeAt(thread_, code, name));
  byte reference[] = {100, 0, 83, 0};
  EXPECT_TRUE(isBytesEqualsBytes(result, reference));
}

TEST_F(CodeBuiltinsTest, CoConstsReturnsConsts) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "co_consts"));
  Object result_obj(&scope, runtime_->attributeAt(thread_, code, name));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 1);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "const0"));
}

TEST_F(CodeBuiltinsTest, CoNamesReturnsNames) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "co_names"));
  Object result_obj(&scope, runtime_->attributeAt(thread_, code, name));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 1);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "name0"));
}

TEST_F(CodeBuiltinsTest, CoVarnamesReturnsVarnames) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "co_varnames"));
  Object result_obj(&scope, runtime_->attributeAt(thread_, code, name));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "var0"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "var1"));
}

TEST_F(CodeBuiltinsTest, CoFreevarsReturnsFreevars) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "co_freevars"));
  Object result_obj(&scope, runtime_->attributeAt(thread_, code, name));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 1);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "freevar0"));
}

TEST_F(CodeBuiltinsTest, CoCellvarsReturnsCellvars) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "co_cellvars"));
  Object result_obj(&scope, runtime_->attributeAt(thread_, code, name));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 1);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "cellvar0"));
}

TEST_F(CodeBuiltinsTest, CoFilenameReturnsFilename) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "co_filename"));
  Object result(&scope, runtime_->attributeAt(thread_, code, name));
  EXPECT_TRUE(isStrEqualsCStr(*result, "filename0"));
}

TEST_F(CodeBuiltinsTest, CoFilenameReturnsName) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "co_name"));
  Object result(&scope, runtime_->attributeAt(thread_, code, name));
  EXPECT_TRUE(isStrEqualsCStr(*result, "name0"));
}

TEST_F(CodeBuiltinsTest, CoLnotabReturnsLnotab) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "co_lnotab"));
  Object result(&scope, runtime_->attributeAt(thread_, code, name));
  EXPECT_TRUE(isBytesEqualsCStr(result, "lnotab"));
}

}  // namespace testing
}  // namespace py
