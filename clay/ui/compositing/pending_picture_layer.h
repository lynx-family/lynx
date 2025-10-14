// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPOSITING_PENDING_PICTURE_LAYER_H_
#define CLAY_UI_COMPOSITING_PENDING_PICTURE_LAYER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/picture.h"
#include "clay/ui/compositing/frame_builder.h"
#include "clay/ui/compositing/pending_layer.h"

namespace clay {

class ImageResource;

// A pending picture layer contains a [Picture].
//
// Picture layers are always leaves in the layer tree.
class PendingPictureLayer : public PendingLayer {
 public:
  PendingPictureLayer();
  ~PendingPictureLayer() override;

  std::string GetName() const override { return "PendingPictureLayer"; }

  bool IsComplexHint() const { return is_complex_hint_; }
  void SetIsComplexHint() {
    is_complex_hint_ = true;
    MarkNeedsAddToFrame();
  }

  bool WillChangeHint() const { return will_change_hint_; }
  void SetWillChangeHint() {
    will_change_hint_ = true;
    MarkNeedsAddToFrame();
  }

  void set_picture(std::unique_ptr<Picture> picture) {
    picture_ = std::move(picture);
    MarkNeedsAddToFrame();
  }
  const Picture* picture() const { return picture_.get(); }

 private:
  void AddToFrame(FrameBuilder* builder, const FloatPoint& offset) override;

  // Hints that the painting in this layer is complex and would benefit from
  // caching.
  bool is_complex_hint_ = false;
  // Hints that the painting in this layer is likely to change next frame.
  bool will_change_hint_ = false;
  std::unique_ptr<Picture> picture_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPOSITING_PENDING_PICTURE_LAYER_H_
