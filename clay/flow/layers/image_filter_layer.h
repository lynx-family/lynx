// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_IMAGE_FILTER_LAYER_H_
#define CLAY_FLOW_LAYERS_IMAGE_FILTER_LAYER_H_

#include <memory>
#include <string>

#include "clay/flow/layers/cacheable_layer.h"
#include "clay/flow/layers/layer.h"

namespace clay {

class ImageFilterLayer : public CacheableContainerLayer {
 public:
  explicit ImageFilterLayer(std::shared_ptr<const clay::ImageFilter> filter,
                            const skity::Vec2& offset = skity::Vec2(0, 0));

  void Diff(DiffContext* context, const Layer* old_layer) override;

  void Preroll(PrerollContext* context) override;

  void Paint(PaintContext& context) const override;

#ifndef NDEBUG
  std::string DebugName() const override { return "ImageFilterLayer"; }
#endif

 private:
  skity::Vec2 offset_;
  std::shared_ptr<const clay::ImageFilter> filter_;
  std::shared_ptr<const clay::ImageFilter> transformed_filter_;

  BASE_DISALLOW_COPY_AND_ASSIGN(ImageFilterLayer);
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_IMAGE_FILTER_LAYER_H_
