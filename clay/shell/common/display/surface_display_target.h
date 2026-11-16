// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_SURFACE_DISPLAY_TARGET_H_
#define CLAY_SHELL_COMMON_DISPLAY_SURFACE_DISPLAY_TARGET_H_

#include <functional>
#include <memory>

#include "base/include/fml/memory/ref_counted.h"
#include "clay/shell/common/display/display_compositor.h"

namespace clay {
class OutputSurface;
namespace display {
class TextureBudget;
class OutputMailbox;
// Display uses the same GPU surface/delegate contract as Raster. Native
// backends may specialize submission while retaining this output lifetime.
std::unique_ptr<DisplayTarget> CreateSurfaceDisplayTarget(
    fml::RefPtr<OutputSurface> output, std::shared_ptr<TextureBudget> budget,
    fml::RefPtr<fml::TaskRunner> runner,
    std::shared_ptr<OutputMailbox> mailbox = nullptr,
    std::function<void()> available = {});
}  // namespace display
}  // namespace clay

#endif  // CLAY_SHELL_COMMON_DISPLAY_SURFACE_DISPLAY_TARGET_H_
