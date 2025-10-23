// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/surface.h"

namespace clay {

Surface::Surface() = default;

Surface::~Surface() = default;

std::unique_ptr<GLContextResult> Surface::MakeRenderContextCurrent() {
  return std::make_unique<GLContextDefaultResult>(true);
}

bool Surface::ClearRenderContext() { return false; }

bool Surface::AllowsDrawingWhenGpuDisabled() const { return true; }

bool Surface::EnableRasterCache() const { return true; }

}  // namespace clay
