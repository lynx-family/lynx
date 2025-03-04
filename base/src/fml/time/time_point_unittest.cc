// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <thread>

#include "base/include/fml/time/chrono_timestamp_provider.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace fml {
namespace {

TEST(TimePoint, Control) {
  EXPECT_LT(TimePoint::Min(), ChronoTicksSinceEpoch());
  EXPECT_GT(TimePoint::Max(), ChronoTicksSinceEpoch());
}

}  // namespace
}  // namespace fml
}  // namespace lynx
