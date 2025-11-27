// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_SURFACE_SOFTWARE_H_
#define CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_SURFACE_SOFTWARE_H_

#include <memory>

#include "base/include/fml/macros.h"
#include "clay/shell/gpu/gpu_surface_software.h"
#include "clay/shell/platform/embedder/embedder_surface.h"

namespace clay {

class EmbedderSurfaceSoftwareDelegate {
 public:
  virtual ~EmbedderSurfaceSoftwareDelegate() = default;
  virtual bool OnPresentBackingStore(const void* allocation, size_t row_bytes,
                                     size_t height) = 0;
};

class EmbedderSurfaceSoftware final : public EmbedderSurface,
                                      public GPUSurfaceSoftwareDelegate {
 public:
  explicit EmbedderSurfaceSoftware(EmbedderSurfaceSoftwareDelegate* delegate);

  ~EmbedderSurfaceSoftware() override;

 private:
  bool valid_ = false;
  EmbedderSurfaceSoftwareDelegate* delegate_;
  clay::GrSurfacePtr sk_surface_;

  // |OutputSurface|
  bool IsValid() const override;

  // |OutputSurface|
  std::unique_ptr<Surface> CreateGPUSurface(clay::GrContext*) override;

  // |GPUSurfaceSoftwareDelegate|
  clay::GrSurfacePtr AcquireBackingStore(const skity::Vec2& size) override;

  // |GPUSurfaceSoftwareDelegate|
  bool PresentBackingStore(clay::GrSurfacePtr backing_store) override;

  BASE_DISALLOW_COPY_AND_ASSIGN(EmbedderSurfaceSoftware);
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_SURFACE_SOFTWARE_H_
