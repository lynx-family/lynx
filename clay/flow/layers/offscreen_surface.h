// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_OFFSCREEN_SURFACE_H_
#define CLAY_FLOW_LAYERS_OFFSCREEN_SURFACE_H_

#include "base/include/fml/macros.h"
#include "skity/geometry/point.hpp"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace clay {

class OffscreenSurface {
 public:
  explicit OffscreenSurface(GrDirectContext* surface_context,
                            const skity::Vec2& size, bool opaque = true);

  ~OffscreenSurface() = default;

  sk_sp<SkData> GetRasterData(bool compressed) const;

  sk_sp<SkImage> GetRasterImage() const;

  SkCanvas* GetCanvas() const;

  bool IsValid() const;

 private:
  sk_sp<SkSurface> offscreen_surface_;

  BASE_DISALLOW_COPY_AND_ASSIGN(OffscreenSurface);
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_OFFSCREEN_SURFACE_H_
