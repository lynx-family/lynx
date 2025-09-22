// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list.h"

namespace lynx {
namespace tasm {

void DisplayList::Clear() {
  if (content_data_.has_value()) {
    content_data_->ops.clear();
    content_data_->int_data.clear();
    content_data_->float_data.clear();
    content_data_.reset();
  }
  if (subtree_property_data_.has_value()) {
    subtree_property_data_->ops.clear();
    subtree_property_data_->int_data.clear();
    subtree_property_data_->float_data.clear();
    subtree_property_data_.reset();
  }
}

void DisplayList::ClearSubtreeProperties() {
  if (subtree_property_data_.has_value()) {
    subtree_property_data_->ops.clear();
    subtree_property_data_->int_data.clear();
    subtree_property_data_->float_data.clear();
    subtree_property_data_.reset();
  }
}

}  // namespace tasm
}  // namespace lynx
