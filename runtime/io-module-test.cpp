#include "gtest/gtest.h"

#include "io-module.h"
#include "os.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using IoModuleTest = testing::RuntimeFixture;

TEST_F(IoModuleTest, PostInitializeSetsBuiltinBaseToSupertype) {
  HandleScope scope(thread_);

  Type raw_io_base(&scope, runtime_.typeAt(LayoutId::kUnderRawIOBase));
  EXPECT_EQ(raw_io_base.builtinBase(), LayoutId::kUnderIOBase);

  Type buffered_io_base(&scope,
                        runtime_.typeAt(LayoutId::kUnderBufferedIOBase));
  EXPECT_EQ(buffered_io_base.builtinBase(), LayoutId::kUnderRawIOBase);

  Type bytes_io(&scope, runtime_.typeAt(LayoutId::kBytesIO));
  EXPECT_EQ(bytes_io.builtinBase(), LayoutId::kUnderBufferedIOBase);
}

TEST_F(IoModuleTest, ReadFileBytesAsString) {
  int fd;
  testing::unique_file_ptr filename(OS::temporaryFile("filebytes-test", &fd));
  char c_filedata[] = "Foo, Bar, Baz";
  ssize_t filedata_len = std::strlen(c_filedata);
  ssize_t written_bytes = write(fd, c_filedata, filedata_len + 1);
  ASSERT_EQ(written_bytes, filedata_len + 1);
  close(fd);

  HandleScope scope(thread_);
  Str pyfile(&scope, runtime_.newStrFromFmt(R"(
import _io
file_bytes = _io._readfile("%s")
filestr = _io._readbytes(file_bytes)
)",
                                            filename.get()));
  unique_c_ptr<char> c_pyfile(pyfile.toCStr());
  ASSERT_FALSE(testing::runFromCStr(&runtime_, c_pyfile.get()).isError());

  Str filestr(&scope, testing::mainModuleAt(&runtime_, "filestr"));
  EXPECT_TRUE(filestr.equalsCStr(c_filedata));
}

}  // namespace python
