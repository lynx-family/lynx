// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_BOX_SHADOW_VALUE_H_
#define CLAY_GFX_GEOMETRY_BOX_SHADOW_VALUE_H_

namespace clay {

struct BoxShadowValue {
  float h_offset;
  float v_offset;
  float blur;
  float spread;
  unsigned int color;
  unsigned int option;
};

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_BOX_SHADOW_VALUE_H_
