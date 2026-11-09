// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/application_directory.h"
#include "base/src/application_directory/application_directory_impl.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

TEST(ApplicationDirectoryTest, InitializationAfterEarlyQuery) {
  const auto previous = GetApplicationDataDirectory();
  SetApplicationDataDirectory({});
  EXPECT_TRUE(GetApplicationDataDirectory().empty());
  SetApplicationDataDirectory("/application/files");
  EXPECT_EQ(GetApplicationDataDirectory(), "/application/files");
  SetApplicationDataDirectory(previous);
}

}  // namespace base
}  // namespace lynx
