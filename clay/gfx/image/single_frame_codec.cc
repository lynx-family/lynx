// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/single_frame_codec.h"

#include <utility>

#include "clay/gfx/graphics_isolate.h"
#include "clay/gfx/image/image_decoder.h"

namespace clay {

SingleFrameCodec::SingleFrameCodec(fml::RefPtr<ImageDescriptor> descriptor,
                                   uint32_t target_width,
                                   uint32_t target_height)
    : status_(Status::kNew),
      descriptor_(std::move(descriptor)),
      target_width_(target_width),
      target_height_(target_height),
      mutex_(fml::SharedMutex::Create()) {}

SingleFrameCodec::~SingleFrameCodec() = default;

int SingleFrameCodec::FrameCount() const { return 1; }

void SingleFrameCodec::NextFrame(const CodecCallback& callback) {
  {
    fml::UniqueLock lock(*mutex_);
    if (status_ == Status::kComplete) {
      callback(FrameInfo{cached_image_, 0L});
      return;
    }

    pending_callbacks_.emplace_back(std::move(callback));

    if (status_ == Status::kInProgress) {
      // Another call to NextFrame is in progress and will invoke the pending
      // callbacks when decoding completes.
      return;
    }

    status_ = Status::kInProgress;
  }

  auto decoder = GraphicsIsolate::Instance().GetImageDecoder();
  if (!decoder) {
    FML_DLOG(ERROR) << "Image decoder not available.";
    status_ = Status::kComplete;
    for (const CodecCallback& callback : pending_callbacks_) {
      callback(FrameInfo{nullptr, 0L});
    }
    pending_callbacks_.clear();
    return;
  }

  // The SingleFrameCodec must be deleted on the UI thread.  Allocate a RefPtr
  // on the heap to ensure that the SingleFrameCodec remains alive until the
  // decoder callback is invoked on the IO thread.  The callback can then
  // drop the reference.
  fml::RefPtr<SingleFrameCodec>* raw_codec_ref =
      new fml::RefPtr<SingleFrameCodec>(this);

  decoder->Decode(
      descriptor_, target_width_, target_height_, [raw_codec_ref](auto image) {
        std::unique_ptr<fml::RefPtr<SingleFrameCodec>> codec_ref(raw_codec_ref);
        fml::RefPtr<SingleFrameCodec> codec(std::move(*codec_ref));
        {
          fml::UniqueLock lock(*codec->mutex_);
          if (image.get()) {
            codec->cached_image_ = std::move(image);
          }

          // The cached frame is now available and should be returned to any
          // future callers.
          codec->status_ = Status::kComplete;
        }

        // Invoke any callbacks that were provided before the frame was decoded.
        for (const CodecCallback& callback : codec->pending_callbacks_) {
          callback(FrameInfo{codec->cached_image_, 0L});
        }
        codec->pending_callbacks_.clear();
      });

  // The encoded data is no longer needed now that it has been handed off
  // to the decoder.
  descriptor_ = nullptr;
}

size_t SingleFrameCodec::GetAllocationSize() const {
  FML_DCHECK(descriptor_);
  const auto& data_size = descriptor_->GetAllocationSize();
  size_t frame_byte_size = 0;
  fml::UniqueLock lock(*mutex_);
  if (cached_image_) {
    const auto& info = cached_image_->imageInfo();
    const auto kMipmapOverhead = 4.0 / 3.0;
    frame_byte_size = info.computeMinByteSize() * kMipmapOverhead;
  }
  return data_size + frame_byte_size + sizeof(this);
}

}  // namespace clay
