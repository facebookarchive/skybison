#include <fcntl.h>
#include <unistd.h>

#include "gtest/gtest.h"

#include "builtins.h"
#include "file.h"
#include "test-utils.h"

namespace py {
namespace testing {
using UnderPathModuleTest = RuntimeFixture;

TEST_F(UnderPathModuleTest, PathIsdirWithFileReturnsFalse) {
  TemporaryDirectory tempdir;
  std::string file_path = tempdir.path + "foo.py";
  writeFile(file_path, "");

  HandleScope scope(thread_);
  const char* path = file_path.c_str();
  Object path_obj(&scope, runtime_->newStrFromCStr(path));
  EXPECT_EQ(runBuiltin(FUNC(_path, isdir), path_obj), Bool::falseObj());
}

TEST_F(UnderPathModuleTest, PathIsdirWithDirectoryReturnsTrue) {
  TemporaryDirectory tempdir;

  HandleScope scope(thread_);
  const char* path = tempdir.path.c_str();
  Object path_obj(&scope, runtime_->newStrFromCStr(path));
  EXPECT_EQ(runBuiltin(FUNC(_path, isdir), path_obj), Bool::trueObj());
}

TEST_F(UnderPathModuleTest, PathIsfileWithFileReturnsTrue) {
  TemporaryDirectory tempdir;
  std::string file_path = tempdir.path + "foo.py";
  writeFile(file_path, "");

  HandleScope scope(thread_);
  const char* path = file_path.c_str();
  Object path_obj(&scope, runtime_->newStrFromCStr(path));
  EXPECT_EQ(runBuiltin(FUNC(_path, isfile), path_obj), Bool::trueObj());
}

TEST_F(UnderPathModuleTest, PathIsfileWithDirectoryReturnsFalse) {
  TemporaryDirectory tempdir;

  HandleScope scope(thread_);
  const char* path = tempdir.path.c_str();
  Object path_obj(&scope, runtime_->newStrFromCStr(path));
  EXPECT_EQ(runBuiltin(FUNC(_path, isfile), path_obj), Bool::falseObj());
}

}  // namespace testing
}  // namespace py
