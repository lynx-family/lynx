// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LAYOUT_TYPES_H_
#define CLAY_UI_COMPONENT_LIST_LAYOUT_TYPES_H_

#include <optional>

#include "clay/ui/component/text/text_style.h"

namespace clay {

enum class AlignTo { kNone, kStart, kEnd, kMiddle };

// State that passed to LayoutManager subclasses so that the layout logic
// doesn't need to access ListView directly.
struct ListViewState {
  bool structure_changed = false;
  size_t item_count = 0;
};

// Defines the direction in which the data adapter is traversed.
enum class ListItemDirection {
  kToHead = -1,
  kToTail = 1,
};

// Defines the direction in which the layout is filled.
enum class ListLayoutDirection {
  kToStart = -1,
  kToEnd = 1,
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LAYOUT_TYPES_H_
