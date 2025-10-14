// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/surface_frame.h"

#include <limits>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"

namespace clay {

SurfaceFrame::SurfaceFrame(clay::GrSurfacePtr surface,
                           FramebufferInfo framebuffer_info,
                           const EncodeCallback& encode_callback,
                           const SubmitCallback& submit_callback,
                           skity::Vec2 frame_size,
                           std::unique_ptr<GLContextResult> context_result)
    : surface_(std::move(surface)),
      framebuffer_info_(framebuffer_info),
      encode_callback_(encode_callback),
      submit_callback_(submit_callback),
      context_result_(std::move(context_result)) {
  FML_DCHECK(submit_callback_);
  if (surface_) {
    [[maybe_unused]] bool clear = !framebuffer_info.supports_partial_repaint;
    canvas_ = SURFACE_GET_CANVAS(surface_, clear);
  }
}

clay::GrCanvas* SurfaceFrame::GetCanvas() { return canvas_; }

clay::GrSurfacePtr SurfaceFrame::GetSurface() const { return surface_; }

void SurfaceFrame::Prepare(std::optional<skity::Rect> damage) {
  if (prepared_) {
    FML_LOG(ERROR) << "Surface frame already prepared.";
    return;
  }

  if (prepared_callback_) {
    prepared_callback_(damage);
  }

  prepared_ = true;
}

bool SurfaceFrame::Encode() {
  TRACE_EVENT("clay", "SurfaceFrame::Encode");
  if (encoded_) {
    return false;
  }

  encoded_ = PerformEncode();

  return encoded_;
}

std::pair<SurfaceFrame::SubmitCallback, SurfaceFrame::SubmitInfo>
SurfaceFrame::PrepareSubmit() {
  if (!encoded_ && !Encode()) {
    return {nullptr, {}};
  }

  return {[submitted = submitted_,
           submit_callback = submit_callback_](const SubmitInfo& info) {
            if (*submitted) {
              FML_LOG(ERROR) << "Surface frame already submitted.";
              return false;
            }
            *submitted = true;
            return submit_callback(info);
          },
          submit_info_};
}

bool SurfaceFrame::Submit() {
  auto [submit_callback, submit_info] = PrepareSubmit();
  if (submit_callback == nullptr) {
    return false;
  }
  return submit_callback(std::move(submit_info));
}

bool SurfaceFrame::PerformEncode() {
  if (encode_callback_ == nullptr) {
    return false;
  }

  if (encode_callback_(*this, GetCanvas())) {
    return true;
  }
  return false;
}

}  // namespace clay
