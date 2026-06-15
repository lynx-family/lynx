// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_OUTPUT_SURFACE_H_
#define CLAY_SHELL_COMMON_OUTPUT_SURFACE_H_

#include <memory>
#include <optional>

#include "base/include/fml/memory/ref_counted.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {
class GLContextResult;
class Surface;

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
      clay::GrContext* gr_context = nullptr) = 0;

  virtual bool IsValid() const = 0;

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
