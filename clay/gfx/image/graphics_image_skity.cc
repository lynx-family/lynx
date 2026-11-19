// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/graphics_image_skity.h"

#include <algorithm>
#include <limits>
#include <mutex>
#include <utility>

#include "clay/memory/clay_memory_tracker.h"
#include "skity/geometry/rect.hpp"
#include "skity/gpu/gpu_context.hpp"
#include "skity/gpu/texture.hpp"
#include "skity/graphic/alpha_type.hpp"
#include "skity/graphic/image.hpp"
#include "skity/include/skity/geometry/vector.hpp"
#include "skity/io/pixmap.hpp"

namespace clay {
namespace {

// For descriptor-created textures, GetTextureSize() reports level zero.
// Clay requests the full chain when mipmapping is enabled.
size_t ImageTextureBytes(skity::Texture& texture, bool mipmapped) {
  size_t bytes = texture.GetTextureSize();
  if (!mipmapped || bytes == 0) {
    return bytes;
  }
  size_t width = texture.Width();
  size_t height = texture.Height();
  const size_t bytes_per_pixel = bytes / width / height;
  while (width > 1 || height > 1) {
    width = std::max(width / 2, size_t{1});
    height = std::max(height / 2, size_t{1});
    const size_t level_bytes = width * height * bytes_per_pixel;
    if (level_bytes > std::numeric_limits<size_t>::max() - bytes) {
      return 0;
    }
    bytes += level_bytes;
  }
  return bytes;
}

// Retain Skity's lazy upload behavior. Registration observes the backing,
// whose lifetime also includes any references in Skity's release queue.
class ImageMemoryTexture final : public skity::Texture {
 public:
  ImageMemoryTexture(std::shared_ptr<skity::Texture> texture,
                     std::shared_ptr<ClayMemoryTracker> memory_tracker,
                     bool mipmapped)
      : texture_(std::move(texture)),
        memory_tracker_(std::move(memory_tracker)),
        mipmapped_(mipmapped) {}

  size_t Width() override { return texture_->Width(); }
  size_t Height() override { return texture_->Height(); }
  skity::AlphaType GetAlphaType() override { return texture_->GetAlphaType(); }
  skity::TextureFormat GetFormat() const override {
    return texture_->GetFormat();
  }
  size_t GetTextureSize() override { return texture_->GetTextureSize(); }

  void DeferredUploadImage(std::shared_ptr<skity::Pixmap> pixmap) override {
    texture_->DeferredUploadImage(std::move(pixmap));
  }
  void UploadImage(std::shared_ptr<skity::Pixmap> pixmap) override {
    texture_->UploadImage(std::move(pixmap));
  }

  std::shared_ptr<skity::GPUTexture> GetGPUTexture() override {
    auto backing = texture_->GetGPUTexture();
    if (backing) {
      std::call_once(register_once_, [&] {
        memory_tracker_->TrackImageTexture(
            backing, ImageTextureBytes(*texture_, mipmapped_));
      });
    }
    return backing;
  }

 private:
  const std::shared_ptr<skity::Texture> texture_;
  const std::shared_ptr<ClayMemoryTracker> memory_tracker_;
  const bool mipmapped_;
  std::once_flag register_once_;
};

}  // namespace

std::shared_ptr<skity::Pixmap> TrackDecodedPixmap(
    const std::shared_ptr<ClayMemoryTracker>& tracker,
    std::shared_ptr<skity::Pixmap> pixmap) {
  if (!tracker || !pixmap || !pixmap->Addr() || !pixmap->RowBytes() ||
      pixmap->Height() >
          std::numeric_limits<size_t>::max() / pixmap->RowBytes()) {
    return pixmap;
  }
  const size_t bytes = pixmap->RowBytes() * pixmap->Height();
  return tracker->TrackImageData(ImageMemoryType::kDecoded, std::move(pixmap),
                                 bytes);
}

void SetDeferredRasterImageTexture(
    skity::GPUContext& context, skity::DeferredTextureImage& image,
    std::shared_ptr<skity::Pixmap> pixmap, bool mipmapped,
    const std::shared_ptr<ClayMemoryTracker>& memory_tracker) {
  skity::TextureDescriptor desc{};
  desc.format = skity::Texture::FormatFromColorType(pixmap->GetColorType());
  desc.width = pixmap->Width();
  desc.height = pixmap->Height();
  desc.alpha_type = pixmap->GetAlphaType();
  desc.mipmapped = mipmapped;
  auto texture = context.CreateTextureWithDesc(&desc);
  if (!texture) {
    return;
  }
  texture->DeferredUploadImage(std::move(pixmap));
  if (memory_tracker) {
    texture = std::make_shared<ImageMemoryTexture>(std::move(texture),
                                                   memory_tracker, mipmapped);
  }
  image.SetTexture(std::move(texture));
}

GraphicsImageSkity::GraphicsImageSkity(std::shared_ptr<skity::Image> image)
    : image_(std::move(image)) {
  if (image_) {
    image_info_ = ImageInfo::makeWH(image_->Width(), image_->Height());
  }
}

// |GraphicsImage|
GraphicsImageSkity::~GraphicsImageSkity() = default;

std::shared_ptr<skity::Image> GraphicsImageSkity::gr_image() const {
  return image_;
}

// |GraphicsImage|
bool GraphicsImageSkity::isOpaque() const {
  return image_ ? image_->GetAlphaType() == skity::AlphaType::kOpaque_AlphaType
                : false;
}

// |GraphicsImage|
bool GraphicsImageSkity::isTextureBacked() const {
  return image_ ? image_->IsTextureBackend() : false;
}

// |GraphicsImage|
skity::Vec2 GraphicsImageSkity::dimensions() const {
  return image_ ? skity::Vec2(image_->Width(), image_->Height())
                : skity::Vec2{0, 0};
}

// |GraphicsImage|
size_t GraphicsImageSkity::GetApproximateByteSize() const {
  auto size = sizeof(this);
  if (image_) {
    const size_t image_byte_size =
        width() * height() * imageInfo().bytesPerPixel();
    size += image_byte_size;
  }
  return size;
}

const ImageInfo& GraphicsImageSkity::imageInfo() const {
  static const ImageInfo default_image_info = ImageInfo();
  return image_ ? image_info_ : default_image_info;
}

fml::RefPtr<GraphicsImage> GraphicsImageSkity::makeWithFilter(
    skity::GPUContext* context, const ImageFilter* filter,
    const skity::Rect& subset, const skity::Rect& clipBounds,
    skity::Rect* outSubset, GrPoint* offset) const {
  // TODO(zhangxiao.ninja) implement later
  FML_UNIMPLEMENTED()
  return nullptr;
}

std::shared_ptr<skity::Pixmap> GraphicsImageSkity::peekPixels() const {
  return image_ ? *image_->GetPixmap() : nullptr;
}

fml::RefPtr<GraphicsImage> GraphicsImageSkity::makeRasterImage() const {
  // TODO(zhangxiao.ninja) implement later
  FML_UNIMPLEMENTED()
  return nullptr;
}

fml::RefPtr<GraphicsImage> GraphicsImageSkity::makeTextureImage(
    skity::GPUContext* context) const {
  return image_ ? GraphicsImage::Make(
                      skity::Image::MakeImage(*image_->GetPixmap(), context))
                : nullptr;
}

bool GraphicsImageSkity::scalePixels(
    std::shared_ptr<skity::Pixmap> dst, skity::GPUContext* context,
    const GrSamplingOptions& sampling_options) const {
  return image_ ? image_->ScalePixels(dst, context, sampling_options) : false;
}

}  // namespace clay
