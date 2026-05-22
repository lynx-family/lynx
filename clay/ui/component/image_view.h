// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/base_image_view.h"

#ifndef CLAY_UI_COMPONENT_IMAGE_VIEW_H_
#define CLAY_UI_COMPONENT_IMAGE_VIEW_H_

namespace clay {

class ImageView : public BaseImageView {
 public:
  ImageView(int id, PageView* page_view);
  ~ImageView() override = default;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_IMAGE_VIEW_H_
