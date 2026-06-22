// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/shell/platform/common/desktop/codec/desktop_image.h"

#include <memory>
#include <utility>
#include <vector>

#include "clay/gfx/image/image_info.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "skity/io/data.hpp"

namespace clay {

DesktopImage::DesktopImage(std::shared_ptr<skity::Codec> codec)
    : width_(0),
      height_(0),
      is_animated_(false),
      auto_play_(true),
      loop_count_(0),
      is_playing_(false),
      current_frame_index_(0),
      current_frame_duration_(0),
      frame_loop_count_(loop_count_),
      codec_(codec) {
  if (codec_) {
    current_pixmap_ = codec_->Decode();
    if (current_pixmap_ && current_pixmap_->Addr() &&
        current_pixmap_->Width() > 0 && current_pixmap_->Height() > 0 &&
        current_pixmap_->RowBytes() > 0) {
      width_ = current_pixmap_->Width();
      height_ = current_pixmap_->Height();
      current_pixmap_->SetColorInfo(skity::AlphaType::kPremul_AlphaType,
                                    current_pixmap_->GetColorType());
      decoder_ = codec_->DecodeMultiFrame();
      if (decoder_) {
        is_animated_ = true;
      }
    } else {
      current_pixmap_.reset();
    }
  }
}
DesktopImage::~DesktopImage() {}
int DesktopImage::GetWidth() { return width_; }
int DesktopImage::GetHeight() { return height_; }

int64_t DesktopImage::GetDuration() { return current_frame_duration_; }

std::shared_ptr<skity::Pixmap> DesktopImage::ToBitmap() {
  if (!current_pixmap_ && !is_animated_ && width_ > 0 && height_ > 0) {
    current_pixmap_ = codec_->Decode();
  }
  return is_animated_ ? current_pixmap_ : std::move(current_pixmap_);
}
void DesktopImage::DrawFrame(std::function<void()> on_frame_changed) {
  if (is_playing_ && is_animated_ && decoder_) {
    auto frame_count = decoder_->GetFrameCount();
    if (frame_count <= 1 || current_frame_index_ >= frame_count) {
      return;
    }
    if (current_frame_index_ < frame_count) {
      DrawFrameInternal();
      if (current_frame_index_ >= frame_count) {
        if (frame_loop_count_ > 0) {
          --frame_loop_count_;
          if (frame_loop_count_ <= 0) {
            StopAnimation();
          }
        }
        current_frame_index_ = 0;
      }
      if (current_pixmap_ && on_frame_changed) {
        on_frame_changed();
      }
    }
  }
  // For animated images, this would advance to the next frame
}
bool DesktopImage::IsAnimated() { return is_animated_; }
void DesktopImage::SetAutoPlay(bool auto_play) {
  auto_play_ = auto_play;
  if (auto_play_) {
    StartAnimation();
  } else {
    StopAnimation();
  }
}
void DesktopImage::SetLoopCount(int loop_count) {
  loop_count_ = loop_count;
  frame_loop_count_ = loop_count_;
}
void DesktopImage::StartAnimation() {
  if (!is_animated_ || is_playing_) {
    return;
  }
  is_playing_ = true;
  current_frame_index_ = 0;
  frame_loop_count_ = loop_count_;
  current_frame_duration_ =
      decoder_->GetFrameInfo(current_frame_index_)->GetDuration();
  DrawFrameInternal();
}
void DesktopImage::StopAnimation() { is_playing_ = false; }
void DesktopImage::PauseAnimation() { is_playing_ = false; }
void DesktopImage::ResumeAnimation() {
  if (!is_playing_) {
    is_playing_ = true;
  }
}

void DesktopImage::DrawFrameInternal() {
  auto frame_info = decoder_->GetFrameInfo(current_frame_index_++);
  current_pixmap_ = decoder_->DecodeFrame(frame_info, current_pixmap_);
  current_frame_duration_ = frame_info->GetDuration();
}
}  // namespace clay
