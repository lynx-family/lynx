// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SELECTION_POPUP_VIEW_H_
#define CLAY_UI_COMPONENT_SELECTION_POPUP_VIEW_H_

#include <string>
#include <vector>

#include "clay/ui/component/base_view.h"

namespace clay {

using TextFunctionListener = std::function<void()>;

class InternalTextView;

enum class ActionType {
  kPaste,
  kCopy,
  kCut,
  kSelectAll,
};

enum class MenuIndex {
  kLeft,
  kMid,
  kRight,
};

class SelectionPopupView : public WithTypeInfo<SelectionPopupView, BaseView> {
 public:
  explicit SelectionPopupView(PageView* page_view);

  ~SelectionPopupView();

  void SetAnchorOffset(std::vector<FloatPoint> offset) {
    anchor_offset_ = offset;
  }

  void SetCopyFunction(TextFunctionListener handle_copy) {
    handle_copy_ = handle_copy;
  }

  void SetSelectAllFunction(TextFunctionListener select_all) {
    select_all_ = select_all;
  }

  void BuildSelectionPopup(const std::vector<ActionType>& types);

  FloatPoint GetPositionForChild(FloatSize size, FloatSize child_size);

  double CenterOn(double position, double width, double max);

  void SetBoundsHeightAndWidth(int width, int height) {
    bounds_width_ = width;
    bounds_height_ = height;
  }

 private:
  InternalTextView* CreateTextViewByText(const std::string& text, int left,
                                         int top, MenuIndex index) const;
  TextFunctionListener handle_copy_;
  TextFunctionListener select_all_;

  std::vector<FloatPoint> anchor_offset_;

  int bounds_width_ = 0;
  int bounds_height_ = 0;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_SELECTION_POPUP_VIEW_H_
