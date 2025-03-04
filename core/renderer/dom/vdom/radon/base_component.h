// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_BASE_COMPONENT_H_
#define CORE_RENDERER_DOM_VDOM_RADON_BASE_COMPONENT_H_

#include <string>
#include <unordered_map>

#include "core/renderer/utils/base/base_def.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

// BaseComponent is an abstraction for RadonComponent and ComponentElement to
// support worklet on NoDiff TTML.
class BaseComponent {
 public:
  BaseComponent() = default;
  virtual ~BaseComponent() = default;

  const std::unordered_map<base::String, ClassList>& external_classes() const {
    return external_classes_;
  }

  void SetExternalClass(const base::String& key, const base::String& value);

  std::unordered_map<std::string, lepus::Value>& worklet_instances() {
    return worklet_instances_;
  }

  void InsertWorklet(const std::string& worklet_name,
                     const lepus::Value& worklet) {
    worklet_instances_[worklet_name] = worklet;
  }

  lepus::Value inner_state() const { return inner_state_; }

  void set_inner_state(const lepus::Value& state) { inner_state_ = state; }

  virtual const lepus::Value& GetData() = 0;

  virtual const lepus::Value& GetProperties() = 0;

  virtual const std::string& GetEntryName() const = 0;

  virtual std::string ComponentStrId() = 0;

  virtual bool IsPageForBaseComponent() const { return false; }

 protected:
  std::unordered_map<base::String, ClassList> external_classes_{};
  std::unordered_map<std::string, lepus::Value> worklet_instances_{};
  lepus::Value inner_state_{};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_BASE_COMPONENT_H_
