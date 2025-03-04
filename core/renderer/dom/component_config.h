// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_COMPONENT_CONFIG_H_
#define CORE_RENDERER_DOM_COMPONENT_CONFIG_H_

namespace lynx {
namespace tasm {

enum class BooleanProp {
  NotSet,
  TrueValue,
  FalseValue,
};

class ComponentConfig {
 public:
  ComponentConfig() = default;

  inline void SetEnableRemoveExtraData(bool enable) {
    enable_remove_extra_data_ =
        enable ? BooleanProp::TrueValue : BooleanProp::FalseValue;
  }

  inline BooleanProp GetEnableRemoveExtraData() const {
    return enable_remove_extra_data_;
  }

  inline void SetRemoveComponentElement(bool enable) {
    remove_component_element_ =
        enable ? BooleanProp::TrueValue : BooleanProp::FalseValue;
  }

  inline BooleanProp GetRemoveComponentElement() const {
    return remove_component_element_;
  }

 private:
  ComponentConfig(ComponentConfig& config) = delete;
  ComponentConfig& operator=(ComponentConfig& config) = delete;

  BooleanProp enable_remove_extra_data_{BooleanProp::NotSet};
  BooleanProp remove_component_element_{BooleanProp::NotSet};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_COMPONENT_CONFIG_H_
