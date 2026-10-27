// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_EDITABLE_TEXTAREA_VIEW_H_
#define CLAY_UI_COMPONENT_EDITABLE_TEXTAREA_VIEW_H_

#include <limits>
#include <memory>

#include "clay/ui/component/base_view.h"
#include "clay/ui/component/editable/input_view.h"
#include "clay/ui/component/measurable.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/scroll_wrapper.h"

namespace clay {

class TextAreaView : public WithTypeInfo<TextAreaView, BaseView>,
                     public Measurable {
 public:
  TextAreaView(int id, PageView* page_view);
  ~TextAreaView() override;
  void SetAttribute(const char* attr_c, const clay::Value& value) override;
  bool IsLayoutRootCandidate() const override { return true; }
  void OnLayout(LayoutContext* context) override;
  void Measure(const MeasureConstraint& constraint,
               MeasureResult& result) override;
  void SetBound(float left, float top, float width, float height) override;

#ifdef ENABLE_ACCESSIBILITY
  bool EnableAccessibilityElement() const override { return true; }
#endif

  void ScheduleCaretOnScreen();

  void ResetGestureRecognizers();

  void OnGestureTap(const PointerEvent& pointer);

  txt::Paragraph* GetParagraph() { return editable_view_->GetParagraph(); }

  // Lynx module UI method
#define UI_METHOD_LIST_DECLARATION(V) \
  V(setValue)                         \
  V(addText)                          \
  V(sendDelEvent)                     \
  V(blur)                             \
  V(controlKeyBoard)                  \
  V(focus)                            \
  V(setInputFilter)                   \
  V(select)                           \
  V(setSelectionRange)                \
  V(beginEdit)                        \
  V(quitEdit)                         \
  V(getValue)
  UI_METHOD_LIST_DECLARATION(UI_METHOD_DEF);
#undef UI_METHOD_LIST_DECLARATION

 private:
  void OnDestroy() override;

  InputView* editable_view_ = nullptr;
  ScrollView* editable_scroll_ = nullptr;
  GestureRecognizer* tap_recognizer_ = nullptr;
  float min_height_ = -1;
  float max_height_ = -1;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_EDITABLE_TEXTAREA_VIEW_H_
