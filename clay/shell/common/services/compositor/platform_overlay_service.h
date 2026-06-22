// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_COMPOSITOR_PLATFORM_OVERLAY_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_COMPOSITOR_PLATFORM_OVERLAY_SERVICE_H_

#include <memory>
#include <vector>

#include "clay/common/service/service.h"
#include "clay/flow/embedded_views.h"
#include "clay/flow/surface_frame.h"
#include "clay/flow/view_slicer.h"
#include "clay/shell/common/output_surface.h"

namespace clay {

class PlatformOverlay;

class PlatformOverlay {
 public:
  virtual ~PlatformOverlay() = default;

  virtual fml::RefPtr<OutputSurface> GetOutputSurface() const = 0;

  // Called from the raster thread before acquiring the overlay SurfaceFrame.
  // Implementations must be thread-safe and MUST NOT touch UI-thread-affine
  // objects directly. Post work to the platform thread if needed.
  virtual void PrepareSurface(const OverlayData& overlay_data) {}

  virtual void OnSurfaceUpdated() {}
};

// PlatformOverlayService is a service that can create platform overlay.
// It's called from raster thread, the implementation must be thread safe.
class PlatformOverlayService
    : public clay::Service<PlatformOverlayService, clay::Owner::kPlatform,
                           clay::ServiceFlags::kMultiThread> {
 public:
  virtual std::vector<std::shared_ptr<PlatformOverlay>> CreatePlatformOverlay(
      size_t num) = 0;

  static std::shared_ptr<PlatformOverlayService> Create();
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_COMPOSITOR_PLATFORM_OVERLAY_SERVICE_H_
