// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_GRAPHICS_IMAGE_H_
#define CLAY_GFX_IMAGE_GRAPHICS_IMAGE_H_

#include <memory>
#include <optional>
#include <string>

#include "base/include/fml/macros.h"
#include "clay/gfx/image/image_info.h"
#include "clay/gfx/paint_decoding_image.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/style/image_filter.h"
#include "skity/geometry/vector.hpp"

namespace clay {

class Image;

//------------------------------------------------------------------------------
/// @brief      Represents an image whose allocation is (usually) resident on
///             device memory.
///
///             Since it is usually impossible or expensive to transmute images
///             for one rendering backend to another, these objects are backend
///             specific.
///
class GraphicsImage : public PaintDecodingImage {
 public:
  // Describes which GPU context owns this image.
  enum class OwningContext { kRaster, kIO };

  static fml::RefPtr<GraphicsImage> MakeLazy(std::shared_ptr<Image> image);

#ifndef ENABLE_SKITY
  static fml::RefPtr<GraphicsImage> Make(const SkImage* image);

  static fml::RefPtr<GraphicsImage> Make(sk_sp<SkImage> image);

  static fml::RefPtr<GraphicsImage> MakeFromBitmap(const SkBitmap& bitmap);

  static fml::RefPtr<GraphicsImage> MakeRasterData(const SkImageInfo& info,
                                                   sk_sp<SkData> pixels,
                                                   size_t rowBytes);

  static fml::RefPtr<GraphicsImage> MakeRasterCopy(const SkPixmap& pixmap);

  static fml::RefPtr<GraphicsImage> MakePromiseTexture(
      sk_sp<GrContextThreadSafeProxy> gpuContextProxy,
      const GrBackendFormat& backendFormat, SkISize dimensions,
      GrMipmapped mipMapped, GrSurfaceOrigin origin, SkColorType colorType,
      SkAlphaType alphaType, sk_sp<SkColorSpace> colorSpace,
      SkImages::PromiseImageTextureFulfillProc textureFulfillProc,
      SkImages::PromiseImageTextureReleaseProc textureReleaseProc,
      SkImages::PromiseImageTextureContext textureContext);
#else
  static fml::RefPtr<GraphicsImage> Make(std::shared_ptr<skity::Image> image);
  static fml::RefPtr<GraphicsImage> MakeFromBitmap(const skity::Bitmap& bitmap);
  static fml::RefPtr<GraphicsImage> MakeRasterData(
      const ImageInfo& info, std::shared_ptr<skity::Data> pixels,
      size_t rowBytes);
  static fml::RefPtr<GraphicsImage> MakePromiseTexture(
      const skity::TextureFormat& texture_format, size_t width, size_t height,
      skity::AlphaType alpha_type,
      skity::GetPromiseTexture texture_fulfill_proc,
      skity::ReleaseCallback texture_release_proc,
      skity::PromiseTextureContext texture_context);
#endif  // ENABLE_SKITY

  ~GraphicsImage() override;

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

#ifndef ENABLE_SKITY
  virtual fml::RefPtr<GraphicsImage> makeWithFilter(GrRecordingContext* context,
                                                    const ImageFilter* filter,
                                                    const SkIRect& subset,
                                                    const SkIRect& clipBounds,
                                                    SkIRect* outSubset,
                                                    SkIPoint* offset) const = 0;

  virtual bool peekPixels(SkPixmap* pixmap) const = 0;

  virtual fml::RefPtr<GraphicsImage> makeTextureImage(
      GrDirectContext*, GrMipmapped = GrMipmapped::kNo,
      skgpu::Budgeted = skgpu::Budgeted::kYes) const = 0;

  virtual fml::RefPtr<GraphicsImage> makeRasterImage(
      SkImage::CachingHint cachingHint =
          SkImage::CachingHint::kDisallow_CachingHint) const = 0;

  virtual void flushAndSubmit(GrDirectContext*) = 0;

  virtual bool scalePixels(
      const SkPixmap& dst, const SkSamplingOptions&,
      SkImage::CachingHint cachingHint = SkImage::kAllow_CachingHint) const = 0;

  virtual const SkImageInfo& imageInfo() const = 0;
#else
  virtual fml::RefPtr<GraphicsImage> makeWithFilter(
      skity::GPUContext* context, const ImageFilter* filter,
      const skity::Rect& subset, const skity::Rect& clipBounds,
      skity::Rect* outSubset, GrPoint* offset) const = 0;

  virtual std::shared_ptr<skity::Pixmap> peekPixels() const = 0;

  virtual fml::RefPtr<GraphicsImage> makeRasterImage() const = 0;

  virtual const ImageInfo& imageInfo() const = 0;

  virtual fml::RefPtr<GraphicsImage> makeTextureImage(
      skity::GPUContext*) const {
    return nullptr;
  }

  virtual void flushAndSubmit(skity::GPUContext*) {}

  virtual bool scalePixels(
      std::shared_ptr<skity::Pixmap> dst, skity::GPUContext* context,
      const skity::SamplingOptions& sampling_options) const = 0;
#endif  // ENABLE_SKITY

  //----------------------------------------------------------------------------
  /// @return     The width of the pixel grid. A convenience method that
  /// calls
  ///             |GraphicsImage::dimensions|.
  ///
  int width() const;

  //----------------------------------------------------------------------------
  /// @return     The height of the pixel grid. A convenience method that calls
  ///             |GraphicsImage::dimensions|.
  ///
  int height() const;

  //----------------------------------------------------------------------------
  /// @return     The bounds of the pixel grid with 0, 0 as origin. A
  ///             convenience method that calls |GraphicsImage::dimensions|.
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

  virtual bool IsLazyImage() const { return false; }

  bool Equals(const GraphicsImage* other) const {
    if (!other) {
      return false;
    }
    if (this == other) {
      return true;
    }
    return gr_image() == other->gr_image();
  }

  bool Equals(const GraphicsImage& other) const { return Equals(&other); }

  bool Equals(fml::RefPtr<const GraphicsImage> other) const {
    return Equals(other.get());
  }

 protected:
  GraphicsImage();
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_GRAPHICS_IMAGE_H_
