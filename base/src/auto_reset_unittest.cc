// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/auto_reset.h"

#include <utility>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

TEST(AutoReset, Move) {
  int value = 10;
  {
    AutoReset<int> ar1{&value, 20};
    EXPECT_EQ(20, value);
    {
      value = 15;
      AutoReset<int> ar2 = std::move(ar1);
      // Moving to a new re-setter does not change the value;
      EXPECT_EQ(15, value);
    }
    // Moved-to `ar2` is out of scoped, and resets to the original value
    // that was in moved-from `ar1`.
    EXPECT_EQ(10, value);
    value = 105;
  }
  // Moved-from `ar1` does not reset to anything.
  EXPECT_EQ(105, value);
}

}  // namespace base
}  // namespace lynx
