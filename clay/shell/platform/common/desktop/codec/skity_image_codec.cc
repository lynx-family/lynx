// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/common/desktop/codec/skity_image_codec.h"

#include <limits>
#include <memory>
#include <utility>

#include "clay/gfx/image/graphics_image.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "skity/codec/codec.hpp"
#include "skity/graphic/image.hpp"

namespace clay {
namespace {

bool IsValidPixmap(const std::shared_ptr<skity::Pixmap>& pixmap) {
  return pixmap && pixmap->Addr() && pixmap->Width() > 0 &&
         pixmap->Height() > 0 && pixmap->RowBytes() > 0;
}

std::shared_ptr<skity::Pixmap> CopyPixmap(
    const std::shared_ptr<skity::Pixmap>& source) {
  if (!IsValidPixmap(source) ||
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

fml::RefPtr<GraphicsImage> MakeGraphicsImage(
    std::shared_ptr<skity::Pixmap> pixmap) {
  if (!IsValidPixmap(pixmap)) {
    return nullptr;
  }
  auto image = skity::Image::MakeImage(std::move(pixmap));
  return image ? GraphicsImage::Make(std::move(image)) : nullptr;
}

FrameInfo MakeFrameInfo(std::shared_ptr<skity::Pixmap> pixmap, int duration) {
  return {MakeGraphicsImage(std::move(pixmap)), duration};
}

std::shared_ptr<skity::Pixmap> DecodeInitialPixmap(
    const std::shared_ptr<skity::Codec>& codec, const Size& decode_size,
    bool is_animated) {
  if (is_animated || decode_size.IsZero()) {
    return codec->Decode();
  }
  auto pixmap = codec->Decode(
      skity::DecodeOptions{decode_size.width(), decode_size.height()});
  return pixmap ? pixmap : codec->Decode();
}

}  // namespace

class SkityImageCodec::State {
 public:
  explicit State(std::shared_ptr<SkityImageCodecGenerator> generator)
      : generator_(std::move(generator)) {
    frame_count_ = generator_->FrameCount();
  }

  int FrameCount() const { return frame_count_; }

  int FrameDuration(int index) const {
    return generator_->FrameDuration(index);
  }

  FrameInfo GetNextFrame() {
    if (!current_pixmap_) {
      auto image = generator_->GetImage();
      current_pixmap_ = image ? image->peekPixels() : nullptr;
    }
    if (!current_pixmap_) {
      return {};
    }
    auto result = generator_->DecodeFrame(next_frame_index_, current_pixmap_);
    if (result.image) {
      auto next_pixmap = CopyPixmap(result.image->peekPixels());
      if (!next_pixmap) {
        return {};
      }
      current_pixmap_ = std::move(next_pixmap);
      next_frame_index_ = (next_frame_index_ + 1) % frame_count_;
    }
    return result;
  }

 private:
  const std::shared_ptr<SkityImageCodecGenerator> generator_;
  std::shared_ptr<skity::Pixmap> current_pixmap_;
  int frame_count_ = 0;
  int next_frame_index_ = 0;
};

std::shared_ptr<ImageCodecGenerator> SkityImageCodecGenerator::Create(
    std::shared_ptr<skity::Data> encoded_data, const Size& decode_size) {
  if (!encoded_data || encoded_data->IsEmpty()) {
    return nullptr;
  }
  auto codec = skity::Codec::MakeFromData(encoded_data);
  if (!codec) {
    return nullptr;
  }
  codec->SetData(encoded_data);
  auto multi_frame_decoder = codec->DecodeMultiFrame();
  const int decoded_frame_count =
      multi_frame_decoder ? multi_frame_decoder->GetFrameCount() : 0;
  const bool is_animated = decoded_frame_count > 1;

  auto current_pixmap = DecodeInitialPixmap(codec, decode_size, is_animated);
  if (!IsValidPixmap(current_pixmap)) {
    return nullptr;
  }
  const int frame_count = is_animated ? decoded_frame_count : 1;
  current_pixmap->SetColorInfo(skity::AlphaType::kPremul_AlphaType,
                               current_pixmap->GetColorType());
  std::shared_ptr<skity::MultiFrameDecoder> animation_decoder;
  if (is_animated) {
    animation_decoder = std::move(multi_frame_decoder);
  }
  return std::shared_ptr<SkityImageCodecGenerator>(new SkityImageCodecGenerator(
      std::move(codec), std::move(animation_decoder), std::move(current_pixmap),
      frame_count));
}

SkityImageCodecGenerator::SkityImageCodecGenerator(
    std::shared_ptr<skity::Codec> codec,
    std::shared_ptr<skity::MultiFrameDecoder> multi_frame_decoder,
    std::shared_ptr<skity::Pixmap> initial_pixmap, int frame_count)
    : codec_(std::move(codec)),
      multi_frame_decoder_(std::move(multi_frame_decoder)),
      initial_pixmap_(std::move(initial_pixmap)),
      image_info_(initial_pixmap_->Width(), initial_pixmap_->Height(),
                  ConvertToClayColorType(initial_pixmap_->GetColorType()),
                  ConvertToClayAlphaType(initial_pixmap_->GetAlphaType())),
      frame_count_(frame_count) {}

const ImageInfo& SkityImageCodecGenerator::GetImageInfo() const {
  return image_info_;
}

int SkityImageCodecGenerator::FrameCount() const { return frame_count_; }

fml::RefPtr<GraphicsImage> SkityImageCodecGenerator::GetImage() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return MakeGraphicsImage(CopyPixmap(initial_pixmap_));
}

fml::RefPtr<Codec> SkityImageCodecGenerator::CreateCodec() {
  if (frame_count_ <= 1 || !multi_frame_decoder_) {
    return nullptr;
  }
  return fml::MakeRefCounted<SkityImageCodec>(
      std::make_shared<SkityImageCodec::State>(shared_from_this()));
}

int SkityImageCodecGenerator::FrameDuration(int index) const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!multi_frame_decoder_ || index < 0 || index >= frame_count_) {
    return -1;
  }
  auto frame_info = multi_frame_decoder_->GetFrameInfo(index);
  return frame_info ? frame_info->GetDuration() : -1;
}

FrameInfo SkityImageCodecGenerator::DecodeFrame(
    int frame_index, std::shared_ptr<skity::Pixmap> previous_frame) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!multi_frame_decoder_ || frame_index < 0 || frame_index >= frame_count_) {
    return {};
  }

  auto frame_info = multi_frame_decoder_->GetFrameInfo(frame_index);
  if (!frame_info) {
    return {};
  }
  auto frame =
      multi_frame_decoder_->DecodeFrame(frame_info, std::move(previous_frame));
  return MakeFrameInfo(std::move(frame), frame_info->GetDuration());
}

SkityImageCodec::SkityImageCodec(std::shared_ptr<State> state)
    : state_(std::move(state)) {}

SkityImageCodec::~SkityImageCodec() = default;

int SkityImageCodec::FrameCount() const { return state_->FrameCount(); }

void SkityImageCodec::NextFrame(const CodecCallback& callback) {
  callback(state_->GetNextFrame());
}

int SkityImageCodec::FrameDuration(int index) const {
  return state_->FrameDuration(index);
}

}  // namespace clay
