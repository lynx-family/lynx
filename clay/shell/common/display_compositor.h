// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_H_
#define CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_H_

#include <memory>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/flow/surface.h"
#include "clay/shell/common/services/compositor/compositor_service.h"

namespace clay {

class SharedImageBacking;

// Runs on the raster task runner, sharing its graphics context. The output
// surface and platform presentation remain owned by the display stage.
class DisplayCompositor {
 public:
  DisplayCompositor(
      std::unique_ptr<Surface> surface,
      Puppet<Owner::kRaster, CompositorService> compositor_service);
  ~DisplayCompositor();

  Surface* surface() const { return surface_.get(); }

  bool Present(const fml::RefPtr<SharedImageBacking>& backing,
               const SurfaceFrame::SubmitInfo& submit_info,
               std::unique_ptr<CompositorState> compositor_state);

 private:
  std::unique_ptr<Surface> surface_;
  Puppet<Owner::kRaster, CompositorService> compositor_service_;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_H_
