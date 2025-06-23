// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_text.h"

#include <stack>
#include <string>
#include <utility>

#include "base/include/string/string_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/font_collection_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/style_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"

namespace lynx {
namespace tasm {
namespace harmony {

UIText::UIText(LynxContext* context, int sign, const std::string& tag)
    : UIBase(context, ARKUI_NODE_CUSTOM, sign, tag) {
  if (context->IsEnableTextOverflow()) {
    overflow_ = {true, true};
  }
  InitAccessibilityAttrs(LynxAccessibilityMode::kEnable, "text");
}

void UIText::Update(ParagraphHarmony* paragraph) {
  paragraph_ = fml::RefPtr<ParagraphHarmony>(paragraph);
}

void UIText::UpdateExtraData(
    const fml::RefPtr<fml::RefCountedThreadSafeStorage>& bundle) {
  auto* paragraph = reinterpret_cast<ParagraphHarmony*>(bundle.get());
  paragraph_ = fml::RefPtr<ParagraphHarmony>(paragraph);
  paragraph_->SetEventTargetParent(this);
  Invalidate();
}

void UIText::FrameDidChanged() {
  UIBase::FrameDidChanged();
  if (paragraph_) {
    translate_left_offset_ =
        (padding_left_ + GetBorderLeftWidth()) * context_->ScaledDensity() +
        paragraph_->GetTranslateLeftOffset();
    translate_top_offset_ =
        (padding_top_ + GetBorderTopWidth()) * context_->ScaledDensity();

    UpdateInlineImageFrame();
    SetAccessibilityLabelDirtyFlag();
  }
}

const std::string& UIText::GetAccessibilityLabel() const {
  if (UIBase::GetAccessibilityLabel().empty() && paragraph_) {
    return paragraph_->GetText();
  }
  return UIBase::GetAccessibilityLabel();
}

void UIText::UpdateInlineImageFrame() {
  auto rects = paragraph_->GetRectsForPlaceholders();
  size_t count = rects.GetCount();
  const auto& placeholder_signs = paragraph_->GetPlaceholders();
  for (auto& child_ui : Children()) {
    auto iterator = std::find(placeholder_signs.begin(),
                              placeholder_signs.end(), child_ui->Sign());
    if (iterator == placeholder_signs.end()) {
      child_ui->SetVisibility(false);
      continue;
    }
    size_t index = iterator - placeholder_signs.begin();
    if (index < count) {
      child_ui->SetVisibility(true);
    } else {
      // inline image or view will be omitted
      child_ui->SetVisibility(false);
    }
  }
}

void UIText::OnDraw(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node) {
  UIBase::OnDraw(canvas, node);
  if (NeedDraw(node)) {
    Render(canvas);
  }
}

void UIText::Render(OH_Drawing_Canvas* canvas) const {
  if (paragraph_) {
    paragraph_->Draw(canvas, translate_left_offset_, translate_top_offset_);
  }
}

EventTarget* UIText::HitTest(float point[2]) {
  if (paragraph_) {
    float point_exclude_padding_border[] = {
        (point[0] - (padding_left_ + GetBorderLeftWidth())) *
            context_->ScaledDensity(),
        (point[1] - (padding_top_ + GetBorderTopWidth())) *
            context_->ScaledDensity()};
    EventTarget* ret = paragraph_->HitTest(point_exclude_padding_border);
    if (ret != nullptr) {
      return ret;
    }
  }

  return UIBase::HitTest(point);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
