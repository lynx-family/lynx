// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/testing/mock_drawable_image.h"

#include "clay/flow/layers/layer.h"
#include "clay/flow/testing/gpu_object_layer_test.h"

namespace clay {
namespace testing {

void MockDrawableImage::Paint(PaintContext& context, const skity::Rect& bounds,
                              bool freeze, const SkSamplingOptions& sampling,
                              FitMode fit_mode) {
  paint_calls_.emplace_back(PaintCall{*(context.canvas), bounds, freeze,
                                      context.gr_context, sampling,
                                      context.sk_paint, fit_mode});
}

bool operator==(const MockDrawableImage::PaintCall& a,
                const MockDrawableImage::PaintCall& b) {
  return &a.canvas == &b.canvas && a.bounds == b.bounds &&
         a.context == b.context && a.freeze == b.freeze &&
         a.sampling == b.sampling && a.paint == b.paint &&
         a.fit_mode == b.fit_mode;
}

std::ostream& operator<<(std::ostream& os,
                         const MockDrawableImage::PaintCall& data) {
  return os << &data.canvas << " " << data.bounds << " " << data.context << " "
            << data.freeze << " " << data.sampling << " " << data.paint << " "
            << static_cast<uint32_t>(data.fit_mode);
}

}  // namespace testing
}  // namespace clay
