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
struct ScreenshotData;

// Composes retained frames on the raster thread using the output surface.
class DisplayCompositor {
 public:
  DisplayCompositor(
      std::unique_ptr<Surface> surface,
      Puppet<Owner::kRaster, CompositorService> compositor_service);
  ~DisplayCompositor();

  Surface* surface() const { return surface_.get(); }

  bool Present(const fml::RefPtr<SharedImageBacking>& backing,
               const SurfaceFrame::SubmitInfo& submit_info,
               std::unique_ptr<CompositorState> compositor_state,
               GrImagePtr image = nullptr);

  // Present the retained scene without traversing the LayerTree. The caller
  // supplies timing for this display frame rather than reusing raster timing.
  bool Present(const SurfaceFrame::SubmitInfo& submit_info);

  // Read pixels from the retained scene without publishing a display frame.
  ScreenshotData Screenshot(uint32_t background_color);

  // Call with the render context current when invalidating a page or surface.
  void Clear();
  bool CanPresentImage(int64_t image_id) const;

 private:
  bool PresentCurrent(const SurfaceFrame::SubmitInfo& submit_info);

  std::unique_ptr<Surface> surface_;
  Puppet<Owner::kRaster, CompositorService> compositor_service_;
  fml::RefPtr<SharedImageBacking> backing_;
  GrImagePtr image_;
  std::unique_ptr<CompositorState> compositor_state_;
  skity::Matrix root_transform_;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_H_
