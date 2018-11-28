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
  Layout layout(&scope, runtime.layoutCreateEmpty(thread));

  // Should fail to find an attribute that isn't present
  Object attr(&scope, runtime.newStrFromCStr("myattr"));
  AttributeInfo info;
  EXPECT_FALSE(runtime.layoutFindAttribute(thread, layout, attr, &info));

  // Update the layout to include the new attribute as an in-object attribute
  Tuple entry(&scope, runtime.newTuple(2));
  entry->atPut(0, *attr);
  entry->atPut(
      1, AttributeInfo(2222, AttributeInfo::Flag::kInObject).asSmallInt());
  Tuple array(&scope, runtime.newTuple(1));
  array->atPut(0, *entry);
  RawLayout::cast(*layout)->setInObjectAttributes(*array);

  // Should find the attribute
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout, attr, &info));
  EXPECT_EQ(info.offset(), 2222);
  EXPECT_TRUE(info.isInObject());
}

TEST(LayoutTest, AddNewAttributes) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Layout layout(&scope, runtime.layoutCreateEmpty(thread));

  // Should fail to find an attribute that isn't present
  Object attr(&scope, runtime.newStrFromCStr("myattr"));
  AttributeInfo info;
  ASSERT_FALSE(runtime.layoutFindAttribute(thread, layout, attr, &info));

  // Adding a new attribute should result in a new layout being created
  Layout layout2(&scope, runtime.layoutAddAttribute(thread, layout, attr, 0));
  ASSERT_NE(*layout, *layout2);

  // Should be able find the attribute as an overflow attribute in the new
  // layout
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout2, attr, &info));
  EXPECT_TRUE(info.isOverflow());
  EXPECT_EQ(info.offset(), 0);

  // Adding another attribute should transition the layout again
  Object attr2(&scope, runtime.newStrFromCStr("another_attr"));
  ASSERT_FALSE(runtime.layoutFindAttribute(thread, layout2, attr2, &info));
  Layout layout3(&scope, runtime.layoutAddAttribute(thread, layout2, attr2, 0));
  ASSERT_NE(*layout2, *layout3);

  // We should be able to find both attributes in the new layout
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout3, attr, &info));
  EXPECT_TRUE(info.isOverflow());
  EXPECT_EQ(info.offset(), 0);
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout3, attr2, &info));
  EXPECT_TRUE(info.isOverflow());
  EXPECT_EQ(info.offset(), 1);
}

TEST(LayoutTest, AddDuplicateAttributes) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Layout layout(&scope, runtime.layoutCreateEmpty(thread));

  // Add an attribute
  Object attr(&scope, runtime.newStrFromCStr("myattr"));
  AttributeInfo info;
  ASSERT_FALSE(runtime.layoutFindAttribute(thread, layout, attr, &info));

  // Adding a new attribute should result in a new layout being created
  Layout layout2(&scope, runtime.layoutAddAttribute(thread, layout, attr, 0));
  EXPECT_NE(*layout, *layout2);

  // Adding the attribute on the old layout should follow the edge and result
  // in the same layout being returned
  Layout layout3(&scope, runtime.layoutAddAttribute(thread, layout, attr, 0));
  EXPECT_EQ(*layout2, *layout3);

  // Should be able to find the attribute in the new layout
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout3, attr, &info));
  EXPECT_EQ(info.offset(), 0);
  EXPECT_TRUE(info.isOverflow());
}

TEST(LayoutTest, DeleteNonExistentAttribute) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Layout layout(&scope, runtime.layoutCreateEmpty(thread));
  Object attr(&scope, runtime.newStrFromCStr("myattr"));
  RawObject result = runtime.layoutDeleteAttribute(thread, layout, attr);
  EXPECT_EQ(result, Error::object());
}

TEST(LayoutTest, DeleteInObjectAttribute) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  // Create a new layout with a single in-object attribute
  Object attr(&scope, runtime.newStrFromCStr("myattr"));
  Tuple entry(&scope, runtime.newTuple(2));
  entry->atPut(0, *attr);
  entry->atPut(
      1, AttributeInfo(2222, AttributeInfo::Flag::kInObject).asSmallInt());
  Tuple array(&scope, runtime.newTuple(1));
  array->atPut(0, *entry);
  Layout layout(&scope, runtime.layoutCreateEmpty(thread));
  layout->setInObjectAttributes(*array);

  // Deleting the attribute should succeed and return a new layout
  RawObject result = runtime.layoutDeleteAttribute(thread, layout, attr);
  ASSERT_TRUE(result->isLayout());
  Layout layout2(&scope, result);
  EXPECT_NE(layout->id(), layout2->id());

  // The new layout should have the entry for the attribute marked as deleted
  ASSERT_TRUE(layout2->inObjectAttributes()->isTuple());
  Tuple inobject(&scope, layout2->inObjectAttributes());
  ASSERT_EQ(inobject->length(), 1);
  ASSERT_TRUE(inobject->at(0)->isTuple());
  entry = inobject->at(0);
  EXPECT_EQ(entry->at(0), NoneType::object());
  ASSERT_TRUE(entry->at(1)->isSmallInt());
  EXPECT_EQ(AttributeInfo(entry->at(1)).flags(), 2);

  // Performing the same deletion should follow the edge created by the
  // previous deletion and arrive at the same layout
  result = runtime.layoutDeleteAttribute(thread, layout, attr);
  ASSERT_TRUE(result->isLayout());
  Layout layout3(&scope, result);
  EXPECT_EQ(*layout3, *layout2);
}

TEST(LayoutTest, DeleteOverflowAttribute) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  // Create a new layout with several overflow attributes
  Object attr(&scope, runtime.newStrFromCStr("myattr"));
  Object attr2(&scope, runtime.newStrFromCStr("myattr2"));
  Object attr3(&scope, runtime.newStrFromCStr("myattr3"));
  Tuple attrs(&scope, runtime.newTuple(3));
  Object* names[] = {&attr, &attr2, &attr3};
  for (word i = 0; i < attrs->length(); i++) {
    Tuple entry(&scope, runtime.newTuple(2));
    entry->atPut(0, **names[i]);
    entry->atPut(1, AttributeInfo(i, 0).asSmallInt());
    attrs->atPut(i, *entry);
  }
  Layout layout(&scope, runtime.layoutCreateEmpty(thread));
  layout->setOverflowAttributes(*attrs);

  // Delete the middle attribute. Make sure a new layout is created and the
  // entry after the deleted attribute has its offset updated correctly.
  RawObject result = runtime.layoutDeleteAttribute(thread, layout, attr2);
  ASSERT_TRUE(result->isLayout());
  Layout layout2(&scope, result);
  EXPECT_NE(layout2->id(), layout->id());
  // The first attribute should have the same offset
  AttributeInfo info;
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout2, attr, &info));
  EXPECT_EQ(info.offset(), 0);
  // The second attribute should not exist
  ASSERT_FALSE(runtime.layoutFindAttribute(thread, layout2, attr2, &info));
  // The third attribute should have been shifted down by 1
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout2, attr3, &info));
  EXPECT_EQ(info.offset(), 1);

  // Delete the first attribute. A new layout should be created and the last
  // entry is shifted into the first position.
  result = runtime.layoutDeleteAttribute(thread, layout2, attr);
  ASSERT_TRUE(result->isLayout());
  Layout layout3(&scope, result);
  EXPECT_NE(layout3->id(), layout->id());
  EXPECT_NE(layout3->id(), layout2->id());
  // The first attribute should not exist
  EXPECT_FALSE(runtime.layoutFindAttribute(thread, layout3, attr, &info));
  // The second attribute should not exist
  EXPECT_FALSE(runtime.layoutFindAttribute(thread, layout3, attr2, &info));

  // The third attribute should now occupy the first position
  EXPECT_TRUE(runtime.layoutFindAttribute(thread, layout3, attr3, &info));
  EXPECT_EQ(info.offset(), 0);

  // Delete the remaining attribute. A new layout should be created and the
  // overflow array should be empty.
  result = runtime.layoutDeleteAttribute(thread, layout3, attr3);
  ASSERT_TRUE(result->isLayout());
  Layout layout4(&scope, result);
  EXPECT_NE(layout4->id(), layout->id());
  EXPECT_NE(layout4->id(), layout2->id());
  EXPECT_NE(layout4->id(), layout3->id());
  // No attributes should exist
  EXPECT_FALSE(runtime.layoutFindAttribute(thread, layout4, attr, &info));
  EXPECT_FALSE(runtime.layoutFindAttribute(thread, layout4, attr2, &info));
  EXPECT_FALSE(runtime.layoutFindAttribute(thread, layout4, attr3, &info));
}

static RawObject createLayoutAttribute(Runtime* runtime, const Object& name,
                                       uword flags) {
  HandleScope scope;
  Tuple entry(&scope, runtime->newTuple(2));
  entry->atPut(0, *name);
  entry->atPut(1, AttributeInfo(0, flags).asSmallInt());
  Tuple result(&scope, runtime->newTuple(1));
  result->atPut(0, *entry);
  return *result;
}

TEST(LayoutTest, DeleteAndAddInObjectAttribute) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  // Create a new layout with one overflow attribute and one in-object
  // attribute
  Layout layout(&scope, runtime.layoutCreateEmpty(thread));
  Object inobject(&scope, runtime.newStrFromCStr("inobject"));
  layout->setInObjectAttributes(createLayoutAttribute(
      &runtime, inobject, AttributeInfo::Flag::kInObject));
  Object overflow(&scope, runtime.newStrFromCStr("overflow"));
  layout->setOverflowAttributes(createLayoutAttribute(&runtime, overflow, 0));

  // Delete the in-object attribute and add it back. It should be re-added as
  // an overflow attribute.
  RawObject result = runtime.layoutDeleteAttribute(thread, layout, inobject);
  ASSERT_TRUE(result->isLayout());
  Layout layout2(&scope, result);
  result = runtime.layoutAddAttribute(thread, layout2, inobject, 0);
  ASSERT_TRUE(result->isLayout());
  Layout layout3(&scope, result);
  AttributeInfo info;
  ASSERT_TRUE(runtime.layoutFindAttribute(thread, layout3, inobject, &info));
  EXPECT_EQ(info.offset(), 1);
  EXPECT_TRUE(info.isOverflow());
}

TEST(LayoutTest, VerifyChildLayout) {
  Runtime runtime;
  HandleScope scope;
  Layout parent(&scope, runtime.newLayout());
  Object attr(&scope, runtime.newStrFromCStr("foo"));
  Layout child(&scope,
               runtime.layoutAddAttribute(Thread::currentThread(), parent, attr,
                                          AttributeInfo::Flag::kNone));

  EXPECT_NE(child->id(), parent->id());
  EXPECT_EQ(child->numInObjectAttributes(), parent->numInObjectAttributes());
  EXPECT_EQ(child->inObjectAttributes(), parent->inObjectAttributes());
  // Child should have an additional overflow attribute
  EXPECT_NE(child->overflowAttributes(), parent->overflowAttributes());
  EXPECT_NE(child->additions(), parent->additions());
  EXPECT_EQ(RawList::cast(child->additions())->numItems(), 0);
  EXPECT_NE(child->deletions(), parent->deletions());
  EXPECT_EQ(RawList::cast(child->deletions())->numItems(), 0);
  EXPECT_EQ(child->describedType(), parent->describedType());
  EXPECT_EQ(child->instanceSize(), parent->instanceSize());
}

}  // namespace python
