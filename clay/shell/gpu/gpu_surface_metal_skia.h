// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_GPU_GPU_SURFACE_METAL_SKIA_H_
#define CLAY_SHELL_GPU_GPU_SURFACE_METAL_SKIA_H_

#include <map>
#include <memory>

#include "base/include/fml/macros.h"
#include "clay/common/graphics/msaa_sample_count.h"
#include "clay/flow/surface.h"
#include "clay/shell/gpu/gpu_surface_metal_delegate.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"
#include "third_party/skia/include/gpu/mtl/GrMtlTypes.h"

namespace clay {

class SK_API_AVAILABLE_CA_METAL_LAYER GPUSurfaceMetalSkia : public Surface {
 public:
  GPUSurfaceMetalSkia(GPUSurfaceMetalDelegate* delegate,
                      sk_sp<GrDirectContext> context,
                      MsaaSampleCount msaa_samples,
                      bool render_to_surface = true);

  // |Surface|
  ~GPUSurfaceMetalSkia();

  // |Surface|
  bool IsValid() override;

 private:
  const GPUSurfaceMetalDelegate* delegate_;
  const MTLRenderTargetType render_target_type_;
  sk_sp<GrDirectContext> context_;
  GrDirectContext* precompiled_sksl_context_ = nullptr;
  MsaaSampleCount msaa_samples_ = MsaaSampleCount::kNone;
  // TODO(38466): Refactor GPU surface APIs take into account the fact that an
  // external view embedder may want to render to the root surface. This is a
  // hack to make avoid allocating resources for the root surface when an
  // external view embedder is present.
  bool render_to_surface_ = true;

  // Accumulated damage for each framebuffer; Key is address of underlying
  // MTLTexture for each drawable
  std::map<intptr_t, skity::Rect> damage_;

  // Memoize the previous frame size
  // If changed, damage history should be reset
  skity::Vec2 size_ = {0, 0};

  // |Surface|
  std::unique_ptr<SurfaceFrame> AcquireFrame(const skity::Vec2& size) override;

  // |Surface|
  skity::Matrix GetRootTransformation() const override;

  // |Surface|
  GrDirectContext* GetContext() override;

  // |Surface|
  std::unique_ptr<GLContextResult> MakeRenderContextCurrent() override;

  // |Surface|
  bool AllowsDrawingWhenGpuDisabled() const override;

  std::unique_ptr<SurfaceFrame> AcquireFrameFromCAMetalLayer(
      const skity::Vec2& frame_info);

  std::unique_ptr<SurfaceFrame> AcquireFrameFromMTLTexture(
      const skity::Vec2& frame_info);

  void PrecompileKnownSkSLsIfNecessary();

  BASE_DISALLOW_COPY_AND_ASSIGN(GPUSurfaceMetalSkia);
};

}  // namespace clay

#endif  // CLAY_SHELL_GPU_GPU_SURFACE_METAL_SKIA_H_
