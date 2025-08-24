// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/basic_animation/keyframe.h"

#include "core/animation/basic_animation/animator_target.h"

namespace lynx {
namespace animation {
namespace basic {

void Keyframe::SetDefaultPropertyValue(
    const std::string& type, const std::shared_ptr<AnimatorTarget>& target) {
  AddPropertyValue(target->GetStyle(type));
}

}  // namespace basic
}  // namespace animation
}  // namespace lynx
