// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/common/desktop/codec/skity_image_codec.h"

#include <memory>
#include <utility>

#include "clay/gfx/image/graphics_image.h"
#include "skity/codec/codec.hpp"
#include "skity/graphic/image.hpp"

namespace clay {
namespace {

bool IsValidPixmap(const std::shared_ptr<skity::Pixmap>& pixmap) {
  return pixmap && pixmap->Addr() && pixmap->Width() > 0 &&
         pixmap->Height() > 0 && pixmap->RowBytes() > 0;
}

FrameInfo MakeFrameInfo(std::shared_ptr<skity::Pixmap> pixmap, int duration) {
  if (!IsValidPixmap(pixmap)) {
    return {};
  }
  pixmap->SetColorInfo(skity::AlphaType::kPremul_AlphaType,
                       pixmap->GetColorType());
  auto image = skity::Image::MakeImage(std::move(pixmap));
  if (!image) {
    return {};
  }
  return {GraphicsImage::Make(std::move(image)), duration};
}

}  // namespace

class SkityImageCodec::Impl {
 public:
  explicit Impl(std::shared_ptr<skity::Codec> codec)
      : codec_(std::move(codec)) {}

  int FrameCount() const { return frame_count_; }

  int FrameDuration(int index) const {
    if (!multi_frame_decoder_ || index < 0 || index >= frame_count_) {
      return -1;
    }
    auto frame_info = multi_frame_decoder_->GetFrameInfo(index);
    return frame_info ? frame_info->GetDuration() : -1;
  }

  FrameInfo NextFrame() {
    if (!multi_frame_decoder_) {
      auto frame = codec_->Decode();
      if (!IsValidPixmap(frame)) {
        return {};
      }
      multi_frame_decoder_ = codec_->DecodeMultiFrame();
      frame_count_ =
          multi_frame_decoder_ ? multi_frame_decoder_->GetFrameCount() : 1;
      if (frame_count_ <= 0) {
        return {};
      }

      int duration = 0;
      if (multi_frame_decoder_) {
        auto frame_info = multi_frame_decoder_->GetFrameInfo(0);
        duration = frame_info ? frame_info->GetDuration() : 0;
      }
      previous_frame_ = frame;
      next_frame_index_ = 1 % frame_count_;
      return MakeFrameInfo(std::move(frame), duration);
    }

    const int frame_index = next_frame_index_;
    auto frame_info = multi_frame_decoder_->GetFrameInfo(frame_index);
    if (!frame_info) {
      return {};
    }
    auto frame = multi_frame_decoder_->DecodeFrame(frame_info, previous_frame_);
    if (!IsValidPixmap(frame)) {
      return {};
    }
    previous_frame_ = frame;

    auto result = MakeFrameInfo(std::move(frame), frame_info->GetDuration());
    if (result.image) {
      next_frame_index_ = (next_frame_index_ + 1) % frame_count_;
    }
    return result;
  }

 private:
  const std::shared_ptr<skity::Codec> codec_;
  std::shared_ptr<skity::MultiFrameDecoder> multi_frame_decoder_;
  std::shared_ptr<skity::Pixmap> previous_frame_;
  int frame_count_ = 0;
  int next_frame_index_ = 0;
};

fml::RefPtr<Codec> SkityImageCodec::Create(
    std::shared_ptr<skity::Data> encoded_data) {
  if (!encoded_data || encoded_data->IsEmpty()) {
    return nullptr;
  }
  auto codec = skity::Codec::MakeFromData(encoded_data);
  if (!codec) {
    return nullptr;
  }
  codec->SetData(encoded_data);
  return fml::MakeRefCounted<SkityImageCodec>(
      std::make_unique<Impl>(std::move(codec)));
}

SkityImageCodec::SkityImageCodec(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl)) {}

SkityImageCodec::~SkityImageCodec() = default;

int SkityImageCodec::FrameCount() const { return impl_->FrameCount(); }

void SkityImageCodec::NextFrame(const CodecCallback& callback) {
  callback(impl_->NextFrame());
}

int SkityImageCodec::FrameDuration(int index) const {
  return impl_->FrameDuration(index);
}

}  // namespace clay
