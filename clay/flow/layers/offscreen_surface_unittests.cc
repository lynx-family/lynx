// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/flow/layers/offscreen_surface.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkData.h"

namespace clay::testing {

TEST(OffscreenSurfaceTest, EmptySurfaceIsInvalid) {
  auto surface = std::make_unique<OffscreenSurface>(nullptr, skity::Vec2());
  ASSERT_FALSE(surface->IsValid());
}

TEST(OffscreenSurfaceTest, OnexOneSurfaceIsValid) {
  auto surface = std::make_unique<OffscreenSurface>(nullptr, skity::Vec2(1, 1));
  ASSERT_TRUE(surface->IsValid());
}

TEST(OffscreenSurfaceTest, PaintSurfaceBlack) {
  auto surface = std::make_unique<OffscreenSurface>(nullptr, skity::Vec2(1, 1));

  SkCanvas* canvas = surface->GetCanvas();
  canvas->clear(SK_ColorBLACK);
  canvas->flush();

  auto raster_data = surface->GetRasterData(false);
  const uint32_t* actual =
      reinterpret_cast<const uint32_t*>(raster_data->data());

  // picking black as the color since byte ordering seems to matter.
  ASSERT_EQ(actual[0], 0xFF000000u);
}

}  // namespace clay::testing
