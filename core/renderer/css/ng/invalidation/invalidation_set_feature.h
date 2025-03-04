// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_CSS_NG_INVALIDATION_INVALIDATION_SET_FEATURE_H_
#define CORE_RENDERER_CSS_NG_INVALIDATION_INVALIDATION_SET_FEATURE_H_

#include <string>

#include "base/include/vector.h"

namespace lynx {
namespace css {

class InvalidationSetFeature {
 public:
  bool HasFeature() const {
    return !classes.empty() || !ids.empty() || !tag_names.empty();
  }

  void SetClass(const std::string& class_name) {
    if (Size() == 1 && (!ids.empty() || !classes.empty())) return;
    Clear();
    classes.push_back(class_name);
  }
  void SetId(const std::string& id) {
    if (Size() == 1 && !ids.empty()) return;
    Clear();
    ids.push_back(id);
  }
  void SetTag(const std::string& tag_name) {
    if (Size() == 1) return;
    Clear();
    tag_names.push_back(tag_name);
  }

  bool FullInvalid() const { return full_invalid_; }
  void SetFullInvalid(bool value) { full_invalid_ = value; }

  void Clear() {
    classes.clear();
    ids.clear();
    tag_names.clear();
  }
  size_t Size() const { return classes.size() + ids.size() + tag_names.size(); }

  base::InlineVector<std::string, 4> classes;
  base::InlineVector<std::string, 2> ids;
  base::InlineVector<std::string, 2> tag_names;

 private:
  bool full_invalid_{false};
};

}  // namespace css
}  // namespace lynx
#endif  // CORE_RENDERER_CSS_NG_INVALIDATION_INVALIDATION_SET_FEATURE_H_
