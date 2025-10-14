// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_RENDER_EXTERNAL_CONTENT_H_
#define CLAY_UI_RENDERING_RENDER_EXTERNAL_CONTENT_H_

#include "clay/common/graphics/drawable_image.h"
#include "clay/ui/rendering/render_box.h"

namespace clay {

class RenderExternalContent : public RenderBox {
 public:
  enum class RenderMode { kHybrid = 0, kExternalTexture = 1 };
  RenderExternalContent();

  ~RenderExternalContent() override = default;

  const char* GetName() const override { return "RenderExternalContent"; }
  bool IsRepaintBoundary() const override { return true; }
  bool HasClip() const override;
  void SetClip() { has_clip_ = true; }

  void Paint(PaintingContext& context, const FloatPoint& offset) override;

  // For hybrid composition
  void SetViewId(int64_t view_id);
  bool HasViewId() const { return view_id_.has_value(); }

  // For drawable image layer
  void SetDrawableImageId(int64_t drawable_image_id);
  void SetFitMode(DrawableImage::FitMode fit_mode) { fit_mode_ = fit_mode; }

  // For video
  void SetRenderMode(RenderMode render_mode);
  RenderMode GetRenderMode() const { return render_mode_.value(); }

 private:
  DrawableImage::FitMode fit_mode_ = DrawableImage::FitMode::kScaleToFill;
  std::optional<bool> has_clip_;
  std::optional<int64_t> view_id_;  // has value if use hybrid composition
  std::optional<int64_t>
      drawable_image_id_;  // has value if use drawable image layer
  // has value if use video
  std::optional<RenderMode> render_mode_ = RenderMode::kExternalTexture;
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_RENDER_EXTERNAL_CONTENT_H_
