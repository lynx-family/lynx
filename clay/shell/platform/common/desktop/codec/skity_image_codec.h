// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_SKITY_IMAGE_CODEC_H_
#define CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_SKITY_IMAGE_CODEC_H_

#include <memory>
#include <mutex>

#include "base/include/fml/macros.h"
#include "clay/gfx/geometry/size.h"
#include "clay/gfx/image/codec.h"
#include "clay/shell/platform/common/desktop/codec/image_codec_generator.h"
#include "skity/codec/codec.hpp"
#include "skity/io/data.hpp"

namespace clay {

class SkityImageCodecGenerator;

class SkityImageCodec final : public Codec {
 public:
  ~SkityImageCodec() override;

  int FrameCount() const override;
  void NextFrame(const CodecCallback& callback) override;
  int FrameDuration(int index) const override;

 private:
  class State;

  explicit SkityImageCodec(std::shared_ptr<State> state);

  std::shared_ptr<State> state_;

  friend class SkityImageCodecGenerator;
  FML_FRIEND_MAKE_REF_COUNTED(SkityImageCodec);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(SkityImageCodec);
};

class SkityImageCodecGenerator final
    : public ImageCodecGenerator,
      public std::enable_shared_from_this<SkityImageCodecGenerator> {
 public:
  static std::shared_ptr<ImageCodecGenerator> Create(
      std::shared_ptr<skity::Data> encoded_data, const Size& decode_size);

  const ImageInfo& GetImageInfo() const override;
  int FrameCount() const override;
  fml::RefPtr<GraphicsImage> GetImage() const override;
  fml::RefPtr<Codec> CreateCodec() override;

  int FrameDuration(int index) const;
  FrameInfo DecodeFrame(int frame_index,
                        std::shared_ptr<skity::Pixmap> previous_frame);

 private:
  SkityImageCodecGenerator(
      std::shared_ptr<skity::Codec> codec,
      std::shared_ptr<skity::MultiFrameDecoder> multi_frame_decoder,
      std::shared_ptr<skity::Pixmap> initial_pixmap, int frame_count);

  const std::shared_ptr<skity::Codec> codec_;
  const std::shared_ptr<skity::MultiFrameDecoder> multi_frame_decoder_;
  const std::shared_ptr<skity::Pixmap> initial_pixmap_;
  const ImageInfo image_info_;
  const int frame_count_;
  mutable std::mutex mutex_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_SKITY_IMAGE_CODEC_H_
