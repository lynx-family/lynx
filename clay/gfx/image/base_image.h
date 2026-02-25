// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_BASE_IMAGE_H_
#define CLAY_GFX_IMAGE_BASE_IMAGE_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/rect.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/image/base_image_instance.h"
#include "clay/gfx/image/graphics_image.h"
#include "clay/gfx/image/image_info.h"
#include "clay/gfx/image/platform_image.h"

namespace clay {
class ImageFetcher;

enum class ImageType {
  kStatic,
  kAnimated,
  kSVG,
};

class BaseImage : public std::enable_shared_from_this<BaseImage> {
 public:
  virtual ~BaseImage() = default;

  ImageType GetType() const { return type_; }
  virtual int GetWidth() const { return orig_info_.width(); }
  virtual int GetHeight() const { return orig_info_.height(); }
  virtual void Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) = 0;
  fml::RefPtr<GraphicsImage> GetGraphicsImage() const {
    return gpu_image_.object();
  }
  size_t GetGraphicsImageAllocSize() const {
    return gpu_image_.object() ? gpu_image_.object()->width() *
                                     gpu_image_.object()->height() * 4
                               : 0;
  }

  void SetExpectSizeCalculator(
      std::function<skity::Vec2(skity::Vec2)> calculator,
      bool force_use_original_size);
  const std::string& GetUrl() const { return url_; }
  const std::string& GetCacheIdentifier() const { return cache_identifier_; }
  void SetCacheIdentifier(const std::string& cache_identifier) {
    cache_identifier_ = cache_identifier;
  }

  bool IsSVG() const { return type_ == ImageType::kSVG; }

  std::unique_ptr<BaseImageInstance> NewInstance();
  void OnInstanceCreated(BaseImageInstance* instance);
  void OnInstanceDestroyed(BaseImageInstance* instance);

 protected:
  fml::WeakPtr<ImageFetcher> image_fetcher_;
  std::unordered_set<BaseImageInstance*> instances_;
  std::string url_;
  // For Data images, this is the MD5 of the Data content. For other images,
  // this is the trimmed URL.
  std::string cache_identifier_;
  ImageType type_ = ImageType::kStatic;
  std::shared_ptr<PlatformImage> image_;
  GPUObject<GraphicsImage> gpu_image_;
  ImageInfo render_info_;
  ImageInfo orig_info_;
};

}  // namespace clay
#endif  // CLAY_GFX_IMAGE_BASE_IMAGE_H_
