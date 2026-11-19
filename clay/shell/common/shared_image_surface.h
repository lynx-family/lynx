// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SHARED_IMAGE_SURFACE_H_
#define CLAY_SHELL_COMMON_SHARED_IMAGE_SURFACE_H_

#include <memory>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/flow/surface.h"

namespace clay {

class SharedImageBacking;

// Offscreen rendering in the display surface's context. The display surface
// outlives this adapter; no native window frame is acquired while rasterizing.
class SharedImageSurface final : public Surface {
 public:
  static bool IsSupported(Surface* surface);

  explicit SharedImageSurface(Surface* display_surface,
                              uint32_t sample_count = 1);
  ~SharedImageSurface() override;

  bool IsValid() override;
  std::unique_ptr<SurfaceFrame> AcquireFrame(const skity::Vec2& size) override;
  skity::Matrix GetRootTransformation() const override;
  GrContext* GetContext() override;
  GrContextPtr GetContextPtr() override;
  std::unique_ptr<GLContextResult> MakeRenderContextCurrent() override;
  bool ClearRenderContext() override;
  bool AllowsDrawingWhenGpuDisabled() const override;
  bool EnableRasterCache() const override;

  const fml::RefPtr<SharedImageBacking>& backing() const { return backing_; }

 private:
  Surface* display_surface_;
  [[maybe_unused]] uint32_t sample_count_;
  fml::RefPtr<SharedImageBacking> backing_;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SHARED_IMAGE_SURFACE_H_
