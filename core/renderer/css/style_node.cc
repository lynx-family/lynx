// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/style_node.h"

namespace lynx {
namespace css {

// TODO(songshourui.null): Sink some logic down to StyleNode and place the
// function implementation in this file.

bool StyleNode::ContainsIdSelector(const std::string& selector) const {
  return idSelector().str() == selector;
}

bool StyleNode::ContainsClassSelector(const std::string& selector) const {
  for (const auto& c : classes()) {
    if (c.str() == selector) {
      return true;
    }
  }
  return false;
}

bool StyleNode::ContainsTagSelector(const std::string& selector) const {
  return tag().str() == selector;
}

}  // namespace css
}  // namespace lynx
