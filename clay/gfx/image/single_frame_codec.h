// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_SINGLE_FRAME_CODEC_H_
#define CLAY_GFX_IMAGE_SINGLE_FRAME_CODEC_H_

#include <memory>
#include <vector>

#include "base/include/fml/macros.h"
#include "base/include/fml/synchronization/shared_mutex.h"
#include "clay/gfx/image/codec.h"
#include "clay/gfx/image/frame_info.h"
#include "clay/gfx/image/image_descriptor.h"

namespace clay {

class SingleFrameCodec : public Codec {
 public:
  SingleFrameCodec(fml::RefPtr<ImageDescriptor> descriptor,
                   uint32_t target_width, uint32_t target_height);

  ~SingleFrameCodec() override;

  // |Codec|
  int FrameCount() const override;

  // |Codec|
  void NextFrame(const CodecCallback& callback) override;

  size_t GetAllocationSize() const;

 private:
  enum class Status { kNew, kInProgress, kComplete };
  Status status_;
  fml::RefPtr<ImageDescriptor> descriptor_;
  uint32_t target_width_;
  uint32_t target_height_;
  fml::RefPtr<GraphicsImage> cached_image_;
  std::vector<CodecCallback> pending_callbacks_;
  std::unique_ptr<fml::SharedMutex> mutex_;

  FML_FRIEND_MAKE_REF_COUNTED(SingleFrameCodec);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(SingleFrameCodec);
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_SINGLE_FRAME_CODEC_H_
