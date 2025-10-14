// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_COMPOSITOR_COMPOSITOR_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_COMPOSITOR_COMPOSITOR_SERVICE_H_

#include <memory>
#include <vector>

#include "clay/flow/compositor/compositor_state.h"
#include "clay/flow/surface_frame.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/shell/common/services/compositor/platform_overlay_service.h"
#include "clay/shell/common/services/compositor/presenter_service.h"

namespace clay {

struct CompositorSurface {
  std::shared_ptr<PlatformOverlay> platform_overlay;
  std::unique_ptr<Surface> surface;
};

class CompositorService
    : public clay::Service<CompositorService, clay::Owner::kRaster> {
 public:
  bool SubmitFrame(clay::GrContext* context,
                   std::unique_ptr<SurfaceFrame> background_frame,
                   std::unique_ptr<CompositorState> compositor_state);

  static std::shared_ptr<CompositorService> Create();

 private:
  // |clay::Service|
  void OnInit(clay::ServiceManager& service_manager,
              const clay::RasterServiceContext& ctx) override;
  // |clay::Service|
  void OnDestroy() override;

  void CreateMissingSurfaces(size_t required_surfaces,
                             clay::GrContext* context);

  CompositorSurface& GetCompositorSurface();

  std::vector<std::shared_ptr<PlatformOverlay>> RemoveUnusedSurfaces();

  void RecycleSurfaces();

  clay::Puppet<clay::Owner::kRaster, PresenterService> presenter_service_;
  fml::RefPtr<fml::TaskRunner> raster_task_runner_;
  bool had_hybrid_composited_ = false;

  clay::Puppet<clay::Owner::kRaster, PlatformOverlayService> overlay_service_;

  // The index of the entry in the layers_ vector that determines the beginning
  // of the unused
  // layers. For example, consider the following vector:
  //  _____
  //  | 0 |
  /// |---|
  /// | 1 | <-- available_layer_index_
  /// |---|
  /// | 2 |
  /// |---|
  ///
  /// This indicates that entries starting from 1 can be reused meanwhile the
  /// entry at position 0 cannot be reused.
  size_t available_layer_index_ = 0;
  std::vector<CompositorSurface> compositor_surfaces_;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_COMPOSITOR_COMPOSITOR_SERVICE_H_
