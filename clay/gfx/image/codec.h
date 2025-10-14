// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_CODEC_H_
#define CLAY_GFX_IMAGE_CODEC_H_

#include <functional>

#include "base/include/fml/memory/ref_counted.h"
#include "clay/gfx/image/frame_info.h"

namespace clay {

typedef std::function<void(FrameInfo)> CodecCallback;

// A handle to an SkCodec object.
//
// Doesn't mirror SkCodec's API but provides a simple sequential access API.
class Codec : public fml::RefCountedThreadSafe<Codec> {
 public:
  virtual ~Codec() = default;
  virtual int FrameCount() const = 0;

  virtual void NextFrame(const CodecCallback& callback) = 0;

  virtual int FrameDuration(int index) const { return -1; }
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_CODEC_H_
