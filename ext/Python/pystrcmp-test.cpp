#include <cstring>

#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using OSExtensionApiTest = ExtensionApi;

TEST_F(OSExtensionApiTest, StricmpIgnoresCase) {
  EXPECT_EQ(PyOS_stricmp("foo", "FOO"), 0);
  EXPECT_EQ(PyOS_stricmp("Foo", "fOO"), 0);
  EXPECT_EQ(PyOS_stricmp("FoO", "fOo"), 0);

  EXPECT_LT(PyOS_stricmp("fob", "FOO"), 0);
  EXPECT_GT(PyOS_stricmp("food", "FoO BaR"), 0);
}

TEST_F(OSExtensionApiTest, StricmpComparesStrings) {
  EXPECT_GT(PyOS_stricmp("food", "foo"), 0);
  EXPECT_EQ(PyOS_stricmp("foo", "foo"), 0);
  EXPECT_LT(PyOS_stricmp("foo", "food"), 0);
}

TEST_F(OSExtensionApiTest, StrnicmpIgnoresCase) {
  EXPECT_EQ(PyOS_strnicmp("foo", "FOO", 3), 0);
  EXPECT_EQ(PyOS_strnicmp("Foo", "fOO", 3), 0);
  EXPECT_EQ(PyOS_strnicmp("FoO", "fOo", 3), 0);

  EXPECT_LT(PyOS_strnicmp("fob", "FOO", 3), 0);
  EXPECT_GT(PyOS_strnicmp("food", "FoO BaR", 7), 0);
}

TEST_F(OSExtensionApiTest, StrnicmpComparesStrings) {
  EXPECT_GT(PyOS_strnicmp("food", "foo", 5), 0);
  EXPECT_EQ(PyOS_strnicmp("foo", "foo", 5), 0);
  EXPECT_LT(PyOS_strnicmp("foo", "food", 5), 0);
}

TEST_F(OSExtensionApiTest, StrnicmpCutsOffAtSize) {
  EXPECT_EQ(PyOS_strnicmp("food", "foo", 3), 0);
  EXPECT_EQ(PyOS_strnicmp("foo", "foo", 3), 0);
  EXPECT_EQ(PyOS_strnicmp("foo", "food", 3), 0);
}

}  // namespace testing
}  // namespace py
