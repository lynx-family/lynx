// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_GRAPHICS_DRAWABLE_IMAGE_H_
#define CLAY_COMMON_GRAPHICS_DRAWABLE_IMAGE_H_

#include <map>
#include <memory>

#include "base/include/closure.h"
#include "base/include/fml/macros.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "clay/gfx/paint.h"
#include "clay/gfx/rendering_backend.h"
#include "skity/geometry/rect.hpp"

class GrDirectContext;

namespace clay {

class ContextListener {
 public:
  ContextListener();
  ~ContextListener();

  // Called from raster thread.
  virtual void OnGrContextCreated() = 0;

  // Called from raster thread.
  virtual void OnGrContextDestroyed() = 0;

 private:
  BASE_DISALLOW_COPY_AND_ASSIGN(ContextListener);
};

class DrawableImage : public ContextListener {
 public:
  enum class FitMode : uint8_t { kScaleToFill, kClipToBounds };

  enum class ImageType : uint8_t {
    kSharedImageTexture,
    kSharedImageBitmap,
    kMock,
  };

  struct PaintContext {
    clay::GrCanvas* canvas = nullptr;
    clay::GrContext* gr_context = nullptr;
    const clay::GrPaint* sk_paint = nullptr;
    const clay::Paint* clay_paint = nullptr;
  };

  DrawableImage();           // Called from UI or raster thread.
  virtual ~DrawableImage();  // Called from raster thread.

  virtual ImageType GetType() const = 0;

  // Called on platform thread.
  virtual void SetFrameAvailableCallback(const fml::closure& callback) = 0;

  // Called from raster thread.
  virtual void Paint(PaintContext& context, const skity::Rect& bounds,
                     bool freeze, const clay::GrSamplingOptions& sampling,
                     FitMode fit_mode) = 0;

  // Called on raster thread.
  virtual void MarkNewFrameAvailable() = 0;

  // Called on raster thread.
  virtual void OnDrawableImageUnregistered() = 0;

  int64_t Id() { return id_; }

 private:
  int64_t id_;
  BASE_DISALLOW_COPY_AND_ASSIGN(DrawableImage);
};

class DrawableImageRegistry {
 public:
  DrawableImageRegistry();

  // Called from raster thread.
  void RegisterDrawableImage(const std::shared_ptr<DrawableImage>& image);

  // Called from raster thread.
  void RegisterContextListener(uintptr_t id,
                               std::weak_ptr<ContextListener> image);

  // Called from raster thread.
  void UnregisterDrawableImage(int64_t id);

  // Called from the raster thread.
  void UnregisterContextListener(uintptr_t id);

  // Called from raster thread.
  std::shared_ptr<DrawableImage> GetDrawableImage(int64_t id);

  // Called from raster thread.
  void OnGrContextCreated();

  // Called from raster thread.
  void OnGrContextDestroyed();

 private:
  std::map<int64_t, std::shared_ptr<DrawableImage>> mapping_;
  std::map<uintptr_t, std::weak_ptr<ContextListener>> images_;

  BASE_DISALLOW_COPY_AND_ASSIGN(DrawableImageRegistry);
};

}  // namespace clay

#endif  // CLAY_COMMON_GRAPHICS_DRAWABLE_IMAGE_H_
