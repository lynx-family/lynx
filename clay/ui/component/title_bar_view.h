// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_TITLE_BAR_VIEW_H_
#define CLAY_UI_COMPONENT_TITLE_BAR_VIEW_H_

#include <string>

#include "clay/ui/component/base_view.h"

namespace clay {

class PageView;

class TitleBarView : public BaseView {
 public:
  TitleBarView(int id, PageView* page_view);
  ~TitleBarView() override = default;

  void HandleEvent(const PointerEvent& event) override;
  void SetAttribute(const char* attr_c, const clay::Value& value) override;

 private:
  void SetMoveable(const std::string& moveable);
  bool moveable_ = false;
  int move_event_continue_ = 0;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_TITLE_BAR_VIEW_H_
