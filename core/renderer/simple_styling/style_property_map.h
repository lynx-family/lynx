// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_SIMPLE_STYLING_STYLE_PROPERTY_MAP_H_
#define CORE_RENDERER_SIMPLE_STYLING_STYLE_PROPERTY_MAP_H_
namespace lynx {
namespace style {
class StylePropertyMap {
 public:
  explicit StylePropertyMap(const tasm::StyleMap& style_map)
      : style_map_(style_map){};
  ~StylePropertyMap() = default;

 private:
  tasm::StyleMap style_map_;
};
}  // namespace style
}  // namespace lynx
#endif  // CORE_RENDERER_SIMPLE_STYLING_STYLE_PROPERTY_MAP_H_
