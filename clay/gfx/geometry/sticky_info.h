// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_STICKY_INFO_H_
#define CLAY_GFX_GEOMETRY_STICKY_INFO_H_

namespace clay {

class StickyInfo {
 public:
  float left;
  float top;
  float right;
  float bottom;
  float offset_x;
  float offset_y;
};

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_STICKY_INFO_H_
