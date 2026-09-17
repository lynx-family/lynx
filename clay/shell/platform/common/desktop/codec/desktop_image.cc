// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/common/desktop/codec/desktop_image.h"

#include <memory>
#include <mutex>
#include <utility>

#include "clay/gfx/skity_to_skia_utils.h"

namespace clay {
namespace {

std::shared_ptr<skity::Pixmap> PixmapFromFrame(const FrameInfo& frame) {
  return frame.image ? frame.image->peekPixels() : nullptr;
}

// Desktop image codecs invoke NextFrame callbacks synchronously.
FrameInfo DecodeNextFrame(const fml::RefPtr<Codec>& codec) {
  FrameInfo frame;
  codec->NextFrame([&frame](FrameInfo result) { frame = std::move(result); });
  return frame;
}

std::shared_ptr<skity::Pixmap> ScalePixmap(
    std::shared_ptr<skity::Pixmap> pixmap, const ImageInfo& render_info) {
  if (!pixmap || render_info.width() <= 0 || render_info.height() <= 0) {
    return pixmap;
  }
  auto target_width = static_cast<uint32_t>(render_info.width());
  auto target_height = static_cast<uint32_t>(render_info.height());
  if (pixmap->Width() == target_width && pixmap->Height() == target_height) {
    return pixmap;
  }

  auto image = skity::Image::MakeImage(pixmap);
  if (!image) {
    return pixmap;
  }
  auto scaled_pixmap = std::make_shared<skity::Pixmap>(
      target_width, target_height, pixmap->GetAlphaType(),
      pixmap->GetColorType());
  if (!image->ScalePixels(scaled_pixmap, nullptr,
                          skity::SamplingOptions(skity::FilterMode::kLinear,
                                                 skity::MipmapMode::kNone))) {
    return pixmap;
  }
  return scaled_pixmap;
}

class DesktopImageAnimation final : public PlatformImageAnimation {
 public:
  explicit DesktopImageAnimation(std::shared_ptr<ImageCodecGenerator> generator)
      : generator_(std::move(generator)) {
    if (generator_) {
      current_frame_.image = generator_->GetImage();
      codec_ = generator_->CreateCodec();
    }
    frame_count_ = codec_ ? codec_->FrameCount() : 0;
  }

  int64_t GetDuration() override {
    std::scoped_lock lock(frame_mutex_);
    return current_frame_.duration;
  }

  std::shared_ptr<skity::Pixmap> ToBitmap(
      const ImageInfo& render_info) override {
    FrameInfo frame;
    {
      std::scoped_lock lock(frame_mutex_);
      frame = current_frame_;
    }
    return ScalePixmap(PixmapFromFrame(frame), render_info);
  }

  FrameResult DrawFrame() override {
    if (!is_playing_ || !codec_) {
      return FrameResult::kNoFrame;
    }
    if (frame_count_ <= 1 || current_frame_index_ >= frame_count_) {
      return FrameResult::kNoFrame;
    }
    if (!DrawFrameInternal()) {
      return FrameResult::kNoFrame;
    }
    ++current_frame_index_;
    if (current_frame_index_ >= frame_count_) {
      if (remaining_loop_count_ > 0) {
        --remaining_loop_count_;
        if (remaining_loop_count_ <= 0) {
          StopAnimation();
          current_frame_index_ = 0;
          return FrameResult::kFinalLoopComplete;
        }
      }
      current_frame_index_ = 0;
      return FrameResult::kLoopComplete;
    }
    return FrameResult::kFrameReady;
  }

  void SetLoopCount(int loop_count) override {
    loop_count_ = loop_count;
    remaining_loop_count_ = loop_count_;
  }

  void StartAnimation() override {
    if (is_playing_) {
      return;
    }
    if (!generator_) {
      return;
    }
    codec_ = generator_->CreateCodec();
    frame_count_ = codec_ ? codec_->FrameCount() : 0;
    if (frame_count_ <= 1) {
      return;
    }
    current_frame_index_ = 0;
    remaining_loop_count_ = loop_count_;
    if (!DrawFrameInternal()) {
      StopAnimation();
      return;
    }
    ++current_frame_index_;
    is_playing_ = true;
  }

  void StopAnimation() override { is_playing_ = false; }

  void PauseAnimation() override { is_playing_ = false; }

  void ResumeAnimation() override { is_playing_ = codec_ != nullptr; }

 private:
  bool DrawFrameInternal() {
    if (!codec_) {
      return false;
    }
    auto frame = DecodeNextFrame(codec_);
    if (!frame.image) {
      return false;
    }
    std::scoped_lock lock(frame_mutex_);
    current_frame_ = std::move(frame);
    return true;
  }

  FrameInfo current_frame_;
  std::shared_ptr<ImageCodecGenerator> generator_;
  fml::RefPtr<Codec> codec_;
  std::mutex frame_mutex_;
  int frame_count_ = 0;
  int loop_count_ = 0;
  int remaining_loop_count_ = 0;
  int32_t current_frame_index_ = 0;
  bool is_playing_ = false;
};

}  // namespace

DesktopImage::DesktopImage(std::shared_ptr<ImageCodecGenerator> generator)
    : generator_(std::move(generator)) {
  if (!generator_) {
    return;
  }
  const auto& image_info = generator_->GetImageInfo();
  if (image_info.isEmpty()) {
    return;
  }
  width_ = image_info.width();
  height_ = image_info.height();
  color_type_ = ConvertToSkityColorType(image_info.colorType());
  alpha_type_ = ConvertToSkityAlphaType(image_info.alphaType());
  is_animated_ = generator_->FrameCount() > 1;
}

DesktopImage::~DesktopImage() = default;

int DesktopImage::GetWidth() { return width_; }

int DesktopImage::GetHeight() { return height_; }

skity::ColorType DesktopImage::GetColorType() { return color_type_; }

skity::AlphaType DesktopImage::GetAlphaType() { return alpha_type_; }

std::shared_ptr<skity::Pixmap> DesktopImage::ToBitmap(
    const ImageInfo& render_info) {
  auto image = generator_ ? generator_->GetImage() : nullptr;
  auto pixmap = image ? image->peekPixels() : nullptr;
  return ScalePixmap(std::move(pixmap), render_info);
}

bool DesktopImage::IsAnimated() { return is_animated_; }

std::unique_ptr<PlatformImageAnimation> DesktopImage::CreateAnimation() {
  if (!is_animated_ || !generator_) {
    return nullptr;
  }
  return std::make_unique<DesktopImageAnimation>(generator_);
}

}  // namespace clay
