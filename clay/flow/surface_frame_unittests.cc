// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define FML_USED_ON_EMBEDDER

#include "clay/flow/surface_frame.h"
#include "clay/testing/testing.h"

namespace clay {

TEST(FlowTest, SurfaceFrameDoesNotSubmitInDtor) {
  SurfaceFrame::FramebufferInfo framebuffer_info;
  auto surface_frame = std::make_unique<SurfaceFrame>(
      /*surface=*/nullptr, framebuffer_info,
      /*encode_callback=*/[](const SurfaceFrame&, SkCanvas*) { return true; },
      /*submit_callback=*/

      [](const SurfaceFrame::SubmitInfo&) {
        EXPECT_FALSE(true);
        return true;
      },
      skity::Vec2{800, 600});
  surface_frame.reset();
}

TEST(FlowTest, SettingPresentWithTransactionPreservesDamage) {
  SurfaceFrame::FramebufferInfo framebuffer_info;
  SurfaceFrame surface_frame(
      /*surface=*/nullptr, framebuffer_info,
      /*encode_callback=*/
      [](SurfaceFrame& frame, clay::GrCanvas*) {
        EXPECT_EQ(frame.submit_info().frame_damage,
                  skity::Rect::MakeLTRB(10, 20, 30, 40));
        EXPECT_EQ(frame.submit_info().buffer_damage,
                  skity::Rect::MakeLTRB(0, 0, 100, 100));
        EXPECT_TRUE(frame.submit_info().present_with_transaction);
        return true;
      },
      /*submit_callback=*/[](const SurfaceFrame::SubmitInfo&) { return true; },
      skity::Vec2{800, 600});

  SurfaceFrame::SubmitInfo submit_info;
  submit_info.frame_damage = skity::Rect::MakeLTRB(10, 20, 30, 40);
  submit_info.buffer_damage = skity::Rect::MakeLTRB(0, 0, 100, 100);
  surface_frame.set_submit_info(submit_info);
  surface_frame.set_present_with_transaction(true);

  EXPECT_TRUE(surface_frame.Encode());
}

}  // namespace clay
