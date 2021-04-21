#include "extension-object.h"

#include "gtest/gtest.h"

#include "capi.h"
#include "linked-list.h"
#include "test-utils.h"

namespace py {
namespace testing {

using ExtensionObjectTest = RuntimeFixture;

TEST_F(ExtensionObjectTest, TrackExtensionObjectAndUntrackExtensionObject) {
  ListEntry entry0{nullptr, nullptr};
  ListEntry entry1{nullptr, nullptr};

  EXPECT_EQ(numExtensionObjects(runtime_), 0);
  EXPECT_TRUE(trackExtensionObject(runtime_, &entry0));
  EXPECT_EQ(numExtensionObjects(runtime_), 1);
  EXPECT_TRUE(trackExtensionObject(runtime_, &entry1));
  EXPECT_EQ(numExtensionObjects(runtime_), 2);
  // Trying to track an already tracked object returns false.
  EXPECT_FALSE(trackExtensionObject(runtime_, &entry0));
  EXPECT_EQ(numExtensionObjects(runtime_), 2);
  EXPECT_FALSE(trackExtensionObject(runtime_, &entry1));
  EXPECT_EQ(numExtensionObjects(runtime_), 2);

  EXPECT_TRUE(untrackExtensionObject(runtime_, &entry0));
  EXPECT_EQ(numExtensionObjects(runtime_), 1);
  EXPECT_TRUE(untrackExtensionObject(runtime_, &entry1));
  EXPECT_EQ(numExtensionObjects(runtime_), 0);

  // Trying to untrack an already untracked object returns false.
  EXPECT_FALSE(untrackExtensionObject(runtime_, &entry0));
  EXPECT_EQ(numExtensionObjects(runtime_), 0);
  EXPECT_FALSE(untrackExtensionObject(runtime_, &entry1));
  EXPECT_EQ(numExtensionObjects(runtime_), 0);

  // Verify untracked entires are reset to nullptr.
  EXPECT_EQ(entry0.prev, nullptr);
  EXPECT_EQ(entry0.next, nullptr);
  EXPECT_EQ(entry1.prev, nullptr);
  EXPECT_EQ(entry1.next, nullptr);
}

}  // namespace testing
}  // namespace py
