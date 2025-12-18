// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_RENDER_IMAGE_H_
#define CLAY_UI_RENDERING_RENDER_IMAGE_H_

#include <array>
#include <memory>
#include <string>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/image/image_resource.h"
#include "clay/gfx/image/image_resource_client.h"
#include "clay/ui/painter/image_painter.h"
#include "clay/ui/painter/painting_context.h"
#include "clay/ui/rendering/render_box.h"
#include "clay/ui/ui_rendering_backend.h"

namespace clay {
namespace testing {
class ImagePainterTest;
}

class RenderImageClient {
 public:
  virtual void OnDecodeFinished(bool success, const std::string& url) = 0;
  virtual void RegisterUploadTask(OneShotCallback<>&& task, int image_id) = 0;
  virtual void OnStartPlay() = 0;
  virtual void OnCurrentLoopComplete() = 0;
  virtual void OnFinalLoopComplete() = 0;
  virtual void AdjustSizeIfNeeded(bool auto_size, float bitmap_width,
                                  float bitmap_height) = 0;

 protected:
  virtual ~RenderImageClient() = default;
};

class RenderImage : public RenderBox, public ImageResourceClient {
 public:
  RenderImage() = default;
  ~RenderImage() override;

  const char* GetName() const override;

  void AddClient(RenderImageClient* client) { client_ = client; }
  void RemoveClient() { client_ = nullptr; }

#ifndef ENABLE_SKITY
  void SetImage(std::unique_ptr<ImageResource> resource);
  const ImageResource* GetImage() { return image_resource_.get(); }
  void SetPlaceholderImage(std::unique_ptr<ImageResource> placeholder_resource);
  const ImageResource* GetPlaceholderImage() {
    return placeholder_resource_.get();
  }
#else
  void SetImage(std::shared_ptr<BaseImage> image);
  BaseImage* GetImage() { return image_resource_.get(); }
  void SetPlaceholderImage(std::shared_ptr<BaseImage> placeholder_resource);
  BaseImage* GetPlaceholderImage() { return placeholder_resource_.get(); }
#endif  // ENABLE_SKITY
  void SetMode(FillMode mode);
  void SetEffect(ImageEffect effect);
  void SetRepeat(ImageRepeat repeat);
  void SetDropShadow(float offset_x, float offset_y, float blur_radius,
                     const GrColor& color);
  void SetCapInsets(const std::array<Length, 4>& cap_insets_vec);
  void SetCapInsetsScale(float scale);
  void SetImageOpacity(float opacity);
  void SetAutoPlay(bool auto_play);
  void SetLoopCount(int loop_count);
  void SetAutoSize(bool auto_size);
  void SetTintColor(const Color& tint_color);

  void OnNodeReady();

  const ImageData& GetOrCreateImageData();

  void Paint(PaintingContext& context, const FloatPoint& offset) override;
  PaintFunction FixupPainterIfNeeded(const PaintFunction& painter) override;

  bool IsRepaintBoundary() const override;

  void StartAnimate();
  void StopAnimation();
  void PauseAnimation();
  void ResumeAnimation();

  void OnStartPlay() override;
  void OnCurrentLoopComplete() override;
  void OnFinalLoopComplete() override;

  void SetNeedDecodeImmediately(bool value) { need_decode_immediately = value; }
  void TryDecodeImmediately();
  void DecodeImages() override;
  void SetEnableLowQuality(bool enable) { enable_low_quality_ = enable; }
  bool EnableLowQuality() const { return enable_low_quality_; }

  void SetDownSampling(bool down_sampling) { down_sampling_ = down_sampling; }
  bool DownSampling() const { return down_sampling_; }

  DecodePriority GetDecodePriority() override {
    return DecodeUtils::GetDecodePriority(this);
  }

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  void WillPaint() override;
  bool WillRenderImage() override { return IsActualVisible(); }
  void RequestRenderImage(ImageResource* image_resource, bool success) override;
  void DecodeImageFinish(bool success, const std::string& url) override;
  void RegisterUploadTask(OneShotCallback<>&& task, int image_id) override;
  void OnImageChanged() override;
  void AdjustSizeIfNeeded();

  void CreateImageData(ImageData& resource);

#ifndef ENABLE_SKITY
  ImageResource* GetResourceForDisplay() {
    return image_resource_ ? image_resource_.get()
                           : placeholder_resource_.get();
  }
#else
  BaseImage* GetResourceForDisplay() {
    return image_resource_ ? image_resource_.get()
                           : placeholder_resource_.get();
  }
#endif  // ENABLE_SKITY
  bool HasCapInsets() const;

  friend class testing::ImagePainterTest;

  FillMode mode_ = FillMode::kScaleToFill;
  ImageEffect effect_ = ImageEffect::kEffectNone;
  ImageRepeat repeat_ = ImageRepeat::kNoRepeat;
  // filter-image-view drop shadow:
  float drop_shadow_offset_x_ = 0.0f;
  float drop_shadow_offset_y_ = 0.0f;
  float drop_shadow_blur_radius_ = 0.0f;
  GrColor drop_shadow_color_ = ToSk(Color::kBlack());
  // 9-patch:
  bool has_cap_insets_ = false;
  std::array<Length, 4> cap_insets_ = {};
  float cap_insets_scale_ = 1.0f;

  float image_opacity_ = 1.0f;
  bool auto_play_ = true;
  int loop_count_ = 0;
  bool auto_size_ = false;

  RenderImageClient* client_ = nullptr;

  bool need_decode_immediately = false;
  // Decide color type as ARGB_8888 or RGB_565.
  bool enable_low_quality_ = false;
  // Decide whether decode image into the size of real rendering areas rather
  // than original size.
  bool down_sampling_ = false;

  std::optional<Color> tint_color_ = std::nullopt;

#ifndef ENABLE_SKITY
  std::unique_ptr<ImageResource> image_resource_;
  std::unique_ptr<ImageResource> pending_image_resource_;
  std::unique_ptr<ImageResource> placeholder_resource_;
#else
  std::shared_ptr<BaseImage> image_resource_;
  std::shared_ptr<BaseImage> pending_image_resource_;
  std::shared_ptr<BaseImage> placeholder_resource_;
#endif  // ENABLE_SKITY
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_RENDER_IMAGE_H_
