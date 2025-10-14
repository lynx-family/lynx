// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_GPU_GPU_SURFACE_SOFTWARE_H_
#define CLAY_SHELL_GPU_GPU_SURFACE_SOFTWARE_H_

#include <memory>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "clay/flow/surface.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/shell/gpu/gpu_surface_software_delegate.h"

namespace clay {

class GPUSurfaceSoftware : public Surface {
 public:
  GPUSurfaceSoftware(GPUSurfaceSoftwareDelegate* delegate,
                     bool render_to_surface);

  ~GPUSurfaceSoftware() override;

  // |Surface|
  bool IsValid() override;

  // |Surface|
  std::unique_ptr<SurfaceFrame> AcquireFrame(const skity::Vec2& size) override;

  // |Surface|
  skity::Matrix GetRootTransformation() const override;

  // |Surface|
  clay::GrContext* GetContext() override;

 private:
  GPUSurfaceSoftwareDelegate* delegate_;
  // TODO(38466): Refactor GPU surface APIs take into account the fact that an
  // external view embedder may want to render to the root surface. This is a
  // hack to make avoid allocating resources for the root surface when an
  // external view embedder is present.
  const bool render_to_surface_;
  fml::WeakPtrFactory<GPUSurfaceSoftware> weak_factory_;
  BASE_DISALLOW_COPY_AND_ASSIGN(GPUSurfaceSoftware);
};

}  // namespace clay

#endif  // CLAY_SHELL_GPU_GPU_SURFACE_SOFTWARE_H_
