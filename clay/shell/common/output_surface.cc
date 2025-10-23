// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/output_surface.h"

namespace clay {

OutputSurface::~OutputSurface() = default;

void OutputSurface::CreateMainGrContext() {}

clay::GrContextPtr OutputSurface::GetMainGrContext() { return nullptr; }

}  // namespace clay
