// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_SET_CSS_VARIABLE_OP_H_
#define CORE_RENDERER_DOM_VDOM_RADON_SET_CSS_VARIABLE_OP_H_

#include <string>

#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

class SetCSSVariableOp {
 public:
  explicit SetCSSVariableOp(const std::string id_selector,
                            const lepus::Value& properties)
      : id_selector_(id_selector), properties_(properties){};

  const std::string& GetIdSelector() { return id_selector_; }
  //  const std::string& GetPropertyName() { return property_name_; }
  //  const std::string& GetPropertyValue() { return property_value_; }
  const lepus::Value& GetProperties() { return properties_; }

  friend bool operator==(const SetCSSVariableOp& left,
                         const SetCSSVariableOp& right) {
    return left.id_selector_ == right.id_selector_ &&
           left.properties_ == right.properties_;
  }

 private:
  std::string id_selector_{};
  lepus::Value properties_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_SET_CSS_VARIABLE_OP_H_
