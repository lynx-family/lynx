// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_SVG_IMAGE_HOLDER_H_
#define CLAY_GFX_IMAGE_SVG_IMAGE_HOLDER_H_

#include <functional>
#include <future>
#include <memory>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/synchronization/shared_mutex.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/image/graphics_image.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

class GraphicsImageWrapper
    : public fml::RefCountedThreadSafe<GraphicsImageWrapper> {
 public:
  virtual fml::RefPtr<GraphicsImage> GetGraphicsImage() const = 0;
  virtual ~GraphicsImageWrapper() = default;
};

class GpuGraphicsImageWrapper : public GraphicsImageWrapper {
 public:
  explicit GpuGraphicsImageWrapper(GPUObject<GraphicsImage> image)
      : gpu_image_(std::move(image)) {}
  ~GpuGraphicsImageWrapper() override = default;

  fml::RefPtr<GraphicsImage> GetGraphicsImage() const override {
    return gpu_image_.object();
  }

 private:
  GPUObject<GraphicsImage> gpu_image_;
};

class CpuGraphicsImageWrapper : public GraphicsImageWrapper {
 public:
  explicit CpuGraphicsImageWrapper(fml::RefPtr<GraphicsImage> image)
      : image_(image) {}
  ~CpuGraphicsImageWrapper() override = default;

  fml::RefPtr<GraphicsImage> GetGraphicsImage() const override {
    return image_;
  }

 private:
  fml::RefPtr<GraphicsImage> image_;
};

class SVGImageHolder : public fml::RefCountedThreadSafe<SVGImageHolder> {
 public:
  SVGImageHolder();
  ~SVGImageHolder();

  SVGDomPtr GetSVGDOM();

  fml::RefPtr<GraphicsImage> GetGraphicsImage() const;
  void SetGraphicsImage(fml::RefPtr<GraphicsImageWrapper> image_wrapper);

  void CreateSVGDOM(GrDataPtr data);
  size_t GetAllocationSize() const {
    return svg_image_wrapper_ ? svg_image_wrapper_->GetGraphicsImage()
                                    ->GetApproximateByteSize()
                              : 0;
  }

 private:
  enum class SVGStatus { kNew, kInProgress, kComplete };
  SVGStatus status_;
  SVGDomPtr svg_dom_;
  std::promise<SVGDomPtr> svg_dom_promise_;
  std::future<SVGDomPtr> svg_dom_future_ = svg_dom_promise_.get_future();

  std::unique_ptr<fml::SharedMutex> mutex_;

  fml::RefPtr<GraphicsImageWrapper> svg_image_wrapper_;

  FML_FRIEND_MAKE_REF_COUNTED(SVGImageHolder);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(SVGImageHolder);
};

}  // namespace clay
#endif  // CLAY_GFX_IMAGE_SVG_IMAGE_HOLDER_H_
