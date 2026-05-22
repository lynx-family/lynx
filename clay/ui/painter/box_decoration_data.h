// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PAINTER_BOX_DECORATION_DATA_H_
#define CLAY_UI_PAINTER_BOX_DECORATION_DATA_H_

namespace clay {

struct BoxDecorationData {
 public:
  bool background_color = false;
  bool has_background = false;
  bool has_border = false;
};

}  // namespace clay

#endif  // CLAY_UI_PAINTER_BOX_DECORATION_DATA_H_
