#include "gtest/gtest.h"

#include "test-utils.h"

namespace python {

using namespace testing;

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

TEST(CodeBuiltinsTest, CoArgcountReturnsArgcount) {
  Runtime runtime;
  HandleScope scope;
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime.newStrFromCStr("co_argcount"));
  Object result(&scope, runtime.attributeAt(Thread::current(), code, key));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST(CodeBuiltinsTest, CoKwonlyargcountReturnsKwonlyargcount) {
  Runtime runtime;
  HandleScope scope;
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime.newStrFromCStr("co_kwonlyargcount"));
  Object result(&scope, runtime.attributeAt(Thread::current(), code, key));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST(CodeBuiltinsTest, CoNlocalsReturnsNLocals) {
  Runtime runtime;
  HandleScope scope;
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime.newStrFromCStr("co_nlocals"));
  Object result(&scope, runtime.attributeAt(Thread::current(), code, key));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST(CodeBuiltinsTest, CoStacksizeReturnsStacksize) {
  Runtime runtime;
  HandleScope scope;
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime.newStrFromCStr("co_stacksize"));
  Object result(&scope, runtime.attributeAt(Thread::current(), code, key));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
}

TEST(CodeBuiltinsTest, CoFlagsReturnsFlags) {
  Runtime runtime;
  HandleScope scope;
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime.newStrFromCStr("co_flags"));
  Object result(&scope, runtime.attributeAt(Thread::current(), code, key));
  EXPECT_TRUE(isIntEqualsWord(*result, 4));
}

TEST(CodeBuiltinsTest, CoCodeReturnsCode) {
  Runtime runtime;
  HandleScope scope;
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime.newStrFromCStr("co_code"));
  Object result(&scope, runtime.attributeAt(Thread::current(), code, key));
  byte reference[] = {100, 0, 83, 0};
  EXPECT_TRUE(isBytesEqualsBytes(result, reference));
}

TEST(CodeBuiltinsTest, CoConstsReturnsConsts) {
  Runtime runtime;
  HandleScope scope;
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime.newStrFromCStr("co_consts"));
  Object result_obj(&scope, runtime.attributeAt(Thread::current(), code, key));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 1);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "const0"));
}

TEST(CodeBuiltinsTest, CoNamesReturnsNames) {
  Runtime runtime;
  HandleScope scope;
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime.newStrFromCStr("co_names"));
  Object result_obj(&scope, runtime.attributeAt(Thread::current(), code, key));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 1);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "name0"));
}

TEST(CodeBuiltinsTest, CoVarnamesReturnsVarnames) {
  Runtime runtime;
  HandleScope scope;
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime.newStrFromCStr("co_varnames"));
  Object result_obj(&scope, runtime.attributeAt(Thread::current(), code, key));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "var0"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "var1"));
}

TEST(CodeBuiltinsTest, CoFreevarsReturnsFreevars) {
  Runtime runtime;
  HandleScope scope;
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime.newStrFromCStr("co_freevars"));
  Object result_obj(&scope, runtime.attributeAt(Thread::current(), code, key));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 1);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "freevar0"));
}

TEST(CodeBuiltinsTest, CoCellvarsReturnsCellvars) {
  Runtime runtime;
  HandleScope scope;
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime.newStrFromCStr("co_cellvars"));
  Object result_obj(&scope, runtime.attributeAt(Thread::current(), code, key));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 1);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "cellvar0"));
}

TEST(CodeBuiltinsTest, CoFilenameReturnsFilename) {
  Runtime runtime;
  HandleScope scope;
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime.newStrFromCStr("co_filename"));
  Object result(&scope, runtime.attributeAt(Thread::current(), code, key));
  EXPECT_TRUE(isStrEqualsCStr(*result, "filename0"));
}

TEST(CodeBuiltinsTest, CoFilenameReturnsName) {
  Runtime runtime;
  HandleScope scope;
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime.newStrFromCStr("co_name"));
  Object result(&scope, runtime.attributeAt(Thread::current(), code, key));
  EXPECT_TRUE(isStrEqualsCStr(*result, "name0"));
}

TEST(CodeBuiltinsTest, CoLnotabReturnsLnotab) {
  Runtime runtime;
  HandleScope scope;
  Object code(&scope, makeTestCode());
  Object key(&scope, runtime.newStrFromCStr("co_lnotab"));
  Object result(&scope, runtime.attributeAt(Thread::current(), code, key));
  EXPECT_TRUE(isBytesEqualsCStr(result, "lnotab"));
}

}  // namespace python
