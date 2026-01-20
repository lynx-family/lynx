// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_MODULE_EXTENSION_MODULE_FACTORY_IMPL_H_
#define PLATFORM_EMBEDDER_MODULE_EXTENSION_MODULE_FACTORY_IMPL_H_

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "core/public/jsb/extension_module_factory.h"
#include "platform/embedder/public/capi/lynx_extension_module_capi.h"
#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace embedder {

class ExtensionModuleFactoryImpl : public runtime::ExtensionModuleFactory {
 public:
  ExtensionModuleFactoryImpl(
      std::unordered_map<std::string,
                         std::tuple<extension_module_creator, bool, void*>>
          extension_module_creators)
      : extension_module_creators_(std::move(extension_module_creators)) {}

  std::shared_ptr<runtime::LynxNativeModule> CreateModule(
      const std::string& name) override;

  void OnLynxViewCreate(lynx_view_t* lynx_view, tasm::UIDelegate* ui_delegate);
  void OnLynxViewCreate(tasm::UIDelegate* ui_delegate) override {}

 private:
  std::unordered_map<std::string,
                     std::tuple<extension_module_creator, bool, void*>>
      extension_module_creators_;
};

}  // namespace embedder
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // PLATFORM_EMBEDDER_MODULE_EXTENSION_MODULE_FACTORY_IMPL_H_
