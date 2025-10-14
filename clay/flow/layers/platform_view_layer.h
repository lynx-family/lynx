// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_PLATFORM_VIEW_LAYER_H_
#define CLAY_FLOW_LAYERS_PLATFORM_VIEW_LAYER_H_

#include <string>

#include "clay/flow/layers/layer.h"

namespace clay {

class PlatformViewLayer : public Layer {
 public:
  PlatformViewLayer(const skity::Vec2& offset, const skity::Vec2& size,
                    int64_t view_id);

  void Preroll(PrerollContext* context) override;
  void Paint(PaintContext& context) const override;

#ifndef NDEBUG
  std::string DebugName() const override { return "PlatformViewLayer"; }
#endif

 private:
  skity::Vec2 offset_;
  skity::Vec2 size_;
  int64_t view_id_;

  BASE_DISALLOW_COPY_AND_ASSIGN(PlatformViewLayer);
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_PLATFORM_VIEW_LAYER_H_
