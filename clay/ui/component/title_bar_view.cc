// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/title_bar_view.h"

#include <memory>

#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/keywords.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/rendering/render_container.h"

namespace clay {

TitleBarView::TitleBarView(int id, PageView* page_view)
    : BaseView(id, "title-bar-view", std::make_unique<RenderContainer>(),
               page_view) {}

void TitleBarView::HandleEvent(const PointerEvent& event) {
  if (!moveable_) {
    return;
  }

  if (event.type == PointerEvent::EventType::kMoveEvent) {
    if (move_event_continue_ > 0) {
      page_view_->MoveWindow();
    } else {
      move_event_continue_++;
    }
  } else {
    move_event_continue_ = 0;
  }
}

void TitleBarView::SetMoveable(const std::string& moveable) {
  if (moveable == "TRUE" || moveable == "true") {
    moveable_ = true;
  } else {
    moveable_ = false;
  }
}

void TitleBarView::SetAttribute(const char* attr_c, const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kw == KeywordID::kMoveable) {
    SetMoveable(attribute_utils::GetCString(value));
  } else {
    BaseView::SetAttribute(attr_c, value);
  }
}

}  // namespace clay
