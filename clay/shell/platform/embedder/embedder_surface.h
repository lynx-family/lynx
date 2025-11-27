// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_SURFACE_H_
#define CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_SURFACE_H_

#include "base/include/fml/macros.h"
#include "clay/shell/common/output_surface.h"

namespace clay {

class EmbedderSurface : public OutputSurface {
 public:
  EmbedderSurface();

  virtual ~EmbedderSurface();

 private:
  BASE_DISALLOW_COPY_AND_ASSIGN(EmbedderSurface);
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_SURFACE_H_
