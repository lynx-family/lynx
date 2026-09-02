// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SCROLL_COORDINATOR_SCROLL_COORDINATOR_HEADER_H_
#define CLAY_UI_COMPONENT_SCROLL_COORDINATOR_SCROLL_COORDINATOR_HEADER_H_

#include "clay/ui/component/base_view.h"

namespace clay {

using ScrollHeightCallback = std::function<void(const int scroll_height)>;

class ScrollCoordinatorHeader
    : public WithTypeInfo<ScrollCoordinatorHeader, BaseView> {
 public:
  ScrollCoordinatorHeader(int id, PageView* page_view);
  ~ScrollCoordinatorHeader() override;

  void OnContentSizeChanged(const FloatRect& old_rect,
                            const FloatRect& new_rect) override;

  void SetScrollHeightCallback(ScrollHeightCallback call_back) {
    scroll_height_call_back_ = call_back;
  }

 private:
  ScrollHeightCallback scroll_height_call_back_;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_SCROLL_COORDINATOR_SCROLL_COORDINATOR_HEADER_H_
