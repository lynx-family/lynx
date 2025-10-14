// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/component.h"

#include <memory>

#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/rendering/render_container.h"

namespace clay {

Component::Component(int id, PageView* page_view)
    : WithTypeInfo(id, "component", std::make_unique<RenderContainer>(),
                   page_view) {}

Component::~Component() = default;

void Component::SetAttribute(const char* attr_c, const clay::Value& value) {
  if (GetKeywordID(attr_c) == KeywordID::kZIndex) {
    z_index_ = attribute_utils::GetInt(value);
    return;
  }
  BaseView::SetAttribute(attr_c, value);
}

void Component::OnNodeReady() {
  BaseView::OnNodeReady();
  if (node_ready_listener_) {
    node_ready_listener_->OnComponentNodeReady(this);
  }
}

}  // namespace clay
