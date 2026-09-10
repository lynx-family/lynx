// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/cover_view.h"

#include <cmath>
#include <memory>

#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/cover_view_platform_delegate.h"
#include "clay/ui/component/keywords.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/rendering/render_external_view.h"

namespace clay {

CoverView::CoverView(int id, PageView* page_view)
    : WithTypeInfo(id, "cover-view", std::make_unique<RenderExternalView>(),
                   page_view) {
  platform_delegate_ =
      std::make_unique<CoverViewPlatformDelegate>(id, page_view);
}

CoverView::~CoverView() = default;

void CoverView::SetAttribute(const char* attr, const clay::Value& value) {
  auto kw = GetKeywordID(attr);
  if (kw == KeywordID::kEventsPassThrough) {
    const bool events_pass_through = attribute_utils::GetBool(value);
    BaseView::SetEventThrough(events_pass_through);
    if (platform_delegate_) {
      platform_delegate_->SetEventsPassThrough(events_pass_through);
    }
    return;
  }
  // cover-view uses events-pass-through instead of event-through.
  if (kw == KeywordID::kEventThrough) {
    return;
  }
  BaseView::SetAttribute(attr, value);
}

bool CoverView::HitTest(const PointerEvent& event, HitTestResult& result) {
  HitTestResult cover_result;
  if (!BaseView::HitTest(event, cover_result)) {
    return false;
  }

  for (const auto& target : cover_result) {
    if (!target) {
      continue;
    }
    if (target->ShouldPassEventToNative()) {
      return false;
    }
    break;
  }

  result.splice(result.end(), cover_result);
  return true;
}

void CoverView::SetBound(float left, float top, float width, float height) {
  BaseView::SetBound(left, top, width, height);
  static_cast<RenderExternalView*>(render_object_.get())
      ->SetBackingSize(skity::Vec2(width, height));
  if (platform_delegate_ && page_view()) {
    const auto physical_bounds = page_view()->ConvertTo<kPixelTypePhysical>(
        FloatRect(left, top, width, height));
    platform_delegate_->SetPreferredSize(
        static_cast<int>(std::ceil(physical_bounds.width())),
        static_cast<int>(std::ceil(physical_bounds.height())));
  }
}

void CoverView::OnAttachToTree() {
  BaseView::OnAttachToTree();
  if (platform_delegate_) {
    platform_delegate_->OnAttachToTree();
  }
}

void CoverView::OnDetachFromTree() {
  if (platform_delegate_) {
    platform_delegate_->OnDetachFromTree();
  }
  BaseView::OnDetachFromTree();
}

void CoverView::OnDestroy() {
  if (platform_delegate_) {
    platform_delegate_->OnDestroy();
  }
  BaseView::OnDestroy();
}

}  // namespace clay
