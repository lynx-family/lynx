// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_TEXT_RAW_TEXT_VIEW_H_
#define CLAY_UI_COMPONENT_TEXT_RAW_TEXT_VIEW_H_

#include "clay/ui/component/base_view.h"

namespace clay {

class RawTextView : public WithTypeInfo<RawTextView, BaseView> {
 public:
  RawTextView(int id, PageView* page_view);
  ~RawTextView() override;

  // avoid some text attribute send to base view
  void SetAttribute(const char* attr, const clay::Value& value) override {}

#ifdef ENABLE_ACCESSIBILITY
  bool EnableAccessibilityElement() const override { return false; }
#endif
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_TEXT_RAW_TEXT_VIEW_H_
