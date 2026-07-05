// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_COMMON_DESKTOP_SVG_SKITY_SVG_CANVAS_DELEGATE_H_
#define CLAY_SHELL_PLATFORM_COMMON_DESKTOP_SVG_SKITY_SVG_CANVAS_DELEGATE_H_
#include <memory>

#include "skity/recorder/recording_canvas.hpp"

namespace clay {
class SkitySVGCanvasDelegate : public skity::Canvas {
 public:
  explicit SkitySVGCanvasDelegate(skity::RecordingCanvas* canvas)
      : canvas_(canvas) {}
  ~SkitySVGCanvasDelegate() override = default;

  bool CanUseSoftwareCanvas() { return can_use_software_canvas_; }

 protected:
  void OnClipRect(skity::Rect const& rect, ClipOp op) override {
    canvas_->ClipRect(rect, op);
  }
  void OnClipPath(skity::Path const& path, ClipOp op) override {
    canvas_->ClipPath(path, op);
  }
  void OnDrawRect(skity::Rect const& rect, skity::Paint const& paint) override {
    canvas_->DrawRect(rect, paint);
    CheckUseSoftwareCanvas(paint);
  }
  void OnDrawPath(skity::Path const& path, skity::Paint const& paint) override {
    canvas_->DrawPath(path, paint);
    CheckUseSoftwareCanvas(paint);
  }
  void OnSaveLayer(const skity::Rect& bounds,
                   const skity::Paint& paint) override {
    canvas_->SaveLayer(bounds, paint);
    can_use_software_canvas_ = false;
  }
  void OnDrawBlob(const skity::TextBlob* blob, float x, float y,
                  skity::Paint const& paint) override {
    canvas_->DrawTextBlob(blob, x, y, paint);
    CheckUseSoftwareCanvas(paint);
  }
  void OnDrawImageRect(std::shared_ptr<skity::Image> image,
                       const skity::Rect& src, const skity::Rect& dst,
                       const skity::SamplingOptions& sampling,
                       skity::Paint const* paint) override {
    canvas_->DrawImageRect(image, src, dst, sampling, paint);
    can_use_software_canvas_ = false;
  }
  void OnDrawGlyphs(uint32_t count, const skity::GlyphID glyphs[],
                    const float position_x[], const float position_y[],
                    const skity::Font& font,
                    const skity::Paint& paint) override {
    canvas_->DrawGlyphs(count, glyphs, position_x, position_y, font, paint);
    CheckUseSoftwareCanvas(paint);
  }
  void OnDrawPaint(skity::Paint const& paint) override {
    canvas_->DrawPaint(paint);
    CheckUseSoftwareCanvas(paint);
  }
  void OnSave() override { canvas_->Save(); }
  void OnRestore() override { canvas_->Restore(); }
  void OnRestoreToCount(int saveCount) override {
    canvas_->RestoreToCount(saveCount);
  }
  void OnTranslate(float dx, float dy) override { canvas_->Translate(dx, dy); }
  void OnScale(float sx, float sy) override { canvas_->Scale(sx, sy); }
  void OnRotate(float degree) override { canvas_->Rotate(degree); }
  void OnRotate(float degree, float px, float py) override {
    canvas_->Rotate(degree, px, py);
  }
  void OnSkew(float sx, float sy) override { canvas_->Skew(sx, sy); }
  void OnConcat(skity::Matrix const& matrix) override {
    canvas_->Concat(matrix);
  }
  void OnSetMatrix(skity::Matrix const& matrix) override {
    canvas_->SetMatrix(matrix);
  }
  void OnResetMatrix() override { canvas_->ResetMatrix(); }
  void OnFlush() override { canvas_->Flush(); }
  uint32_t OnGetWidth() const override { return canvas_->Width(); }
  uint32_t OnGetHeight() const override { return canvas_->Height(); }

  void CheckUseSoftwareCanvas(const skity::Paint& paint) {
    if (paint.GetColorFilter()) {
      can_use_software_canvas_ = false;
      return;
    }
    if (paint.GetMaskFilter()) {
      can_use_software_canvas_ = false;
      return;
    }
    if (paint.GetImageFilter()) {
      can_use_software_canvas_ = false;
      return;
    }
  }

 private:
  skity::RecordingCanvas* canvas_;
  bool can_use_software_canvas_ = true;
};
}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_COMMON_DESKTOP_SVG_SKITY_SVG_CANVAS_DELEGATE_H_
