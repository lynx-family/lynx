// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/path_utils.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

TEST(PathUtils, AbsolutePathTest) {
  std::string dirname = "/pages/index/";
  std::string url = "/assert/logo.png";
  EXPECT_EQ("/assert/logo.png", PathUtils::RedirectUrlPath(dirname, url));
}

TEST(PathUtils, RelativePathTest) {
  std::string dirname = "/pages/index/";
  std::string url = "../../assert/logo.png";
  EXPECT_EQ("/assert/logo.png", PathUtils::RedirectUrlPath(dirname, url));
}

TEST(PathUtils, OtherPathTest) {
  std::string dirname = "/pages/index/";
  std::string url = "ttfile://page/logo.png";
  EXPECT_EQ("ttfile://page/logo.png", PathUtils::RedirectUrlPath(dirname, url));
}

TEST(PathUtils, GetLastPath) {
  static const struct {
    std::string file_name;
    std::string expected;
  } cases[] = {{__FILE__, "path_utils_unittest.cc"},
               {"path_utils.cc", "path_utils.cc"},
               {"", ""}};

  for (const auto& test : cases) {
    EXPECT_EQ(std::string(PathUtils::GetLastPath(test.file_name.c_str(),
                                                 test.file_name.length())),
              test.expected);
  }
}

}  // namespace base
}  // namespace lynx
