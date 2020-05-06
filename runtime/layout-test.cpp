#include "layout.h"

#include "gtest/gtest.h"

#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {

using LayoutTest = testing::RuntimeFixture;

TEST(AttributeInfoTest, WithoutFlags) {
  AttributeInfo info(123, 0);
  EXPECT_EQ(info.offset(), 123);
  EXPECT_FALSE(info.isInObject());
}

TEST(AttributeInfoTest, WithFlags) {
  AttributeInfo info(123, AttributeFlags::kInObject);
  EXPECT_EQ(info.offset(), 123);
  EXPECT_TRUE(info.isInObject());
}

TEST_F(LayoutTest, SmallIntHasEmptyInObjectAttributes) {
  RawLayout layout = Layout::cast(runtime_->layoutAt(LayoutId::kSmallInt));
  EXPECT_EQ(Tuple::cast(layout.inObjectAttributes()).length(), 0);
}

TEST_F(LayoutTest, SmallStrHasEmptyInObjectAttributes) {
  RawLayout layout = Layout::cast(runtime_->layoutAt(LayoutId::kSmallStr));
  EXPECT_EQ(Tuple::cast(layout.inObjectAttributes()).length(), 0);
}

TEST_F(LayoutTest, SmallBytesHasEmptyInObjectAttributes) {
  RawLayout layout = Layout::cast(runtime_->layoutAt(LayoutId::kSmallBytes));
  EXPECT_EQ(Tuple::cast(layout.inObjectAttributes()).length(), 0);
}

TEST_F(LayoutTest, BoolHasEmptyInObjectAttributes) {
  RawLayout layout = Layout::cast(runtime_->layoutAt(LayoutId::kBool));
  EXPECT_EQ(Tuple::cast(layout.inObjectAttributes()).length(), 0);
}

TEST_F(LayoutTest, NoneTypeHasEmptyInObjectAttributes) {
  RawLayout layout = Layout::cast(runtime_->layoutAt(LayoutId::kNoneType));
  EXPECT_EQ(Tuple::cast(layout.inObjectAttributes()).length(), 0);
}

TEST_F(LayoutTest, NotImplementedTypeHasEmptyInObjectAttributes) {
  RawLayout layout =
      Layout::cast(runtime_->layoutAt(LayoutId::kNotImplementedType));
  EXPECT_EQ(Tuple::cast(layout.inObjectAttributes()).length(), 0);
}

TEST_F(LayoutTest, UnboundHasEmptyInObjectAttributes) {
  RawLayout layout = Layout::cast(runtime_->layoutAt(LayoutId::kUnbound));
  EXPECT_EQ(Tuple::cast(layout.inObjectAttributes()).length(), 0);
}

TEST_F(LayoutTest, SmallIntHasZeroInObjectAttributes) {
  EXPECT_EQ(Layout::cast(runtime_->layoutAt(LayoutId::kSmallInt))
                .numInObjectAttributes(),
            0);
}

TEST_F(LayoutTest, SmallStrHasZeroInObjectAttributes) {
  EXPECT_EQ(Layout::cast(runtime_->layoutAt(LayoutId::kSmallStr))
                .numInObjectAttributes(),
            0);
}

TEST_F(LayoutTest, SmallBytesHasZeroInObjectAttributes) {
  EXPECT_EQ(Layout::cast(runtime_->layoutAt(LayoutId::kSmallBytes))
                .numInObjectAttributes(),
            0);
}

TEST_F(LayoutTest, BoolHasZeroInObjectAttributes) {
  EXPECT_EQ(
      Layout::cast(runtime_->layoutAt(LayoutId::kBool)).numInObjectAttributes(),
      0);
}

TEST_F(LayoutTest, NoneTypeHasZeroInObjectAttributes) {
  EXPECT_EQ(Layout::cast(runtime_->layoutAt(LayoutId::kNoneType))
                .numInObjectAttributes(),
            0);
}

TEST_F(LayoutTest, NotImplementedTypeHasZeroInObjectAttributes) {
  EXPECT_EQ(Layout::cast(runtime_->layoutAt(LayoutId::kNotImplementedType))
                .numInObjectAttributes(),
            0);
}

TEST_F(LayoutTest, UnboundHasZeroInObjectAttributes) {
  EXPECT_EQ(Layout::cast(runtime_->layoutAt(LayoutId::kUnbound))
                .numInObjectAttributes(),
            0);
}

TEST_F(LayoutTest, SmallIntHasZeroInstanceSize) {
  EXPECT_EQ(
      Layout::cast(runtime_->layoutAt(LayoutId::kSmallInt)).instanceSize(), 0);
}

TEST_F(LayoutTest, SmallStrHasZeroInstanceSize) {
  EXPECT_EQ(
      Layout::cast(runtime_->layoutAt(LayoutId::kSmallStr)).instanceSize(), 0);
}

TEST_F(LayoutTest, SmallBytesHasZeroInstanceSize) {
  EXPECT_EQ(
      Layout::cast(runtime_->layoutAt(LayoutId::kSmallBytes)).instanceSize(),
      0);
}

TEST_F(LayoutTest, BoolHasZeroInstanceSize) {
  EXPECT_EQ(Layout::cast(runtime_->layoutAt(LayoutId::kBool)).instanceSize(),
            0);
}

TEST_F(LayoutTest, NoneTypeHasZeroInstanceSize) {
  EXPECT_EQ(
      Layout::cast(runtime_->layoutAt(LayoutId::kNoneType)).instanceSize(), 0);
}

TEST_F(LayoutTest, NotImplementedTypeHasZeroInstanceSize) {
  EXPECT_EQ(Layout::cast(runtime_->layoutAt(LayoutId::kNotImplementedType))
                .instanceSize(),
            0);
}

TEST_F(LayoutTest, UnboundHasZeroInstanceSize) {
  EXPECT_EQ(Layout::cast(runtime_->layoutAt(LayoutId::kUnbound)).instanceSize(),
            0);
}

TEST_F(LayoutTest, FindAttribute) {
  HandleScope scope(thread_);
  Layout layout(&scope, testing::layoutCreateEmpty(thread_));

  // Should fail to find an attribute that isn't present
  Object attr(&scope, Runtime::internStrFromCStr(thread_, "myattr"));
  AttributeInfo info;
  EXPECT_FALSE(Runtime::layoutFindAttribute(*layout, attr, &info));

  // Update the layout to include the new attribute as an in-object attribute
  Object info_obj(&scope,
                  AttributeInfo(2222, AttributeFlags::kInObject).asSmallInt());
  Tuple entry(&scope, runtime_->newTupleWith2(attr, info_obj));
  Tuple array(&scope, runtime_->newTupleWith1(entry));
  Layout::cast(*layout).setInObjectAttributes(*array);

  // Should find the attribute
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, attr, &info));
  EXPECT_EQ(info.offset(), 2222);
  EXPECT_TRUE(info.isInObject());
}

TEST_F(LayoutTest, AddNewAttributes) {
  HandleScope scope(thread_);
  Layout layout(&scope, testing::layoutCreateEmpty(thread_));
  runtime_->layoutSetTupleOverflow(*layout);

  // Should fail to find an attribute that isn't present
  Object attr(&scope, Runtime::internStrFromCStr(thread_, "myattr"));
  AttributeInfo info;
  ASSERT_FALSE(Runtime::layoutFindAttribute(*layout, attr, &info));

  // Adding a new attribute should result in a new layout being created
  AttributeInfo info2;
  Layout layout2(
      &scope, runtime_->layoutAddAttribute(thread_, layout, attr, 0, &info2));
  ASSERT_NE(*layout, *layout2);
  EXPECT_TRUE(info2.isOverflow());
  EXPECT_EQ(info2.offset(), 0);

  // Should be able find the attribute as an overflow attribute in the new
  // layout
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout2, attr, &info));
  EXPECT_TRUE(info.isOverflow());
  EXPECT_EQ(info.offset(), 0);

  // Adding another attribute should transition the layout again
  Object attr2(&scope, Runtime::internStrFromCStr(thread_, "another_attr"));
  ASSERT_FALSE(Runtime::layoutFindAttribute(*layout2, attr2, &info));
  AttributeInfo info3;
  Layout layout3(
      &scope, runtime_->layoutAddAttribute(thread_, layout2, attr2, 0, &info3));
  ASSERT_NE(*layout2, *layout3);
  EXPECT_TRUE(info3.isOverflow());
  EXPECT_EQ(info3.offset(), 1);

  // We should be able to find both attributes in the new layout
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout3, attr, &info));
  EXPECT_TRUE(info.isOverflow());
  EXPECT_EQ(info.offset(), 0);
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout3, attr2, &info));
  EXPECT_TRUE(info.isOverflow());
  EXPECT_EQ(info.offset(), 1);
}

TEST_F(LayoutTest, AddDuplicateAttributes) {
  HandleScope scope(thread_);
  Layout layout(&scope, testing::layoutCreateEmpty(thread_));
  runtime_->layoutSetTupleOverflow(*layout);

  // Add an attribute
  Object attr(&scope, Runtime::internStrFromCStr(thread_, "myattr"));
  AttributeInfo info;
  ASSERT_FALSE(Runtime::layoutFindAttribute(*layout, attr, &info));

  // Adding a new attribute should result in a new layout being created
  Layout layout2(&scope,
                 runtime_->layoutAddAttribute(thread_, layout, attr, 0, &info));
  EXPECT_NE(*layout, *layout2);
  EXPECT_EQ(info.offset(), 0);
  EXPECT_TRUE(info.isOverflow());

  // Adding the attribute on the old layout should follow the edge and result
  // in the same layout being returned
  Layout layout3(&scope,
                 runtime_->layoutAddAttribute(thread_, layout, attr, 0, &info));
  EXPECT_EQ(*layout2, *layout3);
  EXPECT_EQ(info.offset(), 0);
  EXPECT_TRUE(info.isOverflow());

  // Should be able to find the attribute in the new layout
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout3, attr, &info));
  EXPECT_EQ(info.offset(), 0);
  EXPECT_TRUE(info.isOverflow());
}

TEST_F(LayoutTest, TransitionTypeReturnsNewLayout) {
  HandleScope scope(thread_);
  Layout from_layout(&scope, testing::layoutCreateEmpty(thread_));

  Object attr(&scope, Runtime::internStrFromCStr(thread_, "__class__"));
  AttributeInfo info;
  ASSERT_FALSE(Runtime::layoutFindAttribute(*from_layout, attr, &info));

  // Transitioning the type will result in a new layout
  ASSERT_FALSE(testing::runFromCStr(runtime_, R"(
class A:
    pass
)")
                   .isError());
  Type to_type(&scope, testing::mainModuleAt(runtime_, "A"));
  Layout target_layout(
      &scope, runtime_->layoutSetDescribedType(thread_, from_layout, to_type));
  EXPECT_NE(*from_layout, *target_layout);

  // Transitioning more than once will result in the same layout
  Layout target_layout2(
      &scope, runtime_->layoutSetDescribedType(thread_, from_layout, to_type));
  EXPECT_EQ(*target_layout2, *target_layout);
}

TEST_F(LayoutTest, TransitionTableResizesWhenNecessary) {
  HandleScope scope(thread_);
  Layout from_layout(&scope, testing::layoutCreateEmpty(thread_));

  Object attr(&scope, Runtime::internStrFromCStr(thread_, "__class__"));
  AttributeInfo info;
  ASSERT_FALSE(Runtime::layoutFindAttribute(*from_layout, attr, &info));

  Tuple transitions(&scope, runtime_->newMutableTuple(1));
  runtime_->setLayoutTypeTransitions(*transitions);

  Type type(&scope, runtime_->newType());
  Layout to_layout(
      &scope, runtime_->layoutSetDescribedType(thread_, from_layout, type));

  EXPECT_NE(*to_layout, *from_layout);

  transitions = runtime_->layoutTypeTransitions();
  EXPECT_GT(transitions.length(), 1);
}

TEST_F(LayoutTest, TransitionTableHoldsWeakRefToLayouts) {
  HandleScope scope(thread_);
  Layout from_layout(&scope, testing::layoutCreateEmpty(thread_));

  Object attr(&scope, Runtime::internStrFromCStr(thread_, "__class__"));
  AttributeInfo info;
  ASSERT_FALSE(Runtime::layoutFindAttribute(*from_layout, attr, &info));

  // Transitioning the type will result in a new layout
  Object to_ref(&scope, NoneType::object());
  Object target_ref(&scope, NoneType::object());
  Object none(&scope, NoneType::object());
  {
    ASSERT_FALSE(testing::runFromCStr(runtime_, R"(
class A:
    pass
)")
                     .isError());
    Type to_type(&scope, testing::mainModuleAt(runtime_, "A"));
    ASSERT_FALSE(testing::runFromCStr(runtime_, R"(
del A
)")
                     .isError());
    WeakRef to_ref_inner(&scope, runtime_->newWeakRef(thread_, to_type, none));
    to_ref = *to_ref_inner;

    Layout target_layout(&scope, runtime_->layoutSetDescribedType(
                                     thread_, from_layout, to_type));
    WeakRef target_ref_inner(
        &scope, runtime_->newWeakRef(thread_, target_layout, none));
    EXPECT_NE(*from_layout, *target_layout);
    target_ref = *target_ref_inner;

    runtime_->collectGarbage();
    EXPECT_EQ(to_ref_inner.referent(), *to_type);
    EXPECT_EQ(target_ref_inner.referent(), *target_layout);
  }

  // Layouts are collected
  runtime_->collectGarbage();
  EXPECT_EQ(WeakRef::cast(*to_ref).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*target_ref).referent(), NoneType::object());
}

TEST_F(LayoutTest, InstanceTransitionTypeReturnsInstanceWithNewLayout) {
  ASSERT_FALSE(testing::runFromCStr(runtime_, R"(
class A:
  def __init__(self):
    self.a = "a"
    self.b = "b"

class B:
  def __init__(self):
    self.b = "b"
    self.c = "c"

a_instance = A()
b_instance = B()
  )")
                   .isError());
  HandleScope scope(thread_);
  Instance a_instance(&scope, testing::mainModuleAt(runtime_, "a_instance"));
  Instance b_instance(&scope, testing::mainModuleAt(runtime_, "b_instance"));
  Layout from_layout(&scope, runtime_->layoutOf(*a_instance));
  Layout to_layout(&scope, runtime_->layoutOf(*b_instance));
  EXPECT_NE(*from_layout, *to_layout);

  Object a_attr(&scope, Runtime::internStrFromCStr(thread_, "a"));
  Object b_attr(&scope, Runtime::internStrFromCStr(thread_, "b"));
  Object c_attr(&scope, Runtime::internStrFromCStr(thread_, "c"));
  AttributeInfo info;
  // a_instance has a, b; does not have c
  ASSERT_TRUE(Runtime::layoutFindAttribute(*from_layout, a_attr, &info));
  ASSERT_TRUE(Runtime::layoutFindAttribute(*from_layout, b_attr, &info));
  ASSERT_FALSE(Runtime::layoutFindAttribute(*from_layout, c_attr, &info));

  // b_instance has b, c; does not have a
  ASSERT_FALSE(Runtime::layoutFindAttribute(*to_layout, a_attr, &info));
  ASSERT_TRUE(Runtime::layoutFindAttribute(*to_layout, b_attr, &info));
  ASSERT_TRUE(Runtime::layoutFindAttribute(*to_layout, c_attr, &info));

  ASSERT_FALSE(testing::runFromCStr(runtime_, R"(
a_instance.__class__ = B
  )")
                   .isError());

  // a_instance still has a, b, but does not magically gain c
  Layout target_layout(&scope, runtime_->layoutOf(*a_instance));
  EXPECT_NE(*to_layout, *target_layout);
  EXPECT_TRUE(Runtime::layoutFindAttribute(*target_layout, a_attr, &info));
  EXPECT_TRUE(Runtime::layoutFindAttribute(*target_layout, b_attr, &info));
  EXPECT_FALSE(Runtime::layoutFindAttribute(*target_layout, c_attr, &info));
}

TEST_F(LayoutTest, DeleteInObjectAttribute) {
  HandleScope scope(thread_);

  // Create a new layout with a single in-object attribute
  Object attr(&scope, Runtime::internStrFromCStr(thread_, "myattr"));
  Object info_obj(&scope,
                  AttributeInfo(2222, AttributeFlags::kInObject).asSmallInt());
  Tuple entry(&scope, runtime_->newTupleWith2(attr, info_obj));
  Tuple array(&scope, runtime_->newTupleWith1(entry));
  Layout layout(&scope, testing::layoutCreateEmpty(thread_));
  layout.setInObjectAttributes(*array);

  // Deleting the attribute should succeed and return a new layout
  AttributeInfo info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, attr, &info));
  RawObject result =
      runtime_->layoutDeleteAttribute(thread_, layout, attr, info);
  ASSERT_TRUE(result.isLayout());
  Layout layout2(&scope, result);
  EXPECT_NE(layout.id(), layout2.id());

  // The new layout should have the entry for the attribute marked as deleted
  ASSERT_TRUE(layout2.inObjectAttributes().isTuple());
  Tuple inobject(&scope, layout2.inObjectAttributes());
  ASSERT_EQ(inobject.length(), 1);
  ASSERT_TRUE(inobject.at(0).isTuple());
  entry = inobject.at(0);
  EXPECT_EQ(entry.at(0), NoneType::object());
  ASSERT_TRUE(entry.at(1).isSmallInt());
  EXPECT_EQ(AttributeInfo(entry.at(1)).flags(), 2);

  // Performing the same deletion should follow the edge created by the
  // previous deletion and arrive at the same layout
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, attr, &info));
  result = runtime_->layoutDeleteAttribute(thread_, layout, attr, info);
  ASSERT_TRUE(result.isLayout());
  Layout layout3(&scope, result);
  EXPECT_EQ(*layout3, *layout2);
}

TEST_F(LayoutTest, DeleteOverflowAttribute) {
  HandleScope scope(thread_);

  // Create a new layout with several overflow attributes
  Object attr(&scope, Runtime::internStrFromCStr(thread_, "myattr"));
  Object info_obj1(&scope, AttributeInfo(0, 0).asSmallInt());
  Tuple entry1(&scope, runtime_->newTupleWith2(attr, info_obj1));

  Object attr2(&scope, Runtime::internStrFromCStr(thread_, "myattr2"));
  Object info_obj2(&scope, AttributeInfo(1, 0).asSmallInt());
  Tuple entry2(&scope, runtime_->newTupleWith2(attr2, info_obj2));

  Object attr3(&scope, Runtime::internStrFromCStr(thread_, "myattr3"));
  Object info_obj3(&scope, AttributeInfo(2, 0).asSmallInt());
  Tuple entry3(&scope, runtime_->newTupleWith2(attr3, info_obj3));

  Layout layout(&scope, testing::layoutCreateEmpty(thread_));
  Tuple attrs(&scope, runtime_->newTupleWith3(entry1, entry2, entry3));
  layout.setOverflowAttributes(*attrs);

  // Delete the middle attribute. Make sure a new layout is created and the
  // entry after the deleted attribute has its offset updated correctly.
  AttributeInfo attr2_info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, attr, &attr2_info));
  RawObject result =
      runtime_->layoutDeleteAttribute(thread_, layout, attr2, attr2_info);
  ASSERT_TRUE(result.isLayout());
  Layout layout2(&scope, result);
  EXPECT_NE(layout2.id(), layout.id());
  // The first and third attribute should have the same offset
  AttributeInfo info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout2, attr, &info));
  EXPECT_EQ(info.offset(), 0);
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout2, attr3, &info));
  EXPECT_EQ(info.offset(), 2);
  // The second attribute should not exist in the new layout.
  ASSERT_FALSE(Runtime::layoutFindAttribute(*layout2, attr2, &info));

  // Delete the first attribute. A new layout should be created and the last
  // entry is shifted into the first position.
  AttributeInfo attr_info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout2, attr, &attr_info));
  result = runtime_->layoutDeleteAttribute(thread_, layout2, attr, attr_info);
  ASSERT_TRUE(result.isLayout());
  Layout layout3(&scope, result);
  EXPECT_NE(layout3.id(), layout.id());
  EXPECT_NE(layout3.id(), layout2.id());
  // The first and second attribute should not exist
  EXPECT_FALSE(Runtime::layoutFindAttribute(*layout3, attr, &info));
  EXPECT_FALSE(Runtime::layoutFindAttribute(*layout3, attr2, &info));
  // The third attribute should still exist.
  EXPECT_TRUE(Runtime::layoutFindAttribute(*layout3, attr3, &info));
  EXPECT_EQ(info.offset(), 2);

  // Delete the remaining attribute. A new layout should be created and the
  // overflow array should be empty.
  AttributeInfo attr3_info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout3, attr3, &attr3_info));
  result = runtime_->layoutDeleteAttribute(thread_, layout3, attr3, attr3_info);
  ASSERT_TRUE(result.isLayout());
  Layout layout4(&scope, result);
  EXPECT_NE(layout4.id(), layout.id());
  EXPECT_NE(layout4.id(), layout2.id());
  EXPECT_NE(layout4.id(), layout3.id());
  // No attributes should exist
  EXPECT_FALSE(Runtime::layoutFindAttribute(*layout4, attr, &info));
  EXPECT_FALSE(Runtime::layoutFindAttribute(*layout4, attr2, &info));
  EXPECT_FALSE(Runtime::layoutFindAttribute(*layout4, attr3, &info));

  // Appending to layout2 should not use the offset of any of the remaining
  // attributes there.
  result = runtime_->layoutAddAttribute(thread_, layout2, attr2, 0, &info);
  ASSERT_TRUE(result.isLayout());
  Layout layout2_added(&scope, result);
  EXPECT_NE(layout2_added.id(), layout2.id());
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout2_added, attr2, &info));
  EXPECT_NE(info.offset(), 0);
  EXPECT_NE(info.offset(), 2);
}

static RawObject createLayoutAttribute(Runtime* runtime, const Object& name,
                                       uword flags) {
  HandleScope scope;
  Object info(&scope, AttributeInfo(0, flags).asSmallInt());
  Tuple entry(&scope, runtime->newTupleWith2(name, info));
  Tuple result(&scope, runtime->newTupleWith1(entry));
  return *result;
}

TEST_F(LayoutTest, DeleteAndAddInObjectAttribute) {
  HandleScope scope(thread_);

  // Create a new layout with one overflow attribute and one in-object
  // attribute
  Layout layout(&scope, testing::layoutCreateEmpty(thread_));
  Object inobject(&scope, Runtime::internStrFromCStr(thread_, "inobject"));
  layout.setInObjectAttributes(
      createLayoutAttribute(runtime_, inobject, AttributeFlags::kInObject));
  Object overflow(&scope, runtime_->newStrFromCStr("overflow"));
  layout.setOverflowAttributes(createLayoutAttribute(runtime_, overflow, 0));

  // Delete the in-object attribute and add it back. It should be re-added as
  // an overflow attribute.
  AttributeInfo info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, inobject, &info));
  RawObject result =
      runtime_->layoutDeleteAttribute(thread_, layout, inobject, info);
  ASSERT_TRUE(result.isLayout());
  Layout layout2(&scope, result);
  result = runtime_->layoutAddAttribute(thread_, layout2, inobject, 0, &info);
  ASSERT_TRUE(result.isLayout());
  Layout layout3(&scope, result);
  AttributeInfo info2;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout3, inobject, &info2));
  EXPECT_EQ(info2.offset(), 1);
  EXPECT_TRUE(info2.isOverflow());
}

TEST_F(LayoutTest, VerifyChildLayout) {
  HandleScope scope(thread_);
  Layout parent(&scope, testing::layoutCreateEmpty(thread_));
  runtime_->layoutSetTupleOverflow(*parent);
  Object attr(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  AttributeInfo info;
  Layout child(&scope,
               runtime_->layoutAddAttribute(Thread::current(), parent, attr,
                                            AttributeFlags::kNone, &info));

  EXPECT_NE(child.id(), parent.id());
  EXPECT_EQ(child.numInObjectAttributes(), parent.numInObjectAttributes());
  EXPECT_EQ(child.inObjectAttributes(), parent.inObjectAttributes());
  // Child should have an additional overflow attribute
  EXPECT_NE(child.overflowAttributes(), parent.overflowAttributes());
  EXPECT_NE(child.additions(), parent.additions());
  EXPECT_EQ(List::cast(child.additions()).numItems(), 0);
  EXPECT_NE(child.deletions(), parent.deletions());
  EXPECT_EQ(List::cast(child.deletions()).numItems(), 0);
  EXPECT_EQ(child.describedType(), parent.describedType());
  EXPECT_EQ(child.instanceSize(), parent.instanceSize());
}

}  // namespace py
