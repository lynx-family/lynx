// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/application_directory.h"

#include <chrono>
#include <filesystem>
#include <fstream>

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
  void SetUp() override {
    default_directory_ = GetApplicationDataDirectory();
    directory_ =
        (std::filesystem::temp_directory_path() /
         ("lynx-data-" +
          std::to_string(
              std::chrono::steady_clock::now().time_since_epoch().count())))
            .u8string();
    std::filesystem::create_directory(std::filesystem::u8path(directory_));
  }
  void TearDown() override {
    SetApplicationDataDirectory(default_directory_);
    std::filesystem::remove_all(std::filesystem::u8path(directory_));
  }

  std::string default_directory_;
  std::string directory_;
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

TEST_F(ApplicationDirectoryTest, CreatesSeparateChildrenAndReusesExistingOnes) {
  SetApplicationDataDirectory(directory_);
  for (const std::string name :
       {"trace", "testbench", u8"\u5e94\u7528 data \U0001f680"}) {
    const auto expected = directory_ + "/" + name;
    EXPECT_EQ(GetApplicationDataSubDirectory(name), expected);
    EXPECT_EQ(GetApplicationDataSubDirectory(name), expected);
    EXPECT_TRUE(
        std::filesystem::is_directory(std::filesystem::u8path(expected)));
  }
  EXPECT_EQ(GetApplicationDataDirectory(), directory_);
  EXPECT_EQ(std::distance(std::filesystem::directory_iterator(
                              std::filesystem::u8path(directory_)),
                          std::filesystem::directory_iterator()),
            3);
  SetApplicationDataDirectory(directory_ + "/");
  EXPECT_EQ(GetApplicationDataSubDirectory("trace"), directory_ + "/trace");
}

TEST_F(ApplicationDirectoryTest, RejectsInvalidChildNames) {
  SetApplicationDataDirectory(directory_);
  for (const std::string name :
       {"", ".", "..", "/trace", "../trace", "a/b", "a\\b", "\\trace"}) {
    SCOPED_TRACE(name);
    EXPECT_TRUE(GetApplicationDataSubDirectory(name).empty());
  }
  EXPECT_TRUE(
      GetApplicationDataSubDirectory(std::string("trace\0suffix", 12)).empty());
  EXPECT_TRUE(std::filesystem::is_empty(std::filesystem::u8path(directory_)));
}

TEST_F(ApplicationDirectoryTest, CreationFailuresCanRetry) {
  SetApplicationDataDirectory(directory_);
  const auto child = std::filesystem::u8path(directory_ + "/trace");
  std::ofstream(child) << "occupied";
  EXPECT_TRUE(GetApplicationDataSubDirectory("trace").empty());
  EXPECT_TRUE(std::filesystem::is_regular_file(child));
  std::filesystem::remove(child);
  EXPECT_EQ(GetApplicationDataSubDirectory("trace"), directory_ + "/trace");
  EXPECT_TRUE(std::filesystem::is_directory(child));

  const auto missing_root = directory_ + "/missing";
  SetApplicationDataDirectory(missing_root);
  EXPECT_TRUE(GetApplicationDataSubDirectory("trace").empty());
  EXPECT_FALSE(std::filesystem::exists(std::filesystem::u8path(missing_root)));
  std::filesystem::create_directory(std::filesystem::u8path(missing_root));
  EXPECT_EQ(GetApplicationDataSubDirectory("trace"), missing_root + "/trace");
}

TEST_F(ApplicationDirectoryTest, FollowsOverridesAndRecreatesRemovedChildren) {
  SetApplicationDataDirectory(directory_);
  EXPECT_EQ(GetApplicationDataSubDirectory("testbench"),
            directory_ + "/testbench");
  const auto replacement = directory_ + "/replacement";
  std::filesystem::create_directory(std::filesystem::u8path(replacement));
  SetApplicationDataDirectory(replacement);
  EXPECT_EQ(GetApplicationDataSubDirectory("testbench"),
            replacement + "/testbench");
  std::filesystem::remove(std::filesystem::u8path(replacement + "/testbench"));
  EXPECT_EQ(GetApplicationDataSubDirectory("testbench"),
            replacement + "/testbench");
  EXPECT_TRUE(std::filesystem::is_directory(
      std::filesystem::u8path(replacement + "/testbench")));
}

}  // namespace base
}  // namespace lynx
