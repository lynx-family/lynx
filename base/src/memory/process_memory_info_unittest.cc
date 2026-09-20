// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/memory/process_memory_info.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

TEST(ProcessMemoryInfoTest, ReturnsPositivePss) {
  EXPECT_GT(GetProcessPssBytes(), 0);
}

}  // namespace base
}  // namespace lynx
