#include "gtest/gtest.h"

#include "object-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ObjectBuiltinsTest, ObjectDunderReprReturnsTypeNameAndPointer) {
  Runtime runtime;
  runtime.runFromCString(R"(
class Foo:
  pass

a = object.__repr__(Foo())
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  // Storage for the class name. It must be shorter than the length of the whole
  // string.
  char* c_str = a->toCString();
  char* class_name = static_cast<char*>(std::malloc(a->length()));
  void* ptr = nullptr;
  int num_written = std::sscanf(c_str, "<%s object at %p>", class_name, &ptr);
  ASSERT_EQ(num_written, 2);
  EXPECT_STREQ(class_name, "Foo");
  // No need to check the actual pointer value, just make sure something was
  // there.
  EXPECT_NE(ptr, nullptr);
  free(class_name);
  free(c_str);
}

}  // namespace python
