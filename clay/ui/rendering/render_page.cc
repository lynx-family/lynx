// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/render_page.h"

namespace clay {

RenderPage::RenderPage() {
  SetLayer(std::make_unique<PendingTransformLayer>(skity::Matrix()));
}

RenderPage::~RenderPage() = default;

const char* RenderPage::GetName() const { return "RenderPage"; }

void RenderPage::Paint(PaintingContext& context, const FloatPoint& offset) {
  RenderBox::Paint(context, offset);
  PaintChildren(context, offset);
}

void RenderPage::SetScaleRatio(float ratio) {
  if (scale_ratio_ == ratio) {
    return;
  }

  scale_ratio_ = ratio;

  skity::Matrix matrix;
  matrix.Reset();
  matrix.SetScaleX(scale_ratio_);
  matrix.SetScaleY(scale_ratio_);

  PendingTransformLayer* layer =
      static_cast<PendingTransformLayer*>(GetLayer());
  if (layer) {
    layer->SetTransform(matrix);
  }
  MarkNeedsPaint();
}

#ifndef NDEBUG
std::string RenderPage::ToString() const {
  std::stringstream ss;
  ss << RenderBox::ToString();
  ss << " scale_ratio_=" << scale_ratio_;
  return ss.str();
}
#endif

}  // namespace clay
