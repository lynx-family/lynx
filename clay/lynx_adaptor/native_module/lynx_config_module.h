// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_CONFIG_MODULE_H_
#define CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_CONFIG_MODULE_H_

#include <memory>
#include <string>

#include "clay/lynx_adaptor/native_module/lynx_module_base.h"

namespace lynx {

class LynxConfigModule : public LynxModuleBase,
                         public std::enable_shared_from_this<LynxConfigModule> {
 public:
  static std::shared_ptr<LynxNativeModule> Create(
      uint32_t view_context_id, fml::RefPtr<fml::TaskRunner> task_runner) {
    return std::make_shared<LynxConfigModule>(view_context_id, task_runner);
  }

  static const std::string& GetName() { return name_; }

  LynxConfigModule(uint32_t view_context_id,
                   fml::RefPtr<fml::TaskRunner> task_runner);
  ~LynxConfigModule() override;

  std::unique_ptr<lynx::pub::Value> setDumpInfoToDevtoolEnabled(
      std::unique_ptr<lynx::pub::Value> args_array,
      const lynx::runtime::CallbackMap& callback_map);

  std::unique_ptr<lynx::pub::Value> setRenderOptions(
      std::unique_ptr<lynx::pub::Value> args_array,
      const lynx::runtime::CallbackMap& callback_map);

 private:
  static const std::string name_;
};

}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_CONFIG_MODULE_H_
