#include "gtest/gtest.h"

#include "runtime.h"

namespace python {

TEST(ListTest, EmptyListInvariants) {
  Runtime runtime;
  List* list = List::cast(runtime.createList());
  ASSERT_EQ(list->capacity(), 0);
  ASSERT_EQ(list->length(), 0);
}

TEST(ListTest, AppendAndGrow) {
  Runtime runtime;
  Object* obj = runtime.createList();
  ASSERT_NE(obj, nullptr);
  List* list = List::cast(obj);

  // Check that list capacity grows according to a doubling schedule
  word expectedCapacity[] = {
      4, 4, 4, 4, 8, 8, 8, 8, 16, 16, 16, 16, 16, 16, 16, 16};
  for (int i = 0; i < 16; i++) {
    List::appendAndGrow(list, SmallInteger::fromWord(i), &runtime);
    ASSERT_EQ(list->capacity(), expectedCapacity[i]);
    ASSERT_EQ(list->length(), i + 1);
  }

  // Sanity check list contents
  for (int i = 0; i < 16; i++) {
    SmallInteger* elem = SmallInteger::cast(list->get(i));
    ASSERT_EQ(elem->value(), i);
  }
}

} // namespace python
