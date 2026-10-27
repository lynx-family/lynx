// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/editable/textarea_view.h"

#include <algorithm>
#include <string>
#include <utility>

#include "clay/ui/component/editable/input_view.h"
#include "clay/ui/rendering/render_container.h"
#include "clay/ui/shadow/editable_shadow_node.h"

namespace clay {
namespace {
LYNX_UI_METHOD_BEGIN(TextAreaView) {
  LYNX_UI_METHOD(TextAreaView, setValue);
  LYNX_UI_METHOD(TextAreaView, addText);
  LYNX_UI_METHOD(TextAreaView, sendDelEvent);
  LYNX_UI_METHOD(TextAreaView, beginEdit);
  LYNX_UI_METHOD(TextAreaView, quitEdit);
  LYNX_UI_METHOD(TextAreaView, getValue);
  LYNX_UI_METHOD(TextAreaView, blur);
  LYNX_UI_METHOD(TextAreaView, controlKeyBoard);
  LYNX_UI_METHOD(TextAreaView, focus);
  LYNX_UI_METHOD(TextAreaView, setInputFilter);
  LYNX_UI_METHOD(TextAreaView, select);
  LYNX_UI_METHOD(TextAreaView, setSelectionRange);
}
LYNX_UI_METHOD_END(TextAreaView);
}  // namespace

TextAreaView::TextAreaView(int id, PageView* page_view)
    : WithTypeInfo(id, "textarea", std::make_unique<RenderContainer>(),
                   page_view) {
  editable_view_ = new InputView(-1, id, page_view, true, false);
  editable_view_->SetKeyboardAction(KeyboardAction::kMultiLine);
  editable_view_->SetMaxLines(std::numeric_limits<uint32_t>::max());
  editable_scroll_ = new ScrollView(-1, ScrollDirection::kVertical, page_view);
  BaseView::AddChild(editable_scroll_);
  editable_scroll_->BaseView::AddChild(editable_view_);
  ResetGestureRecognizers();

#if defined(OS_WIN) || defined(OS_MAC)
  SetCursor({"text"});
#endif
}

TextAreaView::~TextAreaView() = default;

void TextAreaView::OnDestroy() {
  DestroyAllChildren();
  editable_view_ = nullptr;
  editable_scroll_ = nullptr;
}

void TextAreaView::SetAttribute(const char* attr_c, const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kw == KeywordID::kMinHeight) {
    std::string min_height;
    attribute_utils::TryGetString(value, min_height);
    min_height_ =
        attribute_utils::ToPxWithDisplayMetrics(min_height, page_view());
  } else if (kw == KeywordID::kMaxHeight) {
    std::string max_height;
    attribute_utils::TryGetString(value, max_height);
    max_height_ =
        attribute_utils::ToPxWithDisplayMetrics(max_height, page_view());
  } else if (editable_view_->MatchAttrSettings(kw)) {
    editable_view_->SetAttribute(attr_c, value);
  } else {
    BaseView::SetAttribute(attr_c, value);
  }
}

void TextAreaView::OnLayout(LayoutContext* context) {
  editable_scroll_->SetWidth(ContentWidth());
  editable_scroll_->SetHeight(ContentHeight());
  editable_view_->SetWidth(ContentWidth());
  BaseView::OnLayout(context);
  editable_view_->SetHeight(
      std::max(ContentHeight(), editable_view_->EstimateHeightWithMaxLines()));
  bool auto_height =
      min_height_ >= 0 && max_height_ >= 0 && max_height_ >= min_height_;
  // In auto-height mode, the cursor should completely change the height before
  // scrolling to the correct position. MarkDirty will only be measured in the
  // next frame, so the cursor should also scroll in the next frame.
  if (auto_height) {
    // Currently clay cannot post tasks to the layout thread, so autoheight
    // does not currently support opening asynchronous threads.
    auto node =
        static_cast<EditableShadowNode*>(page_view()->GetShadowNodeById(id()));
    node->SetTextHeight(std::max(editable_view_->GetParagraph()->GetHeight(),
                                 editable_view_->PlaceholderHeight()));
    page_view()->PostUIMethodTask([weak_ptr = GetWeakPtr()] {
      if (weak_ptr) {
        auto editable_text_view = static_cast<TextAreaView*>(weak_ptr.get());
        editable_text_view->ScheduleCaretOnScreen();
      }
    });
  } else {
    ScheduleCaretOnScreen();
  }
}

void TextAreaView::Measure(const MeasureConstraint& constraint,
                           MeasureResult& result) {
  editable_view_->Measure(constraint, result);
}

void TextAreaView::SetBound(float left, float top, float width, float height) {
  BaseView::SetBound(left, top, width, height);
  editable_scroll_->SetBound(0, 0, ContentWidth(), ContentHeight());
  editable_view_->SetHeight(
      std::max(ContentHeight(), editable_view_->EstimateHeightWithMaxLines()));
  editable_view_->SetWidth(ContentWidth());
}

void TextAreaView::ScheduleCaretOnScreen() {
  FloatRect wrapped = editable_view_->ComputeCaretRect();
  FloatRect current = {0, 0, ContentWidth(), ContentHeight()};
  FloatSize scroll_offset = editable_scroll_->GetScrollOffset();
  current.Move(scroll_offset.width(), scroll_offset.height());
  FloatPoint offset;
  offset.MoveBy(scroll_offset);
  if (wrapped.y() < current.y()) {
    offset.Move(0.f, wrapped.y() - current.y());
  } else if (wrapped.MaxY() > current.MaxY()) {
    offset.Move(0.f, wrapped.MaxY() - current.MaxY());
  }
  editable_scroll_->ScrollTo(false, offset.y());
}

void TextAreaView::ResetGestureRecognizers() {
  RemoveGestureRecognizer(tap_recognizer_);
  std::unique_ptr<TapGestureRecognizer> tap_recognizer =
      std::make_unique<TapGestureRecognizer>(page_view()->gesture_manager());
  tap_recognizer_ = tap_recognizer.get();
  tap_recognizer->SetTapUpCallback(
      [this](auto&& PH1) { OnGestureTap(std::forward<decltype(PH1)>(PH1)); });
  AddGestureRecognizer(std::move(tap_recognizer));
}

void TextAreaView::OnGestureTap(const PointerEvent& pointer) {
  editable_view_->OnGestureTap(pointer);
}

void TextAreaView::setValue(const LynxModuleValues& args,
                            const LynxUIMethodCallback& callback) {
  editable_view_->setValue(args, callback);
}

void TextAreaView::addText(const LynxModuleValues& args,
                           const LynxUIMethodCallback& callback) {
  editable_view_->addText(args, callback);
}

void TextAreaView::sendDelEvent(const LynxModuleValues& args,
                                const LynxUIMethodCallback& callback) {
  editable_view_->sendDelEvent(args, callback);
}

void TextAreaView::blur(const LynxModuleValues& args,
                        const LynxUIMethodCallback& callback) {
  editable_view_->blur(args, callback);
}

void TextAreaView::controlKeyBoard(const LynxModuleValues& args,
                                   const LynxUIMethodCallback& callback) {
  editable_view_->controlKeyBoard(args, callback);
}

void TextAreaView::focus(const LynxModuleValues& args,
                         const LynxUIMethodCallback& callback) {
  editable_view_->focus(args, callback);
}

void TextAreaView::setInputFilter(const LynxModuleValues& args,
                                  const LynxUIMethodCallback& callback) {
  editable_view_->setInputFilter(args, callback);
}

void TextAreaView::select(const LynxModuleValues& args,
                          const LynxUIMethodCallback& callback) {
  editable_view_->select(args, callback);
}

void TextAreaView::setSelectionRange(const LynxModuleValues& args,
                                     const LynxUIMethodCallback& callback) {
  editable_view_->setSelectionRange(args, callback);
}

// Added by clay
void TextAreaView::beginEdit(const LynxModuleValues& args,
                             const LynxUIMethodCallback& callback) {
  editable_view_->beginEdit(args, callback);
}

void TextAreaView::quitEdit(const LynxModuleValues& args,
                            const LynxUIMethodCallback& callback) {
  editable_view_->quitEdit(args, callback);
}

void TextAreaView::getValue(const LynxModuleValues& args,
                            const LynxUIMethodCallback& callback) {
  editable_view_->getValue(args, callback);
}

};  // namespace clay
