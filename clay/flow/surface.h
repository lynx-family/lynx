// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_SURFACE_H_
#define CLAY_FLOW_SURFACE_H_

#include <memory>

#include "base/include/fml/macros.h"
#include "clay/common/graphics/gl_context_switch.h"
#include "clay/flow/embedded_views.h"
#include "clay/flow/surface_frame.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

/// Abstract Base Class that represents where we will be rendering content.
class Surface {
 public:
  Surface();

  virtual ~Surface();

  virtual bool IsValid() = 0;

  virtual std::unique_ptr<SurfaceFrame> AcquireFrame(
      const skity::Vec2& size) = 0;

  virtual skity::Matrix GetRootTransformation() const = 0;
  virtual clay::GrContext* GetContext() = 0;

  virtual std::unique_ptr<GLContextResult> MakeRenderContextCurrent();

  virtual bool ClearRenderContext();

  virtual bool AllowsDrawingWhenGpuDisabled() const;

  virtual bool EnableRasterCache() const;

 private:
  BASE_DISALLOW_COPY_AND_ASSIGN(Surface);
};

}  // namespace clay

#endif  // CLAY_FLOW_SURFACE_H_
