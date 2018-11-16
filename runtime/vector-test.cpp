#include "gtest/gtest.h"

#include "vector.h"

namespace python {

TEST(VectorTest, BasicTest) {
  Vector<int> int_v;
  for (int i = 0; i < 10; i++) {
    int_v.push_back(i);
  }

  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(int_v[i], i);
  }

  int count = 0;
  for (auto const& el : int_v) {
    EXPECT_EQ(el, count);
    count++;
  }

  int_v[3] = 42;
  EXPECT_EQ(int_v[3], 42);

  auto& ref = int_v[4];
  ref = 123;
  EXPECT_EQ(123, int_v[4]) << "vector should return references";

  int_v.pop_back();
  EXPECT_EQ(9, int_v.size());
}

TEST(VectorTest, StackTest) {
  Vector<int> int_v1;
  FixedVector<int, 10> int_v;

  EXPECT_EQ(int_v.capacity(), 10);
  int_v.push_back(0);
  auto begin = int_v.begin();
  for (int i = 1; i < 10; i++) {
    int_v.push_back(i);
  }

  EXPECT_EQ(begin, int_v.begin()) << "vector should not have been reallocated";
  EXPECT_EQ(int_v.size(), 10);

  int_v.push_back(10);
  EXPECT_NE(begin, int_v.begin()) << "vector should be reallocated";
  for (int i = 0; i < 11; i++) {
    EXPECT_EQ(int_v[i], i);
  }
}

TEST(VectorTest, CopyTest) {
  Vector<int> int_v;
  for (int i = 0; i < 10; i++) {
    int_v.push_back(i);
  }

  Vector<int> copy_v(int_v);
  EXPECT_NE(copy_v.begin(), int_v.begin()) << "copy_v needs its own storage";
  EXPECT_EQ(copy_v.size(), int_v.size());
  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(int_v[i], copy_v[i]);
  }
  auto old_copy_v_begin = copy_v.begin();
  auto old_copy_v_capacity = copy_v.capacity();

  Vector<int> new_v;
  for (int i = 0; i < 4; i++) {
    new_v.push_back(i);
  }
  // has capacity 10 already, should be able to hold a size 4 vector without
  // reallocation.
  copy_v = new_v;
  EXPECT_EQ(old_copy_v_begin, copy_v.begin())
      << "should not require reallocation";
  EXPECT_EQ(new_v.size(), copy_v.size());
  EXPECT_EQ(old_copy_v_capacity, copy_v.capacity())
      << "capacity should not shrink";
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(i, copy_v[i]);
  }
}

TEST(VectorTest, MoveTest) {
  Vector<int> int_v;
  for (int i = 0; i < 10; i++) {
    int_v.push_back(i);
  }
  auto const old_begin = int_v.begin();

  Vector<int> copy_v(std::move(int_v));
  EXPECT_EQ(old_begin, copy_v.begin()) << "Storage should be stolen";
  EXPECT_EQ(nullptr, int_v.begin());
  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(i, copy_v[i]);
  }

  Vector<int> int_v2;
  int_v2 = std::move(copy_v);
  EXPECT_EQ(nullptr, copy_v.begin());
  EXPECT_EQ(old_begin, int_v2.begin()) << "Storage should be stolen";
}

TEST(VectorTest, MoveOutOfStackTest) {
  FixedVector<int, 10> int_v;
  for (int i = 0; i < 10; i++) {
    int_v.push_back(i);
  }
  auto const old_begin = int_v.begin();

  Vector<int> copy_v(std::move(int_v));
  EXPECT_NE(old_begin, copy_v.begin()) << "Storage should not be stolen";
  EXPECT_EQ(nullptr, int_v.begin());
  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(i, copy_v[i]);
  }
  auto const prev_begin = copy_v.begin();

  Vector<int> int_v2;
  int_v2 = std::move(copy_v);
  EXPECT_EQ(nullptr, copy_v.begin());
  EXPECT_EQ(prev_begin, int_v2.begin()) << "Storage should be stolen";
}

TEST(VectorTest, MoveIntoStackTest) {
  Vector<int> int_v;
  for (int i = 0; i < 10; i++) {
    int_v.push_back(i);
  }
  auto const old_begin = int_v.begin();

  FixedVector<int, 10> copy_v(std::move(int_v));
  EXPECT_EQ(old_begin, copy_v.begin()) << "Storage should be stolen";
  EXPECT_EQ(nullptr, int_v.begin());
  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(i, copy_v[i]);
  }

  FixedVector<int, 10> int_v2;
  int_v2 = std::move(copy_v);
  EXPECT_EQ(nullptr, copy_v.begin());
  EXPECT_EQ(old_begin, int_v2.begin()) << "Storage should be stolen";
}

TEST(VectorTest, Reserve) {
  Vector<int> int_v;
  int_v.reserve(10);
  int_v.push_back(0);
  auto begin = int_v.begin();
  for (int i = 1; i < 10; i++) {
    int_v.push_back(i);
  }

  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(int_v[i], i);
  }

  EXPECT_EQ(begin, int_v.begin()) << "vector should not have been reallocated";

  int_v.push_back(10);
  EXPECT_NE(begin, int_v.begin()) << "vector should be reallocated";
  for (int i = 0; i < 11; i++) {
    EXPECT_EQ(int_v[i], i);
  }
}

TEST(VectorTest, TypeErasureTest) {
  FixedVector<int, 10> int_v;

  EXPECT_EQ(int_v.capacity(), 10);
  for (int i = 0; i < 10; i++) {
    int_v.push_back(i);
  }

  const int* observed_begin = nullptr;
  auto fn = [&](const Vector<int>& v) { observed_begin = v.begin(); };

  fn(int_v);
  // Verify the call doesn't create a temporary.
  EXPECT_EQ(int_v.begin(), observed_begin) << "FixedVector should be-a Vector";
}

} // namespace python
