// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <unordered_map>

#include "clay/fml/container.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace fml {
namespace {

TEST(ContainerTest, MapEraseIf) {
  std::unordered_map<int, int> map = {{0, 1}, {2, 3}, {4, 5}};

  fml::erase_if(map, [](std::unordered_map<int, int>::iterator it) {
    return it->first == 0 || it->second == 5;
  });

  EXPECT_EQ(map.size(), 1u);
  EXPECT_TRUE(map.find(2) != map.end());
}

}  // namespace
}  // namespace fml
