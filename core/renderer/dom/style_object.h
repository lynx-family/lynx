// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_STYLE_OBJECT_H_
#define CORE_RENDERER_DOM_STYLE_OBJECT_H_
#include <list>
#include <memory>

#include "core/renderer/simple_styling/style_property_map.h"
#include "core/runtime/vm/lepus/ref_counted_class.h"

namespace lynx {
namespace tasm {

class StyleObject {
 public:
  explicit StyleObject(const StyleMap& style_map)
      : style_map_(std::make_unique<style::StylePropertyMap>(style_map)) {}

  // TODO(renzhongyue): Add functions to bind object with FiberElement and
  // reflects the updates on the style_map_ to the computed_css_value_
  //  of target elements.

 private:
  std::unique_ptr<style::StylePropertyMap> style_map_;
  std::list<FiberElement*> elements_;
};

}  // namespace tasm

template <>
struct lepus::RefTypeTrait<tasm::StyleObject> {
  static constexpr RefType value = RefType::kStyleObject;
};

}  // namespace lynx

#endif  // CORE_RENDERER_DOM_STYLE_OBJECT_H_
