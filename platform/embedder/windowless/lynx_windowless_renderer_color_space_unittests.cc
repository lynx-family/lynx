// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/public/lynx_windowless_renderer.h"
#include "platform/embedder/windowless/lynx_windowless_renderer_priv.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace {
using WindowlessColorSpaceTest =
    ::testing::TestWithParam<lynx_windowless_renderer_type_e>;

TEST_P(WindowlessColorSpaceTest, PreservesOutputEncodingAfterInitialization) {
  auto type = GetParam();
  for (auto space : {kLynxColorSpaceSRGB, kLynxColorSpaceDisplayP3}) {
    auto renderer = std::make_shared<lynx::pub::LynxWindowlessRenderer>(type);
    EXPECT_EQ(renderer->GetColorSpace(), kLynxColorSpaceSRGB);
    if (space == kLynxColorSpaceDisplayP3) {
      ASSERT_TRUE(renderer->SetColorSpace(space));
    } else {
      renderer->InitIfNeeded();
    }
    auto* impl = reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(
        renderer->Impl());
    auto* config = impl->GetRendererConfig();
    auto expected = space == kLynxColorSpaceDisplayP3 ? kClayColorSpaceDisplayP3
                                                      : kClayColorSpaceSRGB;
    if (type == kRendererTypeSoftware) {
      EXPECT_EQ(config->software.color_space, expected);
    } else if (type == kRendererTypeGL || type == kRendererTypeGLDirect) {
      EXPECT_EQ(config->opengl.color_space, expected);
      EXPECT_EQ(config->opengl.enable_shared_image_sink,
                type == kRendererTypeGL);
    } else {
      ASSERT_NE(config->hardware.sink_ref, nullptr);
      const ClaySize size{16, 16};
      ClaySharedImageRef image = nullptr;
      ASSERT_TRUE(ClaySharedImageSinkAcquireBack(config->hardware.sink_ref,
                                                 &size, &image, nullptr));
      EXPECT_EQ(ClaySharedImageGetColorSpace(image), expected);
    }
    EXPECT_FALSE(renderer->SetColorSpace(space == kLynxColorSpaceDisplayP3
                                             ? kLynxColorSpaceSRGB
                                             : kLynxColorSpaceDisplayP3));
    EXPECT_EQ(renderer->GetColorSpace(), space);
  }
}

INSTANTIATE_TEST_SUITE_P(Desktop, WindowlessColorSpaceTest,
                         ::testing::Values(kRendererTypeSoftware,
                                           kRendererTypeGL,
                                           kRendererTypeGLDirect,
                                           kRendererTypeAccelerated));
}  // namespace
