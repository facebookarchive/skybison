#include "gtest/gtest.h"

#include "io-module.h"
#include "os.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

TEST(IoModuleTest, ReadFileBytesAsString) {
  int fd;
  testing::unique_file_ptr filename(OS::temporaryFile("filebytes-test", &fd));
  char c_filedata[] = "Foo, Bar, Baz";
  ssize_t filedata_len = std::strlen(c_filedata);
  ssize_t written_bytes = write(fd, c_filedata, filedata_len + 1);
  ASSERT_EQ(written_bytes, filedata_len + 1);
  close(fd);

  Runtime runtime;
  HandleScope scope;
  Str pyfile(&scope, runtime.newStrFromFormat(R"(
import _io
file_bytes = _io._readfile("%s")
filestr = _io._readbytes(file_bytes)
)",
                                              filename.get()));
  unique_c_ptr<char> c_pyfile(pyfile->toCStr());
  runtime.runFromCStr(c_pyfile.get());

  Str filestr(&scope, testing::moduleAt(&runtime, "__main__", "filestr"));
  EXPECT_TRUE(filestr->equalsCStr(c_filedata));
}

}  // namespace python
