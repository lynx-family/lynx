// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_DESKTOP_IMAGE_H_
#define CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_DESKTOP_IMAGE_H_

#include <memory>
#include <tuple>

#include "clay/gfx/image/image_info.h"
#include "clay/gfx/image/platform_image.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "skity/codec/codec.hpp"
namespace clay {
class DesktopImage : public PlatformImage {
 public:
  explicit DesktopImage(std::shared_ptr<skity::Codec> codec);
  ~DesktopImage() override;
  // PlatformImage implementation
  int GetWidth() override;
  int GetHeight() override;
  int64_t GetDuration() override;
  std::shared_ptr<skity::Pixmap> ToBitmap() override;
  void DrawFrame(std::function<void()> on_frame_changed) override;
  bool IsAnimated() override;
  void SetAutoPlay(bool auto_play) override;
  void SetLoopCount(int loop_count) override;
  void StartAnimation() override;
  void StopAnimation() override;
  void PauseAnimation() override;
  void ResumeAnimation() override;

 private:
  void DrawFrameInternal();

  int width_;
  int height_;
  bool is_animated_;
  bool auto_play_;
  int loop_count_;
  bool is_playing_;
  int32_t current_frame_index_;
  int32_t current_frame_duration_;
  int frame_loop_count_;
  std::shared_ptr<skity::Codec> codec_;
  std::shared_ptr<skity::Pixmap> current_pixmap_;
  std::shared_ptr<skity::MultiFrameDecoder> decoder_;
};
}  // namespace clay
#endif  // CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_DESKTOP_IMAGE_H_
