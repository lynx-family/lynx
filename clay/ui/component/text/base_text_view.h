// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_TEXT_BASE_TEXT_VIEW_H_
#define CLAY_UI_COMPONENT_TEXT_BASE_TEXT_VIEW_H_

#include <memory>
#include <string>

#include "clay/ui/component/base_view.h"
#include "clay/ui/rendering/render_object.h"

namespace clay {

class BaseTextView : public WithTypeInfo<BaseTextView, BaseView> {
 public:
  BaseTextView(uint32_t id, std::string tag,
               std::unique_ptr<RenderObject> render_object,
               PageView* page_view);
  ~BaseTextView() override;

#ifdef ENABLE_ACCESSIBILITY
  bool EnableAccessibilityElement() const override { return true; }
#endif
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_TEXT_BASE_TEXT_VIEW_H_
