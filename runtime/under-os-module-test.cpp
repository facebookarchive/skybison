#include "under-os-module.h"

#include <fcntl.h>
#include <unistd.h>

#include "gtest/gtest.h"

#include "builtins.h"
#include "file.h"
#include "test-utils.h"

namespace py {
namespace testing {
using UnderOsModuleTest = RuntimeFixture;

TEST_F(UnderOsModuleTest, CloseWithBadFdRaisesOsError) {
  HandleScope scope(thread_);
  Object fd_obj(&scope, SmallInt::fromWord(-1));
  EXPECT_TRUE(raised(runBuiltin(FUNC(_os, close), fd_obj), LayoutId::kOSError));
}

static void createDummyFdWithContents(const char* c_str, int* fd) {
  word length = std::strlen(c_str);
  int fds[2];
  int result = ::pipe(fds);
  ASSERT_EQ(result, 0);
  result = ::write(fds[1], c_str, length);
  ASSERT_EQ(result, length);
  result = ::close(fds[1]);
  ASSERT_NE(result, -1);
  *fd = fds[0];
}

TEST_F(UnderOsModuleTest, CloseReturnsNone) {
  HandleScope scope(thread_);
  int fd;
  ASSERT_NO_FATAL_FAILURE(createDummyFdWithContents("hello", &fd));
  Object fd_obj(&scope, SmallInt::fromWord(fd));
  EXPECT_TRUE(runBuiltin(FUNC(_os, close), fd_obj).isNoneType());
  EXPECT_TRUE(raised(runBuiltin(FUNC(_os, close), fd_obj), LayoutId::kOSError));
}

TEST_F(UnderOsModuleTest, FstatSizeWithBadFdRaisesOSError) {
  HandleScope scope(thread_);
  int fd = 99999;
  Object fd_obj(&scope, SmallInt::fromWord(fd));
  EXPECT_TRUE(
      raised(runBuiltin(FUNC(_os, fstat_size), fd_obj), LayoutId::kOSError));
}

TEST_F(UnderOsModuleTest, FstatSizeReturnsSizeOfFile) {
  HandleScope scope(thread_);
  TemporaryDirectory directory;
  std::string path = directory.path + "test.txt";
  const char contents[] = "hello world";
  writeFile(path, contents);
  int fd = File::open(path.c_str(), O_RDONLY, 0755);
  ASSERT_GT(fd, 0);
  Object fd_obj(&scope, SmallInt::fromWord(fd));
  EXPECT_TRUE(isIntEqualsWord(runBuiltin(FUNC(_os, fstat_size), fd_obj),
                              std::strlen(contents)));
}

TEST_F(UnderOsModuleTest, FtruncateWithBadFdRaisesOSError) {
  HandleScope scope(thread_);
  int fd = 99999;
  Object fd_obj(&scope, SmallInt::fromWord(fd));
  Object size(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raised(runBuiltin(FUNC(_os, ftruncate), fd_obj, size),
                     LayoutId::kOSError));
}

TEST_F(UnderOsModuleTest, FtruncateSetsSize) {
  HandleScope scope(thread_);
  TemporaryDirectory directory;
  std::string path = directory.path + "test.txt";
  const char contents[] = "hello world";
  word initial_size = std::strlen(contents);
  writeFile(path, contents);
  int fd = File::open(path.c_str(), O_RDWR, 0755);
  ASSERT_GT(fd, 0);
  EXPECT_EQ(File::size(fd), initial_size);
  Object fd_obj(&scope, SmallInt::fromWord(fd));
  Object size_obj(&scope, SmallInt::fromWord(5));
  EXPECT_TRUE(runBuiltin(FUNC(_os, ftruncate), fd_obj, size_obj).isNoneType());
}

TEST_F(UnderOsModuleTest, IsattyWithBadFdReturnsFalse) {
  HandleScope scope(thread_);
  int fd = 99999;
  Object fd_obj(&scope, SmallInt::fromWord(fd));
  EXPECT_EQ(runBuiltin(FUNC(_os, isatty), fd_obj), Bool::falseObj());
}

TEST_F(UnderOsModuleTest, IsattyWithNonTtyReturnsFalse) {
  HandleScope scope(thread_);
  int fd = ::open("/dev/null", O_RDONLY);
  ASSERT_GE(fd, 0);
  Object fd_obj(&scope, SmallInt::fromWord(fd));
  EXPECT_EQ(runBuiltin(FUNC(_os, isatty), fd_obj), Bool::falseObj());
  EXPECT_EQ(::close(fd), 0);
}

TEST_F(UnderOsModuleTest, IsdirWithFileReturnsFalse) {
  HandleScope scope(thread_);
  int fd = ::open("/dev/null", O_RDONLY);
  ASSERT_GE(fd, 0);
  Object fd_obj(&scope, SmallInt::fromWord(fd));
  EXPECT_EQ(runBuiltin(FUNC(_os, isdir), fd_obj), Bool::falseObj());
  EXPECT_EQ(::close(fd), 0);
}

TEST_F(UnderOsModuleTest, IsdirWithNonExistentFdRaisesOSError) {
  HandleScope scope(thread_);
  int fd = 99999;
  Object fd_obj(&scope, SmallInt::fromWord(fd));
  EXPECT_TRUE(raised(runBuiltin(FUNC(_os, isdir), fd_obj), LayoutId::kOSError));
}

TEST_F(UnderOsModuleTest, IsdirWithDirectoryReturnsTrue) {
  HandleScope scope(thread_);
  int fd = ::open("/", O_RDONLY);
  Object fd_obj(&scope, SmallInt::fromWord(fd));
  EXPECT_EQ(runBuiltin(FUNC(_os, isdir), fd_obj), Bool::trueObj());
}

TEST_F(UnderOsModuleTest, LseekWithBadFdRaisesOSError) {
  HandleScope scope(thread_);
  int fd = 99999;
  Object fd_obj(&scope, SmallInt::fromWord(fd));
  Object offset(&scope, SmallInt::fromWord(0));
  Object whence(&scope, SmallInt::fromWord(SEEK_SET));
  EXPECT_TRUE(raised(runBuiltin(FUNC(_os, lseek), fd_obj, offset, whence),
                     LayoutId::kOSError));
}

TEST_F(UnderOsModuleTest, LseekChangesPosition) {
  HandleScope scope(thread_);
  TemporaryDirectory directory;
  std::string path = directory.path + "test.txt";
  const char contents[] = "hello world";
  writeFile(path, contents);
  int fd = File::open(path.c_str(), O_RDONLY, 0755);
  ASSERT_GE(fd, 0);
  Object fd_obj(&scope, SmallInt::fromWord(fd));
  word offset = 10;
  Object offset_obj(&scope, SmallInt::fromWord(offset));
  Object whence(&scope, SmallInt::fromWord(SEEK_SET));
  Object result(&scope,
                runBuiltin(FUNC(_os, lseek), fd_obj, offset_obj, whence));
  EXPECT_TRUE(isIntEqualsWord(*result, offset));
  // Get the current position
  EXPECT_EQ(::lseek(fd, 0, SEEK_CUR), offset);
  EXPECT_EQ(::close(fd), 0);
}

TEST_F(UnderOsModuleTest, OpenWithNonExistentFileRaisesOSError) {
  HandleScope scope(thread_);
  Str path(&scope, runtime_->newStrFromCStr("/i-should-not-exist"));
  Object flags(&scope, SmallInt::fromWord(0));
  Object mode(&scope, SmallInt::fromWord(0));
  Object dir_fd(&scope, NoneType::object());
  EXPECT_TRUE(raised(runBuiltin(FUNC(_os, open), path, flags, mode, dir_fd),
                     LayoutId::kOSError));
}

TEST_F(UnderOsModuleTest, OpenReturnsInt) {
  HandleScope scope(thread_);
  TemporaryDirectory directory;
  std::string path = directory.path + "test.txt";
  Str path_obj(&scope, runtime_->newStrFromCStr(path.c_str()));
  Object flags_obj(&scope, SmallInt::fromWord(O_RDWR | O_CREAT));
  Object mode_obj(&scope, SmallInt::fromWord(0755));
  Object dir_fd(&scope, NoneType::object());
  Object result_obj(&scope, runBuiltin(FUNC(_os, open), path_obj, flags_obj,
                                       mode_obj, dir_fd));
  ASSERT_TRUE(result_obj.isSmallInt());
  word fd = SmallInt::cast(*result_obj).value();
  ASSERT_GE(fd, 0);

  // It set the right flags
  int flags = ::fcntl(fd, F_GETFL);
  ASSERT_NE(flags, -1);
  EXPECT_EQ(flags & O_ACCMODE, O_RDWR);

  // It set the right mode
  struct stat statbuf;
  int result = ::fstat(fd, &statbuf);
  ASSERT_EQ(result, 0);
  EXPECT_EQ(statbuf.st_mode & 0777, 0755);

  // It set no inheritable
  flags = ::fcntl(fd, F_GETFD);
  ASSERT_NE(flags, -1);
  EXPECT_TRUE(flags & FD_CLOEXEC);

  ::close(fd);
}

TEST_F(UnderOsModuleTest, ParseModeWithXSetsExclAndCreat) {
  HandleScope scope(thread_);
  Object mode(&scope, runtime_->newStrFromCStr("x"));
  EXPECT_TRUE(
      isIntEqualsWord(runBuiltin(FUNC(_os, parse_mode), mode),
                      O_EXCL | O_CREAT | File::kNoInheritFlag | O_WRONLY));
}

TEST_F(UnderOsModuleTest, ParseModeWithRSetsRdonly) {
  HandleScope scope(thread_);
  Object mode(&scope, runtime_->newStrFromCStr("r"));
  EXPECT_TRUE(isIntEqualsWord(runBuiltin(FUNC(_os, parse_mode), mode),
                              File::kNoInheritFlag | O_RDONLY));
}

TEST_F(UnderOsModuleTest, ParseModeWithASetsAppendAndCreat) {
  HandleScope scope(thread_);
  Object mode(&scope, runtime_->newStrFromCStr("a"));
  EXPECT_TRUE(
      isIntEqualsWord(runBuiltin(FUNC(_os, parse_mode), mode),
                      O_APPEND | O_CREAT | File::kNoInheritFlag | O_WRONLY));
}

TEST_F(UnderOsModuleTest, ParseModeWithRPlusSetsRdWr) {
  HandleScope scope(thread_);
  Object mode(&scope, runtime_->newStrFromCStr("r+"));
  EXPECT_TRUE(isIntEqualsWord(runBuiltin(FUNC(_os, parse_mode), mode),
                              File::kNoInheritFlag | O_RDWR));
}

TEST_F(UnderOsModuleTest, ParseModeWithWPlusSetsRdWr) {
  HandleScope scope(thread_);
  Object mode(&scope, runtime_->newStrFromCStr("w+"));
  EXPECT_TRUE(
      isIntEqualsWord(runBuiltin(FUNC(_os, parse_mode), mode),
                      O_CREAT | O_TRUNC | File::kNoInheritFlag | O_RDWR));
}

TEST_F(UnderOsModuleTest, ParseModeWithAPlusSetsRdWr) {
  HandleScope scope(thread_);
  Object mode(&scope, runtime_->newStrFromCStr("a+"));
  EXPECT_TRUE(
      isIntEqualsWord(runBuiltin(FUNC(_os, parse_mode), mode),
                      O_APPEND | O_CREAT | File::kNoInheritFlag | O_RDWR));
}

TEST_F(UnderOsModuleTest, ReadWithBadFdRaisesOSError) {
  HandleScope scope(thread_);
  Object fd_obj(&scope, SmallInt::fromWord(-1));
  Object size(&scope, SmallInt::fromWord(5));
  EXPECT_TRUE(
      raised(runBuiltin(FUNC(_os, read), fd_obj, size), LayoutId::kOSError));
}

TEST_F(UnderOsModuleTest, ReadWithFdNotOpenedForReadingRaisesOSError) {
  HandleScope scope(thread_);
  int fds[2];
  int result = ::pipe(fds);
  ASSERT_EQ(result, 0);
  Object fd_obj(&scope, SmallInt::fromWord(fds[1]));
  Object size(&scope, SmallInt::fromWord(5));
  EXPECT_TRUE(
      raised(runBuiltin(FUNC(_os, read), fd_obj, size), LayoutId::kOSError));
  ::close(fds[0]);
  ::close(fds[1]);
}

TEST_F(UnderOsModuleTest,
       ReadWithFewerThanSizeBytesAvailableReadsFewerThanSizeBytes) {
  HandleScope scope(thread_);
  int fd;
  ASSERT_NO_FATAL_FAILURE(createDummyFdWithContents("h", &fd));
  Object fd_obj(&scope, SmallInt::fromWord(fd));
  Object size(&scope, SmallInt::fromWord(5));
  Object result(&scope, runBuiltin(FUNC(_os, read), fd_obj, size));
  EXPECT_TRUE(isBytesEqualsCStr(result, "h"));
  ::close(fd);
}

TEST_F(UnderOsModuleTest, ReadReadsSizeBytes) {
  HandleScope scope(thread_);
  int fd;
  ASSERT_NO_FATAL_FAILURE(createDummyFdWithContents("hello, world!", &fd));
  Object fd_obj(&scope, SmallInt::fromWord(fd));
  Object size(&scope, SmallInt::fromWord(5));
  Object result(&scope, runBuiltin(FUNC(_os, read), fd_obj, size));
  EXPECT_TRUE(isBytesEqualsCStr(result, "hello"));
  ::close(fd);
}

TEST_F(UnderOsModuleTest, SetNoInheritableWithBadFdRaisesOSError) {
  HandleScope scope(thread_);
  Object fd_obj(&scope, SmallInt::fromWord(-1));
  EXPECT_TRUE(raised(runBuiltin(FUNC(_os, set_noinheritable), fd_obj),
                     LayoutId::kOSError));
}

TEST_F(UnderOsModuleTest, SetNoInheritableWithFdSetsNoInheritable) {
  HandleScope scope(thread_);
  int fd;
  ASSERT_NO_FATAL_FAILURE(createDummyFdWithContents("hello, world!", &fd));
  Object fd_obj(&scope, SmallInt::fromWord(fd));
  Object result(&scope, runBuiltin(FUNC(_os, set_noinheritable), fd_obj));
  EXPECT_TRUE(result.isNoneType());
  EXPECT_EQ(File::isInheritable(fd), 1);
  ::close(fd);
}

}  // namespace testing
}  // namespace py
