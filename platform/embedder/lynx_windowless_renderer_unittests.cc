// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "platform/embedder/public/capi/lynx_windowless_renderer_capi.h"
#include "platform/embedder/public/lynx_windowless_renderer.h"
#include "platform/embedder/windowless/lynx_windowless_renderer_priv.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace embedder {
namespace {

class TestWindowlessRenderer : public pub::LynxWindowlessRenderer {
 public:
  TestWindowlessRenderer()
      : pub::LynxWindowlessRenderer(kRendererTypeSoftware) {}
};

TEST(LynxWindowlessRendererTest, SetBackgroundColorStoresAndForwardsUpdates) {
  lynx_windowless_renderer_set_background_color(nullptr, 0xFF123456u);

  auto renderer = std::make_shared<TestWindowlessRenderer>();
  renderer->InitIfNeeded();
  auto* impl = reinterpret_cast<LynxWindowlessRenderer*>(renderer->Impl());
  ASSERT_NE(impl, nullptr);
  EXPECT_FALSE(impl->background_color.has_value());

  // Setting before engine attachment records the initial canvas color.
  renderer->SetBackgroundColor(0xFF121212u);
  ASSERT_TRUE(impl->background_color.has_value());
  EXPECT_EQ(*impl->background_color, 0xFF121212u);

  // Attaching the live callback forwards subsequent runtime updates.
  std::vector<uint32_t> forwarded;
  impl->set_background_color = [&forwarded](uint32_t color) {
    forwarded.push_back(color);
  };
  renderer->SetBackgroundColor(0xFF345678u);
  lynx_windowless_renderer_set_background_color(renderer->Impl(), 0x00000000u);

  ASSERT_EQ(forwarded.size(), 2u);
  EXPECT_EQ(forwarded[0], 0xFF345678u);
  EXPECT_EQ(forwarded[1], 0x00000000u);
  EXPECT_EQ(*impl->background_color, 0x00000000u);

  // Detaching stops forwarding while retaining the latest configured color.
  impl->set_background_color = nullptr;
  renderer->SetBackgroundColor(0xFFABCDEFu);
  EXPECT_EQ(forwarded.size(), 2u);
  EXPECT_EQ(*impl->background_color, 0xFFABCDEFu);
}

}  // namespace
}  // namespace embedder
}  // namespace lynx
