// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/testing/test_metal_context.h"
#include "clay/testing/test_metal_surface.h"
#include "clay/testing/testing.h"

namespace clay {
namespace testing {

#ifdef SHELL_ENABLE_METAL

TEST(TestMetalSurface, EmptySurfaceIsInvalid) {
  if (!TestMetalSurface::PlatformSupportsMetal()) {
    GTEST_SKIP();
  }

  TestMetalContext metal_context = TestMetalContext();
  auto surface = TestMetalSurface::Create(metal_context);
  ASSERT_NE(surface, nullptr);
  ASSERT_FALSE(surface->IsValid());
}

TEST(TestMetalSurface, CanCreateValidTestMetalSurface) {
  if (!TestMetalSurface::PlatformSupportsMetal()) {
    GTEST_SKIP();
  }

  TestMetalContext metal_context = TestMetalContext();
  auto surface =
      TestMetalSurface::Create(metal_context, SkISize::Make(100, 100));
  ASSERT_NE(surface, nullptr);
  ASSERT_TRUE(surface->IsValid());
  ASSERT_NE(surface->GetSurface(), nullptr);
  ASSERT_NE(surface->GetGrContext(), nullptr);
}

#endif

}  // namespace testing
}  // namespace clay
