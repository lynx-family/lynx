// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_IMAGE_DESCRIPTOR_H_
#define CLAY_GFX_IMAGE_IMAGE_DESCRIPTOR_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/synchronization/shared_mutex.h"
#include "base/include/fml/task_runner.h"
#include "clay/gfx/gfx_rendering_backend.h"
#include "clay/gfx/image/codec.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

// Creates an image descriptor for encoded or decoded image data, describing
// the width, height, and bytes per pixel for that image.
//
// This class will hold a reference on the underlying image data, and in the
// case of compressed data, an SkCodec and SkImageGenerator for the data.
// The Codec initialization actually happens in initEncoded, making
// instantiateCodec a lightweight operation.
class ImageDescriptor : public fml::RefCountedThreadSafe<ImageDescriptor> {
 public:
  explicit ImageDescriptor(GrDataPtr buffer);

  ImageDescriptor(GrDataPtr buffer, std::optional<size_t> row_bytes);

  virtual ~ImageDescriptor() = default;

  ImageDescriptor(const ImageDescriptor&) = delete;
  const ImageDescriptor& operator=(const ImageDescriptor&) = delete;

  enum PixelFormat {
    kRGBA8888,
    kBGRA8888,
    kRGB565,
  };

  // Asynchronously initializes an ImageDescriptor for an encoded image, as
  // long as the format is supported by Skia.
  //
  // Calling this method will result in creating an SkCodec and
  // SkImageGenerator to read EXIF corrected dimensions from the image data.
  static fml::RefPtr<ImageDescriptor> Create(
      GrDataPtr data, bool enable_low_quality_image = false);

  // Associates a Clay Codec object.
  virtual fml::RefPtr<Codec> InstantiateCodec(int target_width,
                                              int target_height) = 0;

  virtual fml::RefPtr<GraphicsImage> image() const = 0;

  virtual skity::Vec2 GetScaledDimensions(float scale) = 0;

#ifndef ENABLE_SKITY
  virtual bool GetPixels(const SkPixmap& pixmap) const = 0;
#endif  // ENABLE_SKITY

  // Whether this descriptor represents compressed (encoded) data or not.
  virtual bool IsCompressed() const = 0;

  // The width of this image, EXIF oriented if applicable.
  int width() const { return image_info_.width(); }

  // The height of this image. EXIF oriented if applicable.
  int height() const { return image_info_.height(); }

  // The bytes per pixel of the image.
  int BytesPerPixel() const { return image_info_.bytesPerPixel(); }

  // The byte length of the first row of the image.
  //
  // Defaults to width() * 4.
  int RowBytes() const {
    return row_bytes_.value_or(
        static_cast<size_t>(image_info_.width() * image_info_.bytesPerPixel()));
  }

  // Whether the given target_width or target_height differ from width() and
  // height() respectively.
  bool ShouldResize(int target_width, int target_height) const {
    return target_width != width() || target_height != height();
  }

  // The underlying buffer for this image.
  GrDataPtr data() const { return buffer_; }

  const GrImageInfo& image_info() const { return image_info_; }

  bool IsSingleFrame() const { return single_frame_; }

  size_t GetAllocationSize() const {
    return sizeof(ImageDescriptor) + sizeof(ImageInfo) + DATA_GET_SIZE(buffer_);
  }

 protected:
  GrDataPtr buffer_;
  GrImageInfo image_info_;
  std::optional<size_t> row_bytes_;
  bool single_frame_ = true;
  std::unique_ptr<fml::SharedMutex> mutex_;

  FML_FRIEND_MAKE_REF_COUNTED(ImageDescriptor);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(ImageDescriptor);
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_IMAGE_DESCRIPTOR_H_
