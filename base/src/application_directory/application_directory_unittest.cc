// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/application_directory.h"

#include "base/src/application_directory/application_directory_impl.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#if defined(_WIN32)
#include <windows.h>

#include "base/include/string/string_conversion_win.h"
#else
#include <sys/stat.h>
#endif

namespace lynx {
namespace base {

class ApplicationDirectoryTest : public ::testing::Test {
 protected:
  void SetUp() override { default_directory_ = GetApplicationDataDirectory(); }
  void TearDown() override { SetApplicationDataDirectory(default_directory_); }

  std::string default_directory_;
};

TEST_F(ApplicationDirectoryTest, ReturnsExistingAbsoluteDirectory) {
  const auto directory = GetApplicationDataDirectory();
  ASSERT_FALSE(directory.empty());
  EXPECT_EQ(GetApplicationDataDirectory(), directory);
#if defined(_WIN32)
  const auto path = Utf16FromUtf8(directory);
  EXPECT_TRUE((path.size() >= 3 && path[1] == L':') ||
              path.compare(0, 2, L"\\\\") == 0);
  const auto attributes = GetFileAttributesW(path.c_str());
  ASSERT_NE(attributes, INVALID_FILE_ATTRIBUTES);
  EXPECT_NE(attributes & FILE_ATTRIBUTE_DIRECTORY, 0u);
#else
  EXPECT_EQ(directory.front(), '/');
  struct stat info {};
  ASSERT_EQ(stat(directory.c_str(), &info), 0);
  EXPECT_TRUE(S_ISDIR(info.st_mode));
#endif
}

TEST_F(ApplicationDirectoryTest, OverrideAfterDefaultLookup) {
  ASSERT_FALSE(default_directory_.empty());
  const auto custom_directory = default_directory_ + "/custom";
  SetApplicationDataDirectory(custom_directory);
  EXPECT_EQ(GetApplicationDataDirectory(), custom_directory);

  const auto replacement_directory = default_directory_ + "/replacement";
  SetApplicationDataDirectory(replacement_directory);
  EXPECT_EQ(GetApplicationDataDirectory(), replacement_directory);
}

TEST_F(ApplicationDirectoryTest, ClearOverrideRestoresDefault) {
  ASSERT_FALSE(default_directory_.empty());
  SetApplicationDataDirectory(default_directory_ + "/custom");
  SetApplicationDataDirectory({});
  EXPECT_EQ(GetApplicationDataDirectory(), default_directory_);
}

}  // namespace base
}  // namespace lynx
