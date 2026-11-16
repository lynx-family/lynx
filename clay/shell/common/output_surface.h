// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_OUTPUT_SURFACE_H_
#define CLAY_SHELL_COMMON_OUTPUT_SURFACE_H_

#include <memory>
#include <optional>

#include "base/include/fml/memory/ref_counted.h"
#include "clay/gfx/rendering_backend.h"
#ifdef ENABLE_CLAY_DISPLAY_COMPOSITOR
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/shell/common/display/raster_output_surface.h"
#endif

namespace clay {
class GLContextResult;
class Surface;
class SharedImageRepresentation;
class SharedImageSink;

// OutputSurface is a ref counted object which is owned by platform thread and
// raster thread. It's used to create GPU Surface for rendering, but also
// connected to it's platform view or maybe offscreen surface in the future.
class OutputSurface : public fml::RefCountedThreadSafe<OutputSurface> {
 public:
  virtual ~OutputSurface();

  virtual void CreateMainGrContext();

  virtual clay::GrContextPtr GetMainGrContext();

  virtual std::unique_ptr<GLContextResult> MakeCurrent();

#ifdef ENABLE_SKITY
  void PrecompileDefaultSkityShaders();
#endif

  virtual std::unique_ptr<Surface> CreateGPUSurface(
      clay::GrContextPtr gr_context = nullptr) = 0;

  virtual bool IsValid() const = 0;

#ifdef ENABLE_CLAY_DISPLAY_COMPOSITOR
  // Platform-only window target; Raster-only allocation and producer fence.
  // Unsupported hosts return null and do not enter the legacy pipeline.
  virtual uint64_t StagedDeviceId() const { return 0; }
  virtual display::PixelFormat StagedPixelFormat() const {
    return display::PixelFormat::kRGBA8888;
  }
  // Platform-only capability snapshot for the current display target. Unknown
  // hosts retain the conservative native-window attachment estimate.
  virtual bool UseStagedWindowAttachments() const { return true; }
  // Create a regular offscreen producer backed by the caller's buffer source.
  virtual fml::RefPtr<OutputSurface> CreateSharedImageOutput(
      fml::RefPtr<SharedImageSink> sink);
  virtual std::unique_ptr<display::RasterBuffer> CreateDisplayRasterBuffer(
      GrContext* context, const skity::Vec2& size) {
    return nullptr;
  }
  virtual std::shared_ptr<display::GPUCompletion> CreateDisplayCompletion(
      GrContext* context, std::unique_ptr<FenceSync> fence = nullptr) {
    return nullptr;
  }
  // Prove completion of the caller's uncommitted GPU work in this owner's
  // ordering domain. A null result leaves the work unsubmitted.
  virtual std::shared_ptr<const display::GPUCompletion> CreateCommandCompletion(
      const void* command) const {
    return nullptr;
  }
  // Raster-only. The production context is current. Queue a GPU dependency
  // on a completion created by this owner; never wait on the CPU. Returning
  // false keeps production deferred until the completion can be polled ready.
  virtual bool EnqueueStagedUploadWait(
      GrContext* production_context,
      const std::shared_ptr<display::GPUCompletion>& completion) {
    return false;
  }
#endif

 protected:
#ifdef ENABLE_SKITY
  struct SkityPrecompileConfig {
    skity::PrecompileColorType color_type;
    bool enable_msaa = false;
  };
  virtual std::optional<SkityPrecompileConfig> GetSkityPrecompileConfig() const;
#endif
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_OUTPUT_SURFACE_H_
