// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/version_util.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace testing {

const constexpr Version V_2_12(2, 12);
const constexpr Version V_2_6(2, 6);

TEST(VersionTest, checkVersion) {
  ASSERT_TRUE(Version("") < Version("1"));
  ASSERT_TRUE(Version("sdasdadadafd") < Version("1"));
  ASSERT_TRUE(Version(2, 12) == V_2_12);
  ASSERT_TRUE(Version(2, 11, 1, 2) == Version(2, 11, 1, 2));
  ASSERT_TRUE(Version(2, 11) != V_2_12);
  ASSERT_TRUE(Version(2, 8) > V_2_6);
  ASSERT_TRUE(Version(2, 6) >= V_2_6);
  ASSERT_TRUE(Version(2, 6) == V_2_6);
  ASSERT_TRUE(Version(2, 6) <= V_2_6);
  ASSERT_TRUE(Version(2, 5) < V_2_6);
}

TEST(VersionTest, ToString) {
  ASSERT_TRUE(Version(2, 11).ToString() == "2.11");
  ASSERT_TRUE(Version(2, 8).ToString() == "2.8");
  ASSERT_TRUE(Version(2, 6).ToString() == "2.6");
  ASSERT_TRUE(Version(2, 5).ToString() == "2.5");
}

}  // namespace testing
}  // namespace base
}  // namespace lynx
