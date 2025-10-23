// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_PAINT_DECODING_IMAGE_H_
#define CLAY_GFX_PAINT_DECODING_IMAGE_H_

#include <functional>
#include <memory>

#include "clay/gfx/gpu_ref_object.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

using LazyImageDecodeCallback = std::function<void(bool)>;

class PaintDecodingImage : public GPURefObject {
 public:
  virtual ~PaintDecodingImage() = default;
  virtual void ScheduleDecodeAndUpload(
      const LazyImageDecodeCallback& callback) {}

  virtual bool MaybeAnimated() const { return false; }

  virtual clay::GrImagePtr gr_image() const = 0;
};

}  // namespace clay

#endif  // CLAY_GFX_PAINT_DECODING_IMAGE_H_
