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
  static std::shared_ptr<SVGImage> Make(const std::string& content);

  explicit SVGImage(const std::string& content);
  fml::RefPtr<GraphicsImage> GetGraphicsImage() const override {
    return gpu_image_.object();
  }
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
  std::shared_ptr<SVGDom> svg_dom_;
  GPUObject<GraphicsImage> gpu_image_;
};

}  // namespace clay
#endif  // CLAY_GFX_IMAGE_SVG_IMAGE_H_
