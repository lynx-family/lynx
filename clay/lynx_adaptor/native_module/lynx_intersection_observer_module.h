// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_INTERSECTION_OBSERVER_MODULE_H_
#define CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_INTERSECTION_OBSERVER_MODULE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/lynx_adaptor/clay_value.h"
#include "clay/lynx_adaptor/native_module/lynx_module_base.h"
#include "clay/ui/component/base_view.h"

namespace lynx {

class LynxIntersectionObserverModule
    : public LynxModuleBase,
      public std::enable_shared_from_this<LynxIntersectionObserverModule> {
 public:
  static std::shared_ptr<LynxIntersectionObserverModule> Create(
      uint32_t view_context_id, fml::RefPtr<fml::TaskRunner> task_runner) {
    return std::make_shared<LynxIntersectionObserverModule>(view_context_id,
                                                            task_runner);
  }

  static const std::string& GetName() { return name_; }
  LynxIntersectionObserverModule(uint32_t view_context_id,
                                 fml::RefPtr<fml::TaskRunner> task_runner);
  ~LynxIntersectionObserverModule() override;

  std::unique_ptr<lynx::pub::Value> CreateIntersectionObserver(
      std::unique_ptr<lynx::pub::Value> args_array,
      const lynx::runtime::CallbackMap& callback_map);

  std::unique_ptr<lynx::pub::Value> RelativeTo(
      std::unique_ptr<lynx::pub::Value> args_array,
      const lynx::runtime::CallbackMap& callback_map);

  std::unique_ptr<lynx::pub::Value> RelativeToScreen(
      std::unique_ptr<lynx::pub::Value> args_array,
      const lynx::runtime::CallbackMap& callback_map);

  std::unique_ptr<lynx::pub::Value> RelativeToViewport(
      std::unique_ptr<lynx::pub::Value> args_array,
      const lynx::runtime::CallbackMap& callback_map);

  std::unique_ptr<lynx::pub::Value> Observe(
      std::unique_ptr<lynx::pub::Value> args_array,
      const lynx::runtime::CallbackMap& callback_map);

  std::unique_ptr<lynx::pub::Value> Disconnect(
      std::unique_ptr<lynx::pub::Value> args_array,
      const lynx::runtime::CallbackMap& callback_map);

 private:
  static const std::string name_;

  std::unordered_map<int, clay::Value::Map> observer_configs_;
  std::unordered_map<int, std::vector<clay::BaseView*>> observers_;
};

}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_INTERSECTION_OBSERVER_MODULE_H_
