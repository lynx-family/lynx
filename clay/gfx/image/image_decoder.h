// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_IMAGE_DECODER_H_
#define CLAY_GFX_IMAGE_IMAGE_DECODER_H_

#include <memory>
#include <optional>

#include "base/include/fml/concurrent_message_loop.h"
#include "base/include/fml/macros.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "clay/common/task_runners.h"
#include "clay/fml/mapping.h"
#include "clay/gfx/image/graphics_image.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

class ImageDescriptor;

class ImageDecoder {
 public:
  explicit ImageDecoder(
      std::shared_ptr<fml::ConcurrentTaskRunner> concurrent_task_runner);

  ~ImageDecoder();

  using ImageResult = std::function<void(fml::RefPtr<GraphicsImage>)>;

  // Takes an image descriptor and returns a SkImage. All image decompression
  // and resizes are done on a worker thread concurrently.
  void Decode(fml::RefPtr<ImageDescriptor> descriptor_ref_ptr,
              uint32_t target_width, uint32_t target_height,
              const ImageResult& result);

  fml::WeakPtr<ImageDecoder> GetWeakPtr() const;

 private:
  std::shared_ptr<fml::ConcurrentTaskRunner> concurrent_task_runner_;
  fml::WeakPtrFactory<ImageDecoder> weak_factory_;

  BASE_DISALLOW_COPY_AND_ASSIGN(ImageDecoder);
};

fml::RefPtr<GraphicsImage> ImageFromCompressedData(ImageDescriptor* descriptor,
                                                   uint32_t target_width,
                                                   uint32_t target_height);

fml::RefPtr<GraphicsImage> ResizeRasterImage(
    fml::RefPtr<GraphicsImage> image, const skity::Vec2& resized_dimensions);

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_IMAGE_DECODER_H_
