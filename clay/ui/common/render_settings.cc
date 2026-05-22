// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/common/render_settings.h"

#include "clay/common/sys_info.h"

namespace clay {

RenderSettings::RenderSettings() = default;
RenderSettings::~RenderSettings() = default;

void RenderSettings::RenewMode() {
  if (is_touching_ || has_animation_) {
    mode_ = kSMOOTH;
    return;
  }
  mode_ = kNORMAL;
}

void RenderSettings::SetIsTouching(bool is_touching) {
  if (is_touching_ == is_touching) {
    return;
  }
  is_touching_ = is_touching;
  RenewMode();
}

void RenderSettings::SetHasAnimation(bool has_animation) {
  if (has_animation_ == has_animation) {
    return;
  }
  has_animation_ = has_animation;
  RenewMode();
}

bool RenderSettings::ShouldLowMemoryUsage() const {
  if (mode_ != kNORMAL) {
    return false;
  }
  if (SysInfo::IsLowEndDevice()) {
    return true;
  }
  return false;
}

}  // namespace clay
