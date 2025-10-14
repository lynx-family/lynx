// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_COLOR_FILTER_LAYER_H_
#define CLAY_FLOW_LAYERS_COLOR_FILTER_LAYER_H_

#include <memory>
#include <string>

#include "clay/flow/layers/cacheable_layer.h"
#include "clay/flow/layers/layer.h"
#include "clay/gfx/style/color_filter.h"

namespace clay {

class ColorFilterLayer : public CacheableContainerLayer {
 public:
  explicit ColorFilterLayer(std::shared_ptr<const clay::ColorFilter> filter);

  void Diff(DiffContext* context, const Layer* old_layer) override;

  void Preroll(PrerollContext* context) override;

  void Paint(PaintContext& context) const override;

#ifndef NDEBUG
  std::string DebugName() const override { return "ColorFilterLayer"; }
#endif

 private:
  std::shared_ptr<const clay::ColorFilter> filter_;

  BASE_DISALLOW_COPY_AND_ASSIGN(ColorFilterLayer);
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_COLOR_FILTER_LAYER_H_
