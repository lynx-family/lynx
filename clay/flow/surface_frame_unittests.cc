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

}  // namespace clay
