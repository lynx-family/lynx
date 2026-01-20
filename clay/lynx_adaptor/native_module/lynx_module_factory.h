// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_MODULE_FACTORY_H_
#define CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_MODULE_FACTORY_H_
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/fml/task_runner.h"
#include "core/public/jsb/native_module_factory.h"

namespace clay {
class ViewContext;
};
namespace lynx {

using ClayModuleCreator =
    std::function<std::shared_ptr<lynx::runtime::LynxNativeModule>(
        uint32_t view_context_id, fml::RefPtr<fml::TaskRunner> task_runner)>;

class LynxModuleFactory : public lynx::runtime::NativeModuleFactory {
 public:
  static lynx::runtime::NativeModuleFactory* CreateModuleFactory(
      clay::ViewContext* view_context);

  explicit LynxModuleFactory(clay::ViewContext* view_context);

  ~LynxModuleFactory() override = default;

  std::shared_ptr<lynx::runtime::LynxNativeModule> CreateModule(
      const std::string& name) override;

  void RegisterCreator(const std::string& name, ClayModuleCreator creator) {
    std::lock_guard<std::mutex> lock(clay_mutex_);
    clay_creators_.emplace(name, std::move(creator));
  }

 private:
  std::mutex clay_mutex_;
  std::unordered_map<std::string, ClayModuleCreator> clay_creators_;
  uint64_t view_context_id_;
  fml::RefPtr<fml::TaskRunner> task_runner_;
};

}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_MODULE_FACTORY_H_
