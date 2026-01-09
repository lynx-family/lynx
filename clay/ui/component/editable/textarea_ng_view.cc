// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/editable/textarea_ng_view.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "clay/ui/component/editable/editable_view.h"
#include "clay/ui/gesture/tap_gesture_recognizer.h"
#include "clay/ui/rendering/render_container.h"
#include "clay/ui/shadow/editable_ng_shadow_node.h"

namespace clay {
namespace {
LYNX_UI_METHOD_BEGIN(TextAreaNGView) {
  LYNX_UI_METHOD(TextAreaNGView, setValue);
  LYNX_UI_METHOD(TextAreaNGView, getValue);
  LYNX_UI_METHOD(TextAreaNGView, blur);
  LYNX_UI_METHOD(TextAreaNGView, focus);
  LYNX_UI_METHOD(TextAreaNGView, setSelectionRange);
}
LYNX_UI_METHOD_END(TextAreaNGView);
}  // namespace

TextAreaNGView::TextAreaNGView(int id, PageView* page_view)
    : WithTypeInfo(id, "textarea-ng", std::make_unique<RenderContainer>(),
                   page_view) {
  editable_view_ = new EditableView(-1, id, "editable", page_view, true, false);
  editable_view_->SetKeyboardAction(KeyboardAction::kMultiLine);
  editable_view_->SetFocusable(true);
  editable_view_->SetMaxLines(std::numeric_limits<uint32_t>::max());
  editable_scroll_ = new ScrollView(-1, ScrollDirection::kVertical, page_view);
  BaseView::AddChild(editable_scroll_);
  editable_scroll_->BaseView::AddChild(editable_view_);
  ResetGestureRecognizers();

#if !defined(OS_ANDROID) && !defined(OS_IOS) && !defined(OS_HARMONY)
  SetCursor({"text"});
#endif
}

TextAreaNGView::~TextAreaNGView() = default;

void TextAreaNGView::OnDestroy() {
  DestroyAllChildren();
  editable_view_ = nullptr;
  editable_scroll_ = nullptr;
}

void TextAreaNGView::SetAttribute(const char* attr_c,
                                  const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (editable_view_->MatchAttrSettings(kw)) {
    editable_view_->SetAttribute(attr_c, value);
  } else {
    BaseView::SetAttribute(attr_c, value);
  }
}

void TextAreaNGView::OnLayout(LayoutContext* context) {
  editable_scroll_->SetWidth(ContentWidth());
  editable_scroll_->SetHeight(ContentHeight());
  editable_view_->SetWidth(ContentWidth());
  BaseView::OnLayout(context);
  editable_view_->SetHeight(
      std::max(ContentHeight(), editable_view_->EstimateHeightWithMaxLines()));
  auto node =
      static_cast<EditableNGShadowNode*>(page_view()->GetShadowNodeById(id()));
  if (node) {
    node->SetTextHeight(editable_view_->GetParagraph()->GetHeight());
  }
  page_view()->PostUIMethodTask([weak_ptr = GetWeakPtr()] {
    if (weak_ptr) {
      auto editable_text_view = static_cast<TextAreaNGView*>(weak_ptr.get());
      editable_text_view->ScheduleCaretOnScreen();
    }
  });
}

void TextAreaNGView::Measure(const MeasureConstraint& constraint,
                             MeasureResult& result) {
  editable_view_->Measure(constraint, result);
}

void TextAreaNGView::SetBound(float left, float top, float width,
                              float height) {
  BaseView::SetBound(left, top, width, height);
  editable_scroll_->SetBound(0, 0, ContentWidth(), ContentHeight());
  editable_view_->SetHeight(
      std::max(ContentHeight(), editable_view_->EstimateHeightWithMaxLines()));
  editable_view_->SetWidth(ContentWidth());
}

void TextAreaNGView::ScheduleCaretOnScreen() {
  FloatRect wrapped = editable_view_->ComputeCaretRect();
  FloatRect current = {0, 0, ContentWidth(), ContentHeight()};
  FloatPoint scroll_offset = editable_scroll_->GetScrollOffset();
  current.MoveBy(scroll_offset);
  FloatPoint offset;
  offset.MoveBy(scroll_offset);
  if (wrapped.y() < current.y()) {
    offset.Move(0.f, wrapped.y() - current.y());
  } else if (wrapped.MaxY() > current.MaxY()) {
    offset.Move(0.f, wrapped.MaxY() - current.MaxY());
  }
  editable_scroll_->ScrollTo(false, offset.y());
}

void TextAreaNGView::ResetGestureRecognizers() {
  RemoveGestureRecognizer(tap_recognizer_);
  std::unique_ptr<TapGestureRecognizer> tap_recognizer =
      std::make_unique<TapGestureRecognizer>(page_view()->gesture_manager());
  tap_recognizer_ = tap_recognizer.get();
  tap_recognizer->SetTapUpCallback(
      [this](auto&& PH1) { OnGestureTap(std::forward<decltype(PH1)>(PH1)); });
  AddGestureRecognizer(std::move(tap_recognizer));
}

void TextAreaNGView::OnGestureTap(const PointerEvent& pointer) {
  editable_view_->OnGestureTap(pointer);
}

void TextAreaNGView::setValue(const LynxModuleValues& args,
                              const LynxUIMethodCallback& callback) {
  editable_view_->setValue(args, callback);
}

void TextAreaNGView::blur(const LynxModuleValues& args,
                          const LynxUIMethodCallback& callback) {
  editable_view_->blur(args, callback);
}

void TextAreaNGView::focus(const LynxModuleValues& args,
                           const LynxUIMethodCallback& callback) {
  editable_view_->focus(args, callback);
}

void TextAreaNGView::setSelectionRange(const LynxModuleValues& args,
                                       const LynxUIMethodCallback& callback) {
  editable_view_->setSelectionRange(args, callback);
}

void TextAreaNGView::getValue(const LynxModuleValues& args,
                              const LynxUIMethodCallback& callback) {
  editable_view_->getValue(args, callback);
}

};  // namespace clay
