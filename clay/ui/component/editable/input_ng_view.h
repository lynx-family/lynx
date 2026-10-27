// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_EDITABLE_INPUT_NG_VIEW_H_
#define CLAY_UI_COMPONENT_EDITABLE_INPUT_NG_VIEW_H_

#include <map>
#include <string>

#include "clay/ui/component/base_view.h"
#include "clay/ui/component/editable/editable_view.h"

namespace clay {

class InputNGView : public WithTypeInfo<InputNGView, EditableView> {
 public:
  InputNGView(int id, PageView* page_view, bool is_multiline = false,
              bool layout_root_candidate = true);
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_EDITABLE_INPUT_NG_VIEW_H_
