// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/graphics_image.h"

#include <utility>

#include "clay/gfx/gfx_rendering_backend.h"

namespace clay {

fml::RefPtr<GraphicsImage> GraphicsImage::MakeLazy(
    std::shared_ptr<Image> image) {
#ifndef ENABLE_SKITY
  return fml::MakeRefCounted<GraphicsImageSkiaLazy>(image);
#else
  return fml::MakeRefCounted<GraphicsImageSkityLazy>(image);
#endif
}

#ifndef ENABLE_SKITY
fml::RefPtr<GraphicsImage> GraphicsImage::Make(const SkImage* image) {
  return Make(sk_ref_sp(image));
}

fml::RefPtr<GraphicsImage> GraphicsImage::Make(sk_sp<SkImage> image) {
  return fml::MakeRefCounted<GraphicsImageSkia>(std::move(image));
}

fml::RefPtr<GraphicsImage> GraphicsImage::MakeFromBitmap(
    const SkBitmap& bitmap) {
  return fml::MakeRefCounted<GraphicsImageSkia>(
      SkImages::RasterFromBitmap(bitmap));
}

fml::RefPtr<GraphicsImage> GraphicsImage::MakeRasterData(
    const SkImageInfo& info, sk_sp<SkData> pixels, size_t rowBytes) {
  return fml::MakeRefCounted<GraphicsImageSkia>(
      SkImages::RasterFromData(info, pixels, rowBytes));
}

fml::RefPtr<GraphicsImage> GraphicsImage::MakeRasterCopy(
    const SkPixmap& pixmap) {
  return fml::MakeRefCounted<GraphicsImageSkia>(
      SkImages::RasterFromPixmapCopy(pixmap));
}

fml::RefPtr<GraphicsImage> GraphicsImage::MakePromiseTexture(
    sk_sp<GrContextThreadSafeProxy> gpuContextProxy,
    const GrBackendFormat& backendFormat, SkISize dimensions,
    GrMipmapped mipMapped, GrSurfaceOrigin origin, SkColorType colorType,
    SkAlphaType alphaType, sk_sp<SkColorSpace> colorSpace,
    SkImages::PromiseImageTextureFulfillProc textureFulfillProc,
    SkImages::PromiseImageTextureReleaseProc textureReleaseProc,
    SkImages::PromiseImageTextureContext textureContext) {
  return fml::MakeRefCounted<GraphicsImageSkia>(SkImages::PromiseTextureFrom(
      gpuContextProxy, backendFormat, dimensions, mipMapped, origin, colorType,
      alphaType, colorSpace, textureFulfillProc, textureReleaseProc,
      textureContext));
}
#else
fml::RefPtr<GraphicsImage> GraphicsImage::Make(
    std::shared_ptr<skity::Image> image) {
  return fml::MakeRefCounted<GraphicsImageSkity>(std::move(image));
}

fml::RefPtr<GraphicsImage> GraphicsImage::MakeFromBitmap(
    const skity::Bitmap& bitmap) {
  return fml::MakeRefCounted<GraphicsImageSkity>(
      skity::Image::MakeImage(bitmap.GetPixmap()));
}

fml::RefPtr<GraphicsImage> GraphicsImage::MakeRasterData(
    const ImageInfo& info, std::shared_ptr<skity::Data> pixels,
    size_t rowBytes) {
  auto skity_pixmap = std::make_shared<skity::Pixmap>(
      pixels, rowBytes, info.width(), info.height());
  return fml::MakeRefCounted<GraphicsImageSkity>(
      skity::Image::MakeImage(skity_pixmap));
}

fml::RefPtr<GraphicsImage> GraphicsImage::MakePromiseTexture(
    const skity::TextureFormat& texture_format, size_t width, size_t height,
    skity::AlphaType alpha_type, skity::GetPromiseTexture texture_fulfill_proc,
    skity::ReleaseCallback texture_release_proc,
    skity::PromiseTextureContext texture_context) {
  return fml::MakeRefCounted<GraphicsImageSkity>(
      skity::Image::MakePromiseTextureImage(
          texture_format, width, height, alpha_type, texture_fulfill_proc,
          texture_release_proc, texture_context));
}
#endif  // ENABLE_SKITY

GraphicsImage::GraphicsImage() = default;

GraphicsImage::~GraphicsImage() = default;

int GraphicsImage::width() const { return dimensions().x; }

int GraphicsImage::height() const { return dimensions().y; }

skity::Rect GraphicsImage::bounds() const {
  return skity::Rect::MakeSize(dimensions());
}

std::optional<std::string> GraphicsImage::get_error() const {
  return std::nullopt;
}

}  // namespace clay
