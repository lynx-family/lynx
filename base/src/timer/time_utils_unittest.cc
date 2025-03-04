// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/timer/time_utils.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

TEST(TimeUtils, GetCurrentTimeSimple) {
  EXPECT_GT(CurrentTimeMicroseconds(), static_cast<uint64_t>(0));
  EXPECT_GT(CurrentTimeMilliseconds(), static_cast<uint64_t>(0));
}

}  // namespace base
}  // namespace lynx
