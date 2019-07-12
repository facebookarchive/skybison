#include "gtest/gtest.h"

#include "test-utils.h"

namespace python {

using namespace testing;

using CodeBuiltinsTest = RuntimeFixture;

static RawObject makeTestCode() {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  const byte bytes_array[] = {100, 0, 83, 0};
  Bytes bytes(&scope, runtime->newBytesWithAll(bytes_array));
  Tuple consts(&scope, runtime->newTuple(1));
  consts.atPut(0, runtime->newStrFromCStr("const0"));
  Tuple names(&scope, runtime->newTuple(1));
  names.atPut(0, runtime->newStrFromCStr("name0"));
  Tuple varnames(&scope, runtime->newTuple(2));
  varnames.atPut(0, runtime->newStrFromCStr("var0"));
  varnames.atPut(1, runtime->newStrFromCStr("var1"));
  Tuple freevars(&scope, runtime->newTuple(1));
  freevars.atPut(0, runtime->newStrFromCStr("freevar0"));
  Tuple cellvars(&scope, runtime->newTuple(1));
  cellvars.atPut(0, runtime->newStrFromCStr("cellvar0"));
  Object filename(&scope, runtime->newStrFromCStr("filename0"));
  Object name(&scope, runtime->newStrFromCStr("name0"));
  byte lnotab_array[] = {'l', 'n', 'o', 't', 'a', 'b'};
  Object lnotab(&scope, runtime->newBytesWithAll(lnotab_array));
  return runtime->newCode(0, 1, 2, 3, 4, bytes, consts, names, varnames,
                          freevars, cellvars, filename, name, 5, lnotab);
}

TEST_F(CodeBuiltinsTest, CoArgcountReturnsArgcount) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime_.newStrFromCStr("co_argcount"));
  Object result(&scope, runtime_.attributeAt(thread_, code, key));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(CodeBuiltinsTest, CoKwonlyargcountReturnsKwonlyargcount) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime_.newStrFromCStr("co_kwonlyargcount"));
  Object result(&scope, runtime_.attributeAt(thread_, code, key));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST_F(CodeBuiltinsTest, CoNlocalsReturnsNLocals) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime_.newStrFromCStr("co_nlocals"));
  Object result(&scope, runtime_.attributeAt(thread_, code, key));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(CodeBuiltinsTest, CoStacksizeReturnsStacksize) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime_.newStrFromCStr("co_stacksize"));
  Object result(&scope, runtime_.attributeAt(thread_, code, key));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
}

TEST_F(CodeBuiltinsTest, CoFlagsReturnsFlags) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime_.newStrFromCStr("co_flags"));
  Object result(&scope, runtime_.attributeAt(thread_, code, key));
  EXPECT_TRUE(isIntEqualsWord(*result, 4));
}

TEST_F(CodeBuiltinsTest, CoCodeReturnsCode) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime_.newStrFromCStr("co_code"));
  Object result(&scope, runtime_.attributeAt(thread_, code, key));
  byte reference[] = {100, 0, 83, 0};
  EXPECT_TRUE(isBytesEqualsBytes(result, reference));
}

TEST_F(CodeBuiltinsTest, CoConstsReturnsConsts) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime_.newStrFromCStr("co_consts"));
  Object result_obj(&scope, runtime_.attributeAt(thread_, code, key));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 1);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "const0"));
}

TEST_F(CodeBuiltinsTest, CoNamesReturnsNames) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime_.newStrFromCStr("co_names"));
  Object result_obj(&scope, runtime_.attributeAt(thread_, code, key));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 1);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "name0"));
}

TEST_F(CodeBuiltinsTest, CoVarnamesReturnsVarnames) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime_.newStrFromCStr("co_varnames"));
  Object result_obj(&scope, runtime_.attributeAt(thread_, code, key));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "var0"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "var1"));
}

TEST_F(CodeBuiltinsTest, CoFreevarsReturnsFreevars) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime_.newStrFromCStr("co_freevars"));
  Object result_obj(&scope, runtime_.attributeAt(thread_, code, key));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 1);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "freevar0"));
}

TEST_F(CodeBuiltinsTest, CoCellvarsReturnsCellvars) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime_.newStrFromCStr("co_cellvars"));
  Object result_obj(&scope, runtime_.attributeAt(thread_, code, key));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 1);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "cellvar0"));
}

TEST_F(CodeBuiltinsTest, CoFilenameReturnsFilename) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime_.newStrFromCStr("co_filename"));
  Object result(&scope, runtime_.attributeAt(thread_, code, key));
  EXPECT_TRUE(isStrEqualsCStr(*result, "filename0"));
}

TEST_F(CodeBuiltinsTest, CoFilenameReturnsName) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime_.newStrFromCStr("co_name"));
  Object result(&scope, runtime_.attributeAt(thread_, code, key));
  EXPECT_TRUE(isStrEqualsCStr(*result, "name0"));
}

TEST_F(CodeBuiltinsTest, CoLnotabReturnsLnotab) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime_.newStrFromCStr("co_lnotab"));
  Object result(&scope, runtime_.attributeAt(thread_, code, key));
  EXPECT_TRUE(isBytesEqualsCStr(result, "lnotab"));
}

}  // namespace python
