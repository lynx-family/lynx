// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_SVG_IMAGE_H_
#define CLAY_GFX_IMAGE_SVG_IMAGE_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "clay/gfx/gpu_object.h"
#include "clay/gfx/image/base_image.h"
#include "clay/gfx/image/graphics_image.h"
#include "clay/gfx/svg/svg_dom.h"

namespace clay {

class SVGImage : public BaseImage {
 public:
  static std::shared_ptr<SVGImage> Make(
      fml::WeakPtr<ImageFetcher> image_fetcher, std::string url,
      const std::string& content);

  explicit SVGImage(const std::string& content);

  void SetCacheKeyHash(size_t cache_key_hash) {
    cache_key_hash_ = cache_key_hash;
  }
  size_t GetCacheKeyHash() const { return cache_key_hash_; }

  void SetContentMD5(const std::string& content_md5) {
    content_md5_ = content_md5;
  }
  const std::string& GetContentMD5() const { return content_md5_; }

  int GetWidth() const override {
    return gpu_image_.object() ? gpu_image_.object()->width() : 1;
  }
  int GetHeight() const override {
    return gpu_image_.object() ? gpu_image_.object()->height() : 1;
  }
  void Upload(fml::RefPtr<GPUUnrefQueue> unref_queue, Size size) override;

 private:
  SVGImage() = default;

 private:
  std::string content_;
  // For SVG images, this is the MD5 of the SVG content.
  // This MD5 is primarily used to cache images within ImageCache.
  std::string content_md5_;
  // For SVG images, this is the hash of the SVG content MD5.
  // This hash is primarily used to cache images within the ImageCache.
  size_t cache_key_hash_ = 0;
  std::shared_ptr<SVGDom> svg_dom_;
};

}  // namespace clay
#endif  // CLAY_GFX_IMAGE_SVG_IMAGE_H_
