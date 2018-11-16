#include "gtest/gtest.h"

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  auto status = RUN_ALL_TESTS();
  if (status != 0) {
    return status;
  }
  return 0;
}
