// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_UI_METHOD_MODULE_H_
#define CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_UI_METHOD_MODULE_H_

#include <memory>
#include <string>
#include <vector>

#include "clay/lynx_adaptor/native_module/lynx_module_base.h"
#include "clay/ui/component/base_view.h"

namespace lynx {

class LynxUIMethodModule
    : public LynxModuleBase,
      public std::enable_shared_from_this<LynxUIMethodModule> {
 public:
  static std::shared_ptr<LynxNativeModule> Create(
      uint32_t view_context_id, fml::RefPtr<fml::TaskRunner> task_runner) {
    return std::make_shared<LynxUIMethodModule>(view_context_id, task_runner);
  }

  static const std::string& GetName() { return name_; }

  LynxUIMethodModule(uint32_t view_context_id,
                     fml::RefPtr<fml::TaskRunner> task_runner);
  ~LynxUIMethodModule() override;

  // for compatibility with old getNodeRef
  std::unique_ptr<lynx::pub::Value> InvokeUIMethodCompatibility(
      std::unique_ptr<lynx::pub::Value> args,
      const lynx::runtime::CallbackMap& callbacks);

 private:
  void InvokeUIMethod(const std::string& component_id,
                      const std::vector<std::string>& nodes,
                      const std::string& method_name,
                      const clay::LynxModuleValues& args,
                      const clay::LynxUIMethodCallback& callback);

  void EnsureInvokeAfterLayout(std::function<void()> invocation);

  friend class LynxUIMethodRegistrar;

  static const std::string name_;
};

}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_UI_METHOD_MODULE_H_
