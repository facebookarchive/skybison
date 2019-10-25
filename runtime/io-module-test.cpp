#include "gtest/gtest.h"

#include "io-module.h"
#include "os.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {

using IoModuleTest = testing::RuntimeFixture;

TEST_F(IoModuleTest, PostInitializeSetsBuiltinBaseToSupertype) {
  HandleScope scope(thread_);

  Type raw_io_base(&scope, runtime_.typeAt(LayoutId::kUnderRawIOBase));
  EXPECT_EQ(raw_io_base.builtinBase(), LayoutId::kUnderIOBase);

  Type buffered_io_base(&scope,
                        runtime_.typeAt(LayoutId::kUnderBufferedIOBase));
  EXPECT_EQ(buffered_io_base.builtinBase(), LayoutId::kUnderIOBase);

  Type bytes_io(&scope, runtime_.typeAt(LayoutId::kBytesIO));
  EXPECT_EQ(bytes_io.builtinBase(), LayoutId::kUnderBufferedIOBase);
}

}  // namespace py
