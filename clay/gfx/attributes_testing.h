// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ATTRIBUTES_TESTING_H_
#define CLAY_GFX_ATTRIBUTES_TESTING_H_

#include <string>

#include "clay/gfx/attributes.h"
#include "clay/gfx/comparable.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

template <class T>
static void TestEquals(T& source1, T& source2) {
  ASSERT_TRUE(source1 == source2);
  ASSERT_TRUE(source2 == source1);
  ASSERT_FALSE(source1 != source2);
  ASSERT_FALSE(source2 != source1);
  ASSERT_EQ(source1, source2);
  ASSERT_EQ(source2, source1);
  ASSERT_TRUE(clay::Equals(&source1, &source2));
  ASSERT_TRUE(clay::Equals(&source2, &source1));
}

template <class T>
static void TestNotEquals(T& source1, T& source2, std::string label) {
  ASSERT_FALSE(source1 == source2) << label;
  ASSERT_FALSE(source2 == source1) << label;
  ASSERT_TRUE(source1 != source2) << label;
  ASSERT_TRUE(source2 != source1) << label;
  ASSERT_NE(source1, source2) << label;
  ASSERT_NE(source2, source1) << label;
  ASSERT_TRUE(clay::NotEquals(&source1, &source2));
  ASSERT_TRUE(clay::NotEquals(&source2, &source1));
}

}  // namespace testing
}  // namespace clay

#endif  // CLAY_GFX_ATTRIBUTES_TESTING_H_
