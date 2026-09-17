// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/common/desktop/codec/desktop_image.h"

#include <limits>
#include <memory>
#include <mutex>
#include <utility>

#include "skity/io/data.hpp"

namespace clay {
namespace {

std::shared_ptr<skity::Pixmap> CopyPixmap(
    const std::shared_ptr<skity::Pixmap>& source) {
  if (!source || !source->Addr() || source->Width() == 0 ||
      source->Height() == 0 || source->RowBytes() == 0 ||
      source->Height() >
          std::numeric_limits<size_t>::max() / source->RowBytes()) {
    return nullptr;
  }
  auto data = skity::Data::MakeWithCopy(source->Addr(),
                                        source->RowBytes() * source->Height());
  if (!data) {
    return nullptr;
  }
  return std::make_shared<skity::Pixmap>(
      std::move(data), source->RowBytes(), source->Width(), source->Height(),
      source->GetAlphaType(), source->GetColorType());
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
  DesktopImageAnimation(std::shared_ptr<skity::MultiFrameDecoder> decoder,
                        std::shared_ptr<skity::Pixmap> first_frame)
      : current_pixmap_(CopyPixmap(first_frame)),
        decoder_(std::move(decoder)) {}

  int64_t GetDuration() override { return current_frame_duration_; }

  std::shared_ptr<skity::Pixmap> ToBitmap(
      const ImageInfo& render_info) override {
    std::shared_ptr<skity::Pixmap> pixmap;
    {
      std::scoped_lock lock(pixmap_mutex_);
      pixmap = CopyPixmap(current_pixmap_);
    }
    return ScalePixmap(std::move(pixmap), render_info);
  }

  bool DrawFrame() override {
    if (!is_playing_ || !decoder_) {
      return false;
    }
    auto frame_count = decoder_->GetFrameCount();
    if (frame_count <= 1 || current_frame_index_ >= frame_count) {
      return false;
    }
    DrawFrameInternal();
    if (current_frame_index_ >= frame_count) {
      if (remaining_loop_count_ > 0) {
        --remaining_loop_count_;
        if (remaining_loop_count_ <= 0) {
          StopAnimation();
        }
      }
      current_frame_index_ = 0;
    }
    return current_pixmap_ != nullptr;
  }

  void SetLoopCount(int loop_count) override {
    loop_count_ = loop_count;
    remaining_loop_count_ = loop_count_;
  }

  void StartAnimation() override {
    if (!decoder_ || is_playing_) {
      return;
    }
    is_playing_ = true;
    current_frame_index_ = 0;
    remaining_loop_count_ = loop_count_;
    auto frame_info = decoder_->GetFrameInfo(current_frame_index_);
    if (frame_info) {
      current_frame_duration_ = frame_info->GetDuration();
      DrawFrameInternal();
    }
  }

  void StopAnimation() override { is_playing_ = false; }

  void PauseAnimation() override { is_playing_ = false; }

  void ResumeAnimation() override { is_playing_ = decoder_ != nullptr; }

 private:
  void DrawFrameInternal() {
    if (!decoder_) {
      return;
    }
    auto frame_info = decoder_->GetFrameInfo(current_frame_index_++);
    if (!frame_info) {
      return;
    }
    std::scoped_lock lock(pixmap_mutex_);
    current_pixmap_ = decoder_->DecodeFrame(frame_info, current_pixmap_);
    current_frame_duration_ = frame_info->GetDuration();
  }

  std::shared_ptr<skity::Pixmap> current_pixmap_;
  std::shared_ptr<skity::MultiFrameDecoder> decoder_;
  std::mutex pixmap_mutex_;
  int loop_count_ = 0;
  int remaining_loop_count_ = 0;
  int32_t current_frame_index_ = 0;
  int32_t current_frame_duration_ = 0;
  bool is_playing_ = false;
};

}  // namespace

DesktopImage::DesktopImage(std::shared_ptr<skity::Codec> codec)
    : codec_(std::move(codec)) {
  if (!codec_) {
    return;
  }
  current_pixmap_ = codec_->Decode();
  if (!current_pixmap_ || !current_pixmap_->Addr() ||
      current_pixmap_->Width() <= 0 || current_pixmap_->Height() <= 0 ||
      current_pixmap_->RowBytes() <= 0) {
    current_pixmap_.reset();
    return;
  }
  width_ = current_pixmap_->Width();
  height_ = current_pixmap_->Height();
  current_pixmap_->SetColorInfo(skity::AlphaType::kPremul_AlphaType,
                                current_pixmap_->GetColorType());
  color_type_ = current_pixmap_->GetColorType();
  alpha_type_ = current_pixmap_->GetAlphaType();
  decoder_ = codec_->DecodeMultiFrame();
  is_animated_ = decoder_ != nullptr;
}

DesktopImage::~DesktopImage() = default;

int DesktopImage::GetWidth() { return width_; }

int DesktopImage::GetHeight() { return height_; }

skity::ColorType DesktopImage::GetColorType() { return color_type_; }

skity::AlphaType DesktopImage::GetAlphaType() { return alpha_type_; }

std::shared_ptr<skity::Pixmap> DesktopImage::ToBitmap(
    const ImageInfo& render_info) {
  auto pixmap = std::move(current_pixmap_);
  if (!pixmap && codec_ && width_ > 0 && height_ > 0) {
    pixmap = codec_->Decode();
  }
  return ScalePixmap(std::move(pixmap), render_info);
}

bool DesktopImage::IsAnimated() { return is_animated_; }

std::unique_ptr<PlatformImageAnimation> DesktopImage::CreateAnimation() {
  if (!is_animated_) {
    return nullptr;
  }
  return std::make_unique<DesktopImageAnimation>(decoder_, current_pixmap_);
}

}  // namespace clay
