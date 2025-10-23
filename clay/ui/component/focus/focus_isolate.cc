// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/focus/focus_isolate.h"

#include <string>

#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_context.h"

namespace clay {

FocusManager::TraversalResult FocusIsolate::OnTraversalOnScope(
    FocusManager::Direction direction,
    FocusManager::TraversalType traversal_type) {
  FocusManager::TraversalResult res =
      FocusNode::OnTraversalOnScope(direction, traversal_type);
  if (!res.succeed) {
    // Try leave, notify.
    OnFocusEscaping(direction);
  }

  if (!allow_escape_) {
    res.succeed = true;
  }

  return res;
}

void FocusIsolate::SetAttribute(const char* attr_c, const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kw == KeywordID::kAllowEscape) {
    allow_escape_ = attribute_utils::GetBool(value, false);
  } else if (kw == KeywordID::kAutoSaveLastChild) {
    save_last_focus_child_ = attribute_utils::GetBool(value, true);
  } else if (kw == KeywordID::kPreferredFocusChild) {
    preferred_child_ = attribute_utils::GetCString(value);
  } else {
    BaseView::SetAttribute(attr_c, value);
  }
}

void FocusIsolate::OnFocusEscaping(FocusManager::Direction direction) {
  auto* focus_node = GetFocusManager()->GetLeafFocusedNode();
  FML_DCHECK(focus_node);
  std::stringstream focus_index;
  std::string id_selector;
  if (focus_node) {
    focus_index << focus_node->GetFocusIndex(Axis::kX) << ","
                << focus_node->GetFocusIndex(Axis::kY);
    id_selector = static_cast<BaseView*>(focus_node)->GetIdSelector();
  }
  // Keep string alive to transfer c_str.
  auto index_str = focus_index.str();
  page_view()->SendEvent(id(), event_attr::kEventFocusEscape,
                         {"direction", "focusIndex", "fromId"},
                         static_cast<int>(direction), index_str.c_str(),
                         id_selector.c_str());
}

void FocusIsolate::OnFocusEntering(FocusManager::Direction direction) {
  FocusNode* preferred_view = nullptr;
  if (save_last_focus_child_ && last_focused_view_) {
    preferred_view = last_focused_view_;
    last_focused_view_ = nullptr;
    FML_DLOG(INFO) << "Use last focused child: " << preferred_view;
  } else if (!preferred_child_.empty()) {
    preferred_view = ViewContext::FindViewByIdSelector(preferred_child_, this);
    FML_DLOG(INFO) << "Use preferred focus child(" << preferred_child_
                   << "): " << preferred_view;
  }
  if (preferred_view) {
    preferred_view->RequestFocus();
  }

  page_view()->SendEvent(id(), event_attr::kEventFocusEnter, {"direction"},
                         static_cast<int>(direction));
}

void FocusIsolate::OnFocusNodeDestructed(FocusNode* node) {
  if (node == last_focused_view_) {
    last_focused_view_ = nullptr;
  }
}

}  // namespace clay
