// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/fml/paths.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(Paths, SanitizeURI) {
  ASSERT_EQ(fml::paths::SanitizeURIEscapedCharacters("hello"), "hello");
  ASSERT_EQ(fml::paths::SanitizeURIEscapedCharacters(""), "");
  ASSERT_EQ(fml::paths::SanitizeURIEscapedCharacters("hello%20world"),
            "hello world");
  ASSERT_EQ(fml::paths::SanitizeURIEscapedCharacters(
                "%5Chello%5cworld%20foo%20bar%21"),
            "\\hello\\world foo bar!");
}
