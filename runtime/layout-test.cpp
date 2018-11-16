#include "gtest/gtest.h"

#include "layout.h"
#include "objects.h"
#include "runtime.h"

namespace python {

TEST(AttributeInfoTest, WithoutFlags) {
  AttributeInfo info(123, 0);
  EXPECT_EQ(info.offset(), 123);
  EXPECT_FALSE(info.isInObject());
}

TEST(AttributeInfoTest, WithFlags) {
  AttributeInfo info(123, AttributeInfo::Flag::kInObject);
  EXPECT_EQ(info.offset(), 123);
  EXPECT_TRUE(info.isInObject());
}

TEST(LayoutTest, FindAttribute) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Handle<Layout> layout(&scope, runtime.newLayout());

  // Should fail to find an attribute that isn't present
  Handle<Object> attr(&scope, runtime.newStringFromCString("myattr"));
  EXPECT_EQ(runtime.layoutFindAttribute(thread, layout, attr), Error::object());

  // Update the layout to include the new attribute as an in-object attribute
  Handle<ObjectArray> entry(&scope, runtime.newObjectArray(2));
  entry->atPut(0, *attr);
  entry->atPut(
      1, AttributeInfo(2222, AttributeInfo::Flag::kInObject).asSmallInteger());
  Handle<ObjectArray> array(&scope, runtime.newObjectArray(1));
  array->atPut(0, *entry);
  Layout::cast(*layout)->setInObjectAttributes(*array);

  // Should find the attribute
  Object* result = runtime.layoutFindAttribute(thread, layout, attr);
  ASSERT_TRUE(result->isSmallInteger());

  AttributeInfo info(result);
  EXPECT_EQ(info.offset(), 2222);
  EXPECT_TRUE(info.isInObject());
}

TEST(LayoutTest, AddNewAttributes) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Handle<Layout> layout(&scope, runtime.newLayout());

  // Should fail to find an attribute that isn't present
  Handle<Object> attr(&scope, runtime.newStringFromCString("myattr"));
  ASSERT_EQ(runtime.layoutFindAttribute(thread, layout, attr), Error::object());

  // Adding a new attribute should result in a new layout being created
  Handle<Layout> layout2(
      &scope, runtime.layoutAddAttribute(thread, layout, attr, 0));
  ASSERT_NE(*layout, *layout2);

  // Should be able find the attribute as an overflow attribute in the new
  // layout
  Object* info = runtime.layoutFindAttribute(thread, layout2, attr);
  ASSERT_FALSE(info->isError());
  EXPECT_TRUE(AttributeInfo(info).isOverflow());
  EXPECT_EQ(AttributeInfo(info).offset(), 0);

  // Adding another attribute should transition the layout again
  Handle<Object> attr2(&scope, runtime.newStringFromCString("another_attr"));
  ASSERT_EQ(
      runtime.layoutFindAttribute(thread, layout2, attr2), Error::object());
  Handle<Layout> layout3(
      &scope, runtime.layoutAddAttribute(thread, layout2, attr2, 0));
  ASSERT_NE(*layout2, *layout3);

  // We should be able to find both attributes in the new layout
  info = runtime.layoutFindAttribute(thread, layout3, attr);
  ASSERT_FALSE(info->isError());
  EXPECT_TRUE(AttributeInfo(info).isOverflow());
  EXPECT_EQ(AttributeInfo(info).offset(), 0);
  info = runtime.layoutFindAttribute(thread, layout3, attr2);
  ASSERT_FALSE(info->isError());
  EXPECT_TRUE(AttributeInfo(info).isOverflow());
  EXPECT_EQ(AttributeInfo(info).offset(), 1);
}

TEST(LayoutTest, AddDuplicateAttributes) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Handle<Layout> layout(&scope, runtime.newLayout());

  // Add an attribute
  Handle<Object> attr(&scope, runtime.newStringFromCString("myattr"));
  ASSERT_EQ(runtime.layoutFindAttribute(thread, layout, attr), Error::object());

  // Adding a new attribute should result in a new layout being created
  Handle<Layout> layout2(
      &scope, runtime.layoutAddAttribute(thread, layout, attr, 0));
  EXPECT_NE(*layout, *layout2);

  // Adding the attribute on the old layout should follow the edge and result
  // in the same layout being returned
  Handle<Layout> layout3(
      &scope, runtime.layoutAddAttribute(thread, layout, attr, 0));
  EXPECT_EQ(*layout2, *layout3);

  // Should be able to find the attribute in the new layout
  Object* info = runtime.layoutFindAttribute(thread, layout3, attr);
  ASSERT_TRUE(info->isSmallInteger());
  EXPECT_EQ(AttributeInfo(info).offset(), 0);
  EXPECT_TRUE(AttributeInfo(info).isOverflow());
}

} // namespace python
