// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_GPU_GPU_SURFACE_SOFTWARE_DELEGATE_H_
#define CLAY_SHELL_GPU_GPU_SURFACE_SOFTWARE_DELEGATE_H_

#include "base/include/fml/macros.h"
#include "clay/flow/embedded_views.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

//------------------------------------------------------------------------------
/// @brief      Interface implemented by all platform surfaces that can present
///             a software backing store to the "screen". The GPU surface
///             abstraction (which abstracts the client rendering API) uses this
///             delegation pattern to tell the platform surface (which abstracts
///             how backing stores fulfilled by the selected client rendering
///             API end up on the "screen" on a particular platform) when the
///             rasterizer needs to allocate and present the software backing
///             store.
///
/// @see        |IOSSurfaceSoftware|, |AndroidSurfaceSoftware|,
///             |EmbedderSurfaceSoftware|.
///
class GPUSurfaceSoftwareDelegate {
 public:
  ~GPUSurfaceSoftwareDelegate();

  //----------------------------------------------------------------------------
  /// @brief      Called when the GPU surface needs a new buffer to render a new
  ///             frame into.
  ///
  /// @param[in]  size  The size of the frame.
  ///
  /// @return     A raster surface returned by the platform.
  ///
  virtual clay::GrSurfacePtr AcquireBackingStore(const skity::Vec2& size) = 0;

  //----------------------------------------------------------------------------
  /// @brief      Called by the platform when a frame has been rendered into the
  ///             backing store and the platform must display it on-screen.
  ///
  /// @param[in]  backing_store  The software backing store to present.
  ///
  /// @return     Returns if the platform could present the backing store onto
  ///             the screen.
  ///
  virtual bool PresentBackingStore(clay::GrSurfacePtr backing_store) = 0;
};

}  // namespace clay

#endif  // CLAY_SHELL_GPU_GPU_SURFACE_SOFTWARE_DELEGATE_H_
