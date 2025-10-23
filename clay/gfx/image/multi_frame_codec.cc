// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/multi_frame_codec.h"

#include <utility>

#include "base/include/fml/make_copyable.h"
#include "clay/gfx/graphics_isolate.h"
#include "third_party/skia/include/codec/SkCodecAnimation.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace clay {

namespace {

// Copied the source bitmap to the destination. If this cannot occur due to
// running out of memory or the image info not being compatible, returns false.
static bool CopyToBitmap(SkBitmap* dst, SkColorType dstColorType,
                         const SkBitmap& src) {
  SkPixmap srcPM;
  if (!src.peekPixels(&srcPM)) {
    return false;
  }

  SkBitmap tmpDst;
  SkImageInfo dstInfo = srcPM.info().makeColorType(dstColorType);
  if (!tmpDst.setInfo(dstInfo)) {
    return false;
  }

  if (!tmpDst.tryAllocPixels()) {
    return false;
  }

  SkPixmap dstPM;
  if (!tmpDst.peekPixels(&dstPM)) {
    return false;
  }

  if (!srcPM.readPixels(dstPM)) {
    return false;
  }

  dst->swap(tmpDst);
  return true;
}

}  // namespace

MultiFrameCodec::MultiFrameCodec(
    std::shared_ptr<SkCodecImageGenerator> generator)
    : state_(new State(std::move(generator))) {}

MultiFrameCodec::~MultiFrameCodec() = default;

MultiFrameCodec::State::State(std::shared_ptr<SkCodecImageGenerator> generator)
    : generator_(std::move(generator)),
      frame_count_(generator_->getFrameCount()),
      next_frame_index_(0) {}

fml::RefPtr<GraphicsImage> MultiFrameCodec::State::GetNextFrameImage() {
  SkBitmap bitmap = SkBitmap();
  SkImageInfo info = generator_->getInfo().makeColorType(kN32_SkColorType);
  if (info.alphaType() == kUnpremul_SkAlphaType) {
    SkImageInfo updated = info.makeAlphaType(kPremul_SkAlphaType);
    info = updated;
  }
  bitmap.allocPixels(info);

  SkCodec::Options options;
  options.fFrameIndex = next_frame_index_;
  SkCodec::FrameInfo frameInfo{0};
  generator_->getFrameInfo(next_frame_index_, &frameInfo);
  const int requiredFrameIndex = frameInfo.fRequiredFrame;
  if (requiredFrameIndex != SkCodec::kNoFrame) {
    if (last_required_frame_ == nullptr) {
      FML_LOG(INFO)
          << "Frame " << next_frame_index_ << " depends on frame "
          << requiredFrameIndex
          << " and no required frames are cached. Using blank slate instead.";
    } else {
      // Copy the previous frame's output buffer into the current frame as the
      // starting point.
      if (last_required_frame_->getPixels() &&
          CopyToBitmap(&bitmap, last_required_frame_->colorType(),
                       *last_required_frame_)) {
        options.fPriorFrame = requiredFrameIndex;
      }
    }
  }

  // Write the new frame to the output buffer. The bitmap pixels as supplied
  // are already set in accordance with the previous frame's disposal policy.
  if (!generator_->getPixels(info, bitmap.getPixels(), bitmap.rowBytes(),
                             &options)) {
    FML_LOG(ERROR) << "Could not getPixels for frame " << next_frame_index_;
    return nullptr;
  }

  // Hold onto this if we need it to decode future frames.
  if (frameInfo.fDisposalMethod == SkCodecAnimation::DisposalMethod::kKeep ||
      last_required_frame_) {
    last_required_frame_ = std::make_unique<SkBitmap>(bitmap);
    last_required_frame_index_ = next_frame_index_;
  }

  // Defer decoding until time of draw later on the raster thread. Can happen
  // when GL operations are currently forbidden such as in the background
  // on iOS.
  return GraphicsImage::MakeFromBitmap(bitmap);
}

void MultiFrameCodec::State::GetNextFrameAndInvokeCallback(
    const CodecCallback& callback) {
  int duration = 0;
  fml::RefPtr<GraphicsImage> image = GetNextFrameImage();
  if (image) {
    SkCodec::FrameInfo skFrameInfo{0};
    generator_->getFrameInfo(next_frame_index_, &skFrameInfo);
    duration = skFrameInfo.fDuration;
  }
  next_frame_index_ = (next_frame_index_ + 1) % frame_count_;

  // Invoke next frame callback.
  callback({image, duration});
}

void MultiFrameCodec::NextFrame(const CodecCallback& callback) {
  GraphicsIsolate::Instance().GetConcurrentWorkerTaskRunner()->PostTask(
      fml::MakeCopyable([callback = std::move(callback),
                         weak_state = std::weak_ptr<MultiFrameCodec::State>(
                             state_)]() mutable {
        auto state = weak_state.lock();
        if (!state) {
          callback({nullptr, 0});
          return;
        }
        state->GetNextFrameAndInvokeCallback(std::move(callback));
      }));
}

int MultiFrameCodec::FrameCount() const { return state_->frame_count_; }

int MultiFrameCodec::FrameDuration(int index) const {
  SkCodec::FrameInfo sk_frame_info{0};
  state_->generator_->getFrameInfo(index, &sk_frame_info);
  return sk_frame_info.fDuration;
}

}  // namespace clay
