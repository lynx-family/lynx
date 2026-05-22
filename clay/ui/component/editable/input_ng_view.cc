// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/editable/input_ng_view.h"

#include "clay/ui/component/page_view.h"

namespace clay {

InputNGView::InputNGView(int id, PageView* page_view, bool is_multiline,
                         bool layout_root_candidate)
    : WithTypeInfo(id, id, "input-ng", page_view, is_multiline,
                   layout_root_candidate) {
  SetFocusable(true);
}

}  // namespace clay
