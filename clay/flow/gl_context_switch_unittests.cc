// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define FML_USED_ON_EMBEDDER

#include <functional>
#include <future>
#include <memory>

#include "clay/common/graphics/gl_context_switch.h"
#include "clay/flow/testing/gl_context_switch_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

TEST(GLContextSwitchTest, SwitchKeepsContextCurrentWhileInScope) {
  {
    auto test_gl_context = std::make_unique<TestSwitchableGLContext>(0);
    auto context_switch = GLContextSwitch(std::move(test_gl_context));
    ASSERT_EQ(TestSwitchableGLContext::GetCurrentContext(), 0);
  }
  ASSERT_EQ(TestSwitchableGLContext::GetCurrentContext(), -1);
}

}  // namespace testing
}  // namespace clay
