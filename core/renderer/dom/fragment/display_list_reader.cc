// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list_reader.h"

namespace lynx {
namespace tasm {

DisplayListReader::DisplayListReader(const DisplayList& list)
    : items_(list.GetContentItemsData()),
      data_(list.GetContentData()),
      item_count_(list.GetContentItemsSize()),
      item_index_(0) {}

}  // namespace tasm
}  // namespace lynx
