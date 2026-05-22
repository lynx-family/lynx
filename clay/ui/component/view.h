// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_VIEW_H_
#define CLAY_UI_COMPONENT_VIEW_H_

#include "clay/ui/component/base_view.h"

namespace clay {

class View : public WithTypeInfo<View, BaseView> {
 public:
  View(int id, PageView* page_view);
  ~View() override;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_VIEW_H_
