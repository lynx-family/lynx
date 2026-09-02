// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/scroll_coordinator/scroll_coordinator.h"

#include <algorithm>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/css_property.h"
#include "clay/ui/component/keywords.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/scroll_coordinator/scroll_coordinator_scroll_view.h"
#include "clay/ui/lynx_module/type_utils.h"
#include "clay/ui/rendering/render_container.h"

namespace clay {

namespace {

LYNX_UI_METHOD_BEGIN(ScrollCoordinator) {
  LYNX_UI_METHOD(ScrollCoordinator, setFoldExpanded);
}
LYNX_UI_METHOD_END(ScrollCoordinator);

const std::unordered_set<KeywordID> kProxyAttributes = {
    KeywordID::kHeaderOverSlot,
    KeywordID::kScrollEnable,
    KeywordID::kGranularity,
};

const std::unordered_set<std::string> kProxyEvents = {
    event_attr::kEventOffset,
};

}  // namespace

ScrollCoordinator::ScrollCoordinator(int32_t id, PageView* page_view)
    : WithTypeInfo(id, "ScrollCoordinator", std::make_unique<RenderContainer>(),
                   page_view) {
  scroll_view_ = new ScrollCoordinatorScrollView(-1, id, page_view);
  BaseView::AddChild(scroll_view_, 0);
}

ScrollCoordinator::~ScrollCoordinator() {
  // scroll_view_ will be destroyed by view_context_->destroy
}

void ScrollCoordinator::AddChild(BaseView* child, int index) {
  if (child->Is<ScrollCoordinatorToolbar>() && !toolbar_) {
    toolbar_ = static_cast<ScrollCoordinatorToolbar*>(child);
    BaseView::AddChild(child, child_count());
  } else {
    scroll_view_->BaseView::AddChild(child);
  }
}

void ScrollCoordinator::RemoveChild(BaseView* child) {
  if (child->Is<ScrollCoordinatorToolbar>()) {
    BaseView::RemoveChild(child);
    toolbar_ = nullptr;
  } else {
    FML_DCHECK(scroll_view_);
    if (scroll_view_) {
      scroll_view_->RemoveChild(child);
    }
  }
}

void ScrollCoordinator::setFoldExpanded(const LynxModuleValues& args,
                                        const LynxUIMethodCallback& callback) {
  clay::Value::Map response;
  if (!args.HasKey("offset")) {
    response.emplace("msg", clay::Value("no offset"));
    response.emplace("success", clay::Value(false));
    callback(LynxUIMethodResult::kSuccess, clay::Value(std::move(response)));
    return;
  }
  std::string offset = "";  // new param key
  bool with_anim = true;
  if (CastNamedLynxModuleArgs({"offset", "smooth"}, args, offset, with_anim)) {
    float header_offset_px =
        attribute_utils::ToPxWithDisplayMetrics(offset, page_view());
    float toolbar_height = toolbar_ ? toolbar_->Height() : 0.f;
    float header_height =
        scroll_view_->GetHeader() ? scroll_view_->GetHeader()->Height() : 0.f;
    header_offset_px = std::max(
        std::min(header_offset_px, header_height - toolbar_height), 0.f);
    scroll_view_->SetFoldExpanded(header_offset_px, with_anim);
    response.emplace("msg", clay::Value(""));
    response.emplace("success", clay::Value(true));
    callback(LynxUIMethodResult::kSuccess, clay::Value(std::move(response)));
  } else {
    response.emplace("msg", clay::Value("decode parameters error"));
    response.emplace("success", clay::Value(false));
    callback(LynxUIMethodResult::kSuccess, clay::Value(std::move(response)));
  }
}

void ScrollCoordinator::OnLayout(LayoutContext* context) {
  FML_DCHECK(scroll_view_);
  if (scroll_view_) {
    scroll_view_->SetToolbarHeight(toolbar_ ? toolbar_->Height() : 0.f);
  }
  BaseView::OnLayout(context);
  if (scroll_view_) {
    scroll_view_->SetBound(0, 0, Width(), Height());
  }
  if (toolbar_) {
    toolbar_->SetY(0);
  }
  if (scroll_view_) {
    scroll_view_->SetY(0);
  }
}

void ScrollCoordinator::AddEventCallback(const char* event) {
  if (kProxyEvents.find(event) != kProxyEvents.end()) {
    scroll_view_->AddEventCallback(event);
  } else {
    BaseView::AddEventCallback(event);
  }
}

void ScrollCoordinator::SetAttribute(const char* attr_c,
                                     const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kProxyAttributes.find(kw) != kProxyAttributes.end()) {
    scroll_view_->SetAttribute(attr_c, value);
  } else {
    BaseView::SetAttribute(attr_c, value);
  }
}
}  // namespace clay
