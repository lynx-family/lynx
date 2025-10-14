// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_SURFACE_FRAME_H_
#define CLAY_FLOW_SURFACE_FRAME_H_

#include <memory>
#include <optional>
#include <utility>

#include "base/include/fml/macros.h"
#include "base/include/fml/time/time_point.h"
#include "clay/common/graphics/gl_context_switch.h"
#include "clay/gfx/rendering_backend.h"
#include "skity/geometry/rect.hpp"

namespace clay {

// This class represents a frame that has been fully configured for the
// underlying client rendering API. A frame may only be submitted once.
class SurfaceFrame {
 public:
  // This callback is called when the frame is prepared to be renderered.
  // Since in EGL
  // https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_partial_update.txt,
  // any render commands must happen after eglSetDamageRegionKHR.
  using PreparedCallback =
      std::function<void(std::optional<skity::Rect> buffer_damage)>;
  using EncodeCallback =
      std::function<bool(SurfaceFrame& surface_frame, clay::GrCanvas* canvas)>;

  struct SubmitInfo {
    // The frame damage for frame n is the difference between frame n and
    // frame (n-1), and represents the area that a compositor must recompose.
    //
    // Corresponds to EGL_KHR_swap_buffers_with_damage
    std::optional<skity::Rect> frame_damage;

    // The buffer damage for a frame is the area changed since that same buffer
    // was last used. If the buffer has not been used before, the buffer damage
    // is the entire area of the buffer.
    //
    // Corresponds to EGL_KHR_partial_update
    std::optional<skity::Rect> buffer_damage;

    // Time at which this frame is scheduled to be presented. This is a hint
    // that can be passed to the platform to drop queued frames.
    std::optional<fml::TimePoint> presentation_time;

    // Whether this surface presents with a CATransaction on Apple platforms.
    //
    // When there are platform views in the scene, the drawable needs to be
    // presented in the same CATransaction as the one created for platform view
    // mutations.
    //
    // If the drawables are being presented from the raster thread, we cannot
    // use a transaction as it will dirty the UIViews being presented. If there
    // is a non-Flutter UIView active, such as in add2app or a
    // presentViewController page transition, then this will cause CoreAnimation
    // assertion errors and exit the app.
    bool present_with_transaction = false;
  };

  // SubmitCallback may be called in Raster thread or Platform thread.
  // The callback should not capture any resources whoese lifetime is bound to
  // raster thread.
  using SubmitCallback = std::function<bool(const SubmitInfo&)>;

  // Information about the underlying framebuffer
  struct FramebufferInfo {
    // Indicates whether or not the surface supports pixel readback as used in
    // circumstances such as a BackdropFilter.
    bool supports_readback = false;

    // Indicates that target device supports partial repaint. At very minimum
    // this means that the surface will provide valid existing damage.
    bool supports_partial_repaint = false;

    // For some targets it may be beneficial or even required to snap clip
    // rect to tile grid. I.e. repainting part of a tile may cause performance
    // degradation if the tile needs to be decompressed first.
    int vertical_clip_alignment = 1;
    int horizontal_clip_alignment = 1;

    // This is the area of framebuffer that lags behind the front buffer.
    //
    // Correctly providing exiting_damage is necessary for supporting double and
    // triple buffering. Embedder is responsible for tracking this area for each
    // of the back buffers used. When doing partial redraw, this area will be
    // repainted alongside of dirty area determined by diffing current and
    // last successfully rasterized layer tree;
    //
    // If existing damage is unspecified (nullopt), entire frame will be
    // rasterized (no partial redraw). To signal that there is no existing
    // damage use an empty skity::Rect.
    std::optional<skity::Rect> existing_damage = std::nullopt;
  };

  SurfaceFrame(clay::GrSurfacePtr surface, FramebufferInfo framebuffer_info,
               const EncodeCallback& encode_callback,
               const SubmitCallback& submit_callback, skity::Vec2 frame_size,
               std::unique_ptr<GLContextResult> context_result = nullptr);

  void Prepare(std::optional<skity::Rect> buffer_damage);

  bool Encode();

  // It's the caller's responsibility to call SubmitCallback to do the real
  // submit in the raster thread or platform thread.
  [[nodiscard]] std::pair<SubmitCallback, SubmitInfo> PrepareSubmit();

  bool Submit();

  clay::GrSurfacePtr GetSurface() const;
  clay::GrCanvas* GetCanvas();

  const FramebufferInfo& framebuffer_info() const { return framebuffer_info_; }

  void set_submit_info(const SubmitInfo& submit_info) {
    submit_info_ = submit_info;
  }
  const SubmitInfo& submit_info() const { return submit_info_; }

  void SetPreparedCallback(const PreparedCallback& callback) {
    prepared_callback_ = callback;
  }

 private:
  bool prepared_ = false;
  bool encoded_ = false;
  std::shared_ptr<bool> submitted_ = std::make_shared<bool>(false);

  clay::GrSurfacePtr surface_;
  clay::GrCanvas* canvas_ = nullptr;
  FramebufferInfo framebuffer_info_;
  SubmitInfo submit_info_;
  PreparedCallback prepared_callback_;
  EncodeCallback encode_callback_;
  SubmitCallback submit_callback_;
  std::unique_ptr<GLContextResult> context_result_;

  bool PerformEncode();

  BASE_DISALLOW_COPY_AND_ASSIGN(SurfaceFrame);
};

}  // namespace clay

#endif  // CLAY_FLOW_SURFACE_FRAME_H_
