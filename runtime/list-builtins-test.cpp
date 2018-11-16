#include "gtest/gtest.h"

#include "list-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ListBuiltinsTest, ListAppend) {
  const char* src = R"(
a = list()
b = list()
a.append(1)
a.append("2")
b.append(3)
a.append(b)
print(a[0], a[1], a[2][0])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2 3\n");
}

TEST(ThreadTest, ListExtend) {
  const char* src = R"(
a = []
b = [1, 2, 3]
r = a.extend(b)
print(r is None, len(b) == 3)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "True True\n");
}

TEST(ListBuiltinsTest, ListInsertExcept) {
  Runtime runtime;
  const char* src1 = R"(
a = [1, 2]
a.insert()
)";
  ASSERT_DEATH(
      runtime.runFromCString(src1),
      "aborting due to pending exception: "
      "insert\\(\\) takes exactly two arguments");

  const char* src2 = R"(
list.insert(1, 2, 3)
)";
  ASSERT_DEATH(
      runtime.runFromCString(src2),
      "aborting due to pending exception: "
      "descriptor 'insert' requires a 'list' object");

  const char* src3 = R"(
a = [1, 2]
a.insert("i", "val")
)";
  ASSERT_DEATH(
      runtime.runFromCString(src3),
      "aborting due to pending exception: "
      "index object cannot be interpreted as an integer");
}

TEST(ListBuiltinsTest, ListPop) {
  const char* src = R"(
a = [1,2,3,4,5]
a.pop()
print(len(a))
a.pop(0)
a.pop(-1)
print(len(a), a[0], a[1])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "4\n2 2 4\n");

  const char* src2 = R"(
a = [1,2,3,4,5]
print(a.pop(), a.pop(0), a.pop(-2))
)";
  std::string output2 = compileAndRunToString(&runtime, src2);
  EXPECT_EQ(output2, "5 1 2\n");
}

TEST(ListBuiltinsTest, ListPopExcept) {
  Runtime runtime;
  const char* src1 = R"(
a = [1, 2]
a.pop(1, 2, 3, 4)
)";
  ASSERT_DEATH(
      runtime.runFromCString(src1),
      "aborting due to pending exception: "
      "pop\\(\\) takes at most 1 argument");

  const char* src2 = R"(
list.pop(1)
)";
  ASSERT_DEATH(
      runtime.runFromCString(src2),
      "aborting due to pending exception: "
      "descriptor 'pop' requires a 'list' object");

  const char* src3 = R"(
a = [1, 2]
a.pop("i")
)";
  ASSERT_DEATH(
      runtime.runFromCString(src3),
      "aborting due to pending exception: "
      "index object cannot be interpreted as an integer");

  const char* src4 = R"(
a = [1]
a.pop()
a.pop()
)";
  ASSERT_DEATH(
      runtime.runFromCString(src4),
      "unimplemented: "
      "Throw an IndexError for an out of range list");

  const char* src5 = R"(
a = [1]
a.pop(3)
)";
  ASSERT_DEATH(
      runtime.runFromCString(src5),
      "unimplemented: "
      "Throw an IndexError for an out of range list");
}

TEST(ListBuiltinsTest, ListRemove) {
  const char* src = R"(
a = [5, 4, 3, 2, 1]
a.remove(2)
a.remove(5)
print(len(a), a[0], a[1], a[2])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "3 4 3 1\n");
}

TEST(ListBuiltinsTest, PrintList) {
  const char* src = R"(
a = [1, 0, True]
print(a)
a[0]=7
print(a)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "[1, 0, True]\n[7, 0, True]\n");
}

TEST(ListBuiltinsTest, ReplicateList) {
  const char* src = R"(
data = [1, 2, 3] * 3
for i in range(9):
  print(data[i])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n3\n1\n2\n3\n1\n2\n3\n");
}

TEST(ListBuiltinsTest, SubscriptList) {
  Runtime runtime;
  const char* src = R"(
l = [1, 2, 3, 4, 5, 6]
print(l[0], l[3], l[5])
l[0] = 6
l[5] = 1
print(l[0], l[3], l[5])
)";
  std::string output = compileAndRunToString(&runtime, src);
  ASSERT_EQ(output, "1 4 6\n6 4 1\n");
}

} // namespace python
