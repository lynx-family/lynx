// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/vdom/radon/base_component.h"

#include <vector>

#include "base/include/string/string_utils.h"

namespace lynx {
namespace tasm {

void BaseComponent::SetExternalClass(const base::String& key,
                                     const base::String& value) {
  if (external_classes_.find(key) != external_classes_.end()) {
    external_classes_[key].clear();
    std::vector<std::string> classes;
    base::SplitString(value.string_view(), ' ', classes);
    for (const std::string& clazz : classes) {
      external_classes_[key].push_back(clazz);
    }
  }
}

}  // namespace tasm
}  // namespace lynx
