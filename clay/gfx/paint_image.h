// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_PAINT_IMAGE_H_
#define CLAY_GFX_PAINT_IMAGE_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include "base/include/fml/macros.h"
#include "clay/gfx/paint_decoding_image.h"
#include "skity/geometry/rect.hpp"
#include "skity/geometry/vector.hpp"

namespace skity {
class Image;
}

namespace clay {

//------------------------------------------------------------------------------
/// @brief      Represents an image whose allocation is (usually) resident on
///             device memory.
///
///             Since it is usually impossible or expensive to transmute images
///             for one rendering backend to another, these objects are backend
///             specific.
///
class PaintImage : public fml::RefCountedThreadSafe<PaintImage> {
 public:
  // Describes which GPU context owns this image.
  enum class OwningContext { kRaster, kIO };

#ifndef ENABLE_SKITY
  static fml::RefPtr<PaintImage> Make(const SkImage* image);

  static fml::RefPtr<PaintImage> Make(sk_sp<SkImage> image);
#endif  // ENABLE_SKITY

  static fml::RefPtr<PaintImage> Make(PaintDecodingImage* image);

  static fml::RefPtr<PaintImage> Make(fml::RefPtr<PaintDecodingImage> image);

  virtual ~PaintImage();

  //----------------------------------------------------------------------------
  /// @brief      If this paint image is meant to be used by the Skia/Skity
  ///             backend, an SkImage instance. Null otherwise.
  ///
  /// @return     A Skia/Skity image instance or null.
  ///
  virtual clay::GrImagePtr gr_image() const = 0;

  //----------------------------------------------------------------------------
  /// @brief      If the pixel format of this image ignores alpha, this returns
  ///             true. This method might conservatively return false when it
  ///             cannot guarantee an opaque image, for example when the pixel
  ///             format of the image supports alpha but the image is made up of
  ///             entirely opaque pixels.
  ///
  /// @return     True if the pixel format of this image ignores alpha.
  ///
  virtual bool isOpaque() const = 0;

  virtual bool isTextureBacked() const = 0;

  //----------------------------------------------------------------------------
  /// @return     The dimensions of the pixel grid.
  ///
  virtual skity::Vec2 dimensions() const = 0;

  //----------------------------------------------------------------------------
  /// @return     The approximate byte size of the allocation of this image.
  ///             This takes into account details such as mip-mapping. The
  ///             allocation is usually resident in device memory.
  ///
  virtual size_t GetApproximateByteSize() const = 0;

  //----------------------------------------------------------------------------
  /// @return     The width of the pixel grid. A convenience method that calls
  ///             |PaintImage::dimensions|.
  ///
  int width() const;

  //----------------------------------------------------------------------------
  /// @return     The height of the pixel grid. A convenience method that calls
  ///             |PaintImage::dimensions|.
  ///
  int height() const;

  //----------------------------------------------------------------------------
  /// @return     The bounds of the pixel grid with 0, 0 as origin. A
  ///             convenience method that calls |PaintImage::dimensions|.
  ///
  skity::Rect bounds() const;

  //----------------------------------------------------------------------------
  /// @return     Specifies which context was used to create this image. The
  ///             image must be collected on the same task runner as its
  ///             context.
  virtual OwningContext owning_context() const { return OwningContext::kIO; }

  //----------------------------------------------------------------------------
  /// @return     An error, if any, that occurred when trying to create the
  ///             image.
  virtual std::optional<std::string> get_error() const;

  virtual fml::RefPtr<PaintDecodingImage> decoding_image() const {
    return nullptr;
  }

  bool Equals(const PaintImage* other) const {
    if (!other) {
      return false;
    }
    if (this == other) {
      return true;
    }
    return decoding_image() == other->decoding_image() &&
           gr_image() == other->gr_image();
  }

  bool Equals(const PaintImage& other) const { return Equals(&other); }

  bool Equals(fml::RefPtr<const PaintImage> other) const {
    return Equals(other.get());
  }

 protected:
  PaintImage();
};

}  // namespace clay

#endif  // CLAY_GFX_PAINT_IMAGE_H_
